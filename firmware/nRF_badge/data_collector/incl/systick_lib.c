#include "systick_lib.h"

#include "app_timer.h"
#include "app_util_platform.h"	// Needed for the definitions of CRITICAL_REGION_EXIT/-ENTER


#define SYSTICK_TIMER_CALLBACK_PERIOD_MS (100*1000)

APP_TIMER_DEF(systick_timer);

static volatile uint64_t ticks_since_start = 0;			/**< Ticks of the 32768Hz oscillator since start/init()-call. */
static volatile uint32_t former_ticks = 0;				/**< The number of real RTC-ticks of the former systick-callback. */

static volatile uint8_t  millis_synced = 0;				/**< Flag if the systick is synced with an external time-source. */
static volatile uint64_t millis_offset = 0;				/**< The millis offset at ticks_at_offset setted externally. It is the y-axis offset of the straightline equation. */
static volatile uint64_t ticks_at_offset = 0;				/**< The ticks at the millis_offset. It is the x-value for the y-axis offset of the straightline equation. */
static volatile float millis_per_ticks = (1000.0f / ((0 + 1) * APP_TIMER_CLOCK_FREQ));		/**< Variable that represents the millis per ticks. It is the slope of the straigtline equation.*/
static float millis_per_ticks_default = (1000.0f / ((0 + 1) * APP_TIMER_CLOCK_FREQ));		/**< Variable that represents the default millis per ticks. It is the slope of the straigtline equation (needed for systick_get_continous_millis()).*/

static void systick_callback(void* p_context);

ret_code_t systick_init(uint8_t prescaler) {
	static uint8_t init_done = 0;
	
	if(!init_done) {
		ret_code_t ret;
		ticks_since_start = 0;
		former_ticks = app_timer_cnt_get();
		
		millis_synced = 0;
		millis_offset = 0;
		ticks_at_offset = systick_get_ticks_since_start();
		millis_per_ticks = (1000.0f / ((prescaler + 1) * APP_TIMER_CLOCK_FREQ));
		
		millis_per_ticks_default = (1000.0f / ((prescaler + 1) * APP_TIMER_CLOCK_FREQ));
		
		// Create the systick_timer
		ret = app_timer_create(&systick_timer, APP_TIMER_MODE_REPEATED, systick_callback);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;

		// Start the systick timer
		ret = app_timer_start(systick_timer, APP_TIMER_TICKS(SYSTICK_TIMER_CALLBACK_PERIOD_MS, 0), NULL);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;	
		
		init_done = 1;
	}
	
	return NRF_SUCCESS;
}


/**@brief Callback-handler function that is called when the systick-timer expires.
 *
 * @details To compensate a possibly deferred execution (> 100secs) of the callback (because
 *			of another job with higher or equal priority is not finished yet), a schema of 
 *			former_ticks and cur_ticks is used, and the calculation is based on these values
 *			not on the constant of ~100secs.
 *
 * @param[in] 	p_context	Pointer to context provided via the timer (should/could be NULL).
 */
static void systick_callback(void* p_context) {
	uint32_t diff_ticks;
	CRITICAL_REGION_ENTER();
	uint32_t cur_ticks = app_timer_cnt_get();
	
	app_timer_cnt_diff_compute(cur_ticks, former_ticks, &diff_ticks);
	former_ticks = cur_ticks;
	
	// Increment the ticks_since_start variable
	ticks_since_start += diff_ticks;	
	
	CRITICAL_REGION_EXIT();
}


uint64_t systick_get_ticks_since_start(void) {
	uint64_t tmp = 0;
	CRITICAL_REGION_ENTER();
	uint32_t diff_ticks;
	uint32_t cur_ticks = app_timer_cnt_get();
	app_timer_cnt_diff_compute(cur_ticks, former_ticks, &diff_ticks);
	tmp = ticks_since_start + diff_ticks;
	CRITICAL_REGION_EXIT();
	return tmp;
}


