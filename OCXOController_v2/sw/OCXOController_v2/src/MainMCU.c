#include "MainMCU.h"

MainHandlers hmain;

void initMain(I2C_HandleTypeDef* hi2c1, I2C_HandleTypeDef* hi2c3, SPI_HandleTypeDef* hspi1,
              TIM_HandleTypeDef* htim1, TIM_HandleTypeDef* htim2, TIM_HandleTypeDef* htim3, 
              TIM_HandleTypeDef* htim4, TIM_HandleTypeDef* htim5, TIM_HandleTypeDef* htim8, 
              TIM_HandleTypeDef* htim15, 
              UART_HandleTypeDef* huart2, 
              DMA_HandleTypeDef* hdma_usart2_rx, DMA_HandleTypeDef* hdma_usart2_tx) {

    hmain.hi2c1 = hi2c1; 
    hmain.hi2c3 = hi2c3; 
    hmain.hspi1 = hspi1; 
    hmain.htim1 = htim1; 
    hmain.htim2 = htim2; 
    hmain.htim3 = htim3; 
    hmain.htim4 = htim4; 
    hmain.htim5 = htim5; 
    hmain.htim8 = htim8; 
    hmain.htim15 = htim15; 
    hmain.huart2 = huart2; 
    hmain.hdma_usart2_rx = hdma_usart2_rx; 
    hmain.hdma_usart2_tx = hdma_usart2_tx; 

    uint8_t startupChecks = 1;

    // Configure the PD (Power Delivery) chip.
    #if !MCU_POWERED_EXTERNALLY
        startupChecks &= startSTUSB4500(hi2c3);
    #endif

    startupChecks &= initGUI(&hmain.gui, hspi1);

    startupChecks &= initEEPROM(&hmain.eeprom, hi2c3, I2C_ADD_EEPROM);

    startupChecks &= initDigitalPot(&hmain.pot, hi2c1, I2C_ADD_POT);
    startupChecks &= setVoltageDigitalPot(&hmain.pot, OCXO_MAX_VCO_VOLTAGE);

    startupChecks &= initMCP4726_DAC(&hmain.dac, hi2c1, I2C_ADD_DAC);

    startupChecks &= initGPIOController(&hmain.gpio, hi2c3);

    startupChecks &= initOCXOController(htim15, htim2, htim5);

    if(!startupChecks) {
      errorTrapMain();
    }
    
    // Turn on the LED.
    HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, 1);
    setVoltageLevel(&hmain.gpio, GPIO_PPS_REF_IN, VOLTAGE_LEVEL_3V3);
    setVoltageLevel(&hmain.gpio, GPIO_OCXO_OUT, VOLTAGE_LEVEL_3V3);

    HAL_TIM_OC_Start(htim1, TIM_CHANNEL_3);
}

void loopMain() {
    static uint8_t ocxoOn = 0;

    if(hmain.gpio.btn1.isClicked) {
        ocxoOn = !ocxoOn;
        powerOCXO(&hmain.gpio, ocxoOn);
    }
    if(hmain.gpio.btn2.isClicked) {
        setVoltageLevel(&hmain.gpio, GPIO_PPS_REF_OUT, VOLTAGE_LEVEL_5V);
    }
    if(hmain.gpio.btn3.isClicked) {
        setVoltageLevel(&hmain.gpio, GPIO_PPS_REF_OUT, VOLTAGE_LEVEL_3V3);
    }
    if(hmain.gpio.btn4.isClicked) {
        setVoltageLevel(&hmain.gpio, GPIO_PPS_REF_OUT, VOLTAGE_LEVEL_1V8);
    }
    
    loopOCXOCOntroller();

    updateGPIOController(&hmain.gpio);
}

void errorTrapMain() {
  for(;;) {
    HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, 0);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, 1);
    HAL_Delay(1000);
  }
}
