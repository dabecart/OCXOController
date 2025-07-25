#include "GUI/Screen.h"
#include "MainMCU.h"
#include "GUI/Bitmaps.h"

OCXOChannel* outCh;
float out_screenInitTime = 0;

const uint16_t out_BoxWidth = 140;
const uint16_t out_BoxHeight = 22;
const uint16_t out_cursorFieldColor = reversed_color565(0xff, 0xdc, 0x8d); 
const uint16_t out_selectedFieldArrowColor = reversed_color565(0xe9, 0x00, 0x04); 
const int16_t out_arrowSelectionHeight = 3;

typedef enum ScreenOut_State {
    SCREEN_OUT_BOX_SELECTION = 0,
    SCREEN_OUT_FIELD_SELECTION,
    SCREEN_OUT_FIELD_EDITING,
    SCREEN_OUT_MAX_STATE_INDEX, // // Used to get the maximum of ScreenOut_State.
} ScreenOut_State;

typedef enum ScreenOut_Box {
    SCREEN_OUT_BACK = 0,
    SCREEN_OUT_FREQUENCY,
    SCREEN_OUT_DUTY_CYCLE,
    SCREEN_OUT_PHASE,
    SCREEN_OUT_VOLTAGE,
    SCREEN_OUT_MAX_BOX_INDEX,   // Used to get the maximum of ScreenOut_Box.
} ScreenOut_Box;

int8_t out_state = SCREEN_OUT_BOX_SELECTION;
int8_t out_boxIndex = SCREEN_OUT_FREQUENCY;
int8_t fieldIndex = 1;

void drawSelectionArrows(Display d, int16_t x0, int16_t yTop, int16_t yBot, uint16_t color) {
    // Draw a piramid (arrow):
    // y = 0   x x x x x
    // y = 1     x x x
    // y = 2       x
    
    for(int16_t y = 0; y < out_arrowSelectionHeight; y++) {
        for(int16_t x = -(out_arrowSelectionHeight-1-y); x <= (out_arrowSelectionHeight-1-y); x++) {
            // Top arrow.
            (*d.buf)[yTop + y][x0 + x] = color;        
            // Bot arrow.
            (*d.buf)[yBot - y][x0 + x] = color;        
        }
    }
}

void drawLeftmostArrow(Display d, int16_t x0, int16_t y0, uint8_t selected) {
    if(selected) {
        setCurrentOrigin(ORIGIN_CENTER | ORIGIN_RIGHT);

        if(out_state == SCREEN_OUT_BOX_SELECTION) {
            setCurrentPalette(TFT_BLACK, TRANSPARENT, TFT_WHITE, TRANSPARENT);
            drawBitmap(d, &rightArrow, x0-3, y0+out_BoxHeight/2 - 1);
        }else {
            if(fieldIndex == 0) {
                setCurrentPalette(TFT_BLACK, TRANSPARENT, out_cursorFieldColor, TRANSPARENT);
            }else {
                setCurrentPalette(TFT_BLACK, TRANSPARENT, TFT_WHITE, TRANSPARENT);
            }
            drawBitmap(d, &leftArrow, x0-3, y0+out_BoxHeight/2 - 1);
        }
    }
}

// Value box:   name  value   units
// Example:     Freq. 100.000 kHz

