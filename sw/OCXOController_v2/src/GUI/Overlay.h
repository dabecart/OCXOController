#ifndef GUI_OVERLAY_h
#define GUI_OVERLAY_h

#include "DrawUtils.h"
#include "GUIUtils.h"
#include "CORDIC/CORDIC.h"

typedef enum OverlayID {
    OVERLAY_CURTAIN_SWEEP_IN_LEFT = 0,
    OVERLAY_CURTAIN_SWEEP_OUT_LEFT,
    OVERLAY_CURTAIN_SWEEP_IN_RIGHT,
    OVERLAY_CURTAIN_SWEEP_OUT_RIGHT,

    OVERLAY_CURTAIN_SWEEP_IN_LEFT_OUT_LEFT,
    OVERLAY_LAST  // used to automatically get the number of overlays.
} OverlayID;

typedef struct Overlay {
    OverlayID id;
    
    // Returns 1 if the frame is to be redrawn.
    uint8_t (*draw)(struct Overlay* ov, Display);

    float startTime;
    uint8_t halfAnimationDone;
    uint8_t animationDone;
} Overlay;

uint8_t createOverlay(Overlay* ov, OverlayID id);

#endif // GUI_OVERLAY_h