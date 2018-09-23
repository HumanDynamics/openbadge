#ifndef __BATERRY_LIB_H
#define __BATERRY_LIB_H

#include <stdbool.h>
#include "sdk_errors.h"

/**@brief Function to initialize the battery-module (with ADC)
 */
void battery_init(void);

/**@brief Function to read the current supply voltage in Volts.
 *
 * @param[out]	voltage		Read supply voltage in Volts.
 *
 * @retval 	NRF_SUCCESS		On success.
 * @retval	NRF_ERROR_BUSY	If the ADC-interface is busy.
 */
ret_code_t battery_read_voltage(float* voltage);


/**@brief Function for testing the battery-module.
 *
 * @retval	0		If selftest failed.
 * @retval 	1		If selftest passed.
 *
 * @note	systick_init() has to be called before.
 */
bool battery_selftest(void);

#endif