void systick_set_millis(uint64_t ticks_since_start_at_sync, uint64_t millis_sync) {
	uint64_t cur_ticks_since_start = systick_get_ticks_since_start();
	if(ticks_since_start_at_sync > cur_ticks_since_start)	// Only a safety query (the ticks_since_start_at_sync has to be <= the current ticks since start)
		ticks_since_start_at_sync = cur_ticks_since_start;
	
	if(ticks_since_start_at_sync < ticks_at_offset) {	// Only a safety query (the ticks_since_start_at_sync has to be >= ticks_at_offset)
		ticks_since_start_at_sync = ticks_at_offset;
	}
	
	if(!millis_synced) {		// If the function is called the first time, the millis_offset and ticks_at_offset is updated immediately.
		millis_offset 	= millis_sync;
		ticks_at_offset = ticks_since_start_at_sync;
		millis_synced = 1;
		return;
	}
	
	/*
	// Easiest way: Just set it
	CRITICAL_REGION_ENTER();
	millis_offset = millis_sync;
	ticks_at_offset = ticks_since_start_at_sync;
	CRITICAL_REGION_EXIT();
	*/
	
	// More complicate way: Adapt the slope (millis_per_ticks) via an moving average filter
	CRITICAL_REGION_ENTER();
	float delta_ticks = (float)(ticks_since_start_at_sync - ticks_at_offset);
	float delta_millis = (float)(millis_sync - millis_offset);
	float new_millis_per_ticks = delta_millis/delta_ticks;
	
	// Now average the new_millis_per_ticks with the global millis_per_ticks via an moving average filter:
	float alpha = 0.1;	// The exponential moving average filter coefficient. Has to be <= 1.
	millis_per_ticks = new_millis_per_ticks*alpha + millis_per_ticks*(1-alpha);
	
	// Calculate the new millis_offset via the new millis_per_ticks 
	millis_offset = ((uint64_t)(millis_per_ticks*(ticks_since_start_at_sync - ticks_at_offset))) + millis_offset;
	ticks_at_offset = ticks_since_start_at_sync;
	CRITICAL_REGION_EXIT();
	
	
	// TODO: Or an even more complex way: Linear regression of N samples!
	/*
	*/
	
	return;
}

void systick_set_timestamp(uint64_t ticks_since_start_at_sync, uint32_t seconds_sync, uint16_t milliseconds_sync) {
	uint64_t millis_sync = ((uint64_t)seconds_sync) * 1000 + ((uint64_t) milliseconds_sync);
	systick_set_millis(ticks_since_start_at_sync, millis_sync);
}

uint64_t systick_get_millis(void) {
	
	uint64_t cur_ticks_since_start = systick_get_ticks_since_start();
	uint64_t millis = 0;
	
	CRITICAL_REGION_ENTER();
	millis = ((uint64_t)(millis_per_ticks*(cur_ticks_since_start - ticks_at_offset))) + millis_offset;
	CRITICAL_REGION_EXIT();
	
	return millis;
}

uint64_t systick_get_continuous_millis(void) {
	uint64_t cur_ticks_since_start = systick_get_ticks_since_start();
	uint64_t millis = 0;
	
	CRITICAL_REGION_ENTER();
	millis = ((uint64_t)(millis_per_ticks_default*(cur_ticks_since_start - 0))) + 0;
	CRITICAL_REGION_EXIT();
	
	return millis;
	
}

void systick_delay_millis(uint64_t millis) {
	uint64_t end_millis = systick_get_millis() + millis;
	while(systick_get_millis() < end_millis);
}


uint8_t systick_is_synced(void) {
	return millis_synced;
}

void systick_get_timestamp(uint32_t* seconds, uint16_t* milliseconds) {
	uint64_t millis = systick_get_millis();
	*seconds = (uint32_t) (millis/1000);
	*milliseconds = (uint16_t) (millis % 1000);
}
