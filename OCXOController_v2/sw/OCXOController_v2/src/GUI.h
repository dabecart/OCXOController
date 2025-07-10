#ifndef GUI_h
#define GUI_h

#include "TFT/ST7735.h"
#include "TFT/Bitmaps.h"

typedef struct GUI {
    TFT tft;
} GUI;

uint8_t initGUI(GUI* gui, SPI_HandleTypeDef* hspi);

uint8_t updateGUI(GUI* gui);

#endif // GUI_h