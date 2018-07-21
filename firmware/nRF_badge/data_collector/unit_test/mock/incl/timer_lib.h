/** @file
 *
 *
 * @brief Timer implementation to call timeout-handlers at specified intervals.
 *
 * @details This module provides the functionality to create and start timer that expires at specific intervals.
 *			If a timer expires a timeout-handler is called.
 *			
 *
 * @note	This module is not based on RTOS, just on normal pthreads. So the main-context can still be executed, 
 *			although the timeout-handler is currently executed.
 *			Each timeout-handler is completely executed before another/next timeout-handler could be called.
 */

#ifndef TIMER_LIB_H
#define TIMER_LIB_H

#include "stdint.h"



#define MAX_NUMBER_OF_TIMERS	100		/**< Maximum number of timers that could be handled */

#define TIMER_PRESCALER			1		/**< Prescaler for the timer. 1 --> time is like the real time. 10 --> time is 10x faster than the real time. */



typedef void (*timer_timeout_handler_t)(void * p_context);	/**< The timeout handler function type. */

typedef enum
{
    TIMER_MODE_SINGLE_SHOT,                 /**< The timer will expire only once. */
    TIMER_MODE_REPEATED                     /**< The timer will restart each time it expires. */
} timer_mode_t;

typedef struct {
	uint32_t 				timer_id;				/**< The timer identifier. */
	
	uint64_t 				microseconds_at_start; 	/**< The startpoint of the timer. */
	uint64_t 				microseconds_at_end;	/**< The endpoint of the timer (when the time-out handler should be called). */
	uint64_t				microseconds_interval;	/**< The interval of the timer in microseconds. */
	
	uint8_t 				timer_priority;			/**< The priority of the timer. */
	
	uint8_t					is_running;				/**< Flag if the timer is running. */
	timer_mode_t            mode;					/**< The timer mode. */
	volatile void * 		next;					/**< Pointer to the next timer-node in queue. */
	timer_timeout_handler_t p_timeout_handler;		/**< The timer identifier. */
	void *                 	p_context;				/**< The timer identifier. */
	
	
} timer_node_t;										/**< Timer-node-struct for managing a timer. */


/**@brief Function for initializing the timer-module.
 *
 * @details	The function resets the number of timers to 0, and starts the internal timer-thread.
 *
 */
void 		timer_init(void);

/**@brief Function for stopping the timer-module.
 *
 * @details	The function stops the internal timer-thread.
 *
 */
void 		timer_stop(void);


/**@brief Function to set the time reference to the current time.
 */
void 		timer_set_start(void);

/**@brief Function to retrieve the current time in microseconds since the reference timepoint.
 *
 * @retval	Microseconds since the reference timepoint.
 */
uint64_t 	timer_get_microseconds_since_start(void);

/**@brief Function to retrieve the current time in milliseconds since the reference timepoint.
 *
 * @retval	Milliseconds since the reference timepoint.
 */
uint64_t 	timer_get_milliseconds_since_start(void);

/**@brief Function to sleep in microseconds.
 *
 * @param[in]	Microseconds to sleep.
 */
void 		timer_sleep_microseconds(uint64_t microseconds);

/**@brief Function to sleep in milliseconds.
 *
 * @param[in]	Milliseconds to sleep.
 */
void 		timer_sleep_milliseconds(uint64_t milliseconds);

/**@brief Function to create a timer.
 *
 * @details	The function creates a timer-node with a certain priority.
 *			The priority is just in case there are two timers that expire exactly at the same time (same microsecond),
 *			then the timer with the higher priority will be executed first.
 *
 * @param[out]	timer_id		The identifier of the created timer.
 * @param[in]	timer_mode		The mode (single_shot, repeated) of the timer.
 * @param[in]	timeout_handler	The handler that should be called when the timer expires.
 * @param[in]	timer_priority	The priority of the timer.
 *
 * @retval		1				If the timer was created successfully.
 * @retval		0				If the timer was not created successfully, because there are already MAX_NUMBER_OF_TIMERS created.
 */
uint8_t 	timer_create_timer(uint32_t* timer_id, timer_mode_t timer_mode, timer_timeout_handler_t timeout_handler, uint8_t timer_priority);

/**@brief Function for starting a timer.
 *
 * @param[in]       timer_id      			The timer identifier.
 * @param[in]       timeout_microseconds 	Microseconds to time-out event.
 * @param[in]       p_context     			General purpose pointer. Will be passed to the time-out handler when
 *                                			the timer expires.
 *
 * @retval     		1						If the timer was started successfully.
 * @retval			0						If the timer is already running. 
 */
uint8_t 	timer_start_timer(uint32_t timer_id, uint64_t timeout_microseconds, void* p_context);


/**@brief Function for stopping a timer.
 *
 * @param[in]       timer_id      			Timer identifier.
 */
void 		timer_stop_timer(uint32_t timer_id);


#endif