// selected: 0/1 if the box is selected
// cursorField: on which field is the cursor.
//              - 0: to exit the box.
//              - 1: name
//              - 2 to 7: value
//              - 8: units
// fieldSelected: 0/1 if the field is selected.
void drawNumberWithUnitsBox(Display d, OCXOChannel* ch, int16_t x0, int16_t y0, 
                            uint8_t selected, const char* name, const char* value, const char* units) {
    drawBox(d, x0, y0, out_BoxWidth, out_BoxHeight, 
            TFT_BLACK, 
            selected ? TFT_WHITE : reversed_color565(230,230,230));

    drawLeftmostArrow(d, x0, y0, selected);

    setCurrentOrigin(ORIGIN_LEFT | ORIGIN_TOP);
    if(selected && (out_state != SCREEN_OUT_BOX_SELECTION)) {
        // Draws orange background below the value.
        switch (fieldIndex) {
            case 1: {
                fillRectangle(d, x0 + 6, y0 + borderBoxSize, 
                              strlen(name)*Font_7x10.width, out_BoxHeight-2*borderBoxSize, 
                              out_cursorFieldColor);
                if(out_state == SCREEN_OUT_FIELD_EDITING) {
                    // If the value is selected, draw arrows above and below it.
                    int16_t x = x0 + 6 + (strlen(name)*Font_7x10.width) / 2;
                    int16_t yTop = y0 + borderBoxSize, yBot = y0 + out_BoxHeight - borderBoxSize -1;
                    drawSelectionArrows(d, x, yTop, yBot, out_selectedFieldArrowColor);
                }
                break;
            }

            case 2: 
            case 3: 
            case 4: {
                int16_t xOffset = (fieldIndex - 2)*Font_7x10.width;
                fillRectangle(d, x0 + 61 + xOffset, y0 + borderBoxSize, 
                              Font_7x10.width, out_BoxHeight-2*borderBoxSize, 
                              out_cursorFieldColor);
                if(out_state == SCREEN_OUT_FIELD_EDITING) {
                    int16_t x = x0 + 61 + xOffset + (Font_7x10.width/2);
                    int16_t yTop = y0 + borderBoxSize, yBot = y0 + out_BoxHeight - borderBoxSize -1;
                    drawSelectionArrows(d, x, yTop, yBot, out_selectedFieldArrowColor);
                }
                break;
            }

            case 5: 
            case 6: 
            case 7: {
                int16_t xOffset = (fieldIndex - 1)*Font_7x10.width;
                fillRectangle(d, x0 + 61 + xOffset, y0 + borderBoxSize, 
                              Font_7x10.width, out_BoxHeight-2*borderBoxSize, 
                              out_cursorFieldColor);
                if(out_state == SCREEN_OUT_FIELD_EDITING) {
                    int16_t x = x0 + 61 + xOffset + (Font_7x10.width/2);
                    int16_t yTop = y0 + borderBoxSize, yBot = y0 + out_BoxHeight - borderBoxSize -1;
                    drawSelectionArrows(d, x, yTop, yBot, out_selectedFieldArrowColor);
                }
                break;
            }

            case 8: {
                fillRectangle(d, x0 + 116, y0 + borderBoxSize, 
                              strlen(units)*Font_7x10.width, out_BoxHeight-2*borderBoxSize, 
                              out_cursorFieldColor);
                if(out_state == SCREEN_OUT_FIELD_EDITING) {
                    int16_t x = x0 + 116 + (strlen(units)*Font_7x10.width) / 2;
                    int16_t yTop = y0 + borderBoxSize, yBot = y0 + out_BoxHeight - borderBoxSize -1;
                    drawSelectionArrows(d, x, yTop, yBot, out_selectedFieldArrowColor);
                }
                break;
            }

            default:    break;
        }
    }

    setCurrentOrigin(ORIGIN_CENTER | ORIGIN_LEFT);
    setCurrentPalette(TFT_BLACK, TRANSPARENT, TRANSPARENT, TRANSPARENT);
    drawString(d, name, Font_7x10, x0 + 6, y0 + out_BoxHeight/2+1);
    drawString(d, value, Font_7x10, x0 + 61, y0 + out_BoxHeight/2+1);
    drawString(d, units, Font_7x10, x0 + 116, y0 + out_BoxHeight/2+1);
}

