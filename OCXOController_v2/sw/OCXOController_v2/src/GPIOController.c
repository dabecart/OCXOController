#include "GPIOController.h"

volatile GPIOController_IRQStates irqState;

uint8_t initGPIOController(GPIOController* gpioc, I2C_HandleTypeDef* i2cHandler) {
    if(gpioc == NULL || i2cHandler == NULL) return 0;
    
    gpioc->initialized = 1;
    gpioc->hi2c = i2cHandler;
    // If no timer or DMA is added, then the function to be used to get the state of the pin from 
    // the GPIO controller is the "pollin" one, without DMA.
#if GPIO_USE_INDIVIDUAL_READS
    gpioc->getStateFunction = getStateGPIOExpander;
#else 
    gpioc->getStateFunction = getStateGPIOExpanderFromPolling;
#endif

    gpioc->irqState = GPIO_IRQ_NOT_INITIALIZED;

    memset(&gpioc->btn1, 0, sizeof(ButtonData));
    memset(&gpioc->btn2, 0, sizeof(ButtonData));
    memset(&gpioc->btn3, 0, sizeof(ButtonData));
    memset(&gpioc->btn4, 0, sizeof(ButtonData));
    memset(&gpioc->btnRot, 0, sizeof(ButtonData));
    gpioc->btn1.btn = BUTTON_1;
    gpioc->btn2.btn = BUTTON_2;
    gpioc->btn3.btn = BUTTON_3;
    gpioc->btn4.btn = BUTTON_4;
    gpioc->btnRot.btn = BUTTON_ROT;

    uint8_t state = initGPIOExpander(&gpioc->buttonGPIOs, i2cHandler, I2C_ADD_LED_BUTTONS);
    state &= initGPIOExpander(&gpioc->voltagesGPIOs, i2cHandler, I2C_ADD_VOLTAGE_SELECTOR);

    // Set initial output values before setting the GPIOs as outputs.
    state &= setVoltageLevel(gpioc, GPIO_OCXO_OUT,      VOLTAGE_LEVEL_OFF);
    state &= setVoltageLevel(gpioc, GPIO_PPS_REF_IN,    VOLTAGE_LEVEL_OFF);
    state &= setVoltageLevel(gpioc, GPIO_PPS_REF_OUT,   VOLTAGE_LEVEL_OFF);
    state &= setVoltageLevel(gpioc, GPIO_OUT1,          VOLTAGE_LEVEL_OFF);
    state &= setVoltageLevel(gpioc, GPIO_OUT2,          VOLTAGE_LEVEL_OFF);
    state &= setVoltageLevel(gpioc, GPIO_OUT3,          VOLTAGE_LEVEL_OFF);
    state &= powerOCXO(gpioc, 0);

    state &= setButtonColor(gpioc, BUTTON_1, BUTTON_COLOR_OFF);
    state &= setButtonColor(gpioc, BUTTON_2, BUTTON_COLOR_OFF);
    state &= setButtonColor(gpioc, BUTTON_3, BUTTON_COLOR_OFF);
    state &= setButtonColor(gpioc, BUTTON_4, BUTTON_COLOR_OFF);

    // Set directions of the GPIOs.
    state &= setGPIOControllerDirections_(gpioc);

    // Init rotary encoder.
    state &= initRotaryEncoder_(&gpioc->rot, 
                               &gpioc->voltagesGPIOs, GPIO_VOLT_ROT_A, GPIO_VOLT_ROT_B);

    // Boot up animation of the colored buttons.
    state &= initialAnimationGPIOController_(gpioc);

    gpioc->initialized = state;
    return state;
}

uint8_t addTimerAndDMAToGPIOController(GPIOController* hgpio, TIM_HandleTypeDef* htim) {
    if(hgpio == NULL || htim == NULL) return 0;

    hgpio->htim = htim;
    hgpio->getStateFunction = getStateGPIOExpanderFromDMA;
    hgpio->irqState = GPIO_IRQ_IDLE;

    // The timer will trigger an IRQ which will read the I2C devices sequentially through DMA.
    HAL_TIM_Base_Start_IT(htim);

    return 1;
}

void gpioControllerTimerIRQ(GPIOController* hgpio) {
    if(hgpio == NULL || hgpio->irqState != GPIO_IRQ_IDLE) return;

    if(readGPIOExpanderRegisterDMA_(&hgpio->voltagesGPIOs)) {
        hgpio->irqState = GPIO_IRQ_READING_VOLTAGES_GPIO;
    }
}

