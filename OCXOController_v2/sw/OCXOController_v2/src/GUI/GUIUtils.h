#ifndef GUI_UTILS_h
#define GUI_UTILS_h

#include "stdint.h"
#include "stddef.h"
#include "math.h"

#include "TFT/ST7735.h"

typedef uint16_t DisplayBufferRow[ST7735_HEIGHT];
typedef DisplayBufferRow DisplayBuffer[ST7735_WIDTH]; 

void hsv2rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b);

void memsetDisplayBufferH(DisplayBuffer* buf, int16_t x0, int16_t y0, uint16_t value, size_t count) __attribute__((optimize("O3")));
void memsetDisplayBufferV(DisplayBuffer* buf, int16_t x0, int16_t y0, uint16_t value, size_t count) __attribute__((optimize("O3")));

extern float guiTime;

#endif // GUI_UTILS_h