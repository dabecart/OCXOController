/***************************************************************************************************
 * @file CircularBuffer.h
 * @brief A simple Circular or Ring buffer implementation.
 * 
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***********************************************************************************************/

#ifndef CIRCULAR_BUFFER_h
#define CIRCULAR_BUFFER_h

#include <string.h>
#include <stdint.h>

#define CIRCULAR_BUFFER_MAX_SIZE 1024

class CircularBuffer {
    public:
    /**
     * @brief Construct a new Circular Buffer object
     */
    CircularBuffer();

    /**
     * @brief Destroy the Circular Buffer object
     */
    ~CircularBuffer();

    /**
     * @brief Empties a CircularBuffer. 
     */
    void empty();

    /**
     * @brief Pushes a single byte into a CircularBuffer. Advances the tail index.
     * 
     * @param item. Byte to be store into the buffer.
     * @return uint8_t 1 if the push was successful. 
     */
    uint8_t push(uint8_t item);

    /**
     * @brief Pushes N bytes into a CircularBuffer. Advances the tail index.
     * 
     * @param items. Bytes to be stored into the buffer. 
     * @param count. Number of bytes to push.  
     * @return uint8_t 1 if the push was successful. 
     */
    uint8_t pushN(uint8_t* items, uint32_t count);

    /**
     * @brief Reads a byte from a CircularBuffer. Advances the head index.
     * 
     * @param item. Where the popped byte will be stored. 
     * @return uint8_t 1 if the read item is valid. 
     */
    uint8_t pop(uint8_t* item);

    /**
     * @brief Reads N bytes from a CircularBuffer. Advances the head index.
     * 
     * @param count. How many bytes want to be popped. 
     * @param items. Where the popped bytes will be stored. If it's NULL the indices will still be 
     * updated but no result will be returned. 
     * @return uint8_t 1 if the read items are valid. 
     */
    uint8_t popN(uint32_t count, uint8_t* items);

    /**
     * @brief Reads a byte from a CircularBuffer. Does not advance the head index.
     * 
     * @param item. Where the read byte will be stored. 
     * @return uint8_t 1 if the read item is valid. 
     */
    uint8_t peek(uint8_t* item);

    /**
     * @brief Reads N bytes from a CircularBuffer. Does not advance the head index.
     * 
     * @param count. How many bytes want to be peeked. 
     * @param items. Where the peeked bytes will be stored. 
     * @return uint8_t 1 if the read items are valid. 
     */
    uint8_t peekN(uint32_t count, uint8_t* items);

    /**
     * @brief Reads a bytes from a CircularBuffer at position "index". Does not advance the head 
     * index.
     * 
     * @param pCB. Pointer to the CircularBuffer struct.
     * @param index. The index into the array to look at.
     * @param items. Where the peeked byte will be stored.
     * @return 1 if the read item is valid. 
     */
    uint8_t peekAt(uint32_t index, uint8_t* item);

    /**
     * @brief The DMA functions automatically treats a buffer as a circular buffer. The callbacks 
     * return the new head of the buffer, so this function is used to update the head index 
     * accordingly.
     * 
     * @param newHeadIndex. The head index returned by the callback. 
     * @return uint8_t 1 if the update was OK. 
     */
    uint8_t updateIndices(uint32_t newHeadIndex);
    
    private:
    /**
     * @brief Returns only when the buffer isn't locked.
     */
    void lockRoutine();

    public:
    uint32_t    size = CIRCULAR_BUFFER_MAX_SIZE; // Full size of the buffer.    
    uint32_t    len = 0;                         // Number of bytes to read (stored bytes count).
    uint32_t    head = 0;                        // Index to read from.
    uint32_t    tail = 0;                        // Index to write to.
    uint8_t     locked = 0;                      // When locked, no modifications can be done.
    uint8_t     data[CIRCULAR_BUFFER_MAX_SIZE];  // Data buffer.
};

#endif // CIRCULAR_BUFFER_h