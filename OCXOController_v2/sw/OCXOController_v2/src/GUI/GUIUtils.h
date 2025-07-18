#ifndef GUI_UTILS_h
#define GUI_UTILS_h

#include "stdint.h"
#include "stddef.h"
#include "math.h"

void hsv2rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b);

void memset_u16(uint16_t *dst, uint16_t value, size_t count) __attribute__((optimize("O3")));

#endif // GUI_UTILS_h