#ifndef DRAW_UTILS_h
#define DRAW_UTILS_h

#include "stdint.h"
#include "stddef.h"

void drawCompressedBitmap(uint16_t** screen, uint8_t* img, int16_t x, int16_t y);

#endif // DRAW_UTILS_h