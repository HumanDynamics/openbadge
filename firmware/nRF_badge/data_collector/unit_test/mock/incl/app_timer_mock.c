/** @file
 *
 * @details This mock-module enables the application to create multiple timer instances based on the
 *			the timer module that is implemented as a thread. It simulates the RTC-based app-timer 
 *			through the transformation of RTC-ticks and milliseconds.
 *			The app-timer must be enabled in the sdk_config.h-file (APP_TIMER_ENABLED 1).
 *
 * @note	Only a prescaler value of 0 is supported for the app_timer-mock module.
 *			The following functions are supported:
 *			app_timer_init, app_timer_create, app_timer_start, app_timer_stop, app_timer_stop_all,
 *			app_timer_cnt_get, app_timer_cnt_diff_compute.
 */

#include "app_timer.h"
#include "sdk_common.h"

#if NRF_MODULE_ENABLED(APP_TIMER)

#include "timer_lib.h"


#define MAX_NUMBER_OF_APP_TIMERS	MAX_NUMBER_OF_TIMERS					/**< Maximum number of app timers, is equal to the max number of timers of the timer-module */


#define MAX_RTC_COUNTER_VAL     0x00FFFFFF                                  /**< Maximum value of the RTC counter. */

/**< Macro for transforming RTC-ticks to milliseconds. */
#define APP_TIMER_TICKS_TO_MS(TICKS, PRESCALER) \
            ((uint32_t)((TICKS) * ((PRESCALER) + 1) * 1000)/ (uint64_t)APP_TIMER_CLOCK_FREQ)


uint32_t timer_ids[MAX_NUMBER_OF_APP_TIMERS];	/**< The mapping between app_timer-index and timer-index/timer-id. */

uint32_t number_of_app_timers = 0;				/**< The number of created app timers. */


/**@brief Function for initializing the timer module.
 *
 * Normally, initialization should be done using the APP_TIMER_INIT() macro, because that macro will both
 *       allocate the buffers needed by the timer module (including aligning the buffers correctly)
 *       and take care of connecting the timer module to the scheduler (if specified).
 *
 * @param[in]  prescaler           Value of the RTC1 PRESCALER register. Set to 0 for no prescaling. Only 0 is supported.
 * @param[in]  op_queue_size       Size of the queue holding timer operations that are pending
 *                                 execution. Note that due to the queue implementation, this size must
 *                                 be one more than the size that is actually needed.
 * @param[in]  p_buffer            Pointer to memory buffer for internal use in the app_timer
 *                                 module. The size of the buffer can be computed using the
 *                                 APP_TIMER_BUF_SIZE() macro. The buffer must be aligned to a
 *                                 4 byte boundary.
 * @param[in]  evt_schedule_func   Function for passing time-out events to the scheduler. Point to
 *                                 app_timer_evt_schedule() to connect to the scheduler. Set to NULL
 *                                 to make the timer module call the time-out handler directly from
 *                                 the timer interrupt handler.
 *
 * @retval     NRF_SUCCESS               If the module was initialized successfully.
 * @retval     NRF_ERROR_INVALID_PARAM   If a parameter was invalid (buffer NULL, op_queue_size > MAX_NUMBER_OF_APP_TIMERS or prescaler != 0).
 */
uint32_t app_timer_init(uint32_t                      prescaler,
                        uint8_t                       op_queue_size,
                        void *                        p_buffer,
                        app_timer_evt_schedule_func_t evt_schedule_func) 
{
	if(op_queue_size > MAX_NUMBER_OF_APP_TIMERS || p_buffer == NULL || prescaler != 0) {
		return NRF_ERROR_INVALID_PARAM;
	}
	
	timer_init();
	
	number_of_app_timers = 0;
	
	return NRF_SUCCESS;
}

/**@brief Function for creating a timer instance.
 *
 * @param[in]  p_timer_id        Pointer to timer identifier.
 * @param[in]  mode              Timer mode.
 * @param[in]  timeout_handler   Function to be executed when the timer expires.
 *
 * @retval     NRF_SUCCESS               If the timer was successfully created.
 * @retval     NRF_ERROR_INVALID_STATE   If the application timer module has not been initialized or
 *                                       the timer is running.
 *
 * @note This function does the timer allocation in the caller's context. It is also not protected
 *       by a critical region. Therefore care must be taken not to call it from several interrupt
 *       levels simultaneously.
 */
