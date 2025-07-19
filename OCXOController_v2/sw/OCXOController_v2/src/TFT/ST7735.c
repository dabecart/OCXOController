#include "ST7735.h"
#include "main.h"

const uint8_t
  init_cmds1[] = {            // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color

  init_cmds2[] = {            // Init for 7735R, part 2 (1.44" display)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F },           //     XEND = 127

  init_cmds3[] = {            // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      120                     //     120 ms delay
};

void initTFT(TFT* tft, SPI_HandleTypeDef* hspi, uint8_t rotation) {
    if(tft == NULL || hspi == NULL) return;

    tft->hspi = hspi;
    tft->hdma = NULL;

    selectTFT_(tft);
    resetTFT_(tft);
    displayInitTFT_(tft, init_cmds1);
    displayInitTFT_(tft, init_cmds2);
    displayInitTFT_(tft, init_cmds3);
    tft->colstart = 0;
    tft->rowstart = 0;
    setRotationTFT(tft, rotation);
    unselectTFT_(tft);
}

void setDMATFT(TFT* tft, DMA_HandleTypeDef* hdma) {
    if(tft == NULL || hdma == NULL) return;
    
    tft->hdma = hdma;
}

void setRotationTFT(TFT* tft, uint8_t m) {
    uint8_t madctl = 0;

    tft->rotation = m % 4; // can't be higher than 3

    switch (tft->rotation) {
        case 0:
            madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_RGB;
            tft->height = ST7735_HEIGHT;
            tft->width = ST7735_WIDTH;
            tft->xstart = tft->colstart;
            tft->ystart = tft->rowstart;
            break;

        case 1:
            madctl = ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_RGB;
            tft->width = ST7735_HEIGHT;
            tft->height = ST7735_WIDTH;
            tft->ystart = tft->colstart;
            tft->xstart = tft->rowstart;
            break;

        case 2:
            madctl = ST7735_MADCTL_RGB;
            tft->height = ST7735_HEIGHT;
            tft->width = ST7735_WIDTH;
            tft->xstart = tft->colstart;
            tft->ystart = tft->rowstart;
            break;
        
        case 3:
            madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_RGB;
            tft->width = ST7735_HEIGHT;
            tft->height = ST7735_WIDTH;
            tft->ystart = tft->colstart;
            tft->xstart = tft->rowstart;
            break;
    }

    selectTFT_(tft);
    writeCommandTFT_(tft, ST7735_MADCTL);
    writeDataTFT_(tft, &madctl,1);
    unselectTFT_(tft);
}

void invertColorsTFT(TFT* tft, uint8_t invert) {
    selectTFT_(tft);
    writeCommandTFT_(tft, invert ? ST7735_INVON : ST7735_INVOFF);
    unselectTFT_(tft);
}

void selectTFT_(TFT* tft) {
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET);
}

void unselectTFT_(TFT* tft) {
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET);
}

void resetTFT_(TFT* tft) {
    HAL_GPIO_WritePin(TFT_RESET_GPIO_Port, TFT_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(TFT_RESET_GPIO_Port, TFT_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
}

void writeCommandTFT_(TFT* tft, uint8_t cmd) {
    HAL_GPIO_WritePin(TFT_A0_GPIO_Port, TFT_A0_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(tft->hspi, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

void writeDataTFT_(TFT* tft, uint8_t* buff, size_t buff_size) {
    HAL_GPIO_WritePin(TFT_A0_GPIO_Port, TFT_A0_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(tft->hspi, buff, buff_size, HAL_MAX_DELAY);
}

void writeDataTFT_DMA_(TFT* tft, uint8_t* buff, size_t buff_size) {
    if(tft->hdma == NULL) return;

    HAL_GPIO_WritePin(TFT_A0_GPIO_Port, TFT_A0_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit_DMA(tft->hspi, buff, buff_size);

    // This disables an interruption that triggers when the transmission is done.
    // __HAL_DMA_DISABLE_IT(tft->hdma, DMA_IT_TC);
    // This disables an interruption that triggers when the buffer gets filled to half its size.
    __HAL_DMA_DISABLE_IT(tft->hdma, DMA_IT_HT);
}

uint16_t toColor565(uint8_t r, uint8_t g, uint8_t b) {
    return (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3));
}

uint16_t toColor565Reversed(uint8_t r, uint8_t g, uint8_t b) {
    return ((toColor565(r, g, b) << 8) | (toColor565(r, g, b) >> 8));
}

void displayInitTFT_(TFT* tft, const uint8_t *addr) {
    uint8_t numCommands, numArgs;
    uint16_t ms;

    numCommands = *addr++;
    while(numCommands--) {
        uint8_t cmd = *addr++;
        writeCommandTFT_(tft, cmd);

        numArgs = *addr++;
        // If high bit set, delay follows args
        ms = numArgs & DELAY;
        numArgs &= ~DELAY;
        if(numArgs) {
            writeDataTFT_(tft, (uint8_t*)addr, numArgs);
            addr += numArgs;
        }

        if(ms) {
            ms = *addr++;
            if(ms == 255) ms = 500;
            HAL_Delay(ms);
        }
    }
}

void setAddressWindowTFT_(TFT* tft, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    // Column address set
    writeCommandTFT_(tft, ST7735_CASET);
    uint8_t data[] = {0x00, (uint8_t)(x0 + tft->xstart), 0x00, (uint8_t)(x1 + tft->xstart)};
    writeDataTFT_(tft, data, sizeof(data));

    // Row address set
    writeCommandTFT_(tft, ST7735_RASET);
    data[1] = y0 + tft->ystart;
    data[3] = y1 + tft->ystart;
    writeDataTFT_(tft, data, sizeof(data));

    // Write to RAM
    writeCommandTFT_(tft, ST7735_RAMWR);
}
