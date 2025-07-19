#ifndef OCXO_CHANNELS_h
#define OCXO_CHANNELS_h

#include "GPIOController.h"

typedef struct OCXOChannel {
    uint8_t id;
    VCIO pin;
    uint8_t isOutputON;

    VoltageLevel voltage;
    float frequency;
    float dutyCycle;
    int64_t phase_ns;
} OCXOChannel;

typedef struct OCXOChannels {
    OCXOChannel ch1;
    OCXOChannel ch2;
    OCXOChannel ch3;
} OCXOChannels;

uint8_t initOCXOChannels(OCXOChannels* outs);

void setOCXOOutputVoltage(OCXOChannels* outs, uint8_t id, VoltageLevel voltage);
void setOCXOOutputFrequency(OCXOChannels* outs, uint8_t id, float frequency);
void setOCXOOutputDutyCycle(OCXOChannels* outs, uint8_t id, float dutyCycle);
void setOCXOOutputPhase(OCXOChannels* outs, uint8_t id, int64_t phase_ns);

void convertFrequencyToString(char* str, int16_t len, float freq);
void convertPhaseToString(char* str, int16_t len, int64_t phase);

uint8_t initOCXOChannel_(OCXOChannel* out, uint8_t id, VCIO pin);
uint8_t getOCXOOutputsFromID_(OCXOChannels* outs, uint8_t id, OCXOChannel** out);

#endif // OCXO_CHANNELS_h