/**@file
 * @details This module provides a chunk-FIFO to store whole chunks with additional infos in a FIFO with a given size.
 *			One application of a chunk-FIFO is for example when different modules need to exchange data in a safe way.
 *			Safe means that while a chunk is not read by the application it couldn't be overwritten.
 *			In difference to other FIFO implementations with a put()- and a get()-function, this implementation
 *			provides two functions for opening and closing an operation like reading or writing.
 *			The open functions directly provide pointers that could be casted to any desired structure to work on.
 *			This has the benefit that no memcpy or sth like this has to be done. Important when used in ISRs.
 */

#ifndef __CHUNK_FIFO_LIB_H
#define __CHUNK_FIFO_LIB_H

#include "stdint.h"
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes


/**@brief   A chunk FIFO instance structure.
 * @details Keeps track of which bytes to read and write next.
 *          Also, it keeps the information about which memory is allocated for the buffer
 *          and its size. This structure must be initialized by chunk_fifo_init() before use.
 */
typedef struct
{
	uint8_t             chunk_num;   				/**< Number of chunks in chunk-fifo. */
    uint32_t            chunk_size;   				/**< Size of one chunk. */
    uint32_t            additional_info_size;   	/**< Size of one additional info. */
	uint8_t *           p_chunk_fifo_buf;     		/**< Pointer to FIFO buffer memory. */
    volatile uint8_t    chunk_read_pos;        		/**< Next read position in the chunk-fifo buffer. */
    volatile uint8_t    chunk_write_pos;       		/**< Next write position in the chunk-fifo buffer. */
	volatile uint8_t    chunk_open_read;        	/**< Flag if currently there is a chunk read operation in progress. */
    volatile uint8_t    chunk_open_write;       	/**< Flag if currently there is a chunk write operation in progress. */
} chunk_fifo_t;


/**@brief Macro to initialize a chunk-fifo identifier and statically allocate memory for the chunk-fifo.
 *
 * @param[in] ret		 			Name of the return-value variable that will be set through chunk_fifo_init().
 * @param[in] chunk_fifo 			Name (not pointer!) of the chunk-fifo identifier variable that will be used to control the chunk-fifo.
 * @param[in] chunk_num	 			Maximum number of chunks in FIFO.
 * @param[in] chunk_size			Size of one chunk.
 * @param[in] additional_info_size 	Size of one associated additional-info.
 */
#define CHUNK_FIFO_INIT(ret, chunk_fifo, chunk_num, chunk_size, additional_info_size) { \
	static uint8_t chunk_fifo##_fifo_buf[(chunk_num+1)*(chunk_size + additional_info_size)]; \
	ret = chunk_fifo_init(&chunk_fifo, chunk_num, chunk_size, additional_info_size, chunk_fifo##_fifo_buf); \
	(void) ret; \
}

/**@brief Function to initialize a chunk-fifo identifier.
 *
 * @param[in] chunk_fifo 			Pointer to chunk-fifo identifier variable.
 * @param[in] chunk_num	 			Maximum number of chunks in FIFO.
 * @param[in] chunk_size			Size of one chunk.
 * @param[in] additional_info_size 	Size of one associated additional-info.
 * @param[in] p_chunk_fifo_buf		Pointer to buffer memory where the chunks and additional-infos should be stored (Minimal size: (chunk_num + 1)*(chunk_size + additional_info_size)).
 *
 * @retval 	NRF_SUCCESS					If the intialization was successful.
 * @retval 	NRF_ERROR_INVALID_PARAM		If p_chunk_fifo_buf == NULL or chunk_num == 0.
 */
ret_code_t chunk_fifo_init(chunk_fifo_t* chunk_fifo, uint8_t chunk_num, uint32_t chunk_size, uint32_t additional_info_size, uint8_t* p_chunk_fifo_buf);


/**@brief Function to open a read operation of the next chunk in the chunk-FIFO.
 *
 * @details	This functions opens a read operation of the next chunk. 
 *			To work with the data (and additional-info) the application needs to provide the address
 *			of a pointer-variable. The data can the be accessed via this pointer-variable.
 *			This pointer-variable could be a pointer to a struct.
 *
 * @param[in] 	chunk_fifo 			Pointer to chunk-fifo identifier variable.
 * @param[out] 	p_chunk	 			Pointer to a pointer variable, where the chunk-data could be retrieved from. Could also be NULL.
 * @param[out] 	p_additional_info	Pointer to a pointer variable, where the chunk-additional-info could be retrieved from.
 *
 * @retval 	NRF_SUCCESS					If there is a chunk in the FIFO.
 * @retval 	NRF_ERROR_NOT_FOUND 		If there is no chunk in the FIFO.
 */
ret_code_t 	chunk_fifo_read_open(chunk_fifo_t* chunk_fifo, void** p_chunk, void** p_additional_info);

/**@brief Function to close/finish a read operation of the currently opened read chunk.
 *
 * @details	This functions closes/finishes the read operation of the currently opened read chunk.
 *			This is equal to the consummation of the chunk (like a normal get()-function) by incrementing the chunk_read_pos.
 *			The closing of the chunk and incrementing of chunk_read_pos only takes place if there was a read-opening operation (by chunk_fifo_read_open()) before.
 *
 * @param[in] 	chunk_fifo 			Pointer to chunk-fifo identifier variable.
 */
void 		chunk_fifo_read_close(chunk_fifo_t* chunk_fifo);

/**@brief Function to open a write operation of a chunk.
 *
 * @details	This functions opens a write operation of a chunk. 
 *			To write data (and additional-info) to the FIFO chunk the application needs to provide the address
 *			of a pointer-variable. The data can the be accessed(written via this pointer-variable.
 *			This pointer-variable could be a pointer to a struct.
 *			If all the available chunks are already filled, this functions will always set the pointer variable
*			to the same address until a read-operation was perforemed so the other chunks won't be overwritten.
 *
 * @param[in] 	chunk_fifo 			Pointer to chunk-fifo identifier variable.
 * @param[out] 	p_chunk	 			Pointer to a pointer variable, where the chunk-data could be written to.
 * @param[out] 	p_additional_info	Pointer to a pointer variable, where the chunk-additional-info could be written to. Could also be NULL.
 */
void 		chunk_fifo_write_open(chunk_fifo_t* chunk_fifo, void** p_chunk, void** p_additional_info);

/**@brief Function to close/finish a write operation of the currently opened write chunk.
 *
 * @details	This functions closes/finishes the write operation of the currently opened write chunk.
 *			This is equal to the put()-function of a normal FIFO by incrementing the chunk_write_pos, 
 *			except it would reach the chunk_read_pos or there was no write-opening operation (by chunk_fifo_write_open()) before.
 *
 * @param[in] 	chunk_fifo 			Pointer to chunk-fifo identifier variable.
 */
void 		chunk_fifo_write_close(chunk_fifo_t* chunk_fifo);

/**@brief Function to retrieve the current number of finished chunks in the chunk-FIFO.
 *
 * @param[in] chunk_fifo 			Pointer to chunk-fifo identifier variable.
 *
 * @retval 	Number of finished chunks.
 */
uint8_t 	chunk_fifo_get_number_of_chunks(chunk_fifo_t* chunk_fifo);

#endif