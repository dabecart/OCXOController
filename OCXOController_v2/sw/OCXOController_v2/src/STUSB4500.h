#ifndef STUSB4500_h
#define STUSB4500_h

#include "stm32g473xx.h"
#include "stm32g4xx_hal.h"
#include "stdint.h"
#include "main.h"

#define STUSB4500_ADDR (0x28 << 1) 
#define STUSB4500_VOLTAGE_V 5.0f
#define STUSB4500_CURRENT_A 1.5f

#define STUSB_SECTOR_SIZE 8 
#define STUSB_SECTOR_COUNT 5

int startSTUSB4500(I2C_HandleTypeDef* hI2C);

int configureNVM();

int checkNVMSectors();

int readSector(uint8_t sector, uint8_t *data);

int writeSector(uint8_t sector, uint8_t *data);

int writeI2C(uint8_t reg, uint8_t *data, uint8_t len);

int writeByte(uint8_t reg, uint8_t data);

int readI2C(uint8_t reg, uint8_t len, uint8_t *data);

#endif // STUSB4500_h
