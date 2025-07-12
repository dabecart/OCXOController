/***************************************************************************************************
 * @file LIFO_d.c
 * @brief A simple LIFO of double (64 bit) numbers implementation.
 * 
 * @project 
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#include "LIFO_d.h"

void init_LIFO_d(LIFO_d* pLIFO_d, double* dataArray, uint32_t bufferSize) {
    if(pLIFO_d == NULL || dataArray == NULL || bufferSize == 0) return;

    pLIFO_d->data = dataArray;
    pLIFO_d->size = bufferSize;
    pLIFO_d->len  = 0;
    pLIFO_d->head = 0;
    pLIFO_d->locked = 0;
}

void empty_LIFO_d(LIFO_d* pLIFO_d) {
    if(pLIFO_d == NULL) return;

    pLIFO_d->head = 0;
    pLIFO_d->len  = 0;
    memset(pLIFO_d->data, 0, pLIFO_d->size);
}

uint8_t push_LIFO_d(LIFO_d* pLIFO_d, double item) {
    if(pLIFO_d->locked) return 0;

    pLIFO_d->head = (pLIFO_d->head + 1) % pLIFO_d->size;
    pLIFO_d->data[pLIFO_d->head] = item;

    pLIFO_d->len++;
    if(pLIFO_d->len > pLIFO_d->size) pLIFO_d->len = pLIFO_d->size;
    return 1;
}

uint8_t pop_LIFO_d(LIFO_d* pLIFO_d, double* item) {
    if(pLIFO_d->len < 1) return 0;

    if(item != NULL) {
        *item = pLIFO_d->data[pLIFO_d->head];
    }

    if(pLIFO_d->head == 0) pLIFO_d->head = pLIFO_d->size - 1;
    else pLIFO_d->head--;

    pLIFO_d->len--;
    return 1;
}

uint8_t peek_LIFO_d(LIFO_d* pLIFO_d, double* item) {
    if(pLIFO_d->len < 1) return 0;
    
    *item = pLIFO_d->data[pLIFO_d->head];
    return 1;
}

uint8_t peekAt_LIFO_d(LIFO_d* pLIFO_d, uint32_t index, double* item) {
    if(index >= pLIFO_d->len) return 0;
    
    uint32_t headIndex;
    if(pLIFO_d->head >= index) {
        headIndex = pLIFO_d->head - index;
    }else {
        // Index overflow.
        headIndex = pLIFO_d->size - (index - pLIFO_d->head);
    }
    *item = pLIFO_d->data[headIndex];
    return 1;
}

uint8_t freeN_LIFO_d(LIFO_d* pLIFO_d, uint32_t count) {
    if(pLIFO_d->len <= count) {
        pLIFO_d->len = 0;
    }else{
        pLIFO_d->len -= count;
    }
    return 1;
}
