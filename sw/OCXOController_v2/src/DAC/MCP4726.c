#include "MCP4726.h"

// Default configuration to be set so that the chip uses it on reset.
MCP4726_Configuration defaultNVMConfig = {
    .vref = MCP4726_VREF_BUFFERED,
    .powerDown = MCP4726_POWER_DOWN_100k, // Start powered off.
    .gain = MCP4726_GAIN_1x,
    .dac = MCP4726_STEPS/2,
};

uint8_t initMCP4726_DAC(MCP4726_DAC* dac, I2C_HandleTypeDef* hi2c, uint8_t i2cAddrs) {
    if(dac == NULL || hi2c == NULL) {
        return 0;
    }

    dac->hi2c = hi2c;
    dac->i2cAddrs = i2cAddrs << 1;
    dac->initalized = 1;

    MCP4726_Configuration dummy;
    uint8_t status = getMCP7426NonVolatileConfig(dac, &dummy);
    status &= getMCP7426VolatileConfig(dac, &dummy);

    if(!areMCP7426ConfigurationsEqual_(dac->config_nv, defaultNVMConfig)) {
        // This sets both volatile and non volatile configurations.
        status &= setMCP7426Config(dac, defaultNVMConfig);
    }else if(!areMCP7426ConfigurationsEqual_(dac->config_v, defaultNVMConfig)) {
        // This only sets the volatile configuration.
        status &= setMCP7426VolatileConfig(dac, defaultNVMConfig);
    }
    
    dac->initalized = status;
    return status;
}

uint8_t setMCP4726DAC(MCP4726_DAC* dac, uint16_t out) {
    if(dac == NULL || out >= MCP4726_STEPS) {
        return 0;
    }

    // This function uses the command "Write Volatile DAC Register C2:C0=00x".

    uint8_t outBuf[2];
    outBuf[0] = (((uint8_t) MCP4726_NORMAL_OPERATION) << 4) | ((out >> 8) & 0b1111);
    outBuf[1] = out & 0xFF;

    uint8_t status = HAL_I2C_Master_Transmit(
        dac->hi2c, dac->i2cAddrs, outBuf, sizeof(outBuf), 1000) == HAL_OK;
    
    if(status) {
        // If the DAC value is to be set, then normal operation must be used.
        dac->config_v.powerDown = MCP4726_NORMAL_OPERATION;
        dac->config_v.dac = out;
    }

    // I could double check that the DAC value is being written, but it would take too much time of 
    // execution.

    return status;
}

uint8_t getMCP4726DAC(MCP4726_DAC* dac, uint16_t *out) {
    if(dac == NULL) {
        return 0;
    }

    MCP4726_ReadResult read;
    if(!readMCP7426Memory_(dac, &read)) return 0;

    *out = getDACFromMCP4726_ReadResult(read.data_nv_high, read.data_nv_low);
    dac->config_v.dac = *out;

    return 1;
}

uint8_t setMCP7426VolatileConfig(MCP4726_DAC* dac, MCP4726_Configuration config) {
    if(dac == NULL) {
        return 0;
    }

    if(areMCP7426ConfigurationsEqual_(config, dac->config_v)) {
        // Nothing more to do here.
        return 1;
    }

    // This function uses the command "Write Volatile Memory C2:C0=010".

    uint8_t out[3];
    out[0] = 0b01000000 | ((uint8_t) config.vref << 3) | 
                          ((uint8_t) config.powerDown << 1) |
                          ((uint8_t) config.gain);
    out[1] = (config.dac >> 4) & 0xFF;
    out[2] = (config.dac << 4) & 0xF0;

    uint8_t status = HAL_I2C_Master_Transmit(
        dac->hi2c, dac->i2cAddrs, out, sizeof(out), 1000) == HAL_OK;
    
    if(!status) return 0;

    MCP4726_Configuration readConfig;
    status &= getMCP7426VolatileConfig(dac, &readConfig);
    status &= areMCP7426ConfigurationsEqual_(config, readConfig);

    return status;
}

uint8_t setMCP7426Config(MCP4726_DAC* dac, MCP4726_Configuration config) {
    if(dac == NULL) {
        return 0;
    }

    // This function uses the command "Write All Memory C2:C0=011".
    // Its structure is the same as "Write Volatile Memory" but writes the same to both volatile and
    // non volatile memory.

    uint8_t out[3];
    out[0] = 0b01100000 | ((uint8_t) config.vref << 3) | 
                          ((uint8_t) config.powerDown << 1) |
                          ((uint8_t) config.gain);
    out[1] = (config.dac >> 4) & 0xFF;
    out[2] = (config.dac << 4) & 0xF0;

    uint8_t status = HAL_I2C_Master_Transmit(
        dac->hi2c, dac->i2cAddrs, out, sizeof(out), 1000) == HAL_OK;
    
    if(!status) return 0;

    // Wait 50 ms until the EEPROM is written.
    HAL_Delay(50);

    // Check that both volatile and non-volatile are written correctly. The getters also update the 
    // configurations of dac.
    MCP4726_Configuration readConfig;
    status &= getMCP7426NonVolatileConfig(dac, &readConfig);
    status &= areMCP7426ConfigurationsEqual_(config, readConfig);
    status &= getMCP7426VolatileConfig(dac, &readConfig);
    status &= areMCP7426ConfigurationsEqual_(config, readConfig);

    return status;
}

uint8_t getMCP7426VolatileConfig(MCP4726_DAC* dac, MCP4726_Configuration* config) {
    if(dac == NULL || config == NULL) {
        return 0;
    }

    MCP4726_ReadResult read;
    uint8_t status = readMCP7426Memory_(dac, &read);
    if(!status) return 0;

    config->vref        = read.vref_v;
    config->powerDown   = read.power_v;
    config->gain        = read.gain_v;
    config->dac         = getDACFromMCP4726_ReadResult(read.data_v_high, read.data_v_low);

    // Save the configuration in config_v.
    memcpy(&dac->config_v, config, sizeof(MCP4726_Configuration));

    return 1;
}

uint8_t getMCP7426NonVolatileConfig(MCP4726_DAC* dac, MCP4726_Configuration* config) {
    if(dac == NULL || config == NULL) {
        return 0;
    }

    MCP4726_ReadResult read;
    uint8_t status = readMCP7426Memory_(dac, &read);
    if(!status) return 0;

    config->vref        = read.vref_nv;
    config->powerDown   = read.power_nv;
    config->gain        = read.gain_nv;
    config->dac         = getDACFromMCP4726_ReadResult(read.data_nv_high, read.data_nv_low);

    // Save the configuration in config_v.
    memcpy(&dac->config_nv, config, sizeof(MCP4726_Configuration));

    return 1;
}

uint8_t readMCP7426Memory_(MCP4726_DAC* dac, MCP4726_ReadResult* out) {
    if(out == NULL) return 0;

    return HAL_I2C_Master_Receive(dac->hi2c, dac->i2cAddrs, 
            (uint8_t*) out, sizeof(MCP4726_ReadResult), 1000) == HAL_OK;
}

uint8_t areMCP7426ConfigurationsEqual_(
    MCP4726_Configuration config1, MCP4726_Configuration config2) {
    return (config1.vref == config2.vref) && 
           (config1.powerDown == config2.powerDown) &&
           (config1.gain == config2.gain) && 
           (config1.dac == config2.dac);
}

uint16_t getDACFromMCP4726_ReadResult(uint8_t high, uint8_t low) {
    return (((uint16_t) high) << 4) | (low >> 4);
}
