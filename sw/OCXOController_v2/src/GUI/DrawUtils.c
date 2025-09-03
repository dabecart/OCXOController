#include "DrawUtils.h"

uint16_t palette[4];
Dithering dithering = DITHERING_OFF;
uint8_t origin = ORIGIN_LEFT | ORIGIN_TOP;

int16_t pwX = 0, pwY = 0, pwW = 0, pwH = 0;
int16_t pwCurrentX = 0, pwCurrentY = 0;

void setPlottingWindow(Display display, int16_t x, int16_t y, int16_t width, int16_t height) {
    pwX = x;
    pwY = y;
    pwW = width;
    pwH = height;

    // Reset plotting indexes.
    pwCurrentX = 0;
    pwCurrentY = 0;
}

// This function works supposing this memset is for a horizontal line on the display.
void clippedMemsetH(Display display, int16_t x, int16_t y, uint16_t color, uint16_t count) {
    // No need to plot the transparent areas.
    if(color == TRANSPARENT || count == 0) return;

    // Check if the line is inside the display on the vertical axis. 
    if((y < 0) || (y >= display.height)) return;
    
    // Truth table. Count(x, ex):

    //              |   x < 0   |   0 <= x < w   |   x >= w
    // ex < 0       |     0     |   impossible   |  impossible
    // 0 <= ex < w  | count + x |      count     |  impossible
    // ex >= w      |     w     |      w - x     |      0

    int16_t endX = x + count;
    if(((x < 0) && (endX < 0)) || ((x > display.width) && (endX > display.width))) return;

    if(x < 0) {
        if(endX >= display.width)    count = display.width;
        else                         count += x;
        // Start drawing from x = 0.
        x = 0;
    }else {
        if(endX >= display.width)    count = display.width - x;
    }

    switch (dithering) {
        default:
        case DITHERING_OFF:
            memsetDisplayBufferH(display.buf, x, y, color, count);
            break;

        case DITHERING_CROSSING:
            memsetDisplayBufferH_PattDithering(display.buf, x, y, color, count, 2, 0, 0);
            break;

        case DITHERING_PATTERNED:
            switch(y & 0x03) {
                case 0:
                case 2:
                    memsetDisplayBufferH_PattDithering(display.buf, x, y, color, count, 2, 0, 1);
                    break;
                case 1:
                    memsetDisplayBufferH_PattDithering(display.buf, x, y, color, count, 4, 2, 0);
                    break;
                case 3:
                    memsetDisplayBufferH_PattDithering(display.buf, x, y, color, count, 2, 0, 0);
                    break;
            }
            break;
    }
}

void clippedMemsetV(Display display, int16_t x, int16_t y, uint16_t color, uint16_t count) {
    // No need to plot the transparent areas.
    if(color == TRANSPARENT || count == 0) return;

    // Check if the line is inside the display on the horizontal axis. 
    if((x < 0) || (x >= display.width)) return;
    
    int16_t endY = y + count;
    if(((y < 0) && (endY < 0)) || ((y > display.height) && (endY > display.height))) return;

    if(y < 0) {
        if(endY >= display.height)  count = display.height;
        else                        count += y;
        y = 0;
    }else {
        if(endY >= display.height)  count = display.height - y;
    }

    memsetDisplayBufferV(display.buf, x, y, color, count);
}

// First, set the plotting window. Tell the number of bytes to fill with a color, starting from the 
// last pixel plotted. The filling is done horizontally (rows first, from left to right). When the
// next pixel falls outside the plotting window it goes back the line and one row down. When the 
// window is filled it goes to the upper corner again and the process restarts.
void fillWindowH(Display display, uint16_t color, uint16_t count) {
    if(count == 0) return;

    int16_t firstRowWrite;

    while((pwCurrentX + count) >= pwW) {
        // Need to do more than one set, one for each row.
        firstRowWrite = pwW - pwCurrentX;
        clippedMemsetH(display, pwX + pwCurrentX, pwY + pwCurrentY, color, firstRowWrite);
        count -= firstRowWrite;
        
        pwCurrentY++;
        if(pwCurrentY >= pwH) pwCurrentY = 0;
        pwCurrentX = 0;
    }
 
    clippedMemsetH(display, pwX + pwCurrentX, pwY + pwCurrentY, color, count);
    pwCurrentX += count;
}

void fillWindowV(Display display, uint16_t color, uint16_t count) {
    if(count == 0) return;

    int16_t firstColumnWrite;

    while((pwCurrentY + count) >= pwH) {
        // Need to do more than one set, one for each column.
        firstColumnWrite = pwH - pwCurrentY;
        clippedMemsetV(display, pwX + pwCurrentX, pwY + pwCurrentY, color, firstColumnWrite);
        count -= firstColumnWrite;
        
        pwCurrentX++;
        if(pwCurrentX >= pwW) pwCurrentX = 0;
        pwCurrentY = 0;
    }
 
    clippedMemsetV(display, pwX + pwCurrentX, pwY + pwCurrentY, color, count);
    pwCurrentY += count;
}

uint8_t isOnDisplay(Display d, int16_t x, int16_t y) {
    return (x >= 0) && (y >= 0) && (x < d.width) && (y < d.height);
}

void transformOrigin(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t* xout, int16_t* yout) {
    switch (origin & 0x0F) {
        default:
        case ORIGIN_LEFT:   *xout = x0;       break;

        case ORIGIN_MIDDLE: *xout = x0 - w/2; break;

        case ORIGIN_RIGHT:  *xout = x0 - w;   break;
    }

    switch (origin & 0xF0) {
        default:
        case ORIGIN_TOP:    *yout = y0;       break;

        case ORIGIN_CENTER: *yout = y0 - h/2; break;

        case ORIGIN_RIGHT:  *yout = y0 - h;   break;
    }
}

