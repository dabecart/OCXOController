#include "MainMCU.h"

MainHandlers hmain;

void initMain(I2C_HandleTypeDef* hi2c1, I2C_HandleTypeDef* hi2c3, 
              SPI_HandleTypeDef* hspi1, DMA_HandleTypeDef* hdma_spi1_tx,
              TIM_HandleTypeDef* htim1, TIM_HandleTypeDef* htim2, TIM_HandleTypeDef* htim3, 
              TIM_HandleTypeDef* htim4, TIM_HandleTypeDef* htim5, TIM_HandleTypeDef* htim6, 
              TIM_HandleTypeDef* htim8, TIM_HandleTypeDef* htim15, 
              UART_HandleTypeDef* huart2, 
              DMA_HandleTypeDef* hdma_usart2_rx, DMA_HandleTypeDef* hdma_usart2_tx,
              CORDIC_HandleTypeDef* hcordic) {

    hmain.hi2c1 = hi2c1; 
    hmain.hi2c3 = hi2c3; 
    hmain.hspi1 = hspi1;
    hmain.hdma_spi1_tx = hdma_spi1_tx; 
    hmain.htim1 = htim1; 
    hmain.htim2 = htim2; 
    hmain.htim3 = htim3; 
    hmain.htim4 = htim4; 
    hmain.htim5 = htim5; 
    hmain.htim6 = htim6;
    hmain.htim8 = htim8; 
    hmain.htim15 = htim15; 
    hmain.huart2 = huart2; 
    hmain.hdma_usart2_rx = hdma_usart2_rx; 
    hmain.hdma_usart2_tx = hdma_usart2_tx; 
    hmain.hcordic = hcordic;

    initCORDIC(hcordic);

    uint8_t startupChecks = 1;

    startupChecks &= initGUI(hmain.hspi1, hmain.hdma_spi1_tx, hmain.htim6);
    // Once the GUI has started and its timer is working, set this to initialization. This will make
    // the "tick" of the STM32 be calculated from the IRQ of the GUI instead of the SysTick_Handler.
    hmain.doingInitialization = 1;
    hmain.initialized = 0;

    // Configure the PD (Power Delivery) chip.
    #if !MCU_POWERED_EXTERNALLY
        logMessage("USB-C...");
        HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);
        startupChecks &= startSTUSB4500(hi2c3);
        if(startupChecks) logMessage("USB-C OK");
        else{
            logMessage("USB-C ERROR");
            errorTrapMain();
        } 
        HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);
    #endif

    logMessage("EEPROM...");
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);
    startupChecks &= initEEPROM(&hmain.eeprom, hmain.hi2c3, I2C_ADD_EEPROM);
    if(startupChecks) logMessage("EEPROM OK");
    else{
        logMessage("EEPROM ERROR");
        errorTrapMain();
    } 
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);

    logMessage("DAC Vref...");
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);
    startupChecks &= initDigitalPot(&hmain.pot, hmain.hi2c1, I2C_ADD_POT);
    startupChecks &= setVoltageDigitalPot(&hmain.pot, OCXO_MAX_VCO_VOLTAGE);
    if(startupChecks) logMessage("DAC Vref OK");
    else{
        logMessage("DAC Vref ERROR");
        errorTrapMain();
    } 
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);

    logMessage("DAC...");
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);
    startupChecks &= initMCP4726_DAC(&hmain.dac, hmain.hi2c1, I2C_ADD_DAC);
    if(startupChecks) logMessage("DAC OK");
    else{
        logMessage("DAC ERROR");
        errorTrapMain();
    }
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);

    logMessage("GPIO...");
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);
    startupChecks &= initGPIOController(&hmain.gpio, hmain.hi2c3);
    if(startupChecks) logMessage("GPIO OK");
    else{
        logMessage("GPIO ERROR");
        errorTrapMain();
    } 
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);

    logMessage("OCXO...");
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);
    startupChecks &= initOCXOController(hmain.htim15, hmain.htim2, hmain.htim5);
    if(startupChecks) logMessage("OCXO OK");
    else{
        logMessage("OCXO ERROR");
        errorTrapMain();
    } 
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);

    logMessage("Channels...");
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);
    startupChecks &= initOCXOChannels(&hmain.chOuts);
    if(startupChecks) logMessage("Channels OK");
    else{
        logMessage("Channels ERROR");
        errorTrapMain();
    } 
    HAL_Delay(GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms);

    // Turn on the LED.
    HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, 1);

    setVoltageLevel(&hmain.gpio, GPIO_OCXO_OUT,     VOLTAGE_LEVEL_3V3);
    setVoltageLevel(&hmain.gpio, GPIO_PPS_REF_IN,   VOLTAGE_LEVEL_3V3);

    HAL_TIM_OC_Start(hmain.htim1, TIM_CHANNEL_3);

    logMessage("Enjoy!");

    updateGUIInIRQ = 0;
    requestScreenChange(SCREEN_MAIN);
    
    hmain.initialized = 1;
    hmain.doingInitialization = 0;
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

    updateGUI();

    // static uint32_t lastTimeHere = 0;
    // if((HAL_GetTick() - lastTimeHere) < 100) return;
    // lastTimeHere = HAL_GetTick();
    // updateGUI();
}

void errorTrapMain() {
  for(;;) {
    HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, 0);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, 1);
    HAL_Delay(1000);
  }
}
