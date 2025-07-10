#ifndef ST7735_h
#define ST7735_h

#include "stm32g4xx_hal.h"
#include "fonts.h"

/****** TFT DEFINES ******/
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 160

#define DELAY 0x80

#define ST7735_MADCTL_MY  0x80
#define ST7735_MADCTL_MX  0x40
#define ST7735_MADCTL_MV  0x20
#define ST7735_MADCTL_ML  0x10
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH  0x04

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// Color definitions
#define	TFT_BLACK   0x0000
#define	TFT_BLUE    0x001F
#define	TFT_RED     0xF800
#define	TFT_GREEN   0x07E0
#define TFT_CYAN    0x07FF
#define TFT_MAGENTA 0xF81F
#define TFT_YELLOW  0xFFE0
#define TFT_WHITE   0xFFFF
#define color565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

typedef struct TFT {
    SPI_HandleTypeDef* hspi;
    int16_t cursor_x; // x location to start printing text
    int16_t cursor_y; // y location to start printing text
    uint8_t rotation; // Display rotation (0 thru 3)
    uint8_t colstart; // Some displays need this changed to offset
    uint8_t rowstart; // Some displays need this changed to offset
    uint8_t xstart;
    uint8_t ystart;

    int16_t width;    // Display width as modified by current rotation
    int16_t height;   // Display height as modified by current rotation
} TFT;

void initTFT(TFT* tft, SPI_HandleTypeDef* hspi, uint8_t rotation);
void setRotationTFT(TFT* tft, uint8_t m);
void drawPixelTFT(TFT* tft, uint16_t x, uint16_t y, uint16_t color);
void writeStringTFT(TFT* tft, uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor);
void fillRectangleTFT(TFT* tft, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void fillScreenTFT(TFT* tft, uint16_t color);
void drawImageTFT(TFT* tft, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data);
void invertColorsTFT(TFT* tft, uint8_t invert);

void selectTFT_(TFT* tft);
void unselectTFT_(TFT* tft);
void resetTFT_(TFT* tft);
void writeCommandTFT_(TFT* tft, uint8_t cmd);
void writeDataTFT_(TFT* tft, uint8_t* buff, size_t buff_size);
void displayInitTFT_(TFT* tft, const uint8_t *addr);
void setAddressWindowTFT_(TFT* tft, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void writeCharTFT_(TFT* tft, uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor);

#endif // ST7735_h

