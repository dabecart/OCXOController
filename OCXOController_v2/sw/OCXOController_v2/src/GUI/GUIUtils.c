#include "GUIUtils.h"

float guiTime = 0.0f;

void hsv2rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
	int i = floor(h * 6);
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	
    float rf, gf, bf;
    switch (i % 6) {
        default:
        case 0: rf = v; gf = t; bf = p; break;
        case 1: rf = q; gf = v; bf = p; break;
        case 2: rf = p; gf = v; bf = t; break;
        case 3: rf = p; gf = q; bf = v; break;
        case 4: rf = t; gf = p; bf = v; break;
        case 5: rf = v; gf = p; bf = q; break;
    }

    *r = (uint8_t)(rf * 255.0f);
    *g = (uint8_t)(gf * 255.0f);
    *b = (uint8_t)(bf * 255.0f);
}

void memset_u16(uint16_t *dst, uint16_t value, size_t count) {
    for (size_t i = 0; i < count; i++) {
        dst[i] = value;
    }
}

void memsetDisplayBufferH(DisplayBuffer* buf, int16_t x0, int16_t y0, uint16_t value, size_t count) {
    if(count == 0) return;
    uint16_t* arr = (*buf)[y0] + x0;
    for(uint16_t x = 0; x < count; x++) {
        arr[x] = value;
    }
}

void memsetDisplayBufferH_Dithering(DisplayBuffer* buf, int16_t x0, int16_t y0, uint16_t value, 
                                    size_t count, uint16_t dither, uint16_t skip) {
    if(count == 0) return;
    uint16_t mod = (x0 + y0) % dither;

    uint16_t* arr = (*buf)[y0] + x0;
    for(uint16_t x = 0; x < count; x++) {
        // if (x+y) % dithering <= skip then draw it.
        if(mod <= skip) arr[x] = value;

        // Next mod...
        mod++;
        if(mod >= dither) mod = 0;
    }
}

void memsetDisplayBufferH_PattDithering(
    DisplayBuffer* buf, int16_t x0, int16_t y0, uint16_t value, size_t count, 
    uint16_t dither, uint16_t skip, uint16_t offset) {
    
    if(count == 0) return;
    uint16_t mod = (x0 % dither) + offset;
    if(mod >= dither) mod = 0;

    uint16_t* arr = (*buf)[y0] + x0;
    for(uint16_t x = 0; x < count; x++) {
        if(mod <= skip) arr[x] = value;

        // Next mod...
        mod++;
        if(mod >= dither) mod = 0;
    }
}

void memsetDisplayBufferV(DisplayBuffer* buf, int16_t x0, int16_t y0, uint16_t value, size_t count) {
    if(count == 0) return;
    for(uint16_t y = y0; y < y0 + count; y++) {
        (*buf)[y][x0] = value;
    }
}
