#ifndef BITMAPS_h
#define BITMAPS_h

#include <stdint.h>

typedef enum BitmapCompression {
    BITMAP_NOT_COMPRESSED = 0,
    BITMAP_COMPRESSED = 1,
} BitmapCompression;

typedef struct Bitmap {
    const uint8_t* buf;
    uint16_t bufLen;
    uint16_t width, height;
    BitmapCompression compressed;
} Bitmap;

extern const Bitmap OCXOLogo;

#endif // BITMAPS_h
