#include "MainMCU.h"

HandlersMCU mcuHandlers;
uint32_t vcoValue = CONTROL_INITIAL_VCO;

LIFO_d risingEdgesFrequencies;
LIFO_d fallingEdgesFrequencies;
double risingEdgesArray [CONTROL_POINTS_IN_MEMORY];
double fallingEdgesArray[CONTROL_POINTS_IN_MEMORY];
volatile uint8_t newRisingEdge = 0;
volatile uint8_t newFallingEdge = 0;

LIFO_u32 risingPPSRef;
LIFO_u32 fallingPPSRef;
uint32_t risingPPSRefArray [CONTROL_POINTS_IN_MEMORY];
uint32_t fallingPPSRefArray[CONTROL_POINTS_IN_MEMORY];

LIFO_u32 risingOCXORef;
LIFO_u32 fallingOCXORef;
uint32_t risingOCXORefArray [CONTROL_POINTS_IN_MEMORY];
uint32_t fallingOCXORefArray[CONTROL_POINTS_IN_MEMORY];

double Kp = 0.01;
double Ki = 0.006;
double Kd = 0.01;
// Low pass filter of the VCO.
double Nf = 0.05;

double freqOffset = 1.3;

uint8_t txBuffer[100];
const double TIME_BETWEEN_PPS =  1.0 / PPS_REF_FREQ;
const double timePerIncrement = 1.0 / PPS_TIMER_FREQ;

