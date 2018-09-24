/**@file
 */

#ifndef __CIRCULAR_FIFO_LIB_H
#define __CIRCULAR_FIFO_LIB_H

#include "stdint.h"
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes


/**@brief   A FIFO instance structure.
 * @details Keeps track of which bytes to read and write next.
 *          Also, it keeps the information about which memory is allocated for the buffer
 *          and its size. This structure must be initialized by circular_fifo_init() before use.
 */
typedef struct
{
    uint8_t *          p_buf;           /**< Pointer to FIFO buffer memory.                       	*/
    uint16_t           buf_size;   		/**< Size of the FIFO. 									  	*/
    volatile uint32_t  read_pos;        /**< Next read position in the FIFO buffer.               	*/
    volatile uint32_t  write_pos;       /**< Next write position in the FIFO buffer.              	*/
	volatile uint8_t   read_flag;		/**< Flag if currently reading, to synchronize with write.	*/
} circular_fifo_t;


/**@brief Macro to initialize a circular-fifo identifier and statically allocate memory for the circular-fifo.
 *
 * @param[in] ret		 			Name of the return-value variable that will be set through circular_fifo_init().
 * @param[in] circular_fifo 		Name (not pointer!) of the circular-fifo identifier variable that will be used to control the circular-fifo.
 * @param[in] buf_size				Number of elements storable in the circular fifo.
 */
#define CIRCULAR_FIFO_INIT(ret, circular_fifo, buf_size) { \
	static uint8_t circular_fifo##_fifo_buf[buf_size + 1]; \
	ret = circular_fifo_init(&circular_fifo, circular_fifo##_fifo_buf, buf_size); \
	(void) ret; \
}

/**@brief Function for initializing the FIFO.
 *
 * @param[out] p_fifo   FIFO object.
 * @param[in]  p_buf    FIFO buffer for storing data. The buffer size must be a power of two.
 * @param[in]  buf_size Size of the FIFO buffer provided. This size must be a power of two.
 *
 * @retval     NRF_SUCCESS              If initialization was successful.
 * @retval     NRF_ERROR_NULL           If a NULL pointer is provided as buffer.
 *
 * @note 	   	The buffer (p_buf) has to have a size of at least (buf_size + 1) bytes. 
 * 				That's why it's recommended to use the CIRCULAR_FIFO_INIT-macro to initzialize it.
 */
ret_code_t circular_fifo_init(circular_fifo_t * p_fifo, uint8_t * p_buf, uint16_t buf_size);


/**@brief Function for flushing the FIFO.
 *
 * @param[in]  p_fifo   Pointer to the FIFO.
 */
void circular_fifo_flush(circular_fifo_t * p_fifo);



/**@brief Function for adding an element to the FIFO.
 *
 * @param[in]  p_fifo   Pointer to the FIFO.
 * @param[in]  byte     Data byte to add to the FIFO.
 */
void circular_fifo_put(circular_fifo_t * p_fifo, uint8_t byte);
 
 
/**@brief Function for getting the next element from the FIFO.
 *
 * @param[in]  p_fifo   Pointer to the FIFO.
 * @param[out] byte   	Byte fetched from the FIFO.
 *
 * @retval     NRF_SUCCESS              If an element was returned.
 * @retval     NRF_ERROR_NOT_FOUND      If there are no more elements in the queue.
 */
ret_code_t circular_fifo_get(circular_fifo_t * p_fifo, uint8_t* byte);


/**@brief Function for reading bytes from the FIFO.
 *
 * @param[in]    p_fifo        	Pointer to the FIFO. Must not be NULL.
 * @param[out]   p_byte_array  	Memory pointer where the read bytes are fetched from the FIFO. 
 *								This field must not be NULL.
 * @param[inout] p_size        	Address to memory indicating the maximum number of bytes to be read.
 *                             	The provided memory is overwritten with the actual number of bytes
 *                             	read. This field must not be NULL.
 */
void circular_fifo_read(circular_fifo_t * p_fifo, uint8_t * p_byte_array, uint32_t * p_size);

/**@brief Function for writing bytes to the FIFO.
 *
 * @details	If size is larger than the number of available bytes in FIFO, the data is overwritten circulary. 
 *
 * @param[in]  p_fifo       Pointer to the FIFO. Must not be NULL.
 * @param[in]  p_byte_array Memory pointer containing the bytes to be written to the FIFO.
 *                          This field must not be NULL.
 * @param[in] size     		Number of bytes to be written.
 */
void circular_fifo_write(circular_fifo_t * p_fifo, uint8_t const * p_byte_array, uint32_t size);


/**@brief Function for retrieving the number of available bytes in the FIFO.
 *
 * @param[in]  p_fifo   Pointer to the FIFO.
 *
 * @retval     Number of available bytes in FIFO.
 */
uint32_t circular_fifo_get_size(circular_fifo_t * p_fifo);

#endif