#include "CORDIC.h"

CORDIC_HandleTypeDef* cordic;

#ifdef CORDIC_PRECISSION_16BIT

void initCORDIC(CORDIC_HandleTypeDef* c) {
    cordic = c;

    CORDIC_ConfigTypeDef conf;
    conf.Function = CORDIC_FUNCTION_SINE;
    conf.Precision = CORDIC_PRECISSION;
    conf.Scale = CORDIC_SCALE_0;
    // In 16 bit mode, the number of arguments is always 1. Which allows us to keep the 
    // configuration for all operations, only changing the function register.
    conf.NbWrite = CORDIC_NBWRITE_1;
    conf.NbRead = CORDIC_NBREAD_1;
    conf.InSize = CORDIC_INSIZE_16BITS;
    conf.OutSize = CORDIC_OUTSIZE_16BITS;
    HAL_CORDIC_Configure(cordic, &conf);
}

float sinCORDIC(float x) {
    MODIFY_REG(cordic->Instance->CSR, CORDIC_CSR_FUNC, CORDIC_FUNCTION_SINE);

    uint8_t in_q15[sizeof(uint32_t)] = {0}, out_q15[sizeof(uint32_t)] = {0};
    // Arg1: Angle in radians, normalized by pi.
    *((uint16_t*) in_q15) = floatToQ15(fmod(x, PI) / PI);
    // Arg2: modulus (set to 1).
    *((uint16_t*) (in_q15+2)) = 0x7FFF;

    cordic->Instance->WDATA = *((uint32_t*) in_q15);
    *((uint32_t*) out_q15) = cordic->Instance->RDATA;
    
    return Q15ToFloat(*((uint16_t*) out_q15));
}

float cosCORDIC(float x) {
    MODIFY_REG(cordic->Instance->CSR, CORDIC_CSR_FUNC, CORDIC_FUNCTION_COSINE);

    uint8_t in_q15[sizeof(uint32_t)] = {0}, out_q15[sizeof(uint32_t)] = {0};
    // Arg1: Angle in radians, normalized by pi.
    *((uint16_t*) in_q15) = floatToQ15(fmod(x, PI) / PI);
    // Arg2: modulus (set to 1).
    *((uint16_t*) (in_q15+2)) = 0x7FFF;

    cordic->Instance->WDATA = *((uint32_t*) in_q15);
    *((uint32_t*) out_q15) = cordic->Instance->RDATA;

    return Q15ToFloat(*((uint16_t*) out_q15));
}

float sqrtCORDIC(float x) {
    if(x < 0) return 0;

    // The idea: sqrt(x), where x = x_scaled * 2^n
    // Therefore,
    // sqrt(x_scaled * 2^n) = sqrt(x_scaled) * sqrt(2^n) = sqrt(x_scaled) * 2^(n/2)

    MODIFY_REG(cordic->Instance->CSR, CORDIC_CSR_FUNC, CORDIC_FUNCTION_SQUAREROOT);

    int n;
    float x_scaled = frexpf(x, &n);
    uint8_t rescaled = x_scaled >= CORDIC_MAX_SQRT_RANGE;
    if(rescaled) {
        // If the input (arg1) is over CORDIC_MAX_SQRT_RANGE, CORDIC won't converge.
        // Arg1 has to be scaled down for the algorithm to work.
        MODIFY_REG(cordic->Instance->CSR, CORDIC_CSR_SCALE, CORDIC_SCALE_1);
        x_scaled /= 2.0f;
    }

    uint8_t in_q15[sizeof(uint32_t)] = {0}, out_q15[sizeof(uint32_t)] = {0};
    // Arg1: x.
    *((uint16_t*) in_q15) = floatToQ15(x_scaled);
    // Arg2: none.

    cordic->Instance->WDATA = *((uint32_t*) in_q15);
    *((uint32_t*) out_q15) = cordic->Instance->RDATA;

    float out = Q15ToFloat(*((uint16_t*) out_q15));

    if(rescaled) {
        out *= 2.0f;
        // Restore the original scale.
        MODIFY_REG(cordic->Instance->CSR, CORDIC_CSR_SCALE, CORDIC_SCALE_0);
    }

    float oddScaleFactor = ((n & 0x1) == 0) ? 1.0f : SQRT2;
    return ldexpf(out * oddScaleFactor, n/2);
}

int16_t floatToQ15(float x) {
    if(x >= 1.0f) {
        return 0x7FFF; 
    }
    if(x < -1.0f) {
        return 0x8000;
    }
    return (int16_t)(x * 32768.0f);
}

float Q15ToFloat(int16_t x) {
    return (float)(x) / 32768.0f;
}

#endif