void mcuStart(DAC_HandleTypeDef* dacHandler, 
              TIM_HandleTypeDef* freqDivHandler, TIM_HandleTypeDef* ppsHandler) {
    mcuHandlers.dacHandler = dacHandler;
    mcuHandlers.freqDivHandler = freqDivHandler;
    mcuHandlers.ppsHandler = ppsHandler;

    // Start circular buffers.
    init_LIFO_d(&risingEdgesFrequencies, risingEdgesArray, CONTROL_POINTS_IN_MEMORY);
    init_LIFO_d(&fallingEdgesFrequencies, fallingEdgesArray, CONTROL_POINTS_IN_MEMORY);
    
    init_LIFO_u32(&risingPPSRef, risingPPSRefArray, CONTROL_POINTS_IN_MEMORY);
    init_LIFO_u32(&fallingPPSRef, fallingPPSRefArray, CONTROL_POINTS_IN_MEMORY);
    init_LIFO_u32(&risingOCXORef, risingOCXORefArray, CONTROL_POINTS_IN_MEMORY);
    init_LIFO_u32(&fallingOCXORef, fallingOCXORefArray, CONTROL_POINTS_IN_MEMORY);

    // Initialization of the DAC.
    HAL_DAC_Start(dacHandler, DAC_CHANNEL_1);
    HAL_DAC_SetValue(mcuHandlers.dacHandler, DAC_CHANNEL_1, DAC_ALIGN_12B_R, CONTROL_INITIAL_VCO);

    // Initialization of Frequency Divider. 
    HAL_TIM_OC_Start(mcuHandlers.freqDivHandler, TIM_CHANNEL_1);

    // Initialization of the PPS handler.
    HAL_TIM_IC_Start_IT(ppsHandler, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(ppsHandler, TIM_CHANNEL_2);
    HAL_TIM_IC_Start_IT(ppsHandler, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(ppsHandler, TIM_CHANNEL_4);
    __HAL_TIM_DISABLE_IT(ppsHandler, TIM_IT_UPDATE);

    // Init USB.
    initUSBComms();
}

void mcuLoop() {
    static uint32_t lastUpdateVCOTime = 0;
    static uint32_t currentVCO = CONTROL_INITIAL_VCO;
    if((HAL_GetTick() - lastUpdateVCOTime) < CONTROL_VCO_UPDATE_TIME_ms) return;

    lastUpdateVCOTime = HAL_GetTick();

    // Find matching timestamps to generate the errors and calculate the new VCO voltage if a new
    // error value is found.
    if(newRisingEdge) {
        newRisingEdge = 0;
        calculateNewVCO(&risingEdgesFrequencies);
    }

    // if(newFallingEdge) {
    //     newFallingEdge = 0;
    //     calculateNewVCO(&fallingEdgesFrequencies);
    // }

    // Actuator section.

    // Discrete low pass filter for the VCO.
    currentVCO = currentVCO * Nf + vcoValue * (1.0 - Nf);
    HAL_DAC_SetValue(mcuHandlers.dacHandler, DAC_CHANNEL_1, DAC_ALIGN_12B_R, currentVCO);

    static uint8_t rxBuffer[512];
    uint32_t rxLen;
    if(readMessageUSB(rxBuffer, &rxLen) && (rxLen > 0)) {
        processUSBMessage((char*) rxBuffer, rxLen);
    }

}

uint8_t findMatchedTimestampsAndCalculateFrequency(LIFO_u32* ppsRef, LIFO_u32* ocxo, 
                                                   LIFO_d* freqOut) {
    // Need at least two points to calculate.
    if(ppsRef->len < 1 || ocxo->len < 1) return 0;

    // All time measurements are being done as time of PPS_OCXO minus the time of the PPS of 
    // reference.

    // This function must find a PPS of reference value that is the closest to the latest OCXO 
    // value. If the closest point is much too far, maybe the MCU has not received yet the 
    // corresponding PPS of reference time to that OCXO. Go to a previous OCXO value and look again 
    // for PPS of reference values.
    uint32_t lastPPSRef, lastOCXO;
    int ppsRefIndex, ocxoIndex;
    uint8_t foundPair = 0;

    double deltaTime;
    for(ocxoIndex = 0; ocxoIndex < ocxo->len; ocxoIndex++) {
        peekAt_LIFO_u32(ocxo, ocxoIndex, &lastOCXO);
        for(ppsRefIndex = 0; ppsRefIndex < ppsRef->len; ppsRefIndex++) {
            peekAt_LIFO_u32(ppsRef, ppsRefIndex, &lastPPSRef);
            
            // Center it between [-TIME_BETWEEN_PPS/2, TIME_BETWEEN_PPS/2]
            deltaTime = ((int32_t) (lastOCXO - lastPPSRef)) * timePerIncrement;
            if((deltaTime >= -TIME_BETWEEN_PPS/2) && (deltaTime <= TIME_BETWEEN_PPS/2)) {
                // Found a pair.
                foundPair = 1;
                break;
            } 
        }
    
        if(foundPair) {
            break;
        }
    }

    if(!foundPair) {
        return 0;
    }

    // Remove the numbers that were not used and also the values that were just used.
    freeN_LIFO_u32(ppsRef, ppsRef->len - ppsRefIndex);
    freeN_LIFO_u32(ocxo, ocxo->len - ocxoIndex);

    // Add the current frequency of the OCXO to the output buffer.

    // The delta time can be calculated as: dt = f_OCXO^-1 - f_PPS^-1
    // Solving for f_OCXO = (dt + f_PPS^-1)^-1.
    double currentOCXOFreq = 1.0 / (deltaTime + TIME_BETWEEN_PPS);
    push_LIFO_d(freqOut, currentOCXOFreq);

    return 1;
}

void calculateNewVCO(LIFO_d* freqValues) {
    double lastFrequency;

    // The last value in the FIFO is the last frequency calculated.
    peek_LIFO_d(freqValues, &lastFrequency);

    uint32_t len = sprintf((char*)txBuffer, "F=%.12f\n", lastFrequency);
    sendMessageUSB(txBuffer, len);

    #ifdef CONTROL_HYSTERESIS_ENABLED 
        if((deltaTime >= -CONTROL_HYSTERESIS) && (deltaTime <= CONTROL_HYSTERESIS)) {
            return;
        }
    #endif

    // In "step" mode, the MCU takes fixed steps of the VCO.
    // step_controlMode(deltaTime);

    // In "PID" mode, the VCO voltage is proportional to the frequency error, its integral and 
    // derivative.
    pid_controlMode(freqValues);
}

void pid_controlMode(LIFO_d* freqValues) {
    double currentOCXOFreq;
    peek_LIFO_d(freqValues, &currentOCXOFreq);

    double frequencyError = PPS_REF_FREQ - currentOCXOFreq;

    // Remember that the first element in the LIFO is the newest!
    double freq0, freq1;
    // This one is the previous frequency from the "currentOCXOFreq". 
    peekAt_LIFO_d(freqValues, 1, &freq0);
    freq1 = currentOCXOFreq;

    double frequencyDerivative = (freq1 - freq0) / TIME_BETWEEN_PPS;

    // Instead of calculating the integral of the error, subtract the integral of PPS_REF_FREQ from 
    // the integral of the frequencies.
    double frequencyIntegral = PPS_REF_FREQ * TIME_BETWEEN_PPS * (freqValues->len - 1);
    for(int i = 1; i < freqValues->len; i++) {
        freq0 = freq1;
        peekAt_LIFO_d(freqValues, i, &freq1);

        // Area of the trapezoid to calculate the components of the integral.
        frequencyIntegral -= (freq0 + freq1) * TIME_BETWEEN_PPS / 2.0;
    }


    double actuatorInput = frequencyError * Kp + frequencyIntegral * Ki + frequencyDerivative * Kd;

    // Calculate the offset necessary to match the PPS of reference.
    
    // Remember that in this case, the MCU outputs up to 3.3V. The VCO is connected to an Op-Amp 
    // that takes that 3.3 volts to 5 volts. Furthermore, the MCU generates the voltage with a 12 
    // bit DAC.
    
    // The OCXO has a control of +- 0.7 ppm. That is, it's frequency is 10 *10^6 +- 7 Hz.
    // For 0V, the offset is -7 Hz, for 5V is +7 Hz. 
    // Remember that the OCXO frequency is being divided to math that of the reference PPS.

    double newVCO = lerp(-OCXO_CONTROL_FREQUENCY_RANGE, 0.0, 
                              OCXO_CONTROL_FREQUENCY_RANGE,  4096.0,
                              actuatorInput * PPS_TIMER_FREQ / PPS_REF_FREQ + freqOffset);
    
    if(newVCO > 4095.0) {
        vcoValue = 4095;
    }else if(newVCO < 0.0) {
        vcoValue = 0;
    }else {
        vcoValue = (int) newVCO;
    }

    uint32_t len = sprintf((char*)txBuffer, "VCO=%.12f, %ld\n", newVCO, vcoValue);
    sendMessageUSB(txBuffer, len);
    len = sprintf((char*)txBuffer, "e=%.12f, Kp=%.12f\n", frequencyError, Kp);
    sendMessageUSB(txBuffer, len);
    len = sprintf((char*)txBuffer, "i=%.12f, Ki=%.12f\n", frequencyIntegral, Ki);
    sendMessageUSB(txBuffer, len);
    len = sprintf((char*)txBuffer, "d=%.12f, Kd=%.12f\n", frequencyDerivative, Kd);
    sendMessageUSB(txBuffer, len);
    len = sprintf((char*)txBuffer, "Of=%.12f\n", freqOffset);
    sendMessageUSB(txBuffer, len);

}

void step_controlMode(double deltaTime) {
    // Increment/Decrement step for the VCO control signal.
    const uint32_t CONTROL_SINGLE_STEP_VCO = 10;

    // The deltaTime can be either above or below TIME_BETWEEN_PPS / 2. If the PPS is bellow this 
    // threshold the OCXO should be slowed down. If it's above, then it should run faster.
    if(deltaTime >= 0) {
        // Run faster!
        if(vcoValue < (4095 - CONTROL_SINGLE_STEP_VCO)) vcoValue += CONTROL_SINGLE_STEP_VCO;
        else vcoValue = 4095;
    }else {
        // Run slower!
        if(vcoValue > CONTROL_SINGLE_STEP_VCO) vcoValue -= CONTROL_SINGLE_STEP_VCO;
        else vcoValue = 0;
    }
}

void processUSBMessage(char* buf, uint32_t len) {
    if(len == 0) return;
    
    uint32_t msgLen = 0;

    // Detect the Kp, Ki and Kd values for the PID.
    if(len > 3 && buf[2] == '=') {
        buf[len - 1] = 0;

        if(buf[0] == 'K'){
            if(buf[1] == 'p') {
                Kp = atof(buf + 3);
                msgLen = sprintf((char*)txBuffer, "New Kp = %.10f\n", Kp);
            }else if(buf[1] == 'i') {
                Ki = atof(buf + 3);
                msgLen = sprintf((char*)txBuffer, "New Ki = %.10f\n", Ki);
            }else if(buf[1] == 'd') {
                Kd = atof(buf + 3);
                msgLen = sprintf((char*)txBuffer, "New Kd = %.10f\n", Kd);
            }
        }else if(buf[0] == 'N' && buf[1] == 'f') {
            Nf = atof(buf + 3);
            msgLen = sprintf((char*)txBuffer, "New Nf = %.10f\n", Nf);
        }else if(buf[0] == 'O' && buf[1] == 'f') {
            freqOffset = atof(buf + 3);
            msgLen = sprintf((char*)txBuffer, "New Freq Offset = %.10f\n", freqOffset);
        }
    }

    if(strncmp(buf, "CONN", 4) == 0) {
        setUSBConnected(1);
        msgLen = sprintf((char*)txBuffer, "### OCXOController v0.1 ###\n");
    }

    if(strncmp(buf, "DISC", 4) == 0) {
        setUSBConnected(0);
    }

    sendMessageUSB(txBuffer, msgLen);
}

void freqDivider_IRQ() {
    // Nothing to do for this IRQ (yet).
}

void PPS_IRQ() {
    uint32_t itEnabled = mcuHandlers.ppsHandler->Instance->DIER;
    uint32_t itSource   = mcuHandlers.ppsHandler->Instance->SR;

    uint8_t newRising = 0;
    uint8_t newFalling = 0;

    // Channel 1 gets triggered on the rising edge of the PPS of reference.
    if (((itSource & TIM_FLAG_CC1) == TIM_FLAG_CC1) && 
        ((itEnabled & TIM_IT_CC1) == TIM_IT_CC1)) {
	    __HAL_TIM_CLEAR_FLAG(mcuHandlers.ppsHandler, TIM_FLAG_CC1);

        push_LIFO_u32(&risingPPSRef, HAL_TIM_ReadCapturedValue(mcuHandlers.ppsHandler, TIM_CHANNEL_1));
        newRising = 1;
    }

    // Channel 2 gets triggered on the falling edge of the PPS of reference.
    if (((itSource & TIM_FLAG_CC2) == TIM_FLAG_CC2) && 
        ((itEnabled & TIM_IT_CC2) == TIM_IT_CC2)) {
	    __HAL_TIM_CLEAR_FLAG(mcuHandlers.ppsHandler, TIM_FLAG_CC2);

        push_LIFO_u32(&fallingPPSRef, HAL_TIM_ReadCapturedValue(mcuHandlers.ppsHandler, TIM_CHANNEL_2));
        newFalling = 1;
    }

    // Channel 3 gets triggered on the rising edge of the OCXO PPS.
    if (((itSource & TIM_FLAG_CC3) == TIM_FLAG_CC3) && 
        ((itEnabled & TIM_IT_CC3) == TIM_IT_CC3)) {
	    __HAL_TIM_CLEAR_FLAG(mcuHandlers.ppsHandler, TIM_FLAG_CC3);

        push_LIFO_u32(&risingOCXORef, HAL_TIM_ReadCapturedValue(mcuHandlers.ppsHandler, TIM_CHANNEL_3));
        newRising = 1;
    }

    // Channel 4 gets triggered on the falling edge of the OCXO PPS.
    if (((itSource & TIM_FLAG_CC4) == TIM_FLAG_CC4) && 
        ((itEnabled & TIM_IT_CC4) == TIM_IT_CC4)) {
	    __HAL_TIM_CLEAR_FLAG(mcuHandlers.ppsHandler, TIM_FLAG_CC4);

        push_LIFO_u32(&fallingOCXORef, HAL_TIM_ReadCapturedValue(mcuHandlers.ppsHandler, TIM_CHANNEL_4));
        newFalling = 1;
    }

    if(newRising) {
        newRisingEdge |= 
            findMatchedTimestampsAndCalculateFrequency(&risingPPSRef, &risingOCXORef, &risingEdgesFrequencies);
    }

    if(newFalling) {
        newFallingEdge |= 
            findMatchedTimestampsAndCalculateFrequency(&fallingPPSRef, &fallingOCXORef, &fallingEdgesFrequencies);
    }

    __HAL_TIM_CLEAR_FLAG(mcuHandlers.ppsHandler, TIM_FLAG_UPDATE);
}
