/**@file
 * 	This module provides a timeout-handling mechanism.
 *	The application can register up to MAX_NUMBER_OF_TIMEOUTS timeouts.
 *	After starting the timeout (timeout_start) the application needs
 *	to reset the timeout. If it is not resetet in timeout_ms, 
 *	the timeout-handler is called.
 *
 * 	Internally, the module uses one app-timer to create alarms at certain intervals,
 *	to check for timeouts.
 */

#ifndef __TIMEOUT_LIB_H
#define __TIMEOUT_LIB_H

#include "stdint.h"
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes


#define MAX_NUMBER_OF_TIMEOUTS 									20		/**< Maximal number of registerable timeouts. */

/**@brief Timeout handler type. */
typedef void (*timeout_handler_t)(void);

/**@brief Function to initialize the timeout-module.
 *
 * @retval	NRF_SUCCESS					If the module was initialized successfully.
 * @retval	NRF_ERROR_INVALID_STATE		If the application timer module has not been initialized or
 *                                      the timer is running.
 *
 * @note app_timer_init() has to be called before.
 * @note systick_init() has to be called before.
 */
ret_code_t timeout_init(void);

/**@brief Function to register a timeout.
 *
 * @param[out]	timeout_id			Pointer to timeout_id that is set by this function.
 * @param[in]	timeout_handler		The timeout handler that is called when a timeout occurs.
 *
 * @retval	NRF_SUCCESS				If the module was initialized successfully.
 * @retval	NRF_ERROR_NO_MEM		If already MAX_NUMBER_OF_TIMEOUTS were registered.
 */
ret_code_t timeout_register(uint32_t* timeout_id, timeout_handler_t timeout_handler);

/**@brief Function to start a timeout.
 *
 * @param[in]	timeout_id			The timeout_id that should be started.
 * @param[in]	timeout_ms			The time in milliseconds after the timeout should occur.
 *									Could also be 0, then no timeout will be started.
 *
 * @retval	NRF_SUCCESS					 If the module was initialized successfully.
 * @retval	NRF_ERROR_INVALID_PARAM		 If timeout_id was not registered.
 * @retval  NRF_ERROR_INVALID_STATE   	 If the application timer module has not been initialized or the timer
 *                                       has not been created.
 * @retval  NRF_ERROR_NO_MEM          	 If the timer operations queue was full.
 */
ret_code_t timeout_start(uint32_t timeout_id, uint32_t timeout_ms);

/**@brief Function to stop a timeout.
 *
 * @param[in]	timeout_id			The timeout_id that should be stopped.
 */
void timeout_stop(uint32_t timeout_id);

/**@brief Function to reset a timeout.
 *
 * @param[in]	timeout_id			The timeout_id that should be resetted.
 */
void timeout_reset(uint32_t timeout_id);

#endif