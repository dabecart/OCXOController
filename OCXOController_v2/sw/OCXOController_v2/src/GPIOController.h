#ifndef GPIO_CONTROLLER_h
#define GPIO_CONTROLLER_h

#include <string.h>
#include "GPIOExpander/TCA6416.h"
#include "Defines.h"

// If set, the GPIOs will be read individually, in other words, the GPIO expander will be read for
// each read of a GPIO's state.
#define GPIO_USE_INDIVIDUAL_READS 0

// Voltage Controlled IO
typedef enum VCIO {
    GPIO_OCXO_OUT = 1,
    GPIO_PPS_REF_IN,
    GPIO_PPS_REF_OUT,
    GPIO_OUT1,
    GPIO_OUT2,
    GPIO_OUT3,
} VCIO;

typedef enum VoltageLevel {
    VOLTAGE_LEVEL_OFF   = 0b00,
    VOLTAGE_LEVEL_5V    = 0b01,
    VOLTAGE_LEVEL_3V3   = 0b10,
    VOLTAGE_LEVEL_1V8   = 0b11,
} VoltageLevel;

#define IS_VOLTAGE_LEVEL(x) ((x == VOLTAGE_LEVEL_OFF)  || \
                             (x == VOLTAGE_LEVEL_5V)   || \
                             (x == VOLTAGE_LEVEL_3V3)  || \
                             (x == VOLTAGE_LEVEL_1V8))

typedef enum Button {
    BUTTON_1 = 1,
    BUTTON_2,
    BUTTON_3,
    BUTTON_4,
    BUTTON_ROT,
} Button;

typedef enum ButtonColor {
    BUTTON_COLOR_OFF    = 0b000,
    BUTTON_COLOR_BLUE   = 0b001,
    BUTTON_COLOR_GREEN  = 0b010,
    BUTTON_COLOR_CYAN   = 0b011,
    BUTTON_COLOR_RED    = 0b100,
    BUTTON_COLOR_PINK   = 0b101,
    BUTTON_COLOR_YELLOW = 0b110,
    BUTTON_COLOR_WHITE  = 0b111,
} ButtonColor;

typedef enum VoltageGPIO {
    GPIO_VOLT_OCXO_OUT_V2 = 0,
    GPIO_VOLT_OCXO_OUT_V1,
    GPIO_VOLT_PPS_REF_OUT_V2,
    GPIO_VOLT_PPS_REF_OUT_V1,
    GPIO_VOLT_OUT3_V2,
    GPIO_VOLT_OUT3_V1,
    GPIO_VOLT_OUT2_V2,
    GPIO_VOLT_OUT2_V1,
    GPIO_VOLT_OUT1_V2,
    GPIO_VOLT_OUT1_V1,
    GPIO_VOLT_PPS_REF_IN_V2,
    GPIO_VOLT_PPS_REF_IN_V1,
    GPIO_VOLT_ROT_BTN,
    GPIO_VOLT_ROT_A,
    GPIO_VOLT_ROT_B,
    GPIO_VOLT_OCXO_EN_,
} VoltageGPIO;

typedef enum ButtonsGPIO {
    GPIO_BTN_1_Blue = 0,
    GPIO_BTN_1_Green,
    GPIO_BTN_1_Red,
    GPIO_BTN_1,
    GPIO_BTN_2,
    GPIO_BTN_2_Red,
    GPIO_BTN_2_Green,
    GPIO_BTN_2_Blue,
    GPIO_BTN_3,
    GPIO_BTN_3_Blue,
    GPIO_BTN_3_Green,
    GPIO_BTN_3_Red,
    GPIO_BTN_4,
    GPIO_BTN_4_Blue,
    GPIO_BTN_4_Green,
    GPIO_BTN_4_Red,
} ButtonsGPIO;

typedef enum GPIOController_IRQStates {
    GPIO_IRQ_NOT_INITIALIZED = 0,
    GPIO_IRQ_IDLE,
    GPIO_IRQ_READING_VOLTAGES_GPIO,
    GPIO_IRQ_READING_BUTTONS_GPIO,
} GPIOController_IRQStates;

typedef struct ButtonData {
    Button btn;
    uint8_t isPressed;
    uint8_t isClicked;
} ButtonData;

typedef struct RotaryEncoder {
    GPIOExpander* gpio;
    uint8_t pinA, pinB;

    uint8_t previous;
    int8_t increment;
} RotaryEncoder;

typedef struct GPIOController {
    I2C_HandleTypeDef* hi2c;
    GPIOExpander voltagesGPIOs;
    GPIOExpander buttonGPIOs;
    uint8_t initialized;

    uint8_t (*getStateFunction)(GPIOExpander*, uint8_t, GPIOEx_State*);
    TIM_HandleTypeDef* htim;
    GPIOController_IRQStates irqState;

    ButtonData btn1;
    ButtonData btn2;
    ButtonData btn3;
    ButtonData btn4;
    ButtonData btnRot;

    RotaryEncoder rot;
} GPIOController;

uint8_t initGPIOController(GPIOController* hgpio, I2C_HandleTypeDef* i2cHandler);
uint8_t updateGPIOController(GPIOController* hgpio);

uint8_t addTimerAndDMAToGPIOController(GPIOController* hgpio, TIM_HandleTypeDef* htim);
void gpioControllerTimerIRQ(GPIOController* hgpio);
void gpioControllerDMA(GPIOController* hgpio);

uint8_t setVoltageLevel(GPIOController* hgpio, VCIO gpio, VoltageLevel voltage);
uint8_t getVoltageLevel(GPIOController* hgpio, VCIO gpio, VoltageLevel* voltage);

uint8_t setButtonColor(GPIOController* hgpio, Button btn, ButtonColor color);
uint8_t getButtonColor(GPIOController* hgpio, Button btn, ButtonColor* color);

uint8_t getButtonState_(GPIOController* hgpio, Button btn, GPIOEx_State* state);

uint8_t powerOCXO(GPIOController* hgpio, uint8_t powerOn);

uint8_t getV1V2Pins_(VCIO gpio, VoltageGPIO* v1Pin, VoltageGPIO* v2Pin);
uint8_t getV1V2States_(VoltageLevel voltage, GPIOEx_State* v1State, GPIOEx_State* v2State);
uint8_t getStateFromV1V2_(GPIOEx_State v1State, GPIOEx_State v2State, VoltageLevel* voltage);
uint8_t setGPIOControllerDirections_(GPIOController* hgpio);

uint8_t getRGBPins_(Button btn, ButtonsGPIO* rPin, ButtonsGPIO* gPin, ButtonsGPIO* bPin);
uint8_t getButtonPin_(GPIOController* hgpio, Button btn, GPIOExpander** gpioex, uint8_t* pin);

uint8_t initialAnimationGPIOController_(GPIOController* hgpio);

uint8_t initRotaryEncoder_(RotaryEncoder* rot, GPIOExpander* gpio, uint8_t pinA, uint8_t pinB);
void updateRotaryEncoder_(RotaryEncoder* rot, uint8_t (*getStateFunction)(GPIOExpander*, uint8_t, GPIOEx_State*));
int8_t getRotaryIncrement(RotaryEncoder* rot);

#endif // GPIO_CONTROLLER_h