void gpioControllerDMA(GPIOController* hgpio) {
    if(hgpio == NULL || hgpio->irqState == GPIO_IRQ_IDLE) return;
    
    switch (hgpio->irqState) {
        case GPIO_IRQ_READING_VOLTAGES_GPIO: {
            if(readGPIOExpanderRegisterDMA_(&hgpio->buttonGPIOs)) {
                hgpio->irqState = GPIO_IRQ_READING_BUTTONS_GPIO;
            }
            break;
        }
        
        case GPIO_IRQ_READING_BUTTONS_GPIO: {
            hgpio->irqState = GPIO_IRQ_IDLE;
            break;
        }

        default:    break;
    }
}

uint8_t updateGPIOController(GPIOController* hgpio) {
    if(hgpio == NULL || !hgpio->initialized) return 0;

#if !GPIO_USE_INDIVIDUAL_READS
    // Poll all input registers at once.
    readGPIOExpanderRegisterPolling_(&hgpio->voltagesGPIOs);
    readGPIOExpanderRegisterPolling_(&hgpio->buttonGPIOs);
#endif

    ButtonData* btns[] = {&hgpio->btn1, &hgpio->btn2, &hgpio->btn3, &hgpio->btn4, &hgpio->btnRot};
    
    GPIOEx_State newState;
    ButtonData* currentBtn;
    for(uint8_t i = 0; i < sizeof(btns)/sizeof(ButtonData*); i++) {
        currentBtn = btns[i];

        if(!getButtonState_(hgpio, currentBtn->btn, &newState)) continue;

        if(newState != currentBtn->isPressed) {
            if(newState == GPIOEx_HIGH) {
                currentBtn->isClicked = 1;

                currentBtn->wasClicked = 1;
                currentBtn->lastTimeClicked = HAL_GetTick();
            }else {
            	currentBtn->isClicked = 0;
            }
            currentBtn->isPressed = newState;
        }else {
            // A click only lasts one cycle. On the next call to updateGPIOController it gets 
            // "cleared".
            currentBtn->isClicked = 0;
        }
    }

    updateRotaryEncoder_(&hgpio->rot, hgpio->getStateFunction);

    return 1;
}

uint8_t setVoltageLevel(GPIOController* hgpio, VCIO gpio, VoltageLevel voltage) {
    if(hgpio == NULL || !hgpio->initialized) {
        return 0;
    }

    uint8_t v1Pin, v2Pin;
    if(!getV1V2Pins_(gpio, (VoltageGPIO*) &v1Pin, (VoltageGPIO*) &v2Pin)) {
        return 0;
    }

    uint8_t v1State, v2State;
    if(!getV1V2States_(voltage, (GPIOEx_State*) &v1State, (GPIOEx_State*) &v2State)) {
        return 0;
    }

    // Read the current output register.
    TCA6416Registers outputReg;
    if(v1Pin < 8) {
        outputReg = TCA6416_OUTPUT_PORT_0;
    }else {
        outputReg = TCA6416_OUTPUT_PORT_1;
        v1Pin -= 8;
        v2Pin -= 8;
    }

    uint8_t currentOutputReg = 0;
    if(!readGPIOExpanderRegister_(&hgpio->voltagesGPIOs, outputReg, &currentOutputReg)){
        return 0;
    }

    uint8_t mask = (1 << v1Pin) | (1 << v2Pin);
    uint8_t regs = (v1State << v1Pin) | (v2State << v2Pin);
    uint8_t newOutputReg = (currentOutputReg & ~mask) | regs;

    if(currentOutputReg == newOutputReg) {
        // Nothing new to do.
        return 1;
    }

    return writeGPIOExpanderRegister_(&hgpio->voltagesGPIOs, outputReg, newOutputReg);
}

uint8_t getVoltageLevel(GPIOController* hgpio, VCIO gpio, VoltageLevel* voltage) {
    if(hgpio == NULL || !hgpio->initialized) {
        return 0;
    }

    uint8_t v1Pin, v2Pin;
    if(!getV1V2Pins_(gpio, (VoltageGPIO*) &v1Pin, (VoltageGPIO*) &v2Pin)) {
        return 0;
    }

    // Read the current output register.
    TCA6416Registers outputReg;
    if(v1Pin < 8) {
        outputReg = TCA6416_OUTPUT_PORT_0;
    }else {
        outputReg = TCA6416_OUTPUT_PORT_1;
        v1Pin -= 8;
        v2Pin -= 8;
    }

    uint8_t currentOutputReg = 0;
    if(!readGPIOExpanderRegister_(&hgpio->voltagesGPIOs, outputReg, &currentOutputReg)){
        return 0;
    }

    uint8_t v1State = (currentOutputReg & (1 << v1Pin)) != 0;
    uint8_t v2State = (currentOutputReg & (1 << v2Pin)) != 0;
    return getStateFromV1V2_((GPIOEx_State) v1State, (GPIOEx_State) v2State, voltage);
}

