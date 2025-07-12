/***************************************************************************************************
 * @file CircularBuffer.c
 * @brief A simple Circular or Ring buffer implementation.
 * 
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#include "CircularBuffer.h"

void init_cb(CircularBuffer* cb, uint8_t* buffer, uint32_t bufferSize) {
    if(cb == NULL || buffer == NULL) return;

    cb->data = buffer;
    cb->size = bufferSize;
    
    cb->len = 0;
    cb->head = 0;
    cb->tail = 0;
    cb->locked = 0;
}

void empty_cb(CircularBuffer* cb) {
    cb->head = 0;
    cb->tail = 0;
    cb->len  = 0;
    memset(cb->data, 0, cb->size);
}

uint8_t push_cb(CircularBuffer* cb, uint8_t ucItem) {
    if(cb->len >= cb->size) return 0;

    lockRoutine_cb_(cb);

    cb->data[cb->head] = ucItem;
    cb->head++;
    if(cb->head >= cb->size) cb->head = 0;
    cb->len++; 
    return 1;
}

uint8_t pushN_cb(CircularBuffer* cb, uint8_t* items, uint32_t count) {
    if(items == NULL) return 0;

    if((cb->len + count) > cb->size) return 0;
    
    lockRoutine_cb_(cb);

    uint32_t ullNextHead = cb->head + count;
    if(ullNextHead > cb->size) {
        uint32_t ullHeadBytes = cb->size - cb->head;
        memcpy(cb->data+cb->head, items, ullHeadBytes);
        memcpy(cb->data, items + ullHeadBytes, count - ullHeadBytes);
    }else {
        memcpy(cb->data+cb->head, items, count);
    }

    cb->head = ullNextHead % cb->size;
    cb->len += count; 
    return 1;
}

uint8_t pop_cb(CircularBuffer* cb, uint8_t* item) {
    if(cb->len < 1) return 0;

    lockRoutine_cb_(cb);

    if(item != NULL) *item = cb->data[cb->tail];
    
    cb->tail++;
    if(cb->tail >= cb->size) cb->tail = 0;
    cb->len--;
    return 1;
}

uint8_t popN_cb(CircularBuffer* cb, uint32_t count, uint8_t* items) {
    if(cb->len < count) return 0;
    if(count == 0) return 1;
    
    lockRoutine_cb_(cb);

    uint32_t nextTail = cb->tail + count;
    if(items != NULL) {
        if(nextTail > cb->size) {
            uint32_t tailBytes = cb->size-cb->tail;
            memcpy(items, cb->data+cb->tail, tailBytes);
            memcpy(items + tailBytes, cb->data, count - tailBytes);
        }else {
            memcpy(items, cb->data + cb->tail, count);
        }
    }

    cb->tail = nextTail % cb->size;
    cb->len -= count;
    return 1;
}

uint8_t peek_cb(CircularBuffer* cb, uint8_t* item) {
    if((cb->len < 1) || (item == NULL)) return 0;
    
    *item = cb->data[cb->tail];
    return 1;
}

uint8_t peekN_cb(CircularBuffer* cb, uint32_t count, uint8_t* items) {
    if(items == NULL) return 0;
    if(count == 0) return 1;

    if(cb->len < count) return 0;
    
    uint32_t nextTail = cb->tail + count;
    if(nextTail > cb->size) {
        uint32_t tailBytes = cb->size-cb->tail;
        memcpy(items, cb->data+cb->tail, tailBytes);
        memcpy(items + tailBytes, cb->data, count - tailBytes);
    }else {
        memcpy(items, cb->data + cb->tail, count);
    }

    return 1;
}

uint8_t peekAt_cb(CircularBuffer* cb, uint32_t index, uint8_t* item) {
    if(item == NULL) return 0;

    if(cb->len <= index) return 0;
    
    uint32_t nextTail = cb->tail + index;
    if(nextTail > cb->size) {
        uint32_t tailBytes = cb->size - cb->tail;
        *item = cb->data[index - tailBytes];
    }else {
        *item = cb->data[cb->tail + index];
    }

    return 1;
}

// Useful for DMA circular buffers.
uint8_t updateIndices_cb(CircularBuffer* cb, uint32_t newHeadIndex)
{
    uint32_t readBytes = 0;
    if(newHeadIndex >= cb->head) {
        readBytes = newHeadIndex - cb->head;
    }else {
        readBytes = newHeadIndex + cb->size - cb->head;
    }

    // Is cb->data being overwritten without being processed? 
    if((readBytes + cb->len) > cb->size) {
        // Update the cb->tail index too.
        cb->tail += readBytes - cb->len;
        cb->tail %= cb->size;
        cb->len = cb->size;
    }else {
        cb->len += readBytes;
    }

    cb->head = newHeadIndex;

    return 1;
}

void lockRoutine_cb_(CircularBuffer* cb)
{
    while(cb->locked) {
        // Nothing to do but wait here.
    }
}
