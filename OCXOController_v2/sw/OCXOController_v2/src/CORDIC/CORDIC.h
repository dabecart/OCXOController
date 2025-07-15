#ifndef CORDIC_h
#define CORDIC_h

#include "stm32g4xx.h"
#include "stm32g4xx_hal_cordic.h"
#include "math.h"

#define PI 3.14159265f

float sinCORDIC(float x);
float cosCORDIC(float x);
int32_t floatToQ31(float x);
float Q31ToFloat(int32_t x);

#endif // CORDIC_h