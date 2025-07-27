#include "OCXOChannels.h"
#include "MainMCU.h"

const char* frequencyUnits[] = {"nHz", "uHz", "mHz", " Hz", "kHz", "MHz", "GHz"};
const int16_t frequencyUnitsLen = sizeof(frequencyUnits)/sizeof(char*);
const char* timeUnits[] = {"ns", "us", "ms", "s "};
const int16_t timeUnitsLen = sizeof(timeUnits)/sizeof(char*);
const char* voltageTags[] = {"5V", "3V3", "1V8"};
const int16_t voltageTagsLen = sizeof(voltageTags)/sizeof(char*);

uint8_t initOCXOChannels(OCXOChannels* outs, 
    TIM_HandleTypeDef* htim3, TIM_HandleTypeDef* htim4, TIM_HandleTypeDef* htim8) {
    if(outs == NULL) return 0;

    initOCXOChannel_(&outs->ch1, 1, GPIO_OUT1, BUTTON_2, htim4, TIM_CHANNEL_2, CH_OUT1_GPIO_Port, CH_OUT1_Pin);
    initOCXOChannel_(&outs->ch2, 2, GPIO_OUT2, BUTTON_3, htim8, TIM_CHANNEL_1, CH_OUT2_GPIO_Port, CH_OUT2_Pin);
    initOCXOChannel_(&outs->ch3, 3, GPIO_OUT3, BUTTON_4, htim3, TIM_CHANNEL_2, CH_OUT3_GPIO_Port, CH_OUT3_Pin);

    return applyAllOCXOOutputsFromConfiguration(outs);
}

void initOCXOChannel_(OCXOChannel* out, uint8_t id, VCIO pin, Button btn,
                      TIM_HandleTypeDef* htim, uint32_t timChannel,
                      GPIO_TypeDef* hgpio, uint32_t gpioPin) {
    out->id = id;
    out->pin = pin;
    out->btn = btn;
    out->isOutputON = 0;
    out->htim = htim;
    out->timCh = timChannel;
    out->hgpio = hgpio;
    out->gpioPin = gpioPin;

    if(!readOCXOChannelConfigurationFromEEPROM_(out)) {
        strcpy(out->config.freq, "000.000");
        strcpy(out->config.freqUnits, " Hz");
        strcpy(out->config.duty, "000.000");
        strcpy(out->config.dutyUnits, "%");
        strcpy(out->config.phase, "000.000");
        strcpy(out->config.phaseUnits, "ns");
        strcpy(out->config.voltage, "5V");
    }
}

uint8_t validateNumberArray(const char* number) {
    for(int8_t i = 0; i < 7; i++) {
        if(i == 3) continue;
        
        if(number[i] < '0' || number[i] > '9') return 0;
    }
    return number[3] == '.';
}

float charArrayToFloat(const char* number, const char* units) {
    if(!validateNumberArray(number)) return -1.0f;

    float num = (number[0]-'0') * 100.0f + 
                (number[1]-'0') * 10.0f  + 
                (number[2]-'0') * 1.0f   + 
                (number[4]-'0') * .1f    + 
                (number[5]-'0') * .01f   + 
                (number[6]-'0') * .001f;

    switch (units[0]){
        case 'n': num *= 1e-9f; break;
        case 'u': num *= 1e-6f; break;
        case 'm': num *= 1e-3f; break;
        case 'k': num *= 1e+3f; break;
        case 'M': num *= 1e+6f; break;
        case 'G': num *= 1e+9f; break;
        default : break;
    }
    return num;
}

