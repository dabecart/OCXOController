#include "Screen.h"

Screen* screens[SCREEN_LAST];
const int16_t borderBoxSize = 2; 

void initScreens() {
    screens[SCREEN_INTRO] = &introScreen;
    screens[SCREEN_MAIN] = &mainScreen;
    screens[SCREEN_OUT] = &outScreen;
}

// Ripple distortion (adjust frequency, amplitude, and speed)
const float rippleFrequency = 0.15f;
const float rippleAmplitude = 4.0f;
const float rippleSpeed = 0.8f;
// Ripple center (center of display)
const float cx = ST7735_HEIGHT / 2.0f;
const float cy = ST7735_WIDTH / 2.0f;

float rippleScale = 0;

uint16_t GUI_CHECKERBOARD_COLOR1 = reversed_color565(210,0,0);
uint16_t GUI_CHECKERBOARD_COLOR2 = reversed_color565(160,0,0);

void calculateRippleConstantsForCircle(float radius, float time) {
    float rippleFactor = sinCORDIC(radius * rippleFrequency - time * rippleSpeed) 
                         * rippleAmplitude;

    // Radial distortion factor.
    rippleScale = rippleFactor / (radius + 1.0f);  // avoid division by 0
}

void rippleDisplacement(uint16_t x, uint16_t y, uint16_t r, uint16_t* xout, uint16_t* yout) {
    // Apply radial distortion (toward/away from center)
    *xout = x + r * rippleScale;
    *yout = y + r * rippleScale;
}

uint16_t checkerboard(int16_t x, int16_t y, uint16_t color1, uint16_t color2) {
    // const uint16_t cellSize = 16;
    // return ((((x / cellSize) + (y / cellSize)) & 0x01) == 0) ? color1 : color2;

    // The +10 is so that the pattern generates for negative values of x and y. 
    // It's a cheap fix but it works :)
    return (((((x+10) >> 4) + ((y+10) >> 4)) & 0x0001) == 1) ? color1 : color2;
}

void displacedCheckerboard(Display d, int16_t x, int16_t y, int16_t r, int16_t dithX, int16_t dithY) {
    if(!isOnDisplay(d,x,y)) return;

    uint16_t xx = x, yy = y;
    rippleDisplacement(xx, yy, r, &xx, &yy);
    
    uint16_t pixel = checkerboard(xx, yy, GUI_CHECKERBOARD_COLOR1, GUI_CHECKERBOARD_COLOR2);
    (*d.buf)[y][x] = pixel; 

    // The midpoint center algorithm cannot fill all points on the display. To fill those
    // points draw the circle line a little thicker by drawing two extra pixels on the dithering 
    // direction.
    if(isOnDisplay(d, x+dithX, y+dithY)) (*d.buf)[y+dithY][x+dithX] = pixel;
    if(isOnDisplay(d, x-dithX, y-dithY)) (*d.buf)[y-dithY][x-dithX] = pixel;
}

void checkerboardBackground(Display d, float time) {
    int16_t maxRadius = ceilf(sqrtCORDIC(d.width*d.width + d.height*d.height)/2.0);

    // Fill background.
    // Instead of calculating for each pixel a sine + sqrt, let's traverse the display in circles.
    int16_t t1, t2, x, y;
    for(uint16_t r = 0; r <= maxRadius; r++) {
        calculateRippleConstantsForCircle(r, time);

        // Midpoint circle algorithm.
        t1 = r >> 4;
        x = r;
        y = 0;
        while(x >= y) {
            // (x,y) is in the first octant and belongs to the circle. Replicate on the other 
            // octants to get the full circle.
            // Fill the empty spaces left by the midpoint circle algorithm. These are in radial 
            // direction, outside the circle.
            displacedCheckerboard(d, cx + x, cy + y, r,  1,  1);
            displacedCheckerboard(d, cx + y, cy + x, r,  1,  1);
            
            displacedCheckerboard(d, cx - x, cy + y, r, -1,  1);
            displacedCheckerboard(d, cx - y, cy + x, r, -1,  1);

            displacedCheckerboard(d, cx - x, cy - y, r, -1, -1);
            displacedCheckerboard(d, cx - y, cy - x, r, -1, -1);

            displacedCheckerboard(d, cx + y, cy - x, r,  1, -1);
            displacedCheckerboard(d, cx + x, cy - y, r,  1, -1);

            y++;
            t1 += y;
            t2 = t1 - x;
            if(t2 >= 0) {
                t1 = t2;
                x--;
            }
        }
    }
}

