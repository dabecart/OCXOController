#ifndef GUI_h
#define GUI_h

#include "math.h"
#include "TFT/ST7735.h"
#include "TFT/Bitmaps.h"
#include "CORDIC/CORDIC.h"
#include "Defines.h"

#define GUI_CHECKERBOARD_COLOR0 switched_color565(255,0,0)
#define GUI_CHECKERBOARD_COLOR1 switched_color565(210,0,0)
#define GUI_CHECKERBOARD_COLOR2 switched_color565(160,0,0)

typedef enum GUIState {
    GUI_INITIALIZATION = 0,
    GUI_MAIN = 1,
} GUIState;

uint8_t initGUI(SPI_HandleTypeDef* hspi, DMA_HandleTypeDef* hdma_spi1_tx, 
                TIM_HandleTypeDef* guitim);
void updateGUI();
void setGUIState(GUIState state);

void transferScreenToTFT();
void transferToTFTEnded();

void homeScreen_();

void rippleDisplacement(uint16_t x, uint16_t y, uint16_t r, uint16_t* xout, uint16_t* yout);
uint16_t checkerboard(uint16_t x, uint16_t y, uint16_t color1, uint16_t color2);
uint16_t rainbowGradient(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void hsv2rgb(float H, float S, float V, uint8_t *r, uint8_t *g, uint8_t *b);

#endif // GUI_h