uint32_t app_timer_create(app_timer_id_t const *      p_timer_id,
                          app_timer_mode_t            mode,
                          app_timer_timeout_handler_t timeout_handler)
{
	
	
	uint32_t app_timer_id = number_of_app_timers;
	(*p_timer_id)->data[0] = app_timer_id;
	
	uint32_t t_id;
	timer_mode_t timer_mode = (mode == APP_TIMER_MODE_SINGLE_SHOT) ? TIMER_MODE_SINGLE_SHOT : TIMER_MODE_REPEATED;
	uint8_t ret = timer_create_timer(&t_id, timer_mode, (timer_timeout_handler_t) timeout_handler, 0);
	if(ret == 0)
		return NRF_ERROR_INVALID_STATE;
	
	
	timer_ids[app_timer_id] = t_id;
	
	
	number_of_app_timers++;
	
	
	return NRF_SUCCESS;
}


/**@brief Function for starting a timer.
 *
 * @param[in]       timer_id      Timer identifier.
 * @param[in]       timeout_ticks Number of ticks (of RTC1, including prescaling) to time-out event.
 * @param[in]       p_context     General purpose pointer. Will be passed to the time-out handler when
 *                                the timer expires.
 *
 * @retval     NRF_SUCCESS               If the timer was successfully stopped.
 */
uint32_t app_timer_start(app_timer_id_t timer_id, uint32_t timeout_ticks, void * p_context) {
	uint32_t app_timer_id = (*timer_id).data[0];
	uint32_t t_id = timer_ids[app_timer_id];
	
	uint64_t timeout_microseconds = ((uint64_t)APP_TIMER_TICKS_TO_MS(timeout_ticks, 0)) * 1000;
	
	uint8_t ret = timer_start_timer(t_id, timeout_microseconds, p_context);
	
	if(ret == 0)
		return NRF_ERROR_INVALID_STATE;
	return NRF_SUCCESS;
}

/**@brief Function for stopping the specified timer.
 *
 * @param[in]  timer_id                  Timer identifier.
 *
 * @retval     NRF_SUCCESS               If the timer was successfully stopped.
 */
uint32_t app_timer_stop(app_timer_id_t timer_id) {
	uint32_t app_timer_id = (*timer_id).data[0];
	uint32_t t_id = timer_ids[app_timer_id];
	
	timer_stop_timer(t_id);
	
	return NRF_SUCCESS;
}

/**@brief Function for stopping all running timers.
 *
 * @retval     NRF_SUCCESS               If all timers were successfully stopped.
 */
uint32_t app_timer_stop_all(void) {
	for(uint8_t i = 0; i < number_of_app_timers; i++) {
		uint32_t t_id = timer_ids[i];
		timer_stop_timer(t_id);	
	}
	return NRF_SUCCESS;
}

/**@brief Function for returning the current value of the simulated RTC1 counter (ticks).
 *
 * @return    Current value of the simulated RTC1 counter.
 */
uint32_t app_timer_cnt_get(void) {
	uint32_t ms = timer_get_milliseconds_since_start();
	uint32_t ticks = APP_TIMER_TICKS(ms, 0);
	
	// Map to 0 - MAX_RTC_COUNTER_VAL
	ticks = ticks % (MAX_RTC_COUNTER_VAL + 1);
	
	return ticks;
}


/**@brief Function for computing the difference between two RTC1 counter values.
 *
 * @return     Number of ticks elapsed from ticks_old to ticks_now.
 */
static uint32_t ticks_diff_get(uint32_t ticks_now, uint32_t ticks_old)
{
    return ((ticks_now - ticks_old) & MAX_RTC_COUNTER_VAL);
}

/**@brief Function for computing the difference between two RTC1 counter values.
 *
 * @param[in]  ticks_to       Value returned by app_timer_cnt_get().
 * @param[in]  ticks_from     Value returned by app_timer_cnt_get().
 * @param[out] p_ticks_diff   Number of ticks from ticks_from to ticks_to.
 *
 * @retval     NRF_SUCCESS   If the counter difference was successfully computed.
 */
uint32_t app_timer_cnt_diff_compute(uint32_t   ticks_to,
                                    uint32_t   ticks_from,
                                    uint32_t * p_ticks_diff) 
{
	*p_ticks_diff = ticks_diff_get(ticks_to, ticks_from);
    return NRF_SUCCESS;
}
#endif
