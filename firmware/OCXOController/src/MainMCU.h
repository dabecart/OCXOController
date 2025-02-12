#ifndef MAIN_MCU_h
#define MAIN_MCU_h

#include "stm32g431xx.h"
#include "stm32g4xx_hal.h"

#include "main.h"   // For the GPIO names.

#include "MathUtils.h"
#include "USBComms.h"
#include "LIFO_d.h"
#include "LIFO_u32.h"

#define PPS_TIMER_FREQ  160.0e6 // Hz
#define PPS_REF_FREQ    1.0     // Hz

// Number of previous edge times to be stored in memory (to calculate derivatives and integrals).
#define CONTROL_POINTS_IN_MEMORY 128
// Number of previous edge times to be stored in memory (to calculate current frequencies). They get
// deleted continuously.
#define CONTROL_CLOSE_POINTS_IN_MEMORY 4

// If the absolute value of the delta between the OCXO PPS and the reference PPS is less than this
// value, don't affect the VCO control voltage.  
// #define CONTROL_HYSTERESIS_ENABLED
#define CONTROL_HYSTERESIS 1e-6 // s

// Initial voltage generated for the VCO.
#define CONTROL_INITIAL_VCO 2048 // Start in the middle (the ADC is 12 bits: [0, 4096))

#define CONTROL_VCO_UPDATE_TIME_ms 10

// Depending on the voltage on the VCO pin of the OCXO, its frequency can vary +- this value.
#define OCXO_CONTROL_FREQUENCY_RANGE 7.0

// Take this number of frequency measurements to generate the min/max frequency range of the OCXO.
#define OCXO_CALIBRATION_MEASURE_COUNT 5
// Number of iterations to wait for the OCXO to stabilize with the new VCO voltage.
#define OCXO_CALIBRATION_STABILIZATION_COUNT 5
// Number of measurements to calculate the frequency during calibration.
#define OCXO_CALIBRATION_FREQUENCY_MEASUREMENTS 5

typedef struct HandlersMCU {
    DAC_HandleTypeDef* dacHandler;
    TIM_HandleTypeDef* freqDivHandler;
    TIM_HandleTypeDef* ppsHandler;
} HandlersMCU;

void mcuStart(DAC_HandleTypeDef* dacHandler, 
              TIM_HandleTypeDef* freqDivHandler, TIM_HandleTypeDef* ppsHandler);

void mcuLoop();

void calibrateOCXO(LIFO_d* freq);

double calculateFrequencyFromTimestamps_();

void step_controlMode_(double deltaTime);

void pid_controlMode_(LIFO_d* freq);

void calculateNewVCO_(LIFO_d* freq);

void processUSBMessage_(char* buf, uint32_t len);

void PPS_IRQ();

void freqDivider_IRQ();

uint8_t findMatchedTimestampsAndCalculateFrequency_(LIFO_u32* ppsRef, LIFO_u32* ocxo, 
                                                    LIFO_d* freqOut);

extern HandlersMCU mcuHandlers;

#endif // MAIN_MCU_h
