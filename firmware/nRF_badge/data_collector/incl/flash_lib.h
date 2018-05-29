#ifndef __FLASH_LIB_H
#define __FLASH_LIB_H


// https://devzone.nordicsemi.com/f/nordic-q-a/18083/fds---pstorage---fstorage---which-to-use
// https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v12.0.0%2Flib_fstorage.html&cp=4_0_0_3_32


#include "stdlib.h" // for declaration of NULL-Pointer

#include "fstorage.h"

#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes




// TODO: Dispatch system event to fstorage-module!
// - Move UART, SPI, ADC opertation enum from .h into .c 






ret_code_t flash_init(void);

ret_code_t flash_store(uint32_t word_num, const uint32_t* p_words, uint16_t length_words);

#endif
