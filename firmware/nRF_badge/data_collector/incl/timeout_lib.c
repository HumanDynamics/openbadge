#include "timeout_lib.h"
#include "app_timer.h"
#include "systick_lib.h"
#include "stdio.h"
#include "stdlib.h" // Needed for NULL definition
#include "app_util_platform.h"

#define MAX_MILLIS_PER_TIMEOUT_TIMER	(100*1000)

typedef struct {
	volatile uint32_t 	remaining_ms;
	volatile uint32_t 	timeout_ms;
	volatile uint8_t 	active;
	timeout_handler_t 	timeout_handler;
} timeout_t;


static timeout_t registered_timeouts[MAX_NUMBER_OF_TIMEOUTS];
static uint32_t number_of_registered_timeouts = 0;

APP_TIMER_DEF(timeout_timer);
static void timeout_timer_callback(void* p_context);
volatile uint8_t timeout_timer_running = 0;
static volatile uint64_t timeout_timer_start_ms = 0;

// App-timer has to be initialized before
// Systick has to be initialized before
ret_code_t timeout_init(void) {
	number_of_registered_timeouts = 0;
	timeout_timer_running = 0;	
	
	ret_code_t ret = app_timer_create(&timeout_timer, APP_TIMER_MODE_SINGLE_SHOT, timeout_timer_callback);

	return ret;
}

ret_code_t timeout_register(uint32_t* timeout_id, timeout_handler_t timeout_handler) {
	if(number_of_registered_timeouts >= MAX_NUMBER_OF_TIMEOUTS)
		return NRF_ERROR_NO_MEM;
	
	*timeout_id = number_of_registered_timeouts;
	
	timeout_t timeout;
	timeout.timeout_handler = timeout_handler;
	timeout.active = 0;
	timeout.remaining_ms = 0;
	timeout.timeout_ms = 0;
	
	registered_timeouts[*timeout_id] = timeout;
	
	number_of_registered_timeouts++;
	return NRF_SUCCESS;
}

/**@brief Function to retrieve the minimal remaining milliseconds of all timeouts.
 *
 * @retval 	UINT32_MAX	If no active timeout was found.
 * @retval				Otherwise the minimal remaining milliseconds.
 */
static uint32_t get_minimal_remaining_ms(void) {
	uint32_t minimal_remaining_ms = UINT32_MAX;
	for(uint32_t i = 0; i < number_of_registered_timeouts; i++) {
		if(registered_timeouts[i].active) {
			if(registered_timeouts[i].remaining_ms < minimal_remaining_ms) {
				minimal_remaining_ms = registered_timeouts[i].remaining_ms;
			}
		}
	}
	
	if(minimal_remaining_ms == UINT32_MAX)
		return UINT32_MAX;
	
	minimal_remaining_ms = (minimal_remaining_ms > MAX_MILLIS_PER_TIMEOUT_TIMER) ? MAX_MILLIS_PER_TIMEOUT_TIMER : minimal_remaining_ms;
	// Because APP-Timer needs at least 5 Ticks...
	minimal_remaining_ms = (minimal_remaining_ms < 2) ? 2 : minimal_remaining_ms;
	
	return minimal_remaining_ms;
}

/**@brief Function that adapts the remaining milliseconds of all timeouts and calls the handler if necessary.
 *
 * @retval 	UINT32_MAX	If no active timeout was found.
 * @retval				Otherwise the minimal remaining milliseconds.
 */
static void adapt_remaining_ms(void) {
	
	uint32_t delta_ms = (uint32_t) (systick_get_continuous_millis() - timeout_timer_start_ms);
	for(uint32_t i = 0; i < number_of_registered_timeouts; i++) {
		if(registered_timeouts[i].active) {
			if(registered_timeouts[i].remaining_ms < delta_ms) {
				// Call the handler
				if(registered_timeouts[i].timeout_handler != NULL)
					registered_timeouts[i].timeout_handler();
				
				registered_timeouts[i].active = 0;
			} else {
				// Adapt remaining-ms
				registered_timeouts[i].remaining_ms -= delta_ms;
			}
		}
	}
	
}

/**@brief Handler that is called by the timeout timer.
 *
 * @param[in]	p_context	Pointer to context provided by the timer.
 */
static void timeout_timer_callback(void* p_context) {
	timeout_timer_running = 0;
	
	adapt_remaining_ms();
	
	uint32_t minimal_remaining_ms = get_minimal_remaining_ms();

	// If we have found another active timeout, we activate the timer again
	if(minimal_remaining_ms < UINT32_MAX) {
		timeout_timer_running = 1;
		timeout_timer_start_ms = systick_get_continuous_millis();
		app_timer_start(timeout_timer, APP_TIMER_TICKS(minimal_remaining_ms, 0), NULL);
	} 
}

ret_code_t timeout_start(uint32_t timeout_id, uint32_t timeout_ms) {
	ret_code_t ret = NRF_SUCCESS;
	
	if(timeout_id >= number_of_registered_timeouts)
		return NRF_ERROR_INVALID_PARAM;
	
	if(timeout_ms == 0) {
		registered_timeouts[timeout_id].active = 0;
		return NRF_SUCCESS;
	}

	if(timeout_timer_running) {
		app_timer_stop(timeout_timer);
		adapt_remaining_ms();
		timeout_timer_running = 0;
	}


	registered_timeouts[timeout_id].timeout_ms = timeout_ms;
	registered_timeouts[timeout_id].remaining_ms = timeout_ms;
	registered_timeouts[timeout_id].active = 1;

	
	uint32_t minimal_remaining_ms = get_minimal_remaining_ms();

	if(minimal_remaining_ms < UINT32_MAX) {		
		timeout_timer_running = 1;
		timeout_timer_start_ms = systick_get_continuous_millis();
		ret = app_timer_start(timeout_timer, APP_TIMER_TICKS(minimal_remaining_ms, 0), NULL);
	}

	return ret;
}

void timeout_stop(uint32_t timeout_id) {
	if(timeout_id >= number_of_registered_timeouts)
		return;
	
	registered_timeouts[timeout_id].active = 0;
	uint8_t found_active = 0;
	for(uint32_t i = 0; i < number_of_registered_timeouts; i++) {
		if(registered_timeouts[i].active) {
			found_active = 1;
			break;
		}
	}	
	// If there are no active timeouts anymore --> stop the timer
	if(!found_active) {
		timeout_timer_running = 0;
		app_timer_stop(timeout_timer);
	}	
}

void timeout_reset(uint32_t timeout_id) {
	if(registered_timeouts[timeout_id].active)
		timeout_start(timeout_id, registered_timeouts[timeout_id].timeout_ms);
}



