#include "OCXOChannels.h"
#include "MainMCU.h"

uint8_t initOCXOChannels(OCXOChannels* outs) {
    if(outs == NULL) return 0;

    initOCXOChannel_(&outs->ch1, 1, GPIO_OUT1);
    initOCXOChannel_(&outs->ch2, 2, GPIO_OUT2);
    initOCXOChannel_(&outs->ch3, 3, GPIO_OUT3);

    return 1;
}

uint8_t initOCXOChannel_(OCXOChannel* out, uint8_t id, VCIO pin) {
    out->id = id;
    out->pin = pin;
    out->isOutputON = 0;

    // TODO: Read from EEPROM the previous configuration.

    out->voltage = VOLTAGE_LEVEL_5V;
    out->frequency = 1.0f;
    out->dutyCycle = 0.5;
    out->phase_ns = 0;

    return 1;
}

void setOCXOOutputVoltage(OCXOChannels* outs, uint8_t id, VoltageLevel voltage) {
    if(outs == NULL || !IS_VOLTAGE_LEVEL(voltage)) return;

    OCXOChannel* out;
    if(!getOCXOOutputsFromID_(outs, id, &out)) return;

    if(setVoltageLevel(&hmain.gpio, out->pin, voltage)) {
        out->voltage = voltage;
    }
}

void setOCXOOutputFrequency(OCXOChannels* outs, uint8_t id, float frequency) {
    if(outs == NULL) return;

    OCXOChannel* out;
    if(!getOCXOOutputsFromID_(outs, id, &out)) return;

    out->frequency = frequency;
}

void setOCXOOutputDutyCycle(OCXOChannels* outs, uint8_t id, float dutyCycle) {
    if(outs == NULL) return;

    OCXOChannel* out;
    if(!getOCXOOutputsFromID_(outs, id, &out)) return;

    out->dutyCycle = dutyCycle;
}

void setOCXOOutputPhase(OCXOChannels* outs, uint8_t id, int64_t phase_ns) {
    if(outs == NULL) return;

    OCXOChannel* out;
    if(!getOCXOOutputsFromID_(outs, id, &out)) return;

    out->phase_ns = phase_ns;
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

void convertFrequencyToString(char* str, int16_t len, float freq) {
    const char *units[] = {"nHz", "uHz", "mHz", " Hz", "kHz", "MHz", "GHz"};
    int index = 3;  // Initial index set to Hz.
    
    // Scale frequency up or down to fit in xxx.xxx range
    while ((freq >= 1000.0f) && (index < (sizeof(units)/sizeof(char*)-1))) {
        freq /= 1000.0f;
        index++;
    }
    while ((freq < 1.0f) && (index > 0)) {
        freq *= 1000.0f;
        index--;
    }

    snprintf(str, len, "%7.3f %-3s", freq, units[index]);
}

void convertPhaseToString(char* str, int16_t len, int64_t phase) {
    const char *units[] = {"ns", "us", "ms", "s"};
    int index = 0; 

    uint32_t scaled;
    uint64_t unsignedPhase;
    if(phase > 0) {
        unsignedPhase = phase;
        scaled = 1;
    }else {
        unsignedPhase = -phase;
        scaled = -1;
    }

    while((unsignedPhase >= 1000) && (index < (sizeof(units)/sizeof(char*)-1))) {
        unsignedPhase /= 1000;
        index++;
    }

    scaled *= (uint32_t) unsignedPhase;
    snprintf(str, len, "%c%ld%s", FONTS_DELTA, scaled, units[index]);
}
