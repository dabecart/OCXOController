#ifndef DEFINES_h
#define DEFINES_h

// Do an initial check on the STUSB chip. If it fails, do not start the device.
#define MCU_POWERED_EXTERNALLY 1

// I2C Addresses.
#define I2C_ADD_USB_C               0b0101000
#define I2C_ADD_EEPROM              0b1010000
#define I2C_ADD_VOLTAGE_SELECTOR    0b0100001
#define I2C_ADD_LED_BUTTONS         0b0100000
#define I2C_ADD_TEMPERATURE         0b1110110

#define I2C_ADD_DAC                 0b1100011
#define I2C_ADD_POT                 0b0101110

#endif // DEFINES_h