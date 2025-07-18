#ifndef GUI_h
#define GUI_h

#include "math.h"
#include "stdlib.h"

#include "Defines.h"
#include "TFT/ST7735.h"
#include "GUI/Screen.h"
#include "GUI/Overlay.h"

uint8_t initGUI(SPI_HandleTypeDef* hspi, DMA_HandleTypeDef* hdma_spi1_tx, 
                TIM_HandleTypeDef* guitim);

void updateGUI();

void requestScreenChange(ScreenID nextScreen);

void transferScreenToTFT();
void transferToTFTEnded();

extern uint8_t updateGUIInIRQ;

#endif // GUI_h