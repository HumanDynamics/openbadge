/**
 * Derived from app_trace from NRF51 SDK
 * for enabling debug logging in exclusively application-specific code.
 * (i.e. to ignore debug traces of Nordic libraries)
 */

#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <stdint.h>
#include <stdio.h>

/**
 * @defgroup app_trace Debug Logger
 * @ingroup app_common
 * @{
 * @brief Enables debug logs/ trace over UART.
 * @details Enables debug logs/ trace over UART. Tracing is enabled only if 
 *          ENABLE_DEBUG_LOG_SUPPORT is defined in the project.
 */
#ifdef DEBUG_LOG_ENABLE
/**
 * @brief Module Initialization.
 *
 * @details Initializes the module to use UART as trace output.
 * 
 * @warning This function will configure UART using default board configuration. 
 *          Do not call this function if UART is configured from a higher level in the application. 
 */
void debug_log_init(void);

/**
 * @brief Log debug messages.
 *
 * @details This API logs messages over UART. The module must be initialized before using this API.
 *
 * @note Though this is currently a macro, it should be used used and treated as function.
 */
#define debug_log printf

/**
 * @brief Dump auxiliary byte buffer to the debug trace.
 *
 * @details This API logs messages over UART. The module must be initialized before using this API.
 * 
 * @param[in] p_buffer  Buffer to be dumped on the debug trace.
 * @param[in] len       Size of the buffer.
 */
void debug_log_dump(uint8_t * p_buffer, uint32_t len);

#else // DEBUG_LOG_ENABLE

#define debug_log_init(...)
#define debug_log(...)
#define debug_log_dump(...)

#endif // DEBUG_LOG_ENABLE

/** @} */

#endif //DEBUG_LOG_H
