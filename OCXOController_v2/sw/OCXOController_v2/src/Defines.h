#ifndef DEFINES_h
#define DEFINES_h

// Do an initial check on the STUSB chip. If it fails, do not start the device.
#define MCU_POWERED_EXTERNALLY 1

#define PPS_TIMER_FREQ          170.0e6 // Hz
#define PPS_REF_FREQ            1.0     // Hz
#define OCXO_MAX_VCO_VOLTAGE    4.0     // Volts

// Number of previous edge times to be stored in memory (to calculate derivatives and integrals).
#define CONTROL_POINTS_IN_MEMORY 128
// Number of previous edge times to be stored in memory (to calculate current frequencies). They get
// deleted continuously.
#define CONTROL_CLOSE_POINTS_IN_MEMORY 4

// If the absolute value of the delta between the OCXO PPS and the reference PPS is less than this
// value, don't affect the VCO control voltage.  
// #define CONTROL_HYSTERESIS_ENABLED
#define CONTROL_HYSTERESIS 1e-6 // s

// Initial voltage generated for the VCO.
#define CONTROL_INITIAL_VCO 2048 // Start in the middle (the ADC is 12 bits: [0, 4096))

#define CONTROL_VCO_UPDATE_TIME_ms 10

// Depending on the voltage on the VCO pin of the OCXO, its frequency can vary +- this value.
#define OCXO_CONTROL_FREQUENCY_RANGE 7.0

// Take this number of frequency measurements to generate the min/max frequency range of the OCXO.
#define OCXO_CALIBRATION_MEASURE_COUNT 5
// Number of iterations to wait for the OCXO to stabilize with the new VCO voltage.
#define OCXO_CALIBRATION_STABILIZATION_COUNT 5
// Number of measurements to calculate the frequency during calibration.
#define OCXO_CALIBRATION_FREQUENCY_MEASUREMENTS 5

// I2C Addresses.
#define I2C_ADD_USB_C               0b0101000
#define I2C_ADD_EEPROM              0b1010000
#define I2C_ADD_VOLTAGE_SELECTOR    0b0100001
#define I2C_ADD_LED_BUTTONS         0b0100000
#define I2C_ADD_TEMPERATURE         0b1110110

#define I2C_ADD_DAC                 0b1100011
#define I2C_ADD_POT                 0b0101110

// External EEPROM.
#define EEPROM_SIGNATURE        "OCXOController, by @dabecart"
#define EEPROM_SIGNATURE_ADDRS  0
#define EEPROM_SIGNATURE_LEN    sizeof(EEPROM_SIGNATURE)

// GUI

// Based on the values set for TIM6.
#define GUI_FPS 10

#define GUI_INITIAL_SCREEN SCREEN_INTRO

#if GUI_INITIAL_SCREEN == SCREEN_INTRO
    // Let the logo be shown for this ammount of time.
    #define GUI_INITIAL_SCREEN_DELAY_ms 2000 
    // Leave a little time so that the startup messages can be read.
    #define GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms 300
#else 
    #define GUI_INITIAL_SCREEN_DELAY_ms 0 
    #define GUI_INTERVAL_BETWEEN_INITIALIZATIONS_ms 0
#endif

#endif // DEFINES_h