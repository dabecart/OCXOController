#ifndef BITMAPS_h
#define BITMAPS_h

#include <stdint.h>

typedef struct Bitmap {
    const uint8_t* buf;
    uint16_t bufLen;
    uint16_t width, height;
} Bitmap;

extern const Bitmap OCXOLogo;

#endif // BITMAPS_h
