#include "MainMCU.h"

MainHandlers mainHandlers;

void initMain(I2C_HandleTypeDef* hi2c1, I2C_HandleTypeDef* hi2c3, SPI_HandleTypeDef* hspi1,
              TIM_HandleTypeDef* htim1, TIM_HandleTypeDef* htim2, TIM_HandleTypeDef* htim3, 
              TIM_HandleTypeDef* htim4, TIM_HandleTypeDef* htim5, TIM_HandleTypeDef* htim8, 
              TIM_HandleTypeDef* htim15, 
              UART_HandleTypeDef* huart2, 
              DMA_HandleTypeDef* hdma_usart2_rx, DMA_HandleTypeDef* hdma_usart2_tx) {

    mainHandlers.hi2c1 = hi2c1; 
    mainHandlers.hi2c3 = hi2c3; 
    mainHandlers.hspi1 = hspi1; 
    mainHandlers.htim1 = htim1; 
    mainHandlers.htim2 = htim2; 
    mainHandlers.htim3 = htim3; 
    mainHandlers.htim4 = htim4; 
    mainHandlers.htim5 = htim5; 
    mainHandlers.htim8 = htim8; 
    mainHandlers.htim15 = htim15; 
    mainHandlers.huart2 = huart2; 
    mainHandlers.hdma_usart2_rx = hdma_usart2_rx; 
    mainHandlers.hdma_usart2_tx = hdma_usart2_tx; 

    uint8_t startupChecks = 1;

    // Configure the PD (Power Delivery) chip.
    #if !MCU_POWERED_EXTERNALLY
        startupChecks &= startSTUSB4500(hi2c3);
    #endif

    startupChecks &= initGUI(&mainHandlers.gui, hspi1);

    startupChecks &= initGPIOController(&mainHandlers.gpio, hi2c3);

    startupChecks &= initOCXOController(htim15, htim2, htim5);

    if(!startupChecks) {
      errorTrapMain();
    }
    
    // Turn on the LED.
    HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, 1);
    setVoltageLevel(&mainHandlers.gpio, GPIO_PPS_REF_IN, VOLTAGE_LEVEL_3V3);
    setVoltageLevel(&mainHandlers.gpio, GPIO_OCXO_OUT, VOLTAGE_LEVEL_3V3);

    HAL_TIM_OC_Start(htim1, TIM_CHANNEL_3);
}

void loopMain() {
    static uint8_t ocxoOn = 0;

    if(mainHandlers.gpio.btn1.isClicked) {
        ocxoOn = !ocxoOn;
        powerOCXO(&mainHandlers.gpio, ocxoOn);
    }
    if(mainHandlers.gpio.btn2.isClicked) {
        setVoltageLevel(&mainHandlers.gpio, GPIO_PPS_REF_OUT, VOLTAGE_LEVEL_5V);
    }
    if(mainHandlers.gpio.btn3.isClicked) {
        setVoltageLevel(&mainHandlers.gpio, GPIO_PPS_REF_OUT, VOLTAGE_LEVEL_3V3);
    }
    if(mainHandlers.gpio.btn4.isClicked) {
        setVoltageLevel(&mainHandlers.gpio, GPIO_PPS_REF_OUT, VOLTAGE_LEVEL_1V8);
    }
    
    updateGPIOController(&mainHandlers.gpio);
}

void errorTrapMain() {
  for(;;) {
    HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, 0);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, 1);
    HAL_Delay(1000);
  }
}
