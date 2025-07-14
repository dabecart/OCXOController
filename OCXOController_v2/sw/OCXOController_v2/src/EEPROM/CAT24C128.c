#include "CAT24C128.h"

uint8_t initEEPROM(ExEEPROM* rom, I2C_HandleTypeDef* hi2c, uint8_t i2cAddr) {
    if(hi2c == NULL || rom == NULL) {
        return 0;
    }

    rom->hi2c = hi2c;
    rom->i2cAddrs = i2cAddr << 1;
    rom->connected = 1;

    // Check for the signature of the EEPROM.
    uint8_t buf[EEPROM_SIGNATURE_LEN];
    readEEPROM(rom, EEPROM_SIGNATURE_ADDRS, EEPROM_SIGNATURE_LEN, buf);
    if(strcmp((char*) buf, EEPROM_SIGNATURE) != 0) {
        // Signature is not correct. Sign the EEPROM.
        writeEEPROM(rom, 
            EEPROM_SIGNATURE_ADDRS, (uint8_t*) EEPROM_SIGNATURE, EEPROM_SIGNATURE_LEN);
        // Read it again.
        readEEPROM(rom, EEPROM_SIGNATURE_ADDRS, EEPROM_SIGNATURE_LEN, buf);
        // They should match, if not, set EEPROM as disconnected.
        rom->connected = strcmp((char*) buf, EEPROM_SIGNATURE) == 0;
    }

    return rom->connected;
}

uint8_t readEEPROM(ExEEPROM* rom, uint16_t dir, uint16_t len, uint8_t* buf) {
    if(rom == NULL || !rom->connected || buf == NULL || len == 0 || len > EEPROM_SIZE) {
       return 0;
    }

    HAL_StatusTypeDef status;
    // Set the starting position to read from. The direction has big endianism so shift the 
    // number.
    uint16_t usShiftedDirection = (dir >> 8) | (dir << 8);
    status = HAL_I2C_Master_Transmit(rom->hi2c, rom->i2cAddrs,
                                      (uint8_t*) &usShiftedDirection, sizeof(uint16_t), 1000);
    if(status != HAL_OK) return 0;

    // Contrary to writing to the EEPROM, here the device automatically changes pages whilst 
    // increasing its direction.
    status = HAL_I2C_Master_Receive(rom->hi2c, rom->i2cAddrs, buf, len, 1000);
    return status == HAL_OK;
}

uint8_t writeEEPROM(ExEEPROM* rom, uint16_t dir, const uint8_t* buf, uint16_t len) {
    if(rom == NULL || !rom->connected || buf == NULL || len == 0 || len > EEPROM_SIZE) {
        return 0;
    }
    
    uint8_t aucOutBuffer[EEPROM_PAGE_SIZE + 2];
    HAL_StatusTypeDef status = HAL_OK;
    uint16_t remainingBytesInPage = 0;
    while((len > 0) && (status == HAL_OK)) {
        // The starting position to write to has to be sent first. The direction has big endianism 
        // so shift the number's bytes.
        aucOutBuffer[0] = (dir >> 8) & 0xFF;
        aucOutBuffer[1] = dir & 0xFF;

        // A maximum of EEPROM_PAGE_SIZE bytes can be written at once. After that, a new direction 
        // has to be sent so that the EEPROM passes to the next page. 
        remainingBytesInPage = EEPROM_PAGE_SIZE - dir % EEPROM_PAGE_SIZE;
        if(remainingBytesInPage > len) {
            remainingBytesInPage = len;
        }

        memcpy(aucOutBuffer + 2, buf, remainingBytesInPage);

        status = HAL_I2C_Master_Transmit(rom->hi2c, rom->i2cAddrs,
                                           aucOutBuffer, remainingBytesInPage + 2, 1000);

        buf += remainingBytesInPage;
        dir += remainingBytesInPage;
        dir %= EEPROM_SIZE;
        len -= remainingBytesInPage;
    }
    return status == HAL_OK;
}