void drawSelectionBox(Display d, OCXOChannel* ch, int16_t x0, int16_t y0, 
                uint8_t selected, const char* name, const char* value) {
    drawBox(d, x0, y0, out_BoxWidth, out_BoxHeight, 
            TFT_BLACK, 
            selected ? TFT_WHITE : reversed_color565(230,230,230));

    drawLeftmostArrow(d, x0, y0, selected);

    setCurrentOrigin(ORIGIN_LEFT | ORIGIN_TOP);
    if(selected && (out_state != SCREEN_OUT_BOX_SELECTION)) {
        // Draws orange background below the value.
        switch (fieldIndex) {
            case 1: {
                fillRectangle(d, x0 + 61, y0 + borderBoxSize, 
                              strlen(name)*Font_7x10.width, out_BoxHeight-2*borderBoxSize, 
                              out_cursorFieldColor);
                if(out_state == SCREEN_OUT_FIELD_EDITING) {
                    // If the value is selected, draw arrows above and below it.
                    int16_t x = x0 + 6 + (strlen(name)*Font_7x10.width) / 2;
                    int16_t yTop = y0 + borderBoxSize, yBot = y0 + out_BoxHeight - borderBoxSize -1;
                    drawSelectionArrows(d, x, yTop, yBot, out_selectedFieldArrowColor);
                }
                break;
            }

            default:    break;
        }
    }

    setCurrentPalette(TFT_BLACK, TRANSPARENT, TRANSPARENT, TRANSPARENT);
    setCurrentOrigin(ORIGIN_CENTER | ORIGIN_LEFT);
    drawString(d, name, Font_7x10, x0 + 6, y0 + out_BoxHeight/2+1);
    drawString(d, value, Font_7x10, x0 + 61, y0 + out_BoxHeight/2+1);
}

void outScreen_initScreen(void** screenArgs) {
    out_screenInitTime = guiTime;
    if(screenArgs != NULL) {
        outCh = (OCXOChannel*) screenArgs[0];
    }else {
        outCh = NULL;
    }

    out_state = SCREEN_OUT_BOX_SELECTION;
    out_boxIndex = SCREEN_OUT_FREQUENCY;
}

uint8_t outScreen_draw(Display d) {
    const float backgroundValue1 = 0.82;
    const float backgroundValue2 = 0.63;

    // Full rotation of background color every two minutes.
    float backgroundHue = fmod(guiTime - out_screenInitTime, 120.0f) / 120.0f;

    uint8_t r, g, b;
    hsv2rgb(backgroundHue, 1.0f, backgroundValue1, &r, &g, &b);
    GUI_CHECKERBOARD_COLOR1 = toColor565Reversed(r, g, b);
    hsv2rgb(backgroundHue, 1.0f, backgroundValue2, &r, &g, &b);
    GUI_CHECKERBOARD_COLOR2 = toColor565Reversed(r, g, b);

    // Draw background.
    checkerboardBackgroundMirrored(d, guiTime);

    // Draw small logo.
    setCurrentOrigin(ORIGIN_LEFT | ORIGIN_TOP);
    setCurrentPalette(TFT_BLACK, TRANSPARENT, TFT_WHITE, TRANSPARENT);
    drawBitmap(d, &miniOCXOLogo, 106, 4);

    if(outCh == NULL) {
        setCurrentOrigin(ORIGIN_CENTER | ORIGIN_MIDDLE);
        drawString(d, "No channel selected!", Font_7x10, d.width/2, d.height/2);
        return 1;
    }

    drawBox(d, 32, 4, 60, 16, TFT_BLACK, TFT_WHITE);
    
    char screenHeader[] = "Ch  ";
    screenHeader[3] = '0' + outCh->id;
    setCurrentOrigin(ORIGIN_CENTER | ORIGIN_MIDDLE);
    drawString(d, screenHeader, Font_7x10, 62, 13);

    if(out_state == SCREEN_OUT_BOX_SELECTION && out_boxIndex == SCREEN_OUT_BACK) {
        setCurrentPalette(TFT_BLACK, TRANSPARENT, out_cursorFieldColor, TRANSPARENT);
    }else {
        setCurrentPalette(TFT_BLACK, TRANSPARENT, TFT_WHITE, TRANSPARENT);
    }
    setCurrentOrigin(ORIGIN_LEFT | ORIGIN_TOP);
    drawBitmap(d, &backArrow, 3, 5);

    // Menu boxes.
    drawNumberWithUnitsBox(d, outCh, 15, 25, out_boxIndex == SCREEN_OUT_FREQUENCY, 
                 "Freq", outCh->config.freq, outCh->config.freqUnits);
    drawNumberWithUnitsBox(d, outCh, 15, 51, out_boxIndex == SCREEN_OUT_DUTY_CYCLE, 
                 "Duty", outCh->config.duty, outCh->config.dutyUnits);
    drawNumberWithUnitsBox(d, outCh, 15, 78, out_boxIndex == SCREEN_OUT_PHASE, 
                 "Phase", outCh->config.phase, outCh->config.phaseUnits);
    drawSelectionBox(d, outCh, 15, 105, out_boxIndex == SCREEN_OUT_VOLTAGE, 
                 "Voltage", outCh->config.voltage);

    return 1;
}

