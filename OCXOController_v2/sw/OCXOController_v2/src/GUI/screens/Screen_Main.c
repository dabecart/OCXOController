#include "GUI/Screen.h"
#include "MainMCU.h"
#include "GUI/Bitmaps.h"

float main_screenInitTime = 0;
int8_t main_rotIndex = 0;

void drawChannelBox(Display d, OCXOChannel* ch, int16_t x0, int16_t y0, uint8_t selected) {
    const uint16_t outputBoxWidth = 140;
    const uint16_t outputBoxHeight = 27;
    char str[16];
    const uint16_t strLen = sizeof(str) - 1;

    drawBox(d, x0, y0, outputBoxWidth, outputBoxHeight, 
            TFT_BLACK, 
            selected ? TFT_WHITE : reversed_color565(230,230,230));

    if(selected) {
        setCurrentOrigin(ORIGIN_RIGHT | ORIGIN_CENTER);
        setCurrentPalette(TFT_BLACK, TRANSPARENT, TFT_WHITE, TRANSPARENT);
        drawBitmap(d, &rightArrow, x0-3, y0+outputBoxHeight/2);
    }

    setCurrentOrigin(ORIGIN_LEFT | ORIGIN_TOP);
    setCurrentPalette(TFT_BLACK, TRANSPARENT, TRANSPARENT, TRANSPARENT);

    snprintf(str, strLen, "Out %d", ch->id);
    drawString(d, str, Font_7x10, x0 + 6, y0 + 4);

    drawString(d, ch->config.voltage, Font_7x10, x0 + 6, y0 + 15);

    getFrequencyString(ch, str, strLen);
    drawString(d, str, Font_7x10, x0 + 59, y0 + 4);
    
    snprintf(str, strLen, "%d%%", (uint16_t)(ch->dutyCycle*100));
    drawString(d, str, Font_7x10, x0 + 59, y0 + 15);
    
    setCurrentOrigin(ORIGIN_RIGHT | ORIGIN_TOP);
    getPhaseString(ch, str, strLen);
    drawString(d, str, Font_7x10, x0 + 135, y0 + 15);
}

void mainScreen_initScreen(void** screenArgs) {
    main_screenInitTime = guiTime;
}

uint8_t mainScreen_draw(Display d) {
    const float backgroundValue1 = 0.82;
    const float backgroundValue2 = 0.63;

    // Full rotation of background color every two minutes.
    float backgroundHue = fmod(guiTime - main_screenInitTime, 120.0f) / 120.0f;

    uint8_t r, g, b;
    hsv2rgb(backgroundHue, 1.0f, backgroundValue1, &r, &g, &b);
    GUI_CHECKERBOARD_COLOR1 = toColor565Reversed(r, g, b);
    hsv2rgb(backgroundHue, 1.0f, backgroundValue2, &r, &g, &b);
    GUI_CHECKERBOARD_COLOR2 = toColor565Reversed(r, g, b);

    // Draw background.
    checkerboardBackgroundMirrored(d, guiTime);

    // Draw small logo.
    setCurrentOrigin(ORIGIN_LEFT | ORIGIN_TOP);
    setCurrentPalette(TFT_BLACK, TRANSPARENT, TFT_WHITE, TRANSPARENT);
    drawBitmap(d, &miniOCXOLogo, 106, 4);

    // Menu boxes.
    drawChannelBox(d, &hmain.chOuts.ch1, 15, 25, main_rotIndex == 0);
    drawChannelBox(d, &hmain.chOuts.ch2, 15, 56, main_rotIndex == 1);
    drawChannelBox(d, &hmain.chOuts.ch3, 15, 87, main_rotIndex == 2);

    return 1;
}

void mainScreen_updateInput() {
    if(wasButtonClicked(&hmain.gpio, BUTTON_ROT)) {
        OCXOChannel* ch;
        getOCXOOutputsFromID_(&hmain.chOuts, main_rotIndex+1, &ch);

        void* args[] = {ch};
        requestScreenChange(SCREEN_OUT, args, 0);
        return;
    }

    main_rotIndex += getFilteredRotaryIncrement(&hmain.gpio.rot);

    // Do not allow rollover.
    if(main_rotIndex >= 3)      main_rotIndex = 2;
    else if(main_rotIndex < 0)  main_rotIndex = 0;
}

Screen mainScreen = {
    .id = SCREEN_MAIN, 
    .initScreen = mainScreen_initScreen,
    .draw = mainScreen_draw, 
    .updateInput = mainScreen_updateInput
};