/***************************************************************************************************
 * @file LIFO_u32.c
 * @brief A simple LIFO of unsigned 32 bit numbers implementation.
 * 
 * @project 
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#include "LIFO_u32.h"

void init_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t* dataArray, uint32_t bufferSize) {
    if(pLIFO_u32 == NULL || dataArray == NULL || bufferSize == 0) return;

    pLIFO_u32->data = dataArray;
    pLIFO_u32->size = bufferSize;
    pLIFO_u32->len  = 0;
    pLIFO_u32->head = 0;
}

void empty_LIFO_u32(LIFO_u32* pLIFO_u32) {
    if(pLIFO_u32 == NULL) return;

    pLIFO_u32->head = 0;
    pLIFO_u32->len  = 0;
    memset(pLIFO_u32->data, 0, pLIFO_u32->size);
}

uint8_t push_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t item) {
    pLIFO_u32->head = (pLIFO_u32->head + 1) % pLIFO_u32->size;
    pLIFO_u32->data[pLIFO_u32->head] = item;

    pLIFO_u32->len++;
    if(pLIFO_u32->len > pLIFO_u32->size) pLIFO_u32->len = pLIFO_u32->size;
    return 1;
}

uint8_t pop_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t* item) {
    if(pLIFO_u32->len < 1) return 0;

    if(item != NULL) {
        *item = pLIFO_u32->data[pLIFO_u32->head];
    }
    pLIFO_u32->head = (pLIFO_u32->head - 1) % pLIFO_u32->size;
    pLIFO_u32->len--;
    return 1;
}

uint8_t peek_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t* item) {
    if(pLIFO_u32->len < 1) return 0;
    
    *item = pLIFO_u32->data[pLIFO_u32->head];
    return 1;
}

uint8_t peekAt_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t index, uint32_t* item) {
    if(index >= pLIFO_u32->len) return 0;
    
    uint32_t headIndex = (pLIFO_u32->head - index) % pLIFO_u32->size;
    *item = pLIFO_u32->data[headIndex];
    return 1;
}

uint8_t freeN_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t count) {
    if(pLIFO_u32->len <= count) {
        pLIFO_u32->len = 0;
    }else{
        pLIFO_u32->len -= count;
    }
    return 1;
}
