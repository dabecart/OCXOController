#include "GUI.h"
#include "MainMCU.h"

TFT tft;
TIM_HandleTypeDef* GUI_TIM;

// With rotation taken into account, use [y][x] to access.
DisplayBuffer displayBuf;
Display display;

// Used to monitor on the Real Time variables the time taken to render a frame.
uint32_t drawTime = 0;

// GUI state machine and variables.
ScreenID currentScreen = GUI_INITIAL_SCREEN;
ScreenID previousScreen = GUI_INITIAL_SCREEN;
Overlay transitionOverlay;
uint8_t currentlyTransitioning = 0;
uint8_t updateGUIInIRQ = 1;

volatile uint8_t screenReady = 0;
volatile uint8_t transferInProgress = 0;
volatile uint8_t missedDrawCall = 0;

uint8_t initGUI(SPI_HandleTypeDef* hspi, DMA_HandleTypeDef* hdma_spi, TIM_HandleTypeDef* guitim) {
    GUI_TIM = guitim;

    // TFT is horizontal.
    initTFT(&tft, hspi, 3);
    setDMATFT(&tft, hdma_spi);

    memset(&displayBuf, 0, sizeof(DisplayBuffer));
    display = (Display){tft.width, tft.height, &displayBuf};
    
    // Keep the TFT selected, there isn't any other device connected to the SPI device.
    selectTFT_(&tft);
    // Set the adress window to be set to all the display. All changes of the display content must 
    // be done to the buffer inside the "display" variable.
    setAddressWindowTFT_(&tft, 0, 0, tft.width-1, tft.height-1);

    // Start the display manager.
    initScreens();
    // Init variables of the initial screen.
    screens[currentScreen]->initScreen();

    // This timer is in charge of generating an IRQ to send the display buffer to the TFT display 
    // using DMA. This is only done if the display array is ready to be printed.
    HAL_TIM_Base_Start_IT(guitim);
    
    // Take some time of the initialization to show the logo.
    HAL_Delay(GUI_INITIAL_SCREEN_DELAY_ms);

    return 1;
}

void updateGUI() {
    // Cannot write to the display array if the transfer is in progress or if a previous display is 
    // ready to be transfered to the TFT.
    if(transferInProgress || screenReady) return;

    uint32_t initalT = HAL_GetTick();

    uint8_t updateDisplay = 0;
    if(currentlyTransitioning) {
        if(transitionOverlay.halfAnimationDone) {
            // If half the animation is done, draw the current screen.
            updateDisplay |= screens[currentScreen]->draw(display);
        }else {
            // If the overlay has recently started, draw the previous screen.
            updateDisplay |= screens[previousScreen]->draw(display);
        }

        // Draw the overlay over the screen.
        updateDisplay |= transitionOverlay.draw(&transitionOverlay, display);
    
        if(transitionOverlay.animationDone) {
            currentlyTransitioning = 0;
        }
    }else {
        updateDisplay |= screens[currentScreen]->draw(display);
    }

    drawTime =  HAL_GetTick() - initalT;

    screenReady = updateDisplay;

    // A drawing call was missed! By resetting the TIM it will retrigger the DMA transfer and the 
    // program will catch up with the TIM.
    if(missedDrawCall && hmain.initialized) {
        missedDrawCall = 0;
        __HAL_TIM_SET_COUNTER(GUI_TIM, 0);
    }
}

void requestScreenChange(ScreenID nextScreen) {
    if(nextScreen == currentScreen || currentlyTransitioning) return;

    previousScreen = currentScreen;
    currentScreen = nextScreen;

    screens[currentScreen]->initScreen();

    currentlyTransitioning = 
        createOverlay(&transitionOverlay, OVERLAY_CURTAIN_SWEEP_IN_LEFT_OUT_LEFT);
}

void transferScreenToTFT() {
    // Called when the GUI TIM restarts.
    
    // The GUI timer will be in charge of incrementing the ticks as it has higher priority than the 
    // SysTick_Handler.
    if(hmain.doingInitialization) uwTick += 1000.0f / GUI_FPS;

    // Ignore until the updateGUI() catches up and resets the TIM. Only do this when not in 
    // initialization.
    if(hmain.initialized && missedDrawCall) return;

    // Update the display during the initialization process inside the TIM IRQ.
    if(updateGUIInIRQ) updateGUI();

    if(screenReady) {
        screenReady = 0;
        writeDataTFT_DMA_(&tft, (uint8_t*) &displayBuf, sizeof(displayBuf));
        transferInProgress = 1;
    }else if(!transferInProgress) {
        // The screen was not ready when it should have.
        missedDrawCall = 1;
    }

    // Knowing the frequency at which the timer reloads we can get a better time measurement than if
    // we were using the GetTick function.
    guiTime += 1.0f / GUI_FPS;
}

void transferToTFTEnded() {
    // Called when DMA is done.
    transferInProgress = 0;
}
