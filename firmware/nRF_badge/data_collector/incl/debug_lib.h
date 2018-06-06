#ifndef __DEBUG_LIB_H
#define __DEBUG_LIB_H


// TODO: retrieving flag from Compiler
#define DEBUG_ENABLE


/** @file
 *
 * @brief Debug abstraction library.
 *
 * @details This module can be used to print formatted debug data to the UART-module.
 *			Internally the module uses the uart_lib-module to format and print stuff via UART.
 *			If debug is not enabled, the function calls are replaced by empty functions, causing the compiler to remove them.
 */
 


#ifdef DEBUG_ENABLE

#include "uart_lib.h"
uart_instance_t* p_debug_uart_instance;


/**
 * @brief Function for initializing the debug module.
 *
 * @details Initializes the module to use UART as output.
 * 			This function configures the UART parameters.
 */
void debug_init(void);

/**
 * @brief Function for logging debug messages.
 *
 * @details This function logs messages over UART. The module must be initialized before using this function.
 *
 * @note Although it is a macro, it can be used like a normal function
 */
 
#define debug_log(args...)  uart_printf(p_debug_uart_instance, args)

/**
 * @brief Dump auxiliary byte buffer to the debug trace.
 *
 * @details This Function logs messages over UART. The module must be initialized before using this function.
 * 
 * @param[in] p_buffer  Buffer to be dumped on the debug trace.
 * @param[in] len       Size of the buffer.
 */
void debug_log_dump(uint8_t * p_buffer, uint32_t len);

#else // DEBUG_ENABLE

#define debug_init(...)
#define debug_log(...)
#define debug_log_dump(...)

#endif // DEBUG_ENABLE


#endif //__DEBUG_LIB_H
