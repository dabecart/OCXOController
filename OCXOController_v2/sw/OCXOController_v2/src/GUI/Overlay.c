#include "Overlay.h"

const int16_t barWidth = 10;
const float sweepTime = 1.5f;
const uint16_t animationColor = switched_color565(200,0,0);

// The displacement = a*t^2. At t = sweepTime, the displacement should be the width of the TFT,
// therefore the "a" factor should be:
const float sweepSpeed = 160/sweepTime/sweepTime;

uint8_t overlayCurtainSweepInLeft_draw(Overlay* ov, Display d) {
    float t = guiTime - ov->startTime;
    t *= t*sweepSpeed;  // Multiplied by t so ease in, faster out
    t -= barWidth;      // So that the animation start behind the screen limits.

    // Draws the triangles.
    for(int i = 0; i < d.height; i+=barWidth*2-1){
        for(int j = 0; j <= barWidth; j++){
            drawLineH(d, t, i+j,            j, animationColor);
            drawLineH(d, t, i+barWidth*2-j, j, animationColor);
        }
    }
    fillRectangle(d, 0, 0, t, d.height, animationColor);

    // The animation ends when the triangles surpass the right edge of the screen.
    ov->animationDone = t > (d.width + barWidth + 2);

    return 1;
}

uint8_t overlayCurtainSweepOutLeft_draw(Overlay* ov, Display d) {
    float t = guiTime - ov->startTime;
    t *= t*sweepSpeed;  // Multiplied by t so ease in, faster out
    t -= barWidth;      // So that the animation start behind the screen limits.

    // Draws the triangles.
    for(int i = 0; i < d.height; i += barWidth*2-1){
        for(int j = 0; j <= barWidth; j++){
            drawLineH(d, d.width-t, i+j,            j, animationColor);
            drawLineH(d, d.width-t, i+barWidth*2-j, j, animationColor);
        }
    }
    fillRectangle(d, 0, 0, d.width - t, d.height, animationColor);

    ov->animationDone = t > (d.width + barWidth + 2);
    return 1;
}

uint8_t overlayCurtainSweepInRight_draw(Overlay* ov, Display d) {
    float t = guiTime - ov->startTime;
    t *= t*sweepSpeed;  // Multiplied by t so ease in, faster out
    t -= barWidth;      // So that the animation start behind the screen limits.

    for(int i = barWidth; i < d.height+barWidth; i+=barWidth*2){
        for(int j = 0; j <= barWidth; j++){
            drawLineH(d, d.width-t+j, i+j, barWidth, animationColor);
            drawLineH(d, d.width-t+j, i-j, barWidth, animationColor);
        }
    }
    fillRectangle(d, d.width-t+barWidth, 0, t, d.height, animationColor);

    ov->animationDone = t > (d.width + barWidth + 2);
    return 1;
}

uint8_t overlayCurtainSweepOutRight_draw(Overlay* ov, Display d) {
    return 0;    
}

uint8_t overlayCurtainSweepInLeftOutLeft_draw(Overlay* ov, Display d) {
    if(ov->halfAnimationDone) {
        overlayCurtainSweepOutLeft_draw(ov, d);
    }else {
        overlayCurtainSweepInLeft_draw(ov, d);
        if(ov->animationDone) {
            // First animation ended, go to the next.
            ov->animationDone = 0;
            ov->halfAnimationDone = 1;
            ov->startTime = guiTime;
        }
    }

    return 1;
}


void createEmptyOverlay_(Overlay* ov, OverlayID id) {
    ov->id = id;
    ov->draw = NULL;
    ov->startTime = guiTime;
    ov->halfAnimationDone = 0;
    ov->animationDone = 0;
}

uint8_t createOverlay(Overlay* ov, OverlayID id) {
    if(ov == NULL) return 0;

    createEmptyOverlay_(ov, id);
    switch (id) {
        case OVERLAY_CURTAIN_SWEEP_IN_LEFT:
            ov->draw = overlayCurtainSweepInLeft_draw;
            break;
        case OVERLAY_CURTAIN_SWEEP_OUT_LEFT:
            ov->draw = overlayCurtainSweepOutLeft_draw;
            break;
        case OVERLAY_CURTAIN_SWEEP_IN_RIGHT:
            ov->draw = overlayCurtainSweepInRight_draw;
            break;
        case OVERLAY_CURTAIN_SWEEP_OUT_RIGHT:
            ov->draw = overlayCurtainSweepOutRight_draw;
            break;
        case OVERLAY_CURTAIN_SWEEP_IN_LEFT_OUT_LEFT:
            ov->draw = overlayCurtainSweepInLeftOutLeft_draw;
            break;
        default: 
            return 0; 
    }

    return 1;
}
