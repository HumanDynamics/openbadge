#ifndef __UART_COMMANDS_LIB_H
#define __UART_COMMANDS_LIB_H

#include "sdk_errors.h"

/**@brief Function that initializes the serial-command support.
 *
 * @note This module uses the uart-instance of the debug-module 
 *		 (so when using this module you need to enable debugging and call debug_init() before this function), 
 *		 because the current implementation of uart_lib does not allow 
 *		 different uart-instances to send and to receive data.
 */
ret_code_t uart_commands_init(void);

#endif 
