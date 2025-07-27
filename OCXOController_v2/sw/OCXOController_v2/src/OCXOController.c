#include "OCXOController.h"
#include "main.h"       // GPIO names.
#include "MainMCU.h"    // DAC handler.

TIM_HandleTypeDef* ppsTim;
TIM_HandleTypeDef* ocxoTim;
TIM_HandleTypeDef* ocxoFreqDivTim;

uint32_t vcoValue = CONTROL_INITIAL_VCO;

LIFO_d risingEdgesFreq;
LIFO_d fallingEdgesFreq;
double risingEdgesFreqArray [CONTROL_POINTS_IN_MEMORY];
double fallingEdgesFreqArray[CONTROL_POINTS_IN_MEMORY];
volatile uint8_t newRisingEdge = 0;
volatile uint8_t newFallingEdge = 0;

// Used on calibration. Stores the timestamps of the rising edges of both signals. This one does not
// get cleared continuously. New values will overwrite the oldest values in the LIFO.
LIFO_u32 risingEdgesPPSRefTimestamps;
LIFO_u32 risingEdgesOCXOTimestamps;
uint32_t risingEdgesPPSRefTimestampsArray[OCXO_CALIBRATION_FREQUENCY_MEASUREMENTS];
uint32_t risingEdgesOCXOTimestampsArray[OCXO_CALIBRATION_FREQUENCY_MEASUREMENTS];

// Used to calculate the relative frequency. It is normally cleared when a pair of timestamps have
// been found that are close enough to generate a relative frequency. If no timestamp is found, the
// values remain on the LIFO until a new pair is found. 
LIFO_u32 risingPPSRef;
LIFO_u32 fallingPPSRef;
uint32_t risingPPSRefArray [CONTROL_CLOSE_POINTS_IN_MEMORY];
uint32_t fallingPPSRefArray[CONTROL_CLOSE_POINTS_IN_MEMORY];

LIFO_u32 risingOCXO;
LIFO_u32 fallingOCXO;
uint32_t risingOCXOArray [CONTROL_CLOSE_POINTS_IN_MEMORY];
uint32_t fallingOCXOArray[CONTROL_CLOSE_POINTS_IN_MEMORY];

// Proportional gain.
double Kp = 0.05;
// Integral gain.
double Ki = 0.002;
// Differential gain.
double Kd = 0.001;
// Low pass filter of the VCO.
double Nf = 0.1;
// Filter of the derivative value.
double Df = 0.1;
// Limits the integral value.
double antiwindupLimit = 0.0001;
// Offset frequency for the generation of the VCO.
double phaseOffset = 0;

// Frequency of the OCXO when VCO = 0V.
double minOCXOFrequency = -OCXO_CONTROL_FREQUENCY_RANGE;
// Frequency of the OCXO when VCO = Vcc.
double maxOCXOFrequency = OCXO_CONTROL_FREQUENCY_RANGE;

uint8_t doingCalibration = 0;

uint32_t currentVCO = CONTROL_INITIAL_VCO;

uint8_t txBuffer[100];
const double TIME_BETWEEN_PPS =  1.0 / PPS_REF_FREQ;
const double timePerIncrement = 1.0 / PPS_TIMER_FREQ;

