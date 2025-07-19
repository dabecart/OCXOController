#include "GUI/Screen.h"
#include "MainMCU.h"

const float backgroundValue1 = 0.82;
const float backgroundValue2 = 0.63;

float screenInitTime = 0;

void drawBox(Display d, int16_t x0, int16_t y0, int16_t w, int16_t h, 
             uint16_t borderColor, uint16_t fillColor) {
    const int16_t borderSize = 2; 
    
    // Box filling.
    fillRectangle(d, x0 + borderSize, y0 + borderSize, w-2*borderSize, h - 2*borderSize, fillColor);

    // Top line.
    fillRectangle(d, x0 + 1, y0, w - 2, borderSize, borderColor);
    // Bot line.
    fillRectangle(d, x0 + 1, y0 + h - borderSize, w - 2, borderSize, borderColor);
    // Left line.
    fillRectangle(d, x0, y0 + 1, borderSize, h - 2, borderColor);
    // Right line.
    fillRectangle(d, x0 + w - borderSize, y0 + 1, borderSize, h - 2, borderColor);

    // Inner corner pixels.

    // Top left.
    (*d.buf)[y0 + borderSize][x0 + borderSize] = borderColor;
    // Bottom left.
    (*d.buf)[y0+h - borderSize-1][x0 + borderSize] = borderColor;
    // Top right.
    (*d.buf)[y0 + borderSize][x0+w - borderSize-1] = borderColor;
    // Bottom right.
    (*d.buf)[y0+h - borderSize-1][x0+w - borderSize-1] = borderColor;
}

void drawChannelBox(Display d, OCXOChannel* ch, int16_t x0, int16_t y0) {
    const uint16_t outputBoxWidth = 140;
    const uint16_t outputBoxHeight = 27;
    char str[16];
    const uint16_t strLen = sizeof(str) - 1;

    drawBox(d, x0, y0, outputBoxWidth, outputBoxHeight, TFT_BLACK, TFT_WHITE);

    setCurrentOrigin(ORIGIN_LEFT | ORIGIN_TOP);
    setCurrentPalette(TFT_BLACK, TRANSPARENT, TRANSPARENT, TRANSPARENT);

    snprintf(str, strLen, "Out %d", ch->id);
    drawString(d, str, Font_7x10, x0 + 6, y0 + 4);

    switch (ch->voltage)
    {
        case VOLTAGE_LEVEL_5V:   strcpy(str, "5V");     break;
        case VOLTAGE_LEVEL_3V3:  strcpy(str, "3V3");    break;
        case VOLTAGE_LEVEL_1V8:  strcpy(str, "1V8");    break;
        default:                 strcpy(str, "?V");     break;
    }
    drawString(d, str, Font_7x10, x0 + 6, y0 + 15);

    convertFrequencyToString(str, strLen, ch->frequency);
    drawString(d, str, Font_7x10, x0 + 59, y0 + 4);
    
    snprintf(str, strLen, "%d%%", (uint16_t)(ch->dutyCycle*100));
    drawString(d, str, Font_7x10, x0 + 59, y0 + 15);
    
    setCurrentOrigin(ORIGIN_RIGHT | ORIGIN_TOP);
    convertPhaseToString(str, strLen, ch->phase_ns);
    drawString(d, str, Font_7x10, x0 + 135, y0 + 15);
}

void mainScreen_initScreen() {
    screenInitTime = guiTime;
}

uint8_t mainScreen_draw(Display d) {
    // Full rotation of background color every two minutes.
    float backgroundHue = fmod(guiTime - screenInitTime, 120.0f) / 120.0f;

    uint8_t r, g, b;
    hsv2rgb(backgroundHue, 1.0f, backgroundValue1, &r, &g, &b);
    GUI_CHECKERBOARD_COLOR1 = toColor565Reversed(r, g, b);
    hsv2rgb(backgroundHue, 1.0f, backgroundValue2, &r, &g, &b);
    GUI_CHECKERBOARD_COLOR2 = toColor565Reversed(r, g, b);

    // Draw background.
    checkerboardBackground(d, guiTime);

    // Menu boxes.
    drawChannelBox(d, &hmain.chOuts.ch1, 15, 25);
    drawChannelBox(d, &hmain.chOuts.ch2, 15, 56);
    drawChannelBox(d, &hmain.chOuts.ch3, 15, 87);

    return 1;
}

void mainScreen_updateInput() {

}

Screen mainScreen = {
    .id = SCREEN_MAIN, 
    .initScreen = mainScreen_initScreen,
    .draw = mainScreen_draw, 
    .updateInput = mainScreen_updateInput
};