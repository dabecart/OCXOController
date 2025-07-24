#include "GUI/Screen.h"
#include "MainMCU.h"
#include "GUI/Bitmaps.h"

OCXOChannel* outCh;
float out_screenInitTime = 0;

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

ScreenOut_State out_state = SCREEN_OUT_BOX_SELECTION;
ScreenOut_Box out_boxIndex = SCREEN_OUT_FREQUENCY;
int8_t fieldIndex = 1;

void drawSelectionArrows(Display d, int16_t x, int16_t yTop, int16_t yBot, uint16_t color) {
    (*d.buf)[yTop][x] = (*d.buf)[yTop][x-1] = (*d.buf)[yTop][x+1] = (*d.buf)[yTop+1][x] = 
    (*d.buf)[yBot][x] = (*d.buf)[yBot][x-1] = (*d.buf)[yBot][x+1] = (*d.buf)[yBot-1][x] = color;
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
    const uint16_t outputBoxWidth = 140;
    const uint16_t outputBoxHeight = 18;
    const uint16_t cursorFieldColor = reversed_color565(0xff, 0xdc, 0x8d); 
    const uint16_t selectedFieldArrowColor = reversed_color565(0xe9, 0x00, 0x04); 

    drawBox(d, x0, y0, outputBoxWidth, outputBoxHeight, 
            TFT_BLACK, 
            selected ? TFT_WHITE : reversed_color565(230,230,230));

    setCurrentOrigin(ORIGIN_LEFT | ORIGIN_TOP);
    if(selected && (out_state != SCREEN_OUT_BOX_SELECTION)) {
        // Draws orange background below the value.
        switch (fieldIndex) {
            case 1: {
                fillRectangle(d, x0 + 6, y0 + borderBoxSize, 
                              strlen(name)*Font_7x10.width, outputBoxHeight-2*borderBoxSize, 
                              cursorFieldColor);
                if(out_state == SCREEN_OUT_FIELD_EDITING) {
                    // If the value is selected, draw arrows above and below it.
                    int16_t x = x0 + 6 + (strlen(name)*Font_7x10.width) / 2;
                    int16_t yTop = y0 + 4, yBot = y0 + outputBoxHeight - borderBoxSize;
                    drawSelectionArrows(d, x, yTop, yBot, selectedFieldArrowColor);
                }
                break;
            }

            case 2: 
            case 3: 
            case 4: {
                int16_t xOffset = (fieldIndex - 2)*Font_7x10.width;
                fillRectangle(d, x0 + 61 + xOffset, y0 + borderBoxSize, 
                              Font_7x10.width, outputBoxHeight-2*borderBoxSize, 
                              cursorFieldColor);
                if(out_state == SCREEN_OUT_FIELD_EDITING) {
                    int16_t x = x0 + 61 + xOffset + (Font_7x10.width/2);
                    int16_t yTop = y0 + 4, yBot = y0 + outputBoxHeight - borderBoxSize;
                    drawSelectionArrows(d, x, yTop, yBot, selectedFieldArrowColor);
                }
                break;
            }

            case 5: 
            case 6: 
            case 7: {
                int16_t xOffset = (fieldIndex - 1)*Font_7x10.width;
                fillRectangle(d, x0 + 61 + xOffset, y0 + borderBoxSize, 
                              Font_7x10.width, outputBoxHeight-2*borderBoxSize, 
                              cursorFieldColor);
                if(out_state == SCREEN_OUT_FIELD_EDITING) {
                    int16_t x = x0 + 61 + xOffset + (Font_7x10.width/2);
                    int16_t yTop = y0 + 4, yBot = y0 + outputBoxHeight - borderBoxSize;
                    drawSelectionArrows(d, x, yTop, yBot, selectedFieldArrowColor);
                }
                break;
            }

            case 8: {
                fillRectangle(d, x0 + 116, y0 + borderBoxSize, 
                              strlen(units)*Font_7x10.width, outputBoxHeight-2*borderBoxSize, 
                              cursorFieldColor);
                if(out_state == SCREEN_OUT_FIELD_EDITING) {
                    int16_t x = x0 + 116 + (strlen(units)*Font_7x10.width) / 2;
                    int16_t yTop = y0 + 4, yBot = y0 + outputBoxHeight - borderBoxSize;
                    drawSelectionArrows(d, x, yTop, yBot, selectedFieldArrowColor);
                }
                break;
            }

            default:    break;
        }
    }

    setCurrentPalette(TFT_BLACK, TRANSPARENT, TRANSPARENT, TRANSPARENT);
    drawString(d, name, Font_7x10, x0 + 6, y0 + 4);
    drawString(d, value, Font_7x10, x0 + 61, y0 + 4);
    drawString(d, units, Font_7x10, x0 + 116, y0 + 4);
}

