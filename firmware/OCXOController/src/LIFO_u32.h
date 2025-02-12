/***************************************************************************************************
 * @file LIFO_u32.h
 * @brief A simple LIFO of unsigned 32 bit numbers implementation.
 * 
 * @project 
 * @version 1.0
 * @date    2024-12-07
 * @author  @dabecart
 * 
 * @license This project is licensed under the MIT License - see the LICENSE file for details.
***************************************************************************************************/

#ifndef LIFO_u32_h
#define LIFO_u32_h

#include <string.h>
#include <stdint.h>

typedef struct 
{
    uint32_t    size;                           // Full size of the buffer.    
    uint32_t    len;                            // Number of bytes to read (stored bytes count).
    uint32_t    head;                           // Index to read from. +1 to write to.
    uint32_t*   data;                           // Data buffer.
    uint8_t     locked;
} LIFO_u32;

/**************************************** FUNCTION *************************************************
 * @brief Starts a LIFO_u32.
 * @param pLIFO_u32. Pointer to the LIFO_u32 struct.
 * @param dataArray. Pointer to the data array.
 * @param bufferSize. Size of the buffer to be instantiated.
 * @return None 
***************************************************************************************************/
void init_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t* dataArray, uint32_t bufferSize);

/**************************************** FUNCTION *************************************************
 * @brief Empties a LIFO_u32.
 * @param pLIFO_u32. Pointer to the LIFO_u32 struct.
 * @return None 
***************************************************************************************************/
void empty_LIFO_u32(LIFO_u32* pLIFO_u32);

/**************************************** FUNCTION *************************************************
 * @brief Pushes a single byte into a LIFO_u32. Advances the head index.
 * @param pLIFO_u32. Pointer to the LIFO_u32 struct.
 * @param item. Byte to be store into the buffer.
 * @return 1 if the push was successful. 
***************************************************************************************************/
uint8_t push_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t item);

/**************************************** FUNCTION *************************************************
 * @brief Reads a byte from a LIFO_u32. Decrements the head index.
 * @param pLIFO_u32. Pointer to the LIFO_u32 struct.
 * @param item. Where the popped byte will be stored.
 * @return 1 if the read item is valid. 
***************************************************************************************************/
uint8_t pop_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t* item);

/**************************************** FUNCTION *************************************************
 * @brief Reads a byte from a LIFO_u32. Does not decrement the head index.
 * @param pLIFO_u32. Pointer to the LIFO_u32 struct.
 * @param item. Where the read byte will be stored.
 * @return 1 if the read item is valid. 
***************************************************************************************************/
uint8_t peek_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t* item);

/**************************************** FUNCTION *************************************************
 * @brief Reads a bytes from a LIFO_u32 at position "index". Does not decrement the head index.
 * @param pLIFO_u32. Pointer to the LIFO_u32 struct.
 * @param index. The index into the array to look at.
 * @param items. Where the peeked byte will be stored.
 * @return 1 if the read item is valid. 
***************************************************************************************************/
uint8_t peekAt_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t index, uint32_t* item);

/**************************************** FUNCTION *************************************************
 * @brief Makes room for "count" numbers. Removes numbers that were pushed first.
 * @param pLIFO_u32. Pointer to the LIFO_u32 struct.
 * @param count. Number of positions to free.
 * @return 1 if the freeing was successful. 
***************************************************************************************************/
uint8_t freeN_LIFO_u32(LIFO_u32* pLIFO_u32, uint32_t count);

#endif // LIFO_u32_h