uint8_t setButtonColor(GPIOController* hgpio, Button btn, ButtonColor color) {
    if(hgpio == NULL || !hgpio->initialized) {
        return 0;
    }

    uint8_t rPin, gPin, bPin;
    if(!getRGBPins_(btn, (ButtonsGPIO*) &rPin, (ButtonsGPIO*) &gPin, (ButtonsGPIO*) &bPin)) {
        return 0;
    }

    // Inverse logic is used to power the LEDs.
    uint8_t status = setStateGPIOExpander(&hgpio->buttonGPIOs, rPin, 
                         ((color>>2)&0x1) != 0 ? GPIOEx_LOW : GPIOEx_HIGH);
    status &= setStateGPIOExpander(&hgpio->buttonGPIOs, gPin, 
                         ((color>>1)&0x1) != 0 ? GPIOEx_LOW: GPIOEx_HIGH);
    status &= setStateGPIOExpander(&hgpio->buttonGPIOs, bPin, 
                         (color&0x1) != 0 ? GPIOEx_LOW : GPIOEx_HIGH);

    return status;
}

uint8_t getButtonColor(GPIOController* hgpio, Button btn, ButtonColor* color) {
    if(hgpio == NULL || !hgpio->initialized) {
        return 0;
    }

    uint8_t rPin, gPin, bPin;
    if(!getRGBPins_(btn, (ButtonsGPIO*) &rPin, (ButtonsGPIO*) &gPin, (ButtonsGPIO*) &bPin)) {
        return 0;
    }

    uint8_t rState, gState, bState;
    uint8_t status = hgpio->getStateFunction(&hgpio->buttonGPIOs, rPin, (GPIOEx_State*) &rState); 
    status &= hgpio->getStateFunction(&hgpio->buttonGPIOs, gPin, (GPIOEx_State*) &gState); 
    status &= hgpio->getStateFunction(&hgpio->buttonGPIOs, bPin, (GPIOEx_State*) &bState); 

    if(status){
        *color = (ButtonColor) ((rState << 2) | (gState << 1) | bState);
    }

    return status;
}

uint8_t wasButtonClicked(GPIOController* hgpio, Button btn) {
    if(hgpio == NULL) return 0;
    ButtonData* btnData;

    switch(btn) {
        case BUTTON_1: {
            btnData = &hgpio->btn1;
            break;
        }
        case BUTTON_2: {
            btnData = &hgpio->btn2;
            break;
        }
        case BUTTON_3: {
            btnData = &hgpio->btn3;
            break;
        }
        case BUTTON_4: {
            btnData = &hgpio->btn4;
            break;
        }
        case BUTTON_ROT: {
            btnData = &hgpio->btnRot;
            break;
        }
    }

    uint8_t ret = 0;
    if(btnData->wasClicked && 
       (HAL_GetTick() - btnData->lastTimeClicked) <= GPIO_WAS_CLICKED_TOLERANCE_ms) {
        ret = 1;
    }

    btnData->wasClicked = 0;
    return ret;
}

uint8_t getButtonState_(GPIOController* hgpio, Button btn, GPIOEx_State* state) {
    GPIOExpander* gpioex;
    uint8_t pin;
    if(!getButtonPin_(hgpio, btn, &gpioex, &pin)) {
        return 0;
    }

    return hgpio->getStateFunction(gpioex, pin, state);
}

uint8_t powerOCXO(GPIOController* hgpio, uint8_t powerOn) {
    if(hgpio == NULL || !hgpio->initialized) {
        return 0;
    }
    
    // Inverse logic is used to power the OCXO.
    GPIOEx_State state = powerOn ? GPIOEx_LOW : GPIOEx_HIGH;
    return setStateGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_OCXO_EN_, state);
}