uint8_t applyOCXOOutputFromConfiguration(OCXOChannels* outs, uint8_t id) {
    if(outs == NULL) return 0;

    OCXOChannel* out;
    if(!getOCXOOutputsFromID_(outs, id, &out)) return 0;

    // Converts the char arrays in the config into their respective values.
    float desiredFrequency = charArrayToFloat(out->config.freq, out->config.freqUnits);
    float desiredDutyCycle = charArrayToFloat(out->config.duty, out->config.dutyUnits) * 0.01f;
    float desiredPhase_ns  = charArrayToFloat(out->config.phase, out->config.phaseUnits) * 1e9f;

    VoltageLevel desiredVoltage = VOLTAGE_LEVEL_OFF;
    if(strcmp(out->config.voltage, "5V") == 0)          desiredVoltage = VOLTAGE_LEVEL_5V;
    else if(strcmp(out->config.voltage, "3V3") == 0)    desiredVoltage = VOLTAGE_LEVEL_3V3;
    else if(strcmp(out->config.voltage, "1V8") == 0)    desiredVoltage = VOLTAGE_LEVEL_1V8;

    if(out->isOutputON) {
        // FREQUENCY
        // The frequency of a channel's output is calculated with the timer values PSC and ARR:
        // f_out = f_tim / (PSC+1) / (ARR + 1)
        float psc_arr_f = roundf(OCXO_FREQUENCY / desiredFrequency);
        uint32_t psc_arr = (uint32_t) psc_arr_f;
        if(psc_arr_f >= 0x100000000) psc_arr = 0xFFFFFFFF;
        else if(psc_arr_f < 1)  psc_arr = 1;

        // To get the maximum resolution on the duty cycle, try to maximize the ARR value.
        uint32_t min_diff = -1;
        uint16_t best_psc = 0;
        uint16_t best_arr = 0;

        for (uint32_t arr = 0xFFFF; arr > 1; arr--) {
            uint32_t psc = psc_arr / arr;

            // Enforce psc < arr and both < 65536.
            if (psc >= arr || psc >= 0xFFFF) continue;  

            uint32_t prod = psc * arr;
            uint32_t diff = (prod > psc_arr) ? (prod - psc_arr) : (psc_arr - prod);

            if (diff < min_diff) {
                min_diff = diff;
                best_psc = (uint16_t) (psc - 1);
                best_arr = (uint16_t) (arr - 1);

                // Perfect match found
                if (diff == 0) break;
            }
        }

        // PSC can be 0, but ARR cannot be 0.
        if(best_arr == 0) best_arr = 1;

        // DUTY CYCLE
        // The duty cycle (%) is calculated as CCR/ARR. We have ARR, so now calculate CCR.
        uint32_t bestCCR = roundf(((float) best_arr + 1) * desiredDutyCycle);
        
        // PHASE
        // The phase is set by setting the CNT of the timer before the clock starts running.
        // An increment of the TIM is made every PSC+1 clock pulses.
        float deltaTime = 1e9f/OCXO_FREQUENCY * (best_psc + 1);
        uint16_t initialCNT = desiredPhase_ns / deltaTime;

        // APPLY THE SETTINGS!
        HAL_TIM_PWM_Stop(out->htim, out->timCh);
    
        out->htim->Instance->PSC = best_psc;
        out->htim->Instance->ARR = best_arr;

        TIM_OC_InitTypeDef sConfigOC = {0};
        sConfigOC.OCMode = TIM_OCMODE_PWM1;
        sConfigOC.Pulse = bestCCR;
        sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
        sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
        sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
        sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
        HAL_TIM_PWM_ConfigChannel(out->htim, &sConfigOC, out->timCh);

        HAL_TIM_PWM_Start(out->htim, out->timCh);

        __HAL_TIM_SET_COUNTER(out->htim, initialCNT);

        setVoltageLevel(&hmain.gpio, out->pin, desiredVoltage);
    }else {
        setVoltageLevel(&hmain.gpio, out->pin, VOLTAGE_LEVEL_OFF);

        HAL_TIM_PWM_Stop(out->htim, out->timCh);
    }

    HAL_GPIO_WritePin(out->hgpio, out->gpioPin, GPIO_PIN_SET);

    // If everything went OK set the real values and save in EEPROM.
    out->frequency  = desiredFrequency;
    out->dutyCycle  = desiredDutyCycle;
    out->phase_ns   = desiredPhase_ns;
    out->voltage    = desiredVoltage;

    if(out->isOutputON) {
        switch (out->voltage) {
            case VOLTAGE_LEVEL_5V:
                setButtonColor(&hmain.gpio, out->btn, BUTTON_COLOR_CYAN);
                break;
            case VOLTAGE_LEVEL_3V3:
                setButtonColor(&hmain.gpio, out->btn, BUTTON_COLOR_GREEN);
                break;
            case VOLTAGE_LEVEL_1V8:
                setButtonColor(&hmain.gpio, out->btn, BUTTON_COLOR_PINK);
                break;
            default:    break;
        }
    }else {
        setButtonColor(&hmain.gpio, out->btn, BUTTON_COLOR_RED);
    }

    saveOCXOChannelConfigurationInEEPROM_(out);

    return 1;
}

