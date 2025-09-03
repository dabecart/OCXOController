#ifndef BITMAPS_h
#define BITMAPS_h

#include <stdint.h>

typedef enum BitmapCompression {
    BITMAP_PALETTE = 0,
    BITMAP_COMPRESSED = 1,
} BitmapCompression;

typedef struct Bitmap {
    const uint8_t* buf;
    uint32_t byteLen;
    uint16_t width, height;
    BitmapCompression compressed;
} Bitmap;

extern const Bitmap OCXOLogo;
extern const Bitmap miniOCXOLogo;
extern const Bitmap leftArrow;
extern const Bitmap rightArrow;
extern const Bitmap backArrow;

#endif // BITMAPS_h
