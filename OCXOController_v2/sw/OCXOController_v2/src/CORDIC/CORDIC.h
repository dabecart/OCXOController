#ifndef CORDIC_h
#define CORDIC_h

#include "stm32g4xx.h"
#include "stm32g4xx_hal_cordic.h"
#include "math.h"

#define CORDIC_PRECISSION_16BIT
// #define CORDIC_PRECISSION_32BIT // Not implemented.

#define CORDIC_PRECISSION CORDIC_PRECISION_3CYCLES

#define PI 3.14159265f
#define SQRT2 1.41421356f
#define CORDIC_MAX_SQRT_RANGE 0.75f

void initCORDIC(CORDIC_HandleTypeDef* hcordic);

float sinCORDIC(float x);
float cosCORDIC(float x);
float sqrtCORDIC(float x);

int32_t floatToQ31(float x);
float Q31ToFloat(int32_t x);
int16_t floatToQ15(float x);
float Q15ToFloat(int16_t x);

#endif // CORDIC_h