void updateCellNumber(char* number, int8_t position, int8_t rotIncrement) {
    if(position < 0 || position == 3 || position > 6) return;

    number[position] += rotIncrement;

    int8_t nextPosition = position - 1;
    if(nextPosition == 3) nextPosition = 2;

    if(number[position] > '9') {
        number[position] = '0';
        updateCellNumber(number, nextPosition, 1);
    }else if(number[position] < '0') {
        number[position] = '9';
        updateCellNumber(number, nextPosition, -1);
    }  
}

void updateFrequencyFields(int8_t rotIncrement) {
    switch(fieldIndex) {
        case 1: {
            break;
        }

        case 2: 
        case 3:
        case 4: {
            updateCellNumber(outCh->config.freq, fieldIndex-2, rotIncrement);
            break;
        }

        case 5:
        case 6:
        case 7: {
            updateCellNumber(outCh->config.freq, fieldIndex-1, rotIncrement);
            break;
        }

        case 8: {
            // Find the current units of frequency in the "frequencyUnits" array.
            int8_t index = 0;
            for(; index < frequencyUnitsLen; index++) {
                if(strcmp(outCh->config.freqUnits, frequencyUnits[index]) == 0) {
                    break;
                }
            }

            if(index >= frequencyUnitsLen) {
                // Could not find it... Set index to something inside the array.
                index = 4; // Hz
            }

            // Increment/decrement units.
            index += rotIncrement;
            if(index >= frequencyUnitsLen)  index = frequencyUnitsLen - 1;
            else if(index < 0)              index = 0;

            strncpy(outCh->config.freqUnits, 
                    frequencyUnits[index], 
                    sizeof(outCh->config.freqUnits));

            break;
        }

        default: break;
    }

    applyOCXOOutputFromConfiguration(&hmain.chOuts, outCh->id);
}

void updateDutyCycleFields(int8_t rotIncrement) {
    switch(fieldIndex) {
        case 1: {
            break;
        }

        case 2: 
        case 3:
        case 4: {
            updateCellNumber(outCh->config.duty, fieldIndex-2, rotIncrement);
            break;
        }

        case 5:
        case 6:
        case 7: {
            updateCellNumber(outCh->config.duty, fieldIndex-1, rotIncrement);
            break;
        }

        case 8: {
            strncpy(outCh->config.dutyUnits, "%", sizeof(outCh->config.dutyUnits));
            break;
        }

        default: break;
    }

    applyOCXOOutputFromConfiguration(&hmain.chOuts, outCh->id);
}

void updatePhaseFields(int8_t rotIncrement) {
    switch(fieldIndex) {
        case 1: {
            break;
        }

        case 2: 
        case 3:
        case 4: {
            updateCellNumber(outCh->config.phase, fieldIndex-2, rotIncrement);
            break;
        }

        case 5:
        case 6:
        case 7: {
            updateCellNumber(outCh->config.phase, fieldIndex-1, rotIncrement);
            break;
        }

        case 8: {
            // Find the current units of phase in the "timeUnits" array.
            int8_t index = 0;
            for(; index < timeUnitsLen; index++) {
                if(strcmp(outCh->config.phaseUnits, timeUnits[index]) == 0) {
                    break;
                }
            }

            if(index >= timeUnitsLen) {
                // Could not find it... Set index to something inside the array.
                index = 0; // ns
            }

            // Increment/decrement units.
            index += rotIncrement;
            if(index >= timeUnitsLen)  index = timeUnitsLen - 1;
            else if(index < 0)         index = 0;

            strncpy(outCh->config.phaseUnits, 
                    timeUnits[index], 
                    sizeof(outCh->config.phaseUnits));

            break;
        }

        default: break;
    }

    applyOCXOOutputFromConfiguration(&hmain.chOuts, outCh->id);
}