uint8_t getV1V2Pins_(VCIO gpio, VoltageGPIO* v1Pin, VoltageGPIO* v2Pin) {
    switch (gpio) {
        case GPIO_OCXO_OUT: {
            *v1Pin = GPIO_VOLT_OCXO_OUT_V1;
            *v2Pin = GPIO_VOLT_OCXO_OUT_V2;
            break;
        }

        case GPIO_PPS_REF_IN: {
            *v1Pin = GPIO_VOLT_PPS_REF_IN_V1;
            *v2Pin = GPIO_VOLT_PPS_REF_IN_V2;
            break;
        }

        case GPIO_PPS_REF_OUT: {
            *v1Pin = GPIO_VOLT_PPS_REF_OUT_V1;
            *v2Pin = GPIO_VOLT_PPS_REF_OUT_V2;
            break;
        }

        case GPIO_OUT1: {
            *v1Pin = GPIO_VOLT_OUT1_V1;
            *v2Pin = GPIO_VOLT_OUT1_V2;
            break;
        }

        case GPIO_OUT2: {
            *v1Pin = GPIO_VOLT_OUT2_V1;
            *v2Pin = GPIO_VOLT_OUT2_V2;
            break;
        }

        case GPIO_OUT3: {
            *v1Pin = GPIO_VOLT_OUT3_V1;
            *v2Pin = GPIO_VOLT_OUT3_V2;
            break;
        }    

        default:
            // gpio is not on the VCIO enum.
            return 0;
    }
    return 1;
}

uint8_t getV1V2States_(VoltageLevel voltage, GPIOEx_State* v1State, GPIOEx_State* v2State) {
    switch(voltage) {
        case VOLTAGE_LEVEL_OFF: {
            *v1State = GPIOEx_LOW;
            *v2State = GPIOEx_LOW;
            break;
        }

        case VOLTAGE_LEVEL_5V: {
            *v1State = GPIOEx_HIGH;
            *v2State = GPIOEx_LOW;
            break;
        }

        case VOLTAGE_LEVEL_3V3: {
            *v1State = GPIOEx_LOW;
            *v2State = GPIOEx_HIGH;
            break;
        }

        case VOLTAGE_LEVEL_1V8: {
            *v1State = GPIOEx_HIGH;
            *v2State = GPIOEx_HIGH;
            break;
        }

        default:
            return 0;
    }
    return 1;
}

uint8_t getStateFromV1V2_(GPIOEx_State v1State, GPIOEx_State v2State, VoltageLevel* voltage) {
    uint8_t volt = (v2State << 1) | v1State;
    switch(volt) {
        case VOLTAGE_LEVEL_OFF: {
            *voltage = VOLTAGE_LEVEL_OFF;
            break;
        }

        case VOLTAGE_LEVEL_5V: {
            *voltage = VOLTAGE_LEVEL_5V;
            break;
        }

        case VOLTAGE_LEVEL_3V3: {
            *voltage = VOLTAGE_LEVEL_3V3;
            break;
        }

        case VOLTAGE_LEVEL_1V8: {
            *voltage = VOLTAGE_LEVEL_1V8;
            break;
        }

        default:
            return 0;
    }
    return 1;
}

uint8_t setGPIOControllerDirections_(GPIOController* hgpio) {
    uint8_t state = 
             setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_OCXO_OUT_V2,     GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_OCXO_OUT_V1,     GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_PPS_REF_OUT_V2,  GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_PPS_REF_OUT_V1,  GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_OUT3_V2,         GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_OUT3_V1,         GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_OUT2_V2,         GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_OUT2_V1,         GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_OUT1_V2,         GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_OUT1_V1,         GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_PPS_REF_IN_V2,   GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_PPS_REF_IN_V1,   GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_ROT_BTN,         GPIOEx_Input);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_ROT_A,           GPIOEx_Input);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_ROT_B,           GPIOEx_Input);
    state &= setDirectionGPIOExpander(&hgpio->voltagesGPIOs, (uint8_t) GPIO_VOLT_OCXO_EN_,        GPIOEx_Output);

    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_1_Blue,             GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_1_Green,            GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_1_Red,              GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_1,                  GPIOEx_Input);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_2,                  GPIOEx_Input);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_2_Red,              GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_2_Green,            GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_2_Blue,             GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_3,                  GPIOEx_Input);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_3_Blue,             GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_3_Green,            GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_3_Red,              GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_4,                  GPIOEx_Input);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_4_Blue,             GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_4_Green,            GPIOEx_Output);
    state &= setDirectionGPIOExpander(&hgpio->buttonGPIOs, (uint8_t) GPIO_BTN_4_Red,              GPIOEx_Output);
    return state;
}

