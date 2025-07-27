#ifndef OCXO_CONTROLLER_h
#define OCXO_CONTROLLER_h

#include "stm32g473xx.h"
#include "stm32g4xx_hal.h"

#include "USB/USBComms.h"
#include "buffers/LIFO_d.h"
#include "buffers/LIFO_u32.h"

/**
 * @brief 
 * 
 * @param ppsTim. TIM15. 
 * @param ocxoTim. TIM2.
 * @param ocxoFreqDividerTim. TIM5. 
 * @return uint8_t 
 */
uint8_t initOCXOController(TIM_HandleTypeDef* ppsTim, TIM_HandleTypeDef* ocxoTim, 
                           TIM_HandleTypeDef* ocxoFreqDividerTim);

void loopOCXOCOntroller();

void calibrateOCXO(LIFO_d* freq);

double calculateFrequencyFromTimestamps_();

void step_controlMode_(LIFO_d* freq);

void pid_controlMode_(LIFO_d* freq);

void calculateNewVCO_(LIFO_d* freq);

void processUSBMessage_(char* buf, uint32_t len);

// For TIM15. Timestamps the reference PPS.
void referencePPS_IRQ();

// For TIM2. Timestamps the divided OCXO.
void dividedOCXO_IRQ();

uint8_t findMatchedTimestampsAndCalculateFrequency_(LIFO_u32* ppsRef, LIFO_u32* ocxo, 
                                                    LIFO_d* freqOut);

double lerp(double x0, double y0, double x1, double y1, double x);

#endif // OCXO_CONTROLLER_h