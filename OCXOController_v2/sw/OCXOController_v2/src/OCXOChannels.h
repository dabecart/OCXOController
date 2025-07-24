#ifndef OCXO_CHANNELS_h
#define OCXO_CHANNELS_h

#include "GPIOController.h"

#define OCXO_CH_EEPROM_START_ADDRS 0x1000
#define OCXO_CH_EEPROM_CHANNEL_SIZE 64 // Bytes for each channel-

typedef struct OCXOChannelConfig {
    // User will set the input values as a string through the UI.
    char freq[8];          // xxx.xxx
    char freqUnits[4];
    char duty[8];          // xxx.xxx
    char dutyUnits[4];
    char phase[8];         // xxx.xxx
    char phaseUnits[4];
    char voltage[4];
} OCXOChannelConfig;

typedef struct OCXOChannel {
    uint8_t id;
    VCIO pin;
    uint8_t isOutputON;

    OCXOChannelConfig config;

    // From the user input, the numerical values will be calculated.
    float frequency;
    float dutyCycle;
    float phase_ns;
    VoltageLevel voltage;
} OCXOChannel;

typedef struct OCXOChannels {
    OCXOChannel ch1;
    OCXOChannel ch2;
    OCXOChannel ch3;
} OCXOChannels;

uint8_t initOCXOChannels(OCXOChannels* outs);
void initOCXOChannel_(OCXOChannel* out, uint8_t id, VCIO pin);

uint8_t applyOCXOOutputFromConfiguration(OCXOChannels* outs, uint8_t id);
uint8_t applyAllOCXOOutputsFromConfiguration(OCXOChannels* outs);

void getFrequencyString(OCXOChannel* ch, char* str, int16_t len);
void getPhaseString(OCXOChannel* ch, char* str, int16_t len);

uint8_t getOCXOOutputsFromID_(OCXOChannels* outs, uint8_t id, OCXOChannel** out);

uint8_t saveOCXOChannelConfigurationInEEPROM_(OCXOChannel* ch);
uint8_t readOCXOChannelConfigurationFromEEPROM_(OCXOChannel* ch);

extern const char* frequencyUnits[];
extern const int16_t frequencyUnitsLen;
extern const char* timeUnits[];
extern const int16_t timeUnitsLen;
extern const char* voltageTags[];
extern const int16_t voltageTagsLen;

#endif // OCXO_CHANNELS_h