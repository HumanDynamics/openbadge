/**@file
 * @details This module provides high accuracy timing for the application. 
 *			To synchronize the internal timestamps/millis the set-functions could be used.
 *			Internally the timing works with an app_timer at a low frequency (~100secs).
 *			When this timer expires an internal ticks_since_start-variable is incremented.
 *			The actual computation of the current milliseconds/timestamp is based on a 
 *			straightline equation:
 *			millis = millis_per_ticks * (cur_ticks - ticks_at_offset) + millis_offset.
 *			The parameters millis_per_ticks, ticks_at_offset and millis_offset are set initial in the init-function
 *			and are updated each time a synchronize set-function is called.
 */

#ifndef __SYSTICK_LIB_H
#define __SYSTICK_LIB_H

#include "stdint.h"
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes


/**@brief Function for initializing the systick-module.
 * 
 * @details	The function creates the app-timer and starts it at a period of ~100 secs.
 *
 * @note 	The app_timer-module has to be initalized before, e.g. via APP_TIMER_INIT(...).
 *
 * @param[in]	prescaler		The prescaler of the app-timer module.
 *
 * @retval		NRF_SUCCESS			If the systick-module initialization was successful.
 * @retval		NRF_ERROR_INTERNAL	If there was an error while creating and/or starting the app-timer.
 */
ret_code_t 	systick_init(uint8_t prescaler);


/**@brief Function for retrieving the ticks since start.
 * 
 * @details	Theses ticks are an reflection of the 32768Hz-oscillator with the difference that 
 *			these ticks won't overflow at 0x00FFFFFF like the real oscillator ticks do.
 *
 * @retval	The ticks since start of the module (since the call of the init()-function).
 */
uint64_t 	systick_get_ticks_since_start(void);

/**@brief Function for retrieving the current millis.
 * 
 * @details	If the systick-module is unsynced (because no external timestamp was processed yet)
 *			the millis_offset variable of the straightline equation is 0 and ticks_at_offset 
 *			is the number of ticks at the init()-function call.
 *			Otherwise (if there was an external sync) the millis_offset and ticks_at_offset 
 *			are calculated (and set) in the systick_set_millis()-function.
 *
 * @retval	The current milliseconds of the system (synced or unsynced with an external time-source).
 */
uint64_t	systick_get_millis(void);

/**@brief Function for retrieving contoninuous milliseconds.
 * 
 * @details	This timebasis is not and could not be synchronized with an external time-source.
 *			Therefore, the milliseconds are continious incremented without any jumps.
 *			This is useful if the application wants to do timeout based stuff with time-deltas.
 *
 * @retval	The current continous milliseconds of the system (unsynced with an external time-source). Starts at 0.
 */
uint64_t systick_get_continuous_millis(void);


/**@brief Function for retrieving the current timestamp (seconds, milliseconds): E.g. 50 seconds and 400 milliseconds.
 * 
 * @details	Internally the function just calls the get_millis()-function to calculate the seconds and milliseconds.
 *
 * @param[out] seconds	Pointer to a variable where to store the current seconds of the system (synced or unsynced with an external time-source).
 * @param[out] seconds	Pointer to a variable where to store the current not finished milliseconds of a seconds (synced or unsynced with an external time-source).
 */
void 		systick_get_timestamp(uint32_t* seconds, uint16_t* milliseconds);

/**@brief Function for synchronizing the systick-module with an external time-source via milliseconds.
 * 
 * @details	The function needs the ticks since start at the sync timepoint (e.g. the timepoint a BLE-packet was actually received)
 *			This function adapts the millis_per_ticks parameters of the straightline equation with the use of an exponential moving average filter. 
 *			This method has the advantage of easy implementation and a stability against jitter in the BLE-stack or sth like that.
 *			
 * @param[in]	ticks_since_start_at_sync	The ticks since start (retrieved via systick_get_ticks_since_start()) at the sync-timepoint.
 * @param[in]	millis_sync					The milliseconds of the external time-source received at ticks_since_start_at_sync.
 */
void 		systick_set_millis(uint64_t ticks_since_start_at_sync, uint64_t millis_sync);

/**@brief Function for synchronizing the systick-module with an external time-source via an timestamp.
 * 
 * @details	Internally the function just calls the systick_set_millis()-function.
 *			
 * @param[in]	ticks_since_start_at_sync	The ticks since start (retrieved via systick_get_ticks_since_start()) at the sync-timepoint.
 * @param[in]	seconds_sync				The seconds of the external time-source received at ticks_since_start_at_sync.
 * @param[in]	milliseconds_sync			The milliseconds of the external time-source received at ticks_since_start_at_sync.
 */
void 		systick_set_timestamp(uint64_t ticks_since_start_at_sync, uint32_t seconds_sync, uint16_t milliseconds_sync);

/**@brief Function to check, whether the millis/timestamps of this module are synced with an external time-source or not.
 *
 * @retval	1	If synced with external time-source.
 * @retval	0	If not synced with external time-source.
 */
uint8_t 	systick_is_synced(void);

/**@brief Function for actively waiting for a specific amount of milliseconds.
 *			
 * @param[in]	millis		The number of milliseconds to actively delay the system.
 */
void 		systick_delay_millis(uint64_t millis);


#endif