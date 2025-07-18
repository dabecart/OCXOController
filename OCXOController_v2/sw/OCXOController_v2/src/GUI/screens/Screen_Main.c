#include "GUI/Screen.h"

uint8_t mainScreen_draw(Display d) {
    // Draw background.
    checkerboardBackground(d, guiTime);

    return 1;
}

void mainScreen_updateInput() {

}

Screen mainScreen = {
    .id = SCREEN_MAIN, 
    .draw = mainScreen_draw, 
    .updateInput = mainScreen_updateInput
};