uint8_t initOCXOController(TIM_HandleTypeDef* ppsTim_, TIM_HandleTypeDef* ocxoTim_, 
                        TIM_HandleTypeDef* ocxoFreqDividerTim_) {
    ppsTim = ppsTim_;
    ocxoTim = ocxoTim_;
    ocxoFreqDivTim = ocxoFreqDividerTim_;

    // Start LIFOs.
    init_LIFO_d(&risingEdgesFreq, risingEdgesFreqArray, CONTROL_POINTS_IN_MEMORY);
    init_LIFO_d(&fallingEdgesFreq, fallingEdgesFreqArray, CONTROL_POINTS_IN_MEMORY);

    init_LIFO_u32(&risingEdgesPPSRefTimestamps, risingEdgesPPSRefTimestampsArray, OCXO_CALIBRATION_FREQUENCY_MEASUREMENTS);
    init_LIFO_u32(&risingEdgesOCXOTimestamps, risingEdgesOCXOTimestampsArray, OCXO_CALIBRATION_FREQUENCY_MEASUREMENTS);

    init_LIFO_u32(&risingPPSRef, risingPPSRefArray, CONTROL_CLOSE_POINTS_IN_MEMORY);
    init_LIFO_u32(&fallingPPSRef, fallingPPSRefArray, CONTROL_CLOSE_POINTS_IN_MEMORY);
    init_LIFO_u32(&risingOCXO, risingOCXOArray, CONTROL_CLOSE_POINTS_IN_MEMORY);
    init_LIFO_u32(&fallingOCXO, fallingOCXOArray, CONTROL_CLOSE_POINTS_IN_MEMORY);

    // Initialization of Frequency Divider. 
    uint8_t status = HAL_TIM_OC_Start(ocxoFreqDividerTim_, TIM_CHANNEL_2) == HAL_OK;

    // Initialization of the timestamping timers.
    status &= HAL_TIM_IC_Start_IT(ppsTim, TIM_CHANNEL_1) == HAL_OK;
//    status &= HAL_TIM_IC_Start_IT(ppsTim, TIM_CHANNEL_2) == HAL_OK;
    __HAL_TIM_DISABLE_IT(ppsTim, TIM_IT_UPDATE);
    
    status &= HAL_TIM_IC_Start_IT(ocxoTim, TIM_CHANNEL_1) == HAL_OK;
//    status &= HAL_TIM_IC_Start_IT(ocxoTim, TIM_CHANNEL_3) == HAL_OK;
    __HAL_TIM_DISABLE_IT(ocxoTim, TIM_IT_UPDATE);

    // Init USB.
    initUSBComms();

    // Carry out the initial calibration.
    // doingCalibration = 1;

    return status;
}

void loopOCXOCOntroller() {
    static uint32_t lastUpdateVCOTime = 0;
    if((HAL_GetTick() - lastUpdateVCOTime) < CONTROL_VCO_UPDATE_TIME_ms) return;

    lastUpdateVCOTime = HAL_GetTick();

    // Find matching timestamps to generate the errors and calculate the new VCO voltage if a new
    // error value is found.
    if(newRisingEdge) {
        hmain.isReferenceSignalConnected = 1;

        newRisingEdge = 0;
        if(doingCalibration) {
            calibrateOCXO(&risingEdgesFreq);
        }else {
            calculateNewVCO_(&risingEdgesFreq);
            // Discrete low pass filter for the VCO.
            currentVCO = currentVCO * Nf + vcoValue * (1.0 - Nf);
        }
    }

    // if(newFallingEdge) {
    //     newFallingEdge = 0;
    //     calculateNewVCO_(&fallingEdgesFreq);
    // }

    // Actuator section.
    setMCP4726DAC(&hmain.dac, currentVCO);

    static uint8_t rxBuffer[512];
    uint32_t rxLen;
    if(readMessageUSB(sizeof(rxBuffer), rxBuffer, &rxLen) && (rxLen > 0)) {
        processUSBMessage_((char*) rxBuffer, rxLen);
    }

}