uint8_t getRGBPins_(Button btn, ButtonsGPIO* rPin, ButtonsGPIO* gPin, ButtonsGPIO* bPin) {
    switch (btn) {
        case BUTTON_1: {
            *rPin = GPIO_BTN_1_Red;
            *gPin = GPIO_BTN_1_Green;
            *bPin = GPIO_BTN_1_Blue;
            break;
        }

        case BUTTON_2: {
            *rPin = GPIO_BTN_2_Red;
            *gPin = GPIO_BTN_2_Green;
            *bPin = GPIO_BTN_2_Blue;
            break;
        }

        case BUTTON_3: {
            *rPin = GPIO_BTN_3_Red;
            *gPin = GPIO_BTN_3_Green;
            *bPin = GPIO_BTN_3_Blue;
            break;
        }

        case BUTTON_4: {
            *rPin = GPIO_BTN_4_Red;
            *gPin = GPIO_BTN_4_Green;
            *bPin = GPIO_BTN_4_Blue;
            break;
        }

        default:
            return 0;
    }
    return 1;
}

uint8_t getButtonPin_(GPIOController* hgpio, Button btn, GPIOExpander** gpioex, uint8_t* pin) {
    switch (btn) {
        case BUTTON_1: {
            *gpioex = &hgpio->buttonGPIOs;
            *pin = GPIO_BTN_1;
            break;
        }

        case BUTTON_2: {
            *gpioex = &hgpio->buttonGPIOs;
            *pin = GPIO_BTN_2;
            break;
        }

        case BUTTON_3: {
            *gpioex = &hgpio->buttonGPIOs;
            *pin = GPIO_BTN_3;
            break;
        }

        case BUTTON_4: {
            *gpioex = &hgpio->buttonGPIOs;
            *pin = GPIO_BTN_4;
            break;
        }

        case BUTTON_ROT: {
            *gpioex = &hgpio->voltagesGPIOs;
            *pin = GPIO_VOLT_ROT_BTN;
            break;
        }

        default:
            return 0;
    }
    return 1;
}

uint8_t initialAnimationGPIOController_(GPIOController* hgpio) {
    const ButtonColor colors[] = {
        BUTTON_COLOR_RED,
        BUTTON_COLOR_GREEN,
        BUTTON_COLOR_BLUE,
        BUTTON_COLOR_WHITE,
        BUTTON_COLOR_OFF,
    };

    uint8_t state = 1;
    for(uint8_t button = BUTTON_1; button <= BUTTON_4; button++) {
        for(uint8_t colorIndex = 0; colorIndex < sizeof(colors)/sizeof(ButtonColor); colorIndex++) {
            state &= setButtonColor(hgpio, (Button) button, colors[colorIndex]);
            HAL_Delay(80); // This delay is "stretched" by the initial GUI animation IRQ.
        }
    }
    return state;
}

uint8_t initRotaryEncoder_(RotaryEncoder* rot, GPIOExpander* gpio, uint8_t pinA, uint8_t pinB) {
    if(rot == NULL || gpio == NULL) return 0;

    rot->gpio = gpio;
    rot->pinA = pinA;
    rot->pinB = pinB;
    rot->previous = 0;

    return 1;
}

void updateRotaryEncoder_(RotaryEncoder* rot, uint8_t (*getStateFunction)(GPIOExpander*, uint8_t, GPIOEx_State*)) {
    // Enter this table as [current][previous].
    const int8_t incrementArray[4][4] = {
        { 0, +1, -1,  0},
        {-1,  0,  0, +1},
        {+1,  0,  0, -1},
        { 0, -1, +1,  0},
    };

    GPIOEx_State stateA, stateB;
    getStateFunction(rot->gpio, rot->pinA, &stateA);
    getStateFunction(rot->gpio, rot->pinB, &stateB);

    uint8_t current = (stateA << 1) | stateB;

    rot->increment += incrementArray[current][rot->previous];
    
    rot->previous = current;
}

int8_t getRotaryIncrement(RotaryEncoder* rot) {
    int8_t currentIncrement = rot->increment;
    // Restart the increment after reading the increment.
    rot->increment = 0;
    return currentIncrement;
}

int8_t getFilteredRotaryIncrement(RotaryEncoder* rot) {
    static uint32_t lastIncrementTime = 0;
    uint32_t t = HAL_GetTick();

    // Divide by 2 so that only movements which are "fast" (> 2) register.
    int8_t incr = getRotaryIncrement(rot) / 2;
    if((incr == 0) || ((t - lastIncrementTime) < 100)) return 0;
    lastIncrementTime = HAL_GetTick();
    
    // Even if the incr is big, it will be "cut" to only single increments. If multiple increments 
    // are to be done, then keep moving the encoder for at least 100ms more. 
    if(incr > 0) incr = 1;
    else if(incr < 0) incr = -1;
    
    return incr;
}