void fillRectangle(Display display, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    setPlottingWindow(display, x, y, w, h);
    if(w > h) {
        // Is fastest to fill in rows.
        fillWindowH(display, color, w*h);
    }else {
        // Is fastests to fill in columns.
        fillWindowV(display, color, w*h);
    }
}

void drawLineH(Display display, int16_t x, int16_t y, int16_t length, uint16_t color) {
    if(y < 0 || y >= display.height) return;

    setPlottingWindow(display, x, y, length, 1);
    fillWindowH(display, color, length);
}

void drawLineV(Display display, int16_t x, int16_t y, int16_t length, uint16_t color) {
    if(x < 0 || x >= display.width) return;

    setPlottingWindow(display, x, y, 1, length);
    fillWindowV(display, color, length);
}

void drawLine(Display display, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    uint8_t isSteep = abs(y1 - y0) > abs(x1 - x0);
    if(isSteep) {
        int16_t temp = x0;
        x0 = y0;
        y0 = temp;

        temp = x1;
        x1 = y1;
        y1 = temp;
    }

    if(x0 > x1) {
        int16_t temp = x0;
        x0 = x1;
        x1 = temp;

        temp = y0;
        y0 = y1;
        y1 = temp;
    }

    int16_t deltaX = x1 - x0, deltaY = abs(y1 - y0);
    int16_t err = deltaX >> 1, yStep = -1, xs = x0, lineLength = 0;

    if (y0 < y1) yStep = 1;

    if (isSteep) {
        // There will be more vertical lines than horizontal.
        for(; x0 <= x1; x0++) {
            lineLength++;
            err -= deltaY;
            if(err < 0) {
                // Draw a single pixel.
                if (lineLength == 1) (*display.buf)[xs][y0] = color;
                // Draw a vertical line.
                else drawLineV(display, y0, xs, lineLength, color);
                lineLength = 0;
                y0 += yStep; 
                xs = x0 + 1;
                err += deltaX;
            }
        }
        if(lineLength) drawLineV(display, y0, xs, lineLength, color);
    }else {
        // There will be more horizontal lines than vertical.
        for(; x0 <= x1; x0++) {
            lineLength++;
            err -= deltaY;
            if(err < 0) {
                if(lineLength == 1) (*display.buf)[y0][xs] = color;
                else drawLineH(display, xs, y0, lineLength, color);
                lineLength = 0;
                y0 += yStep; 
                xs = x0 + 1;
                err += deltaX;
            }
        }
        if(lineLength) drawLineH(display, xs, y0, lineLength, color);
    }
}

void drawChar(Display display, char ch, FontDef font, int16_t x0, int16_t y0) {
    if(ch <= ' ') return;
    
    setPlottingWindow(display, x0, y0, font.width, font.height);

    for(uint16_t i = 0; i < font.height; i++) {
        uint8_t b = font.data[(ch - FONTS_ASCII_OFFSET) * font.height + i];
        for(uint8_t j = 0; j < font.width; j++) {
            // If the bit is 1, use the primary color of the palette [0].
            fillWindowH(display, palette[(b & 0x80) == 0], 1);
            b <<= 1;
        }
    }
}

void drawCompressedBitmap_(Display display, const Bitmap* img) {
    for(const uint8_t* chunk = img->buf; chunk < img->buf + img->byteLen; chunk++) {
        // color = palette[(*chunk) >> 6];
        // pixelCount = ((*chunk) & 0x3F) + 1;
        fillWindowH(display, palette[(*chunk) >> 6], ((*chunk) & 0x3F) + 1);
    }
}

void drawPaletteBitmap_(Display display, const Bitmap* img) {
    uint16_t currentBlock;
    for(uint16_t* blockPointer = (uint16_t*) img->buf; 
        blockPointer < ((uint16_t*) img->buf) + (img->byteLen/sizeof(uint16_t));
        blockPointer++) {
        
        // In an u16 there are 8 pixels.
        currentBlock = *blockPointer;
        for(int j = 0; j < 8; j++) {
            fillWindowH(display, palette[currentBlock & 0b11], 1);
            // Go to the next pixel.
            currentBlock >>= 2;
        }
    }
}

void drawBitmap(Display display, const Bitmap* img, int16_t x0, int16_t y0) {
    int16_t x, y;
    transformOrigin(x0, y0, img->width, img->height, &x, &y);
    setPlottingWindow(display, x, y, img->width, img->height);

    switch (img->compressed) {
        case BITMAP_COMPRESSED:     drawCompressedBitmap_(display, img);    break;
        case BITMAP_PALETTE:        drawPaletteBitmap_(display, img);       break;
        default:                    break;
    }   
}

void drawString(Display display, const char* str, FontDef font, int16_t x0, int16_t y0) {
    int16_t x, y;
    transformOrigin(x0, y0, strlen(str) * font.width, font.height, &x, &y);
    while(*str) {
        drawChar(display, *str, font, x, y);
        x += font.width;
        str++;
    }
}

void setCurrentPalette(uint16_t c00, uint16_t c01, uint16_t c10, uint16_t c11) {
    palette[0] = c00;
    palette[1] = c01;
    palette[2] = c10;
    palette[3] = c11;
}

void setCurrentOrigin(uint8_t o) {
    origin = o;
}

void setDithering(Dithering dith) {
    dithering = dith;
}