void calibrateOCXO(LIFO_d* freqs) {
    static uint32_t minFreqSampleCount = 0;
    static uint32_t maxFreqSampleCount = 0;
    static double  minFreqSum = 0;
    static double  maxFreqSum = 0;

    if((risingEdgesPPSRefTimestamps.len < OCXO_CALIBRATION_FREQUENCY_MEASUREMENTS) || 
       (risingEdgesOCXOTimestamps.len < OCXO_CALIBRATION_FREQUENCY_MEASUREMENTS)) {
        // There aren't enough points to calculate the frequency.
        return;
    }

    if(minFreqSampleCount < (OCXO_CALIBRATION_MEASURE_COUNT + OCXO_CALIBRATION_STABILIZATION_COUNT)) {
        currentVCO = 0;
        if(minFreqSampleCount >= OCXO_CALIBRATION_STABILIZATION_COUNT) {
            minFreqSum += calculateFrequencyFromTimestamps_() * PPS_TIMER_FREQ / PPS_REF_FREQ;
        }
        minFreqSampleCount++;
    }else if(maxFreqSampleCount < (OCXO_CALIBRATION_MEASURE_COUNT + OCXO_CALIBRATION_STABILIZATION_COUNT)) {
        currentVCO = 4095;
        if(maxFreqSampleCount >= OCXO_CALIBRATION_STABILIZATION_COUNT) {
            maxFreqSum += calculateFrequencyFromTimestamps_()* PPS_TIMER_FREQ / PPS_REF_FREQ;
        }
        maxFreqSampleCount++;
    }else {
        minOCXOFrequency = minFreqSum / ((double) OCXO_CALIBRATION_MEASURE_COUNT) - PPS_TIMER_FREQ;
        maxOCXOFrequency = maxFreqSum / ((double) OCXO_CALIBRATION_MEASURE_COUNT) - PPS_TIMER_FREQ;

        uint32_t len = sprintf((char*)txBuffer, "Calibration [%.12f, %.12f]\n", 
                               minOCXOFrequency, maxOCXOFrequency);
        sendMessageUSB(txBuffer, len);

        currentVCO = CONTROL_INITIAL_VCO;

        // Reset static fields.
        minFreqSampleCount = 0;
        maxFreqSampleCount = 0;
        minFreqSum = 0;
        maxFreqSum = 0;
        
        // Ended the calibration process.
        doingCalibration = 0;
    }
}

double calculateFrequencyFromTimestamps_() {
    // Lock the LIFOs.
    risingEdgesOCXOTimestamps.locked = 1;
    risingEdgesPPSRefTimestamps.locked = 1;

    double OCXOFreq = 0;
    double deltaOCXO = 0, deltaPPS = 0;
    uint32_t temp1, temp0;
    peek_LIFO_u32(&risingEdgesOCXOTimestamps, &temp0);
    for(int i = 1; i < risingEdgesOCXOTimestamps.len; i++) {
        temp1 = temp0;
        peekAt_LIFO_u32(&risingEdgesOCXOTimestamps, i, &temp0);
        deltaOCXO += (uint32_t) (temp1 - temp0);
    }
    deltaOCXO /= (risingEdgesOCXOTimestamps.len - 1);

    peek_LIFO_u32(&risingEdgesPPSRefTimestamps, &temp0);
    for(int i = 1; i < risingEdgesPPSRefTimestamps.len; i++) {
        temp1 = temp0;
        peekAt_LIFO_u32(&risingEdgesPPSRefTimestamps, i, &temp0);
        deltaPPS += (uint32_t) (temp1 - temp0);
    }
    deltaPPS /= (risingEdgesPPSRefTimestamps.len - 1);

    // The relation between time and frequency is inverse!
    OCXOFreq = PPS_REF_FREQ * deltaPPS / deltaOCXO; 

    // Erase the LIFOs.
    empty_LIFO_u32(&risingEdgesOCXOTimestamps);
    empty_LIFO_u32(&risingEdgesPPSRefTimestamps);

    // Unlock the LIFOs.
    risingEdgesOCXOTimestamps.locked = 0;
    risingEdgesPPSRefTimestamps.locked = 0;

    return OCXOFreq;
}

void calculateNewVCO_(LIFO_d* freqValues) {
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
    // step_controlMode_(freqValues);

    // In "PID" mode, the VCO voltage is proportional to the frequency error, its integral and 
    // derivative.
    pid_controlMode_(freqValues);
}

