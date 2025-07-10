#include "USBComms.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

CircularBuffer rxBuffer;
uint8_t rxBufferArray[512];
uint8_t isUSBConnected = 0;

void initUSBComms() {
    init_cb(&rxBuffer, rxBufferArray, sizeof(rxBufferArray));
}

uint8_t sendMessageUSB(uint8_t* message, uint32_t messageLength) {
    if(!isUSBConnected) return 0;
    
    if(messageLength == 0) return 1;

    uint32_t start = HAL_GetTick();
    uint32_t elapsed = 0;
    while((CDC_Transmit_FS(message, messageLength)  == USBD_BUSY) && elapsed < USB_TIMEOUT_ms) {
        elapsed = HAL_GetTick() - start;
    }

    // If there's a timeout while sending, disconnect the USB.
    if(elapsed >= USB_TIMEOUT_ms) {
        isUSBConnected = 0;
        return 0;
    }
    
    return 1;
}

uint8_t readMessageUSB(uint8_t* message, uint32_t* messageLength) {
    uint8_t lastRead = 0;
    for(uint32_t i = 0; i < rxBuffer.len; i++) {
        peekAt_cb(&rxBuffer, i, &lastRead);
        if(lastRead == '\n') {
            *messageLength = i + 1;
            popN_cb(&rxBuffer, *messageLength, message);
            return 1;
        }
    }
    return 0;
}

void setUSBConnected(uint8_t connected) {
    isUSBConnected = connected;
}

// Defined in usbd_cdc_if.h and integrated in the CDC_Receive_FS handler of usb_cd_if.c.
void USB_RXHandler(uint8_t* buf, uint16_t len) {
    pushN_cb(&rxBuffer, buf, len);
}
