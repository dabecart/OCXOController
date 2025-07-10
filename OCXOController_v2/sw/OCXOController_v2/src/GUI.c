#include "GUI.h"

uint8_t initGUI(GUI* gui, SPI_HandleTypeDef* hspi) {
    if(gui == NULL) return 0;

    // TFT is horizontal.
    initTFT(&gui->tft, hspi, 3);
    drawImageTFT(&gui->tft, 0, 0, gui->tft.width, gui->tft.height, OCXOLogo);

    return 1;
}

uint8_t updateGUI(GUI* gui) {
    return 1;
}
