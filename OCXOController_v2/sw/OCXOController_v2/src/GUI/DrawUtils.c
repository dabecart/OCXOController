#include "DrawUtils.h"

uint16_t palette[4];
int16_t pwX = 0, pwY = 0, pwW = 0, pwH = 0;
int16_t pwCurrentX = 0, pwCurrentY = 0;

void setPlottingWindow(Screen screen, int16_t x, int16_t y, int16_t width, int16_t height) {
    pwX = x;
    pwY = y;
    pwW = width;
    pwH = height;

    // Reset plotting indexes.
    pwCurrentX = 0;
    pwCurrentY = 0;
}

// This function works supposing this memset is for a horizontal line on the screen.
void clippedMemset(Screen screen, int16_t x, int16_t y, uint16_t color, uint16_t count) {
    // No need to plot the transparent areas.
    if(color == TRANSPARENT) return;

    // Check if the line is inside the screen on the vertical axis. 
    if((y < 0) || (y >= screen.height)) return;
    
    // Truth table. Count(x, ex):

    //              |   x < 0   |   0 <= x < w   |   x >= w
    // ex < 0       |     0     |   impossible   |  impossible
    // 0 <= ex < w  | count + x |      count     |  impossible
    // ex >= w      |     w     |      w - x     |      0

    int16_t endX = x + count;
    if(((x < 0) && (endX < 0)) || ((x > screen.width) && (endX > screen.width))) return;

    if(x < 0) {
        if(endX >= screen.width)    count = screen.width;
        else                        count += x;
        // Start drawing from x = 0.
        x = 0;
    }else {
        if(endX >= screen.width)    count = screen.width - x;
    }

    memset_u16((*screen.buf)[y] + x, color, count);
}

void fillWindow(Screen screen, uint16_t color, uint16_t count) {
    int16_t firstRowWrite;

    while((pwCurrentX + count) >= pwW) {
        // Need to do more than one set, one for each row.
        firstRowWrite = pwW - pwCurrentX;
        clippedMemset(screen, pwX + pwCurrentX, pwY + pwCurrentY, color, firstRowWrite);
        count -= firstRowWrite;
        
        pwCurrentY++;
        if(pwCurrentY >= pwH) pwCurrentY = 0;
        pwCurrentX = 0;
    }
 
    clippedMemset(screen, pwX + pwCurrentX, pwY + pwCurrentY, color, count);
    pwCurrentX += count;
}

void drawCompressedBitmap(Screen screen, const Bitmap* img, int16_t x0, int16_t y0) {
    setPlottingWindow(screen, x0, y0, img->width, img->height);

    for(const uint8_t* chunk = img->buf; chunk < img->buf + img->bufLen; chunk++) {
        // color = palette[(*chunk) >> 6];
        // pixelCount = ((*chunk) & 0x3F) + 1;
        fillWindow(screen, palette[(*chunk) >> 6], ((*chunk) & 0x3F) + 1);
    }
}

void setCurrentPalette(uint16_t c00, uint16_t c01, uint16_t c10, uint16_t c11) {
    palette[0] = c00;
    palette[1] = c01;
    palette[2] = c10;
    palette[3] = c11;
}
