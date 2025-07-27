#ifndef MAIN_MCU_h
#define MAIN_MCU_h

#include "stm32g473xx.h"
#include "stm32g4xx_hal.h"
#include "stdint.h"
#include "main.h"

#include "Defines.h"
#include "USB/STUSB4500.h"
#include "GPIOController.h"
#include "GUI.h"
#include "OCXOController.h"
#include "EEPROM/CAT24C128.h"
#include "DAC/MCP4726.h"
#include "DigitalPot/MCP4531.h"
#include "CORDIC/CORDIC.h"
#include "commons/Logs.h"
#include "OCXOChannels.h"

typedef struct MainHandlers {
    I2C_HandleTypeDef*  hi2c1; // OCXO I2C bus.
    I2C_HandleTypeDef*  hi2c3; // Peripherals I2C bus.
    SPI_HandleTypeDef*  hspi1;
    DMA_HandleTypeDef*  hdma_spi1_tx;
    TIM_HandleTypeDef*  htim1;
    TIM_HandleTypeDef*  htim2;
    TIM_HandleTypeDef*  htim3;  // OUT3
    TIM_HandleTypeDef*  htim4;  // OUT1
    TIM_HandleTypeDef*  htim5;
    TIM_HandleTypeDef*  htim6;  // GUI refresh interrupt.
    TIM_HandleTypeDef*  htim7;  // GPIO Controller polling interrupt.
    TIM_HandleTypeDef*  htim8;  // OUT2
    TIM_HandleTypeDef*  htim15;
    UART_HandleTypeDef* huart2;
    DMA_HandleTypeDef*  hdma_usart2_rx;
    DMA_HandleTypeDef*  hdma_usart2_tx;
    CORDIC_HandleTypeDef* hcordic;

    uint8_t doingInitialization;
    uint8_t initialized;
    uint8_t isReferenceSignalConnected;
    uint32_t lastReferenceSignalTime;

    GPIOController      gpio;
    ExEEPROM            eeprom;
    MCP4531_DigitalPot  pot;
    MCP4726_DAC         dac;
    OCXOChannels        chOuts;
} MainHandlers;

void initMain(I2C_HandleTypeDef* hi2c1, I2C_HandleTypeDef* hi2c3, 
              SPI_HandleTypeDef* hspi1, DMA_HandleTypeDef* hdma_spi1_tx,
              TIM_HandleTypeDef* htim1, TIM_HandleTypeDef* htim2, TIM_HandleTypeDef* htim3, 
              TIM_HandleTypeDef* htim4, TIM_HandleTypeDef* htim5, TIM_HandleTypeDef* htim6,
              TIM_HandleTypeDef* htim7, TIM_HandleTypeDef* htim8, TIM_HandleTypeDef* htim15, 
              UART_HandleTypeDef* huart2, 
              DMA_HandleTypeDef* hdma_usart2_rx, DMA_HandleTypeDef* hdma_usart2_tx,
              CORDIC_HandleTypeDef* hcordic);

void loopMain();

void errorTrapMain();

extern MainHandlers hmain;

#endif // MAIN_MCU_h