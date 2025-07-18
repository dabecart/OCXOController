#ifndef DRAW_UTILS_h
#define DRAW_UTILS_h

#include "stdint.h"
#include "stddef.h"
#include "TFT/Bitmaps.h"
#include "TFT/ST7735.h"
#include "GUIUtils.h"

#define TRANSPARENT 0x0821 // The darkest gray that is not black in 565.

typedef uint16_t ScreenBufferRow[ST7735_HEIGHT];
typedef ScreenBufferRow ScreenBuffer[ST7735_WIDTH]; 

typedef struct Screen {
    uint16_t width, height;
    ScreenBuffer* buf;
} Screen;

void drawCompressedBitmap(Screen screen, const Bitmap* img, int16_t x, int16_t y);

void setCurrentPalette(uint16_t c00, uint16_t c01, uint16_t c10, uint16_t c11);

#endif // DRAW_UTILS_h