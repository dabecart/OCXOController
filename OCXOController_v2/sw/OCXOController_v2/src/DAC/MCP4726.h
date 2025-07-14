#ifndef MCP4726_h
#define MCP4726_h

// MCP4726: 12-Bit Voltage Output Digital-to-Analog Converter with EEPROM and I2C Interface from 
// Microchip Technology.

#include <string.h>
#include "stm32g473xx.h"
#include "stm32g4xx_hal.h"
#include "cmsis_gcc.h"

#define MCP4726_STEPS 4096

typedef enum MCP4726_VREF {
    MCP4726_VREF_VDD        = 0b00, // Reference voltage is taken from Vdd.
    MCP4726_VREF_UNBUFFERED = 0b10, // Reference voltage is taken from Vref, without buffering.
    MCP4726_VREF_BUFFERED   = 0b11, // Reference voltage is taken from the buffered Vref.
} MCP4726_VREF;

typedef enum MCP4726_PowerDown {
    MCP4726_NORMAL_OPERATION  = 0b00, // Normal operation.
    MCP4726_POWER_DOWN_1k     = 0b01, // Output is set to 1k pull-down.
    MCP4726_POWER_DOWN_100k   = 0b10, // Output is set to 100k pull-down.
    MCP4726_POWER_DOWN_500k   = 0b11, // Output is set to 500k pull-down.
} MCP4726_PowerDown;

typedef enum MCP4726_Gain {
    MCP4726_GAIN_1x = 0,
    MCP4726_GAIN_2x = 1,
} MCP4726_Gain;

typedef struct MCP4726_Configuration {
    MCP4726_VREF        vref;
    MCP4726_PowerDown   powerDown;
    MCP4726_Gain        gain;
    uint16_t            dac;
} MCP4726_Configuration;

typedef struct MCP4726_DAC {
    I2C_HandleTypeDef* hi2c;
    uint8_t i2cAddrs;
    uint8_t initalized;

    MCP4726_Configuration config_v;   // Chip's configuration bits (stored in volatile memory).
    MCP4726_Configuration config_nv;   // Chip's configuration bits (stored in non volatile memory).
} MCP4726_DAC;

// This struct may look to be reversed if compared with the Figure 6-5 from the datasheet.
// Take into account that I2C bytes are stored as they come, from MSB to LSB, but on a 
// MCP4726_ReadResult are stored as LSB to MSB becuase of the bitfields.
typedef struct __attribute__((__packed__)) MCP4726_ReadResult {
    // Volatile configuration.
    MCP4726_Gain gain_v:        1;
    MCP4726_PowerDown power_v:  2;
    MCP4726_VREF vref_v:        2;
    // Reserved.
    uint8_t rsv1:               1;
    // Status bits.
    uint8_t por:                1;
    uint8_t rdy:                1;

    // Contains the current DAC value.
    uint8_t data_v_high;
    uint8_t data_v_low;

    // Non-Volatile configuration.
    MCP4726_Gain gain_nv:       1;
    MCP4726_PowerDown power_nv: 2;
    MCP4726_VREF vref_nv:       2;
    uint8_t rsv2:               3;

    // Contains the non volatile DAC value.
    uint8_t data_nv_high;
    uint8_t data_nv_low;
} MCP4726_ReadResult;

uint8_t initMCP4726_DAC(MCP4726_DAC* dac, I2C_HandleTypeDef* hi2c, uint8_t i2cAddrs);

uint8_t setMCP4726DAC(MCP4726_DAC* dac, uint16_t out);
uint8_t getMCP4726DAC(MCP4726_DAC* dac, uint16_t *out);

uint8_t setMCP7426VolatileConfig(MCP4726_DAC* dac, MCP4726_Configuration config);
uint8_t setMCP7426Config(MCP4726_DAC* dac, MCP4726_Configuration config);
uint8_t getMCP7426VolatileConfig(MCP4726_DAC* dac, MCP4726_Configuration* config);
uint8_t getMCP7426NonVolatileConfig(MCP4726_DAC* dac, MCP4726_Configuration* config);

uint8_t areMCP7426ConfigurationsEqual_(
    MCP4726_Configuration config1, MCP4726_Configuration config2);
uint8_t readMCP7426Memory_(MCP4726_DAC* dac, MCP4726_ReadResult* out);
uint16_t getDACFromMCP4726_ReadResult(uint8_t high, uint8_t low);

#endif // MCP4726_h