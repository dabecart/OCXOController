#include "GUI.h"

uint16_t width = 0, height = 0;
TFT tft;
// With rotation taken into account, use [y][x] to access.
ScreenBuffer screen;

float time = 0;
GUIState state = GUI_INITIALIZATION;
uint32_t drawTime = 0;

volatile uint8_t screenReady = 0;
volatile uint8_t transferInProgress = 0;

uint8_t initGUI(SPI_HandleTypeDef* hspi, DMA_HandleTypeDef* hdma_spi, TIM_HandleTypeDef* guitim) {
    // TFT is horizontal.
    initTFT(&tft, hspi, 3);
    setDMATFT(&tft, hdma_spi);

    width = tft.width;
    height = tft.height;
    
    // Keep the TFT selected, there isn't any other device connected to the SPI device.
    selectTFT_(&tft);
    // Set the adress window to be set to all the screen. All changes of the screen content must be
    // done to the "screen" variable.
    setAddressWindowTFT_(&tft, 0, 0, width-1, height-1);

    // This timer is in charge of generating an IRQ to send the screen buffer to the TFT screen 
    // using DMA. This is only done if the screen array is ready to be printed.
    HAL_TIM_Base_Start_IT(guitim);

    return 1;
}

void transferScreenToTFT() {
    // Update the screen during the initialization process inside the TIM IRQ.
    if(state == GUI_INITIALIZATION) updateGUI();

    // If the screen is not ready wait for the next timer call.
    if(screenReady) {
        screenReady = 0;
        writeDataTFT_DMA_(&tft, (uint8_t*) &screen, sizeof(screen));
        transferInProgress = 1;
    }
}

void transferToTFTEnded() {
    transferInProgress = 0;
}

void updateGUI() {
    // Cannot write to the screen array if the transfer is in progress or if a previous screen is 
    // ready to be displayed.
    if(transferInProgress || screenReady) return;

    time = ((float) HAL_GetTick())/1000.0f;
    homeScreen_();
    drawTime =  HAL_GetTick() - (uint32_t)(time*1000.0f);

    screenReady = 1;
}

void setGUIState(GUIState inSt) {
    state = inSt;
}

// Ripple distortion (adjust frequency, amplitude, and speed)
const float rippleFrequency = 0.15f;
const float rippleAmplitude = 4.0f;
const float rippleSpeed = 0.8f;
// Ripple center (center of screen)
const float cx = ST7735_HEIGHT / 2.0f;
const float cy = ST7735_WIDTH / 2.0f;

float rippleScale = 0;

void calculateRippleConstantsForCircle(float radius) {
    float rippleFactor = sinCORDIC(radius * rippleFrequency - time * rippleSpeed) 
                         * rippleAmplitude;

    // Radial distortion factor.
    rippleScale = rippleFactor / (radius + 1.0f);  // avoid division by 0
}

inline void rippleDisplacement(uint16_t x, uint16_t y, uint16_t r, uint16_t* xout, uint16_t* yout) {
    // Apply radial distortion (toward/away from center)
    *xout = x + r * rippleScale;
    *yout = y + r * rippleScale;
}

inline uint16_t checkerboard(int16_t x, int16_t y, uint16_t color1, uint16_t color2) {
    // const uint16_t cellSize = 16;
    // return ((((x / cellSize) + (y / cellSize)) & 0x01) == 0) ? color1 : color2;

    // The +10 is to generate the pattern for negative values of x and y. It's a cheap fix but it 
    // works :)
    return (((((x+10) >> 4) + ((y+10) >> 4)) & 0x0001) == 1) ? color1 : color2;
}

uint8_t isOnScreen(int16_t x, int16_t y) {
    return (x >= 0) && (y >= 0) && (x < width) && (y < height);
}

void displacedCheckerboard(int16_t x, int16_t y, int16_t r, int16_t dithX, int16_t dithY) {
    if(!isOnScreen(x,y)) return;

    uint16_t xx = x, yy = y;
    rippleDisplacement(xx, yy, r, &xx, &yy);
    
    uint16_t pixel = checkerboard(xx, yy, GUI_CHECKERBOARD_COLOR1, GUI_CHECKERBOARD_COLOR2);
    screen[y][x] = pixel; 

    // The midpoint center algorithm cannot fill all points on the screen. To fill those
    // points draw the circle line a little thicker by drawing two extra pixels on the dithering 
    // direction.
    if(isOnScreen(x+dithX, y+dithY)) screen[y+dithY][x+dithX] = pixel;
    if(isOnScreen(x-dithX, y-dithY)) screen[y-dithY][x-dithX] = pixel;
}

void homeScreen_() {
    // Function to print the OCXO Logo.
    // Each pixel of the logo has been converted into a 2 bit number. Buffer OCXOLogo contains these 
    // pixels from left to right, top to bottom. 

    // Fill background.
    // Instead of calculating for each pixel a sine + sqrt, let's traverse the screen in circles.
    int16_t maxRadius = ceilf(sqrtCORDIC(width*width + height*height)/2.0);
    int16_t t1, t2, x, y;
    for(uint16_t r = 0; r <= maxRadius; r++) {
        calculateRippleConstantsForCircle(r);

        // Midpoint circle algorithm.
        t1 = r >> 4;
        x = r;
        y = 0;
        while(x >= y) {
            // (x,y) is in the first octant and belongs to the circle. Replicate on the other 
            // octants to get the full circle.
            // Fill the empty spaces left by the midpoint circle algorithm. These are in radial 
            // direction, outside the circle.
            displacedCheckerboard(cx + x, cy + y, r,  1,  1);
            displacedCheckerboard(cx + y, cy + x, r,  1,  1);
            displacedCheckerboard(cx + y, cy - x, r, -1,  1);
            displacedCheckerboard(cx - x, cy + y, r, -1,  1);
            displacedCheckerboard(cx - x, cy - y, r, -1, -1);
            displacedCheckerboard(cx - y, cy - x, r, -1, -1);
            displacedCheckerboard(cx - y, cy + x, r,  1, -1);
            displacedCheckerboard(cx + x, cy - y, r,  1, -1);

            y++;
            t1 += y;
            t2 = t1 - x;
            if(t2 >= 0) {
                t1 = t2;
                x--;
            }
        }
    }

    // Draws logo on top of the background.

    // x0 and yTemp coords to center the logo image in the screen.
    int16_t x0 = (width - OCXOLogo.width) / 2;
    // Little animation on the logo. It goes "up" on the screen when y0 < 0. If y0 >= 0, remains
    // stationary.
    int16_t y0 = sinCORDIC(PI / 4 * time)*2.1;
    y0 = (height - OCXOLogo.height) / 2 + ((y0 < 0) ? y0 : 0);
    setCurrentPalette(TFT_BLACK, GUI_CHECKERBOARD_COLOR0, TFT_WHITE, TRANSPARENT);
    drawCompressedBitmap((Screen){width, height, &screen}, &OCXOLogo, x0, y0);
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
