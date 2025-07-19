#include "GUI/Screen.h"
#include "commons/Logs.h"

void introScreen_initScreen() {
    // Set the initial screen "subtitle" to be this.
    logMessage("by @dabecart");
}

uint8_t introScreen_draw(Display d) {
    // Draw background.
    checkerboardBackground(d, guiTime);

    // Draws logo on top of the background.
    setCurrentOrigin(ORIGIN_MIDDLE | ORIGIN_CENTER);

    int16_t x0 = d.width  / 2;
    int16_t y0 = d.height / 2;
    // int16_t y0 = sinCORDIC(PI / 2 * time)*2.1;
    // y0 = (d.height / 2) + ((y0 < 0) ? y0 : 0);
    setCurrentPalette(TFT_BLACK, TFT_RED, TFT_WHITE, TRANSPARENT);
    drawBitmap(d, &OCXOLogo, x0, y0);

    // Draw author.
    setCurrentOrigin(ORIGIN_MIDDLE | ORIGIN_TOP);
    setCurrentPalette(TFT_WHITE, TRANSPARENT, TRANSPARENT, TRANSPARENT);
    drawString(d, lastLog, Font_7x10, x0, (d.height + OCXOLogo.height)/2 + 7);

    return 1;
}

void introScreen_updateInput() {
    
}

Screen introScreen = {
    .id = SCREEN_INTRO,
    .initScreen = introScreen_initScreen,
    .draw = introScreen_draw, 
    .updateInput = introScreen_updateInput
};
