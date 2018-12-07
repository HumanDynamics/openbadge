#ifndef __BATERRY_LIB_H
#define __BATERRY_LIB_H

#include <stdbool.h>
#include "sdk_errors.h"

/**@brief Function to initialize the battery-module (with ADC)
 *
 * @retval     NRF_SUCCESS               If the timer was successfully created.
 * @retval     NRF_ERROR_INVALID_STATE   If the application timer module has not been initialized or
 *                                       the timer is running.
 * @retval     NRF_ERROR_NO_MEM          If the timer operations queue was full.
 * @retval     NRF_ERROR_INVALID_PARAM   If the specified peripheral in the adc_instance is not correct.
 *
 * @note App-timer has to be initialized before!
 * @note App-scheduler has to be initialized before!
 */
ret_code_t battery_init(void);

/**@brief Function to retrieve the internal averaged battery voltage.
 *
 * @retval The current averaged battery voltage.
 */
float battery_get_voltage(void);


/**@brief Function for testing the battery-module.
 *
 * @retval	0		If selftest failed.
 * @retval 	1		If selftest passed.
 *
 * @note	systick_init() has to be called before.
 */
bool battery_selftest(void);

#endif