void pid_controlMode_(LIFO_d* freqValues) {
    static double frequencyDerivative = 0;

    double frequencyIntegral = 0;
    double currentOCXOFreq = 0, previousOCXOFreq = 0;
    // Remember that the first element in the LIFO is the newest!
    peek_LIFO_d(freqValues, &currentOCXOFreq);

    double frequencyError = PPS_REF_FREQ - currentOCXOFreq;

    if(freqValues->len > 1) {
        // This one is the previous frequency from the "currentOCXOFreq".
        peekAt_LIFO_d(freqValues, 1, &previousOCXOFreq);
        frequencyDerivative = (frequencyDerivative * Df) + 
                              (((currentOCXOFreq - previousOCXOFreq) / TIME_BETWEEN_PPS) * (1.0 - Df));

        // Instead of calculating the integral of the error, subtract the integral of PPS_REF_FREQ
        // from the integral of the frequencies.
        frequencyIntegral = PPS_REF_FREQ * TIME_BETWEEN_PPS * (freqValues->len - 1);
        // Initial value for the integration algorithm.
        previousOCXOFreq = currentOCXOFreq;
        for(int i = 1; i < freqValues->len; i++) {
            currentOCXOFreq = previousOCXOFreq;
            peekAt_LIFO_d(freqValues, i, &previousOCXOFreq);

            // Area of the trapezoid to calculate the components of the integral.
            frequencyIntegral -= (previousOCXOFreq + currentOCXOFreq) * TIME_BETWEEN_PPS / 2.0;
        }
        
        // Anti wind-up control.
        if(frequencyIntegral > antiwindupLimit) frequencyIntegral = antiwindupLimit;
        else if(frequencyIntegral < (-antiwindupLimit)) frequencyIntegral = -antiwindupLimit;

    }

    double actuatorInput = frequencyError * Kp + frequencyIntegral * Ki + frequencyDerivative * Kd;

    // Calculate the offset necessary to match the PPS of reference.
    
    // Remember that in this case, the MCU outputs up to 3.3V. The VCO is connected to an Op-Amp 
    // that takes that 3.3 volts to 5 volts. Furthermore, the MCU generates the voltage with a 12 
    // bit DAC.
    
    // The OCXO has a control of +- 0.7 ppm. That is, it's frequency is 10 *10^6 +- 7 Hz.
    // For 0V, the offset is -7 Hz, for 5V is +7 Hz. 
    // Remember that the OCXO frequency is being divided to match that of the reference PPS.

    double newVCO = lerp(minOCXOFrequency, 0.0, maxOCXOFrequency,  4095.0,
                         actuatorInput * PPS_TIMER_FREQ / PPS_REF_FREQ);
    
    if(newVCO > 4095.0) {
        vcoValue = 4095;
    }else if(newVCO < 0.0) {
        vcoValue = 0;
    }else {
        vcoValue = (int) newVCO;
    }

    // "e=%e, i=%e, d=%e. Kp*e=%e, Ki*i=%e, Kd*d=%e. u=%d\n", frequencyError, frequencyIntegral, frequencyDerivative, frequencyError * Kp, frequencyIntegral * Ki, frequencyDerivative * Kd, newVCO
    uint32_t len = sprintf((char*)txBuffer, "VCO=%.12f, %ld\n", newVCO, vcoValue);
    sendMessageUSB(txBuffer, len);
    len = sprintf((char*)txBuffer, "e=%.12f, Kp=%.12f\n", frequencyError, Kp);
    sendMessageUSB(txBuffer, len);
    len = sprintf((char*)txBuffer, "i=%.12f, Ki=%.12f\n", frequencyIntegral, Ki);
    sendMessageUSB(txBuffer, len);
    len = sprintf((char*)txBuffer, "d=%.12f, Kd=%.12f\n", frequencyDerivative, Kd);
    sendMessageUSB(txBuffer, len);
    len = sprintf((char*)txBuffer, "Of=%.12f\n", phaseOffset);
    sendMessageUSB(txBuffer, len);

}