void updateVoltageFields(int8_t rotIncrement) {
    switch(fieldIndex) {
        case 1: {
            // Find the current voltage in the "voltageTags" array.
            int8_t index = 0;
            for(; index < voltageTagsLen; index++) {
                if(strcmp(outCh->config.voltage, voltageTags[index]) == 0) {
                    break;
                }
            }

            if(index >= voltageTagsLen) {
                // Could not find it... Set index to something inside the array.
                index = 0; // 5V
            }

            // Increment/decrement units.
            index += rotIncrement;
            if(index >= voltageTagsLen) index = voltageTagsLen - 1;
            else if(index < 0)          index = 0;

            strncpy(outCh->config.voltage, 
                    voltageTags[index], 
                    sizeof(outCh->config.voltage));

            break;
        }

        default: break;
    }

    applyOCXOOutputFromConfiguration(&hmain.chOuts, outCh->id);
}

void outScreen_updateInput() {
    int8_t rotIncrement = getFilteredRotaryIncrement(&hmain.gpio.rot);
    uint8_t rotButtonClicked = wasButtonClicked(&hmain.gpio, BUTTON_ROT);

    if(!rotButtonClicked && (rotIncrement == 0)) return;

    switch(out_state) {
        case SCREEN_OUT_BOX_SELECTION: {
            if(rotButtonClicked) {
                if(out_boxIndex == SCREEN_OUT_BACK) {
                    // Go to the previous screen.
                    requestScreenChange(SCREEN_MAIN, NULL, 0);
                }else {
                    // Go to field selection.
                    out_state = SCREEN_OUT_FIELD_SELECTION;
                    fieldIndex = 1; // Field 0 is to go back to BOX_SELECTION.
                }
                return;
            }

            out_boxIndex += rotIncrement;
            // Do not allow rollover.
            if(out_boxIndex >= SCREEN_OUT_MAX_BOX_INDEX) out_boxIndex = SCREEN_OUT_MAX_BOX_INDEX-1;
            else if(out_boxIndex < 0) out_boxIndex = 0;
            break;
        }

        case SCREEN_OUT_FIELD_SELECTION: {
            if(rotButtonClicked) {
                if(fieldIndex == 0) {
                    // Go to box selection.
                    out_state = SCREEN_OUT_BOX_SELECTION;
                }else {
                    // Go to field editing.
                    out_state = SCREEN_OUT_FIELD_EDITING;
                }
                return;
            }

            fieldIndex += rotIncrement;
            
            // Do not allow rollover.
            int8_t maxFieldsInBox = 9;
            if(out_boxIndex == SCREEN_OUT_VOLTAGE) maxFieldsInBox = 2;

            if(fieldIndex >= maxFieldsInBox) fieldIndex = maxFieldsInBox-1;
            else if(fieldIndex < 0) fieldIndex = 0;
            break;
        }

        case SCREEN_OUT_FIELD_EDITING: {
            if(rotButtonClicked) {
                // Go back to field selection.
                out_state = SCREEN_OUT_FIELD_SELECTION;
                return;
            }

            switch(out_boxIndex) {
                case SCREEN_OUT_FREQUENCY:  updateFrequencyFields(rotIncrement);    break;    
                case SCREEN_OUT_DUTY_CYCLE: updateDutyCycleFields(rotIncrement);    break;    
                case SCREEN_OUT_PHASE:      updatePhaseFields(rotIncrement);        break;
                case SCREEN_OUT_VOLTAGE:    updateVoltageFields(rotIncrement);      break;    
                default:                    break;
            }

            break;
        }

        default: return;
    }

}

Screen outScreen = {
    .id = SCREEN_OUT, 
    .initScreen = outScreen_initScreen,
    .draw = outScreen_draw, 
    .updateInput = outScreen_updateInput
};
