#include "MCP4531.h"

uint8_t initDigitalPot(MCP4531_DigitalPot* pot, I2C_HandleTypeDef* hi2c, uint8_t i2cAddrs) {
    if(pot == NULL || hi2c == NULL) {
        return 0;
    }

    pot->hi2c = hi2c;
    pot->i2cAddrs = i2cAddrs << 1;
    pot->initalized = 1;

    double dummy;
    pot->initalized = getVoltageDigitalPot(pot, &dummy);
    
    return pot->initalized;
}

uint8_t setVoltageDigitalPot(MCP4531_DigitalPot* pot, double voltage) {
    if(pot == NULL || !pot->initalized || 
       voltage > MCP4531_MAX_VOLTAGE_ALLOWED || voltage < MCP4531_MIN_VOLTAGE_ALLOWED) {
        return 0;
    }

    // The digital potentiometer acts as a voltage divider: Vout = Rb/(Ra+Rb)*Vin
    // - Ra+Rb = MCP4531_RESISTANCE
    // - Rb = MCP4531_RESISTANCE*N/MCP4531_STEPS, where N is the register value at 
    // MCP4531_REG_VOLATILE_WIPER.

    // Solving for N: Rb = Vout*MCP4531_RESISTANCE/Vin = MCP4531_RESISTANCE*N/MCP4531_STEPS
    // N = Vout*MCP4531_STEPS/Vin

    // Note: The wiper resistance is discarded because the voltage output is fed to a buffer, 
    // therefore not draining much current through the wiper.

    uint8_t n = voltage*MCP4531_STEPS/MCP4531_VCC;

    if(n == pot->currentPotValue){
        // Voltage is already set to this value.
        return 1;
    }

    // See Figure 7-2 of the chip's datasheet.
    uint8_t out[2];
    out[0] = (MCP4531_REG_VOLATILE_WIPER << 4) & 0xF0;
    out[1] = n;
    
    HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(pot->hi2c, pot->i2cAddrs, 
                            out, sizeof(out), 1000);

    if(st != HAL_OK) return 0;

    // Check that the voltage was set properly.
    double voltageRead;
    if(!getVoltageDigitalPot(pot, &voltageRead)) return 0;

    double voltageDiff = voltage - voltageRead;
    if(voltageDiff < 0) voltageDiff = -voltageDiff;

    return voltageDiff <= MCP4531_VOLTAGE_TOLERANCE;
}

uint8_t getVoltageDigitalPot(MCP4531_DigitalPot* pot, double* voltage) {
    if(pot == NULL || !pot->initalized || voltage == NULL) {
        return 0;
    }

    // See the Figure 7-5 of the datasheet.
    uint8_t read[2];
    HAL_StatusTypeDef st = HAL_I2C_Mem_Read(pot->hi2c, pot->i2cAddrs, 
        ((MCP4531_REG_VOLATILE_WIPER << 4) & 0xF0) | 0b1100, 
        I2C_MEMADD_SIZE_8BIT, read, sizeof(read), 1000);
    
    if(st != HAL_OK) return 0;

    pot->currentPotValue = read[1];

    *voltage = ((double) pot->currentPotValue) * MCP4531_VCC / ((double) MCP4531_STEPS);
    return 1;
}
