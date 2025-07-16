#include "GUI.h"

uint16_t width = 0, height = 0;
float t = 0;
TFT* tft = NULL;
// With rotation taken into account, use [y][x] to access.
uint16_t screen[ST7735_WIDTH][ST7735_HEIGHT]; 
uint32_t delta;

uint8_t initGUI(GUI* gui, SPI_HandleTypeDef* hspi, DMA_HandleTypeDef* hdma_spi) {
    if(gui == NULL) return 0;

    tft = &gui->tft;

    // TFT is horizontal.
    initTFT(tft, hspi, 3);
    width = tft->width;
    height = tft->height;
    
    // Keep the TFT selected, there isn't any other device connected to the SPI device.
    selectTFT_(tft);
    // Set the adress window to be set to all the screen. All changes of the screen content must be
    // done to the "screen" variable.
    setAddressWindowTFT_(tft, 0, 0, width-1, height-1);

    homeScreen_();
    drawScreen_();

    return 1;
}

uint8_t updateGUI(GUI* gui) {
    static uint32_t lastTimeHere = 0;
    uint32_t t_ms = HAL_GetTick();

    if((t_ms - lastTimeHere) < GUI_REFRESH_PERIOD_ms) return 0;

    lastTimeHere = t_ms;
    
    t = t_ms/1000;
    homeScreen_();
    drawScreen_();

    delta = HAL_GetTick() - t_ms;

    return 1;
}

void rippleDisplacement(uint16_t x, uint16_t y, float time, uint16_t* xout, uint16_t* yout) {
    // Ripple center (center of screen)
    float cx = width / 2.0f;
    float cy = height / 2.0f;

    // Distance from center
    float dx = x - cx;
    float dy = y - cy;
    float dist = sqrtCORDIC(dx * dx + dy * dy);

    // Ripple distortion (adjust frequency, amplitude, and speed)
    float frequency = 0.15f;
    float amplitude = 5.0f;
    float speed = 0.8f;

    float ripple = sinCORDIC(dist * frequency - time * speed) * amplitude;

    // Apply radial distortion (toward/away from center)
    float scale = ripple / (dist + 1.0f);  // avoid division by 0
    float xd = x + dx * scale;
    float yd = y + dy * scale;

    *xout = xd;
    *yout = yd;
}

uint16_t checkerboard(uint16_t x, uint16_t y, uint16_t color1, uint16_t color2) {
    const uint16_t cellSize = 16;
    return ((((x / cellSize) + (y / cellSize)) % 2) == 0) ? color1 : color2;
}

uint16_t rainbowGradient(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    const float cosTheta = 0.7071067811f;
    const float sinTheta = 0.7071067811f;

    // Project (x, y) onto gradient axis
    // Normalize to [0, 1] based on projected distance
    float max_len = width * cosTheta + height * sinTheta;
    float projection = x * cosTheta + y * sinTheta;
    float normalized = fmod(projection, max_len) / max_len;  // wrap around

    // Convert to HSV and then to RGB
    uint8_t r, g, b;
    hsv2rgb(normalized, 1, 1, &r, &g, &b);

    return toColor565(r, g, b);
}

void hsv2rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
	int i = floor(h * 6);
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	
    float rf, gf, bf;
    switch (i % 6) {
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

void homeScreen_() {
    // Function to print the OCXO Logo.
    // Each pixel of the logo has been converted into a 2 bit number. Buffer OCXOLogo contains these 
    // pixels from left to right, top to bottom. 

    // x0 and y0 coords to center the image in the screen.
    uint16_t x0 = (width - LOGO_WIDTH) / 2;
    uint16_t y0 = (height - LOGO_HEIGHT) / 2;
    uint16_t xend = x0 + LOGO_WIDTH;
    uint16_t yend = y0 + LOGO_HEIGHT;

    // Fill background.

    uint16_t xd, yd;
    uint16_t blockIndex = 0;
    uint16_t logoArrayIndex = 0;

    uint16_t currentBlock = OCXOLogo[blockIndex];
    for(uint16_t y = 0; y < height; y++) {
        for(uint16_t x = 0; x < width; x++) {

            if((y >= y0) && (y < yend) && (x >= x0) && (x < xend)) {
                // This pixel is inside the logo.
                switch(currentBlock & 0b11) {
                    case 0b00:  screen[y][x] = TFT_BLACK;  break;
                    case 0b01:  screen[y][x] = TFT_WHITE;  break;
                    case 0b10:  screen[y][x] = TFT_WHITE;  break;
                    case 0b11:  break;  // Belongs to the background.
                }

                currentBlock >>= 2;
                blockIndex++;
                if(blockIndex >= 8) {
                    // Go to the next block.
                    blockIndex = 0;
                    logoArrayIndex++;
                    currentBlock = OCXOLogo[logoArrayIndex];
                }

                if((currentBlock & 0b11) != 0b11) continue;
            }

            rippleDisplacement(x, y, t, &xd, &yd);
            screen[y][x] = checkerboard(xd, yd, switched_color565(210,0,0), switched_color565(160,0,0));
            // pixelColor = rainbowGradient(x, y, width, height);
        }
    }

}

void drawScreen_() {
    writeDataTFT_DMA_(tft, (uint8_t*) &screen, sizeof(screen));
}
