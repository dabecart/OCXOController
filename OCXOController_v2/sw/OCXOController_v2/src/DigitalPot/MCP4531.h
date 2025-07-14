#ifndef MCP4531_h
#define MCP4531_h

// MCP4531: 7-Bit Single I2C Digital POT with Volatile Memory from Microchip Technology.

#define MCP4531_RESISTANCE  10000 // 10 kOhm
#define MCP4531_WIPER_RES   75
#define MCP4531_VCC         5.0
#define MCP4531_STEPS       128

#define MCP4531_VOLTAGE_TOLERANCE   0.1
// The maximum and minimum voltage allowed is set by the electrical characteristics of the MCP4726 
// ref voltage. The Digital POT controls this Vref voltage.
#define MCP4531_MAX_VOLTAGE_ALLOWED 4.9
#define MCP4531_MIN_VOLTAGE_ALLOWED 0.1

#define MCP4531_REG_VOLATILE_WIPER  0x00
#define MCP4531_REG_VOLTATILE_TCON  0x04

#include "stm32g473xx.h"
#include "stm32g4xx_hal.h"

typedef struct MCP4531_DigitalPot {
    I2C_HandleTypeDef* hi2c;
    uint8_t i2cAddrs;
    uint8_t initalized;
    uint8_t currentPotValue;
} MCP4531_DigitalPot;

uint8_t initDigitalPot(MCP4531_DigitalPot* pot, I2C_HandleTypeDef* hi2c, uint8_t i2cAddrs);

uint8_t setVoltageDigitalPot(MCP4531_DigitalPot* pot, double voltage);

uint8_t getVoltageDigitalPot(MCP4531_DigitalPot* pot, double* voltage);

#endif // MCP4531_h