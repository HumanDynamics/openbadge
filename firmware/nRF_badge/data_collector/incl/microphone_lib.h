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
 *			It starts with recording of a reference microphone value.
 *			Based on this value, either the pattern noise -> no noise -> noise,
 *			or the pattern no noise -> noise -> no noise is searched. The
 *			threshold between no noise and noise is defined by the macro
 *			MICROPHONE_SELFTEST_THRESHOLD.
 *			The pattern has to be found in a max time of MICROPHONE_SELFTEST_TIME_FOR_NOISE_GENERATION_MS.
 *
 * @retval  0	If selftest failed.
 * @retval  1	If selftest passed.
 *
 * @note	systick_init() has to be called before.
 */ 
bool microphone_selftest(void);

#endif
