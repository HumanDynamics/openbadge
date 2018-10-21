#ifndef __MICROPHONE_LIB_H
#define __MICROPHONE_LIB_H

#include <stdbool.h>
#include "sdk_errors.h"


/**@brief Function to initialize the microphone-module (with ADC)
 */
void microphone_init(void);


/**@brief Function to read the current microphone value.
 *
 * @param[out]	value		Read microphone value.
 *
 * @retval 	NRF_SUCCESS		On success.
 * @retval	NRF_ERROR_BUSY	If the ADC-interface is busy.
 */
ret_code_t microphone_read(uint8_t* value) ;


/**@brief   Function for testing the microphone module.
 *
 * @details When the function records average microphone values over ~50ms.
 *			It searches for a specific pattern {no noise, noise, no noise, noise}
 *			of so called "sections".
 *			During the tests: When no noise -> both LEDs on, if noise -> both LEDs off.
 *
 * @retval  0	If selftest failed.
 * @retval  1	If selftest passed.
 *
 * @note	systick_init() has to be called before.
 */ 
bool microphone_selftest(void);

#endif
