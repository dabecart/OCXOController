#ifndef SCREENS_h
#define SCREENS_h

#include "DrawUtils.h"
#include "GUIUtils.h"
#include "CORDIC/CORDIC.h"

typedef enum ScreenID {
    SCREEN_INTRO = 1,
    SCREEN_MAIN,
    SCREEN_OUT,
    SCREEN_LAST  // used to automatically get the number of new screens.
} ScreenID;

typedef struct Screen {
    ScreenID id;
    
    void (*initScreen)(void**);
    // Returns 1 if the frame is to be redrawn.
    uint8_t (*draw)(Display);
    void (*updateInput)();
} Screen;

void initScreens();

void checkerboardBackground(Display d, float time);
void checkerboardBackgroundMirrored(Display d, float time);
void drawBox(Display d, int16_t x0, int16_t y0, int16_t w, int16_t h, 
             uint16_t borderColor, uint16_t fillColor);

extern const int16_t borderBoxSize;
extern uint16_t GUI_CHECKERBOARD_COLOR1;
extern uint16_t GUI_CHECKERBOARD_COLOR2;

extern Screen introScreen;
extern Screen mainScreen;
extern Screen outScreen;

extern Screen* screens[SCREEN_LAST];

#endif // SCREENS_h