#ifndef __STORAGE1_LIB_H
#define __STORAGE1_LIB_H

#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes

ret_code_t storage1_init(void);

ret_code_t storage1_store(uint32_t address, uint8_t* data, uint32_t length_data);


/**@brief   Function for reading the number of bytes in Storage1.
 *
 * @retval  Number of bytes in Storage1.
 */
uint32_t storage1_get_size(void);

#endif 
