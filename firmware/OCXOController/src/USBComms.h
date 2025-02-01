#ifndef USB_COMMS_h
#define USB_COMMS_h

#include "stm32g431xx.h"
#include "stm32g4xx_hal.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

#include "CircularBuffer.h"

void initUSBComms();

uint8_t sendMessageUSB(uint8_t* message, uint32_t messageLength);

uint8_t readMessageUSB(uint8_t* message, uint32_t* messageLength);

#endif // USB_COMMS_h