void drawSelectionBox(Display d, OCXOChannel* ch, int16_t x0, int16_t y0, 
                uint8_t selected, const char* name, const char* value) {
    const uint16_t outputBoxWidth = 140;
    const uint16_t outputBoxHeight = 16;
    const uint16_t cursorFieldColor = reversed_color565(0xff, 0xdc, 0x8d); 
    const uint16_t selectedFieldArrowColor = reversed_color565(0xe9, 0x00, 0x04); 

    drawBox(d, x0, y0, outputBoxWidth, outputBoxHeight, 
            TFT_BLACK, 
            selected ? TFT_WHITE : reversed_color565(230,230,230));

    setCurrentOrigin(ORIGIN_LEFT | ORIGIN_TOP);
    if(selected && (out_state != SCREEN_OUT_BOX_SELECTION)) {
        // Draws orange background below the value.
        switch (fieldIndex) {
            case 1: {
                fillRectangle(d, x0 + 61, y0 + borderBoxSize, 
                              strlen(name)*Font_7x10.width, outputBoxHeight-2*borderBoxSize, 
                              cursorFieldColor);
                if(out_state == SCREEN_OUT_FIELD_EDITING) {
                    // If the value is selected, draw arrows above and below it.
                    int16_t x = x0 + 6 + (strlen(name)*Font_7x10.width) / 2;
                    int16_t yTop = y0 + 4, yBot = y0 + outputBoxHeight - borderBoxSize;
                    drawSelectionArrows(d, x, yTop, yBot, selectedFieldArrowColor);
                }
                break;
            }

            default:    break;
        }
    }

    setCurrentPalette(TFT_BLACK, TRANSPARENT, TRANSPARENT, TRANSPARENT);
    drawString(d, name, Font_7x10, x0 + 6, y0 + 4);
    drawString(d, value, Font_7x10, x0 + 61, y0 + 4);
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

    // Menu boxes.
    drawNumberWithUnitsBox(d, outCh, 15, 25, out_boxIndex == SCREEN_OUT_FREQUENCY, 
                 "Freq", outCh->config.freq, outCh->config.freqUnits);
    drawNumberWithUnitsBox(d, outCh, 15, 47, out_boxIndex == SCREEN_OUT_DUTY_CYCLE, 
                 "Duty", outCh->config.duty, outCh->config.dutyUnits);
    drawNumberWithUnitsBox(d, outCh, 15, 69, out_boxIndex == SCREEN_OUT_PHASE, 
                 "Phase", outCh->config.phase, outCh->config.phaseUnits);
    drawSelectionBox(d, outCh, 15, 91, out_boxIndex == SCREEN_OUT_VOLTAGE, 
                 "Voltage", outCh->config.voltage);

    return 1;
}

void updateCellNumber(char* number, int8_t rotIncrement) {
    *number += rotIncrement;
    if(*number > '9')       *number = '9';
    else if(*number < '0')  *number = '0';
}

void updateFrequencyFields(int8_t rotIncrement) {
    switch(fieldIndex) {
        case 1: {
            break;
        }

        case 2: 
        case 3:
        case 4: {
            updateCellNumber(&outCh->config.freq[fieldIndex-2], rotIncrement);
            break;
        }

        case 5:
        case 6:
        case 7: {
            updateCellNumber(&outCh->config.freq[fieldIndex-1], rotIncrement);
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
            updateCellNumber(&outCh->config.duty[fieldIndex-2], rotIncrement);
            break;
        }

        case 5:
        case 6:
        case 7: {
            updateCellNumber(&outCh->config.duty[fieldIndex-1], rotIncrement);
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
            updateCellNumber(&outCh->config.phase[fieldIndex-2], rotIncrement);
            break;
        }

        case 5:
        case 6:
        case 7: {
            updateCellNumber(&outCh->config.phase[fieldIndex-1], rotIncrement);
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

    if(!rotButtonClicked || (rotIncrement == 0)) return;

    switch(out_state) {
        case SCREEN_OUT_BOX_SELECTION: {
            if(rotButtonClicked) {
                if(out_boxIndex == SCREEN_OUT_BACK) {
                    // Go to the previous screen.
                    requestScreenChange(SCREEN_MAIN, NULL);
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
            if(out_boxIndex == SCREEN_OUT_VOLTAGE) maxFieldsInBox = 3;

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