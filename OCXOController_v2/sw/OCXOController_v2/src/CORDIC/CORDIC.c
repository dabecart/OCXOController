#include "CORDIC.h"
#include "main.h" // Where hcordic is defined.

float sinCORDIC(float x) {
    CORDIC_ConfigTypeDef conf;
    conf.Function = CORDIC_FUNCTION_SINE;
    conf.Precision = CORDIC_PRECISION_6CYCLES;
    conf.Scale = CORDIC_SCALE_0;
    conf.NbWrite = CORDIC_NBWRITE_1;
    conf.NbRead = CORDIC_NBREAD_1;
    conf.InSize = CORDIC_INSIZE_32BITS;
    conf.OutSize = CORDIC_OUTSIZE_32BITS;
    HAL_CORDIC_Configure(&hcordic, &conf);

    // CORDIC needs as input the angle in radians [-pi, pi] but normalised [-1, 1] in Q1.31.
    int32_t in_q31 = floatToQ31(fmod(x, PI) / PI);
    int32_t out_q31 = 0;
    HAL_CORDIC_CalculateZO(&hcordic, &in_q31, &out_q31, 1, 10);
    return Q31ToFloat(out_q31);
}

float cosCORDIC(float x) {
    CORDIC_ConfigTypeDef conf;
    conf.Function = CORDIC_FUNCTION_COSINE;
    conf.Precision = CORDIC_PRECISION_6CYCLES;
    conf.Scale = CORDIC_SCALE_0;
    conf.NbWrite = CORDIC_NBWRITE_1;
    conf.NbRead = CORDIC_NBREAD_1;
    conf.InSize = CORDIC_INSIZE_32BITS;
    conf.OutSize = CORDIC_OUTSIZE_32BITS;
    HAL_CORDIC_Configure(&hcordic, &conf);

    int32_t in_q31 = floatToQ31(fmod(x, PI) / PI);
    int32_t out_q31 = 0;
    HAL_CORDIC_CalculateZO(&hcordic, &in_q31, &out_q31, 1, 10);
    return Q31ToFloat(out_q31);
}

int32_t floatToQ31(float x) {
    if(x >= 1.0f) {
        // Input float is over the allowed range by Q1.31 (1 - 2^-31)
        return 0x7FFFFFFF; 
    }
    if(x < -1.0f) {
        // Input float is below the allowed range by Q1.31 (-1.0)
        return 0x80000000;
    }
    // To convert to Q1.31, multiply by 2^31.
    return (int32_t)(x * 2147483648.0f);
}

float Q31ToFloat(int32_t x) {
    // To convert from Q1.31, divide by 2^31.
    return (float)(x) / 2147483648.0f;
}