#ifndef GUI_h
#define GUI_h

#include "math.h"
#include "TFT/ST7735.h"
#include "TFT/Bitmaps.h"
#include "CORDIC/CORDIC.h"
#include "Defines.h"

#define GUI_REFRESH_PERIOD_ms 1000/GUI_FPS

typedef struct GUI {
    TFT tft;
} GUI;

uint8_t initGUI(GUI* gui, SPI_HandleTypeDef* hspi, DMA_HandleTypeDef* hdma_spi1_tx);

uint8_t updateGUI(GUI* gui);


void homeScreen_();
void drawScreen_();

void rippleDisplacement(uint16_t x, uint16_t y, float time, uint16_t* xout, uint16_t* yout);
uint16_t checkerboard(uint16_t x, uint16_t y, uint16_t color1, uint16_t color2);
uint16_t rainbowGradient(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void hsv2rgb(float H, float S, float V, uint8_t *r, uint8_t *g, uint8_t *b);

#endif // GUI_h