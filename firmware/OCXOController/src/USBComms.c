#include "USBComms.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

CircularBuffer rxBuffer;
uint8_t rxBufferArray[512];

void initUSBComms() {
    init_cb(&rxBuffer, rxBufferArray, sizeof(rxBufferArray));
}

uint8_t sendMessageUSB(uint8_t* message, uint32_t messageLength) {
    uint32_t start = HAL_GetTick();
    uint32_t elapsed = 0;
    while((CDC_Transmit_FS(message, messageLength)  == USBD_BUSY) && elapsed < 500) {
        elapsed = HAL_GetTick() - start;
    }
    return 1;
}

uint8_t readMessageUSB(uint8_t* message, uint32_t* messageLength) {
    uint8_t lastRead;
    for(uint32_t i = 0; i < rxBuffer.len; i++) {
        peekAt_cb(&rxBuffer, i, &lastRead);
        if(lastRead == '\n') {
            popN_cb(&rxBuffer, i, message);
            *messageLength = i;
            return 1;
        }
    }
    return 0;
}


// Defined in usbd_cdc_if.h and integrated in the CDC_Receive_FS handler of usb_cd_if.c.
void USB_RXHandler(uint8_t* buf, uint16_t len) {
    pushN_cb(&rxBuffer, buf, len);
}