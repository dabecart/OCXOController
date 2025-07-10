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

CircularBuffer::CircularBuffer() {}
CircularBuffer::~CircularBuffer() {}

void CircularBuffer::empty() {
    head = 0;
    tail = 0;
    len  = 0;
    memset(data, 0, size);
}

uint8_t CircularBuffer::push(uint8_t ucItem) {
    if(len >= size) return 0;

    lockRoutine();

    data[head] = ucItem;
    head++;
    if(head >= size) head = 0;
    len++; 
    return 1;
}

uint8_t CircularBuffer::pushN(uint8_t* items, uint32_t count) {
    if(items == NULL) return 0;

    if((len + count) > size) return 0;
    
    lockRoutine();

    uint32_t ullNextHead = head + count;
    if(ullNextHead > size) {
        uint32_t ullHeadBytes = size - head;
        memcpy(data+head, items, ullHeadBytes);
        memcpy(data, items + ullHeadBytes, count - ullHeadBytes);
    }else {
        memcpy(data+head, items, count);
    }

    head = ullNextHead % size;
    len += count; 
    return 1;
}

uint8_t CircularBuffer::pop(uint8_t* item) {
    if(len < 1) return 0;

    lockRoutine();

    if(item != NULL) *item = data[tail];
    
    tail++;
    if(tail >= size) tail = 0;
    len--;
    return 1;
}

uint8_t CircularBuffer::popN(uint32_t count, uint8_t* items) {
    if(len < count) return 0;
    if(count == 0) return 1;
    
    lockRoutine();

    uint32_t nextTail = tail + count;
    if(items != NULL) {
        if(nextTail > size) {
            uint32_t tailBytes = size-tail;
            memcpy(items, data+tail, tailBytes);
            memcpy(items + tailBytes, data, count - tailBytes);
        }else {
            memcpy(items, data + tail, count);
        }
    }

    tail = nextTail % size;
    len -= count;
    return 1;
}

uint8_t CircularBuffer::peek(uint8_t* item) {
    if((len < 1) || (item == NULL)) return 0;
    
    *item = data[tail];
    return 1;
}

uint8_t CircularBuffer::peekN(uint32_t count, uint8_t* items) {
    if(items == NULL) return 0;
    if(count == 0) return 1;

    if(len < count) return 0;
    
    uint32_t nextTail = tail + count;
    if(nextTail > size) {
        uint32_t tailBytes = size-tail;
        memcpy(items, data+tail, tailBytes);
        memcpy(items + tailBytes, data, count - tailBytes);
    }else {
        memcpy(items, data + tail, count);
    }

    return 1;
}

uint8_t CircularBuffer::peekAt(uint32_t index, uint8_t* item) {
    if(item == NULL) return 0;

    if(len <= index) return 0;
    
    uint32_t nextTail = tail + index;
    if(nextTail > size) {
        uint32_t tailBytes = size - tail;
        *item = data[index - tailBytes];
    }else {
        *item = data[tail + index];
    }

    return 1;
}

// Useful for DMA circular buffers.
uint8_t CircularBuffer::updateIndices(uint32_t newHeadIndex)
{
    uint32_t readBytes = 0;
    if(newHeadIndex >= head) {
        readBytes = newHeadIndex - head;
    }else {
        readBytes = newHeadIndex + size - head;
    }

    // Is data being overwritten without being processed? 
    if((readBytes + len) > size) {
        // Update the tail index too.
        tail += readBytes - len;
        tail %= size;
        len = size;
    }else {
        len += readBytes;
    }

    head = newHeadIndex;

    return 1;
}

void CircularBuffer::lockRoutine()
{
    while(locked)
    {
        // Nothing to do but wait here.
    }
}
