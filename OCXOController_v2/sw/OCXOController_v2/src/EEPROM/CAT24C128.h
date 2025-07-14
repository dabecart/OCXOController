#ifndef CAT24C128_h
#define CAT24C128_h

// 128Kb I2C EEPROM "CAT24C128" from ON Semiconductors. 

#include <string.h>
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_i2c.h"
#include "Defines.h"

#define EEPROM_SIZE 16384               // bytes            (0x4000)
#define EEPROM_PAGE_SIZE 64             // bytes per page   (0x40)

typedef struct ExEEPROM{
    I2C_HandleTypeDef*  hi2c;
    uint8_t             i2cAddrs;
    uint8_t             connected;
} ExEEPROM;

/**
 * @brief Initializes the EEPROM and sets an starting memory position.
 * 
 * @param rom. The structure where EEPROM access information will be stored.
 * @param hi2c. The handler to do the I2C communications.
 * @param i2cAddr. The ID of the EEPROM in I2C.
 * @return 1 if the initial communications with the EEPROM could be properly done.
 */
 uint8_t initEEPROM(ExEEPROM* rom, I2C_HandleTypeDef* hi2c, uint8_t i2cAddr);

/**
 * @brief Read an specified number of bytes from certain direction from the EEPROM. If the end of 
 * the EEPROM is reached, the direction wraps-around and continues reading from the start.
 * 
 * @param rom. Pointer to the EEPROM.
 * @param dir. The direction on the EEPROM to start reading from. Max value is 16383.
 * @param len. Number of bytes to read.
 * @param buf. Pointer to a buffer where the read bytes will be stored.
 * @return 1 if the reading of the EEPROM was successful.
 */
uint8_t readEEPROM(ExEEPROM* rom, 
                   uint16_t dir, uint16_t len, uint8_t* buf);

/**
 * @brief Write an specified number of bytes starting from a certain direction from the EEPROM. If 
 * the end of the EEPROM is reached, the direction wraps-around and continues reading from the start
 * memory direction.
 * 
 * @param rom. Pointer to the EEPROM.
 * @param dir. The direction on the EEPROM to start writing to. Max value is 16383.
 * @param len. Number of bytes to write.
 * @param buf. Pointer to a buffer where the bytes to be written are stored.
 * @return 1 if the writing to the EEPROM was successful.
 */
 uint8_t writeEEPROM(ExEEPROM* rom, 
                    uint16_t dir, const uint8_t* buf, uint16_t len);

#endif // CAT24C128_h