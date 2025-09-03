#include "MainMCU.h"

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if(hi2c == hmain.gpio.hi2c) {
        gpioControllerDMA(&hmain.gpio);
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if(hspi == guiTFT.hspi) {
        transferToTFTEnded();
    }
}