uint8_t applyAllOCXOOutputsFromConfiguration(OCXOChannels* outs) {
    __disable_irq();

    // Disable the slave mode Trigger of TIM1, which makes TIM1 start when an edge is received from 
    // the reference signal.
    TIM_SlaveConfigTypeDef sSlaveConfig = {0};
    sSlaveConfig.SlaveMode = TIM_SLAVEMODE_DISABLE;
    sSlaveConfig.InputTrigger = TIM_TS_TI2FP2;
    sSlaveConfig.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
    sSlaveConfig.TriggerFilter = 0;
    HAL_TIM_SlaveConfigSynchro(hmain.htim1, &sSlaveConfig);

    // Stop TIM1 (the one generating the clock signal) before setting the OCXO.
    hmain.htim1->Instance->CR1 &= ~TIM_CR1_CEN;
    // Set to 1 so the next clock triggers an update event.
    __HAL_TIM_SET_COUNTER(hmain.htim1, 1); 

    // Also set the divider of the OCXO to PPS timer so that everything falls in phase.
    // Set also so that the next clock triggers an interrupt.
    __HAL_TIM_SET_COUNTER(hmain.htim5, 2499);
    HAL_GPIO_WritePin(OCXO_DIVIDED_GPIO_Port, OCXO_DIVIDED_Pin, GPIO_PIN_RESET);

    // Stop the timestamping timers and reset them.
    hmain.htim2->Instance->CR1 &= ~TIM_CR1_CEN;
    hmain.htim15->Instance->CR1 &= ~TIM_CR1_CEN;
    __HAL_TIM_SET_COUNTER(hmain.htim2, 0);
    __HAL_TIM_SET_COUNTER(hmain.htim15, 0);

    uint8_t ret = applyOCXOOutputFromConfiguration(outs, outs->ch1.id);
    ret &= applyOCXOOutputFromConfiguration(outs, outs->ch2.id);
    ret &= applyOCXOOutputFromConfiguration(outs, outs->ch3.id);

    // This starts all outputs at the same time.
    // hmain.htim1->Instance->CR1 |= TIM_CR1_CEN;

    sSlaveConfig.SlaveMode = TIM_SLAVEMODE_TRIGGER;
    HAL_TIM_SlaveConfigSynchro(hmain.htim1, &sSlaveConfig);

    // Now TIM1 is waiting for the reference signal to be sent.
    if(!hmain.isReferenceSignalConnected) {
        // If the reference signal is not connected, trigger TIM1 manually.
        hmain.htim1->Instance->CR1 |= TIM_CR1_CEN;
    }

    __enable_irq();
    return ret;
}

uint8_t getOCXOOutputsFromID_(OCXOChannels* outs, uint8_t id, OCXOChannel** out) {
    switch (id) {
        case 1:     *out = &outs->ch1;  break;
        case 2:     *out = &outs->ch2;  break;
        case 3:     *out = &outs->ch3;  break;
    
        default:    return 0;
    }
    return 1;
}

void getFrequencyString(OCXOChannel* ch, char* str, int16_t len) {
    if(ch == NULL) return;

    snprintf(str, len, "%s %s", ch->config.freq, ch->config.freqUnits);
}

void getPhaseString(OCXOChannel* ch, char* str, int16_t len) {
    if(ch == NULL) return;

    int index = 0; 

    uint32_t scaled;
    uint64_t unsignedPhase;
    if(ch->phase_ns > 0) {
        unsignedPhase = roundf(ch->phase_ns);
        scaled = 1;
    }else {
        unsignedPhase = -roundf(ch->phase_ns);
        scaled = -1;
    }

    while((unsignedPhase >= 1000) && (index < (timeUnitsLen-1))) {
        unsignedPhase /= 1000;
        index++;
    }

    scaled *= (uint32_t) unsignedPhase;
    snprintf(str, len, "%c%ld%s", FONTS_DELTA, scaled, timeUnits[index]);
}

uint8_t saveOCXOChannelConfigurationInEEPROM_(OCXOChannel* ch) {
    if(ch == NULL) return 0;

    uint8_t buf[OCXO_CH_EEPROM_CHANNEL_SIZE] = {0};
    memcpy(buf, &ch->config, sizeof(ch->config));

    return writeEEPROM(&hmain.eeprom, 
                       OCXO_CH_EEPROM_START_ADDRS + ch->id*OCXO_CH_EEPROM_CHANNEL_SIZE, 
                       buf, sizeof(buf));
}

uint8_t readOCXOChannelConfigurationFromEEPROM_(OCXOChannel* ch) {
    if(ch == NULL) return 0;

    uint8_t buf[OCXO_CH_EEPROM_CHANNEL_SIZE] = {0};
    if(!readEEPROM(&hmain.eeprom, 
                   OCXO_CH_EEPROM_START_ADDRS + ch->id*OCXO_CH_EEPROM_CHANNEL_SIZE, 
                   sizeof(buf), buf)) {
        return 0;
    }

    memcpy(&ch->config, buf, sizeof(ch->config));

    // Validate the fields.
    return validateNumberArray(ch->config.freq) &&
           validateNumberArray(ch->config.duty) && 
           validateNumberArray(ch->config.phase);
}