void checkerboardBackgroundMirrored(Display d, float time) {
    int16_t maxRadius = ceilf(sqrtCORDIC(d.width*d.width + d.height*d.height)/2.0);

    // Fill background.
    // Instead of calculating for each pixel a sine + sqrt, let's traverse the display in circles.
    int16_t t1, t2, x, y;
    for(uint16_t r = 0; r <= maxRadius; r++) {
        calculateRippleConstantsForCircle(r, time);

        // Midpoint circle algorithm.
        t1 = r >> 4;
        x = r;
        y = 0;
        while(x >= y) {
            // (x,y) is in the first octant and belongs to the circle. Replicate on the other 
            // octants to get the full circle.
            // Fill the empty spaces left by the midpoint circle algorithm. These are in radial 
            // direction, outside the circle.
            displacedCheckerboard(d, cx + x, cy + y, r,  1,  1);
            displacedCheckerboard(d, cx + y, cy + x, r,  1,  1);
            
            displacedCheckerboard(d, cx - x, cy + y, r, -1,  1);
            displacedCheckerboard(d, cx - y, cy + x, r, -1,  1);

            y++;
            t1 += y;
            t2 = t1 - x;
            if(t2 >= 0) {
                t1 = t2;
                x--;
            }
        }
    }

    for(int16_t row = 0; row < d.height/2; row++) {
        memcpy((*d.buf)[row], (*d.buf)[d.height-1-row], sizeof(DisplayBufferRow));
    }

}

void drawBox(Display d, int16_t x0, int16_t y0, int16_t w, int16_t h, 
             uint16_t borderColor, uint16_t fillColor) {
    // Box filling.
    fillRectangle(d, x0 + borderBoxSize, y0 + borderBoxSize, 
                  w-2*borderBoxSize, h - 2*borderBoxSize, fillColor);

    // Top line.
    fillRectangle(d, x0 + 1, y0, w - 2, borderBoxSize, borderColor);
    // Bot line.
    fillRectangle(d, x0 + 1, y0 + h - borderBoxSize, w - 2, borderBoxSize, borderColor);
    // Left line.
    fillRectangle(d, x0, y0 + 1, borderBoxSize, h - 2, borderColor);
    // Right line.
    fillRectangle(d, x0 + w - borderBoxSize, y0 + 1, borderBoxSize, h - 2, borderColor);

    // Inner corner pixels.

    // Top left.
    (*d.buf)[y0 + borderBoxSize][x0 + borderBoxSize] = borderColor;
    // Bottom left.
    (*d.buf)[y0+h - borderBoxSize-1][x0 + borderBoxSize] = borderColor;
    // Top right.
    (*d.buf)[y0 + borderBoxSize][x0+w - borderBoxSize-1] = borderColor;
    // Bottom right.
    (*d.buf)[y0+h - borderBoxSize-1][x0+w - borderBoxSize-1] = borderColor;
}

uint16_t rainbowGradient(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    const float cosTheta = 0.7071067811f;
    const float sinTheta = 0.7071067811f;

    // Project (x, y) onto gradient axis.
    // Normalize to [0, 1] based on projected distance.
    float max_len = width * cosTheta + height * sinTheta;
    float projection = x * cosTheta + y * sinTheta;
    float normalized = fmod(projection, max_len) / max_len;  // wrap around

    // Convert to HSV and then to RGB
    uint8_t r, g, b;
    hsv2rgb(normalized, 1, 1, &r, &g, &b);

    return toColor565(r, g, b);
}