void step_controlMode_(LIFO_d* freqValues) {
    // Increment/Decrement step for the VCO control signal.
    const uint32_t CONTROL_SINGLE_STEP_VCO = 10;

    double currentOCXOFreq = 0;
    peek_LIFO_d(freqValues, &currentOCXOFreq);

    double deltaTime = PPS_REF_FREQ - currentOCXOFreq;

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

void processUSBMessage_(char* buf, uint32_t len) {
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
            phaseOffset = atof(buf + 3);
            msgLen = sprintf((char*)txBuffer, "New Phase Offset = %.10f\n", phaseOffset);
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

void referencePPS_IRQ() {
    uint8_t newRising = 0;
    uint8_t newFalling = 0;

    // Channel 1 gets triggered on the rising edge of the PPS of reference.
    if (((ppsTim->Instance->SR & TIM_FLAG_CC1) == TIM_FLAG_CC1) && 
        ((ppsTim->Instance->DIER & TIM_IT_CC1) == TIM_IT_CC1)) {
	    __HAL_TIM_CLEAR_FLAG(ppsTim, TIM_FLAG_CC1);

        push_LIFO_u32(&risingPPSRef, HAL_TIM_ReadCapturedValue(ppsTim, TIM_CHANNEL_1));
        newRising = 1;

        if(doingCalibration) {
            uint32_t temp;
            peek_LIFO_u32(&risingPPSRef, &temp);
            push_LIFO_u32(&risingEdgesPPSRefTimestamps, temp);
        }
    }

    // Channel 2 gets triggered on the falling edge of the PPS of reference.
    if (((ppsTim->Instance->SR & TIM_FLAG_CC2) == TIM_FLAG_CC2) && 
        ((ppsTim->Instance->DIER & TIM_IT_CC2) == TIM_IT_CC2)) {
	    __HAL_TIM_CLEAR_FLAG(ppsTim, TIM_FLAG_CC2);

        push_LIFO_u32(&fallingPPSRef, HAL_TIM_ReadCapturedValue(ppsTim, TIM_CHANNEL_2));
        newFalling = 1;
    }

    if(newRising) {
        newRisingEdge |= 
            findMatchedTimestampsAndCalculateFrequency_(&risingPPSRef, &risingOCXO, 
                                                        &risingEdgesFreq); 
    }

    if(newFalling) {
        newFallingEdge |= 
            findMatchedTimestampsAndCalculateFrequency_(&fallingPPSRef, &fallingOCXO, 
                                                        &fallingEdgesFreq);
    }

    __HAL_TIM_CLEAR_FLAG(ppsTim, TIM_FLAG_UPDATE);
}

void dividedOCXO_IRQ() {
    uint8_t newRising = 0;
    uint8_t newFalling = 0;

    // Channel 1 gets triggered on the rising edge of the OCXO PPS.
    if (((ocxoTim->Instance->SR & TIM_FLAG_CC1) == TIM_FLAG_CC1) && 
        ((ocxoTim->Instance->DIER & TIM_IT_CC1) == TIM_IT_CC1)) {
	    __HAL_TIM_CLEAR_FLAG(ocxoTim, TIM_FLAG_CC1);

        push_LIFO_u32(&risingOCXO, HAL_TIM_ReadCapturedValue(ocxoTim, TIM_CHANNEL_1));
        newRising = 1;

        if(doingCalibration) {
            uint32_t temp;
            peek_LIFO_u32(&risingOCXO, &temp);
            push_LIFO_u32(&risingEdgesOCXOTimestamps, temp);
        }
    }

    // Channel 3 gets triggered on the falling edge of the OCXO PPS.
    if (((ocxoTim->Instance->SR & TIM_FLAG_CC3) == TIM_FLAG_CC3) && 
        ((ocxoTim->Instance->DIER & TIM_IT_CC3) == TIM_IT_CC3)) {
	    __HAL_TIM_CLEAR_FLAG(ocxoTim, TIM_FLAG_CC3);

        push_LIFO_u32(&fallingOCXO, HAL_TIM_ReadCapturedValue(ocxoTim, TIM_CHANNEL_3));
        newFalling = 1;
    }

    if(newRising) {
        newRisingEdge |= 
            findMatchedTimestampsAndCalculateFrequency_(&risingPPSRef, &risingOCXO, 
                                                        &risingEdgesFreq); 
    }

    if(newFalling) {
        newFallingEdge |= 
            findMatchedTimestampsAndCalculateFrequency_(&fallingPPSRef, &fallingOCXO, 
                                                        &fallingEdgesFreq);
    }

    __HAL_TIM_CLEAR_FLAG(ocxoTim, TIM_FLAG_UPDATE);
}

uint8_t findMatchedTimestampsAndCalculateFrequency_(LIFO_u32* ppsRef, LIFO_u32* ocxo, 
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
            
            // deltaTime is between [-TIME_BETWEEN_PPS/2, TIME_BETWEEN_PPS/2]
            deltaTime = ((int16_t) (((uint16_t)lastOCXO) - ((uint16_t)lastPPSRef))) * timePerIncrement;
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

double lerp(double x0, double y0, double x1, double y1, double x) {
    return y1 - (x1 - x)*(y1 - y0)/(x1 - x0);
}
