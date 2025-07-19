#ifndef SCREENS_h
#define SCREENS_h

#include "DrawUtils.h"
#include "GUIUtils.h"
#include "CORDIC/CORDIC.h"

typedef enum ScreenID {
    SCREEN_INTRO = 0,
    SCREEN_MAIN,
    SCREN_LAST  // used to automatically get the number of new screens.
} ScreenID;

typedef struct Screen {
    ScreenID id;
    
    void (*initScreen)();
    // Returns 1 if the frame is to be redrawn.
    uint8_t (*draw)(Display);
    void (*updateInput)();
} Screen;

void initScreens();

void checkerboardBackground(Display d, float time);

extern uint16_t GUI_CHECKERBOARD_COLOR1;
extern uint16_t GUI_CHECKERBOARD_COLOR2;

extern Screen introScreen;
extern Screen mainScreen;

extern Screen* screens[SCREN_LAST];

#endif // SCREENS_h