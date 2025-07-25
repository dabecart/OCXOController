#include "OCXOChannels.h"
#include "MainMCU.h"

const char* frequencyUnits[] = {"nHz", "uHz", "mHz", " Hz", "kHz", "MHz", "GHz"};
const int16_t frequencyUnitsLen = sizeof(frequencyUnits)/sizeof(char*);
const char* timeUnits[] = {"ns", "us", "ms", "s "};
const int16_t timeUnitsLen = sizeof(timeUnits)/sizeof(char*);
const char* voltageTags[] = {"5V", "3V3", "1V8"};
const int16_t voltageTagsLen = sizeof(voltageTags)/sizeof(char*);

uint8_t initOCXOChannels(OCXOChannels* outs) {
    if(outs == NULL) return 0;

    initOCXOChannel_(&outs->ch1, 1, GPIO_OUT1);
    initOCXOChannel_(&outs->ch2, 2, GPIO_OUT2);
    initOCXOChannel_(&outs->ch3, 3, GPIO_OUT3);

    return applyAllOCXOOutputsFromConfiguration(outs);
}

void initOCXOChannel_(OCXOChannel* out, uint8_t id, VCIO pin) {
    out->id = id;
    out->pin = pin;
    out->isOutputON = 0;

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
    if(strcmp(out->config.voltage, "5V") == 0)          out->voltage = VOLTAGE_LEVEL_5V;
    else if(strcmp(out->config.voltage, "3V3") == 0)    out->voltage = VOLTAGE_LEVEL_3V3;
    else if(strcmp(out->config.voltage, "1V8") == 0)    out->voltage = VOLTAGE_LEVEL_1V8;

    // If everything went OK set the real values and save in EEPROM.
    out->frequency  = desiredFrequency;
    out->dutyCycle  = desiredDutyCycle;
    out->phase_ns   = desiredPhase_ns;
    out->voltage    = desiredVoltage;

    saveOCXOChannelConfigurationInEEPROM_(out);

    return 1;
}

uint8_t applyAllOCXOOutputsFromConfiguration(OCXOChannels* outs) {
    uint8_t ret = applyOCXOOutputFromConfiguration(outs, outs->ch1.id);
    ret &= applyOCXOOutputFromConfiguration(outs, outs->ch2.id);
    ret &= applyOCXOOutputFromConfiguration(outs, outs->ch3.id);
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
