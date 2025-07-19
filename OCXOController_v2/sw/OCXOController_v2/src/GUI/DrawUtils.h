#ifndef DRAW_UTILS_h
#define DRAW_UTILS_h

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "GUIUtils.h"
#include "TFT/Bitmaps.h"

#define TRANSPARENT 0x0821 // The darkest gray that is not black in 565.

typedef enum Origin {
    ORIGIN_LEFT     = 0x01,
    ORIGIN_MIDDLE   = 0x02,
    ORIGIN_RIGHT    = 0x04,

    ORIGIN_TOP      = 0x10,
    ORIGIN_CENTER   = 0x20,
    ORIGIN_BOTTOM   = 0x40,
} Origin;

typedef struct Display {
    uint16_t width, height;
    DisplayBuffer* buf;
} Display;

void fillRectangle(Display display, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawLineH(Display display, int16_t x, int16_t y, int16_t length, uint16_t color);
void drawLineV(Display display, int16_t x, int16_t y, int16_t length, uint16_t color);
void drawLine(Display display, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void drawCompressedBitmap(Display display, const Bitmap* img, int16_t x, int16_t y);
void drawString(Display display, const char* str, FontDef font, int16_t x0, int16_t y0); 
uint8_t isOnDisplay(Display d, int16_t x, int16_t y);

void setCurrentPalette(uint16_t c00, uint16_t c01, uint16_t c10, uint16_t c11);
void setCurrentOrigin(uint8_t origin);

#endif // DRAW_UTILS_h