#include "timer_lib.h"


#include "pthread.h"

#include "time.h"







static uint64_t time_start_microseconds = 0;				/**< The microseconds that are set through timer_set_start() to be a reference point for all the other time related functions. */

static pthread_t thread_handle;								/**< Handle of the timer thread. */

static pthread_mutex_t critical_section_mutex;				/**< Mutex for critical sections. */

static volatile uint8_t timer_running = 0;					/**< Flag if the timer-thread is running */

static volatile timer_node_t* queue_head;	 				/**< Pointer to head of the timer node queue*/

volatile timer_node_t timer_nodes[MAX_NUMBER_OF_TIMERS]; 	/**< Timer node queue, where the timer are inserted with respect to their expiration times */

uint32_t number_of_timers = 0;								/**< Number of created timers */


/**@brief Function for entering a critical section by locking the mutex.
 */
static void enter_critical_section(void) {
	pthread_mutex_lock(&critical_section_mutex);
}

/**@brief Function for exiting a critical section by locking the mutex.
 */
static void exit_critical_section(void) {
	pthread_mutex_unlock(&critical_section_mutex);
}

/**@brief Function to retrieve the current time in microseconds since a timepoint in the past.
 *
 * @retval	Microseconds since a timepoint in the past.
 */
static uint64_t timer_get_microseconds(void) {
	struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
	
	uint64_t t =  ((uint64_t) (time.tv_sec *1000*1000)) + ((uint64_t)time.tv_nsec/1000);
	
    
	t = t*TIMER_PRESCALER;
	
    return t;
}


void timer_set_start(void) {
	time_start_microseconds = timer_get_microseconds();
}

uint64_t timer_get_microseconds_since_start(void) {
	return timer_get_microseconds() - time_start_microseconds;
}

uint64_t timer_get_milliseconds_since_start(void) {
	return timer_get_microseconds_since_start() / 1000;
}

void timer_sleep_microseconds(uint64_t microseconds) {
	microseconds = microseconds/TIMER_PRESCALER;
	
	uint64_t nsec = (microseconds*1000) % 1000000000;
	uint64_t sec = microseconds/1000000;
    struct timespec time;
    time.tv_sec = sec;
    time.tv_nsec = nsec;
    clock_nanosleep(CLOCK_MONOTONIC, 0, &time, NULL);
}

void timer_sleep_milliseconds(uint64_t milliseconds) {
	uint64_t microseconds = milliseconds * 1000;
	timer_sleep_microseconds(microseconds);
}






/**@brief Function to insert a timer-node in the correct position in the timer-queue.
 *
 * @details	The function searches for the correct point to insert the new timer node by comparing the microseconds_at_end timepoint of the nodes.
 *
 * @note This function needs to be saved by enter/exit_critical_section()
 *
 * @param[in]	timer_id				The timer identifier.
 * @param[in]	microseconds_interval	The interval of the timer in microseconds.
 */
static void timer_insert_node(uint32_t timer_id, uint32_t microseconds_interval) {
	
	timer_nodes[timer_id].microseconds_at_start = timer_get_microseconds_since_start();
	timer_nodes[timer_id].microseconds_at_end 	= timer_nodes[timer_id].microseconds_at_start + microseconds_interval;
	
	timer_nodes[timer_id].is_running = 1;
	timer_nodes[timer_id].next = NULL;
	
	
	volatile timer_node_t* new_node =  &timer_nodes[timer_id];
	
	if(queue_head == NULL) {
		new_node->next = NULL;
		queue_head = new_node;
	} else {
		
	
		if((queue_head->microseconds_at_end == new_node->microseconds_at_end && queue_head->timer_priority < new_node->timer_priority) ||  queue_head->microseconds_at_end > new_node->microseconds_at_end) {
			// queue_head ersetzen
			new_node->next = queue_head;
			queue_head = new_node;
			
		} else {
			volatile timer_node_t* cur_node 	= queue_head;
			volatile timer_node_t* prev_node 	= queue_head;
		
			while(cur_node != NULL) {
				// Search the node into the right position
				if((cur_node->microseconds_at_end == new_node->microseconds_at_end && cur_node->timer_priority < new_node->timer_priority) ||  cur_node->microseconds_at_end > new_node->microseconds_at_end) {
					// cur_node ersetzen
					break;
				}		
				prev_node = cur_node;
				cur_node = (volatile timer_node_t*) cur_node->next;
			}
			
			new_node->next = cur_node;
			prev_node->next = new_node;			
		}
	}
}


/**@brief Function to remove a timer-node from timer-queue.
 *
 * @note This function needs to be saved by enter/exit_critical_section()
 *
 * @param[in]	timer_id				The timer identifier.
 */
static void timer_remove_node(uint32_t timer_id) {
	timer_nodes[timer_id].is_running = 0;
	// Search for the node that has timer_node[timer_id] as next-node.
	for(uint32_t i = 0; i < MAX_NUMBER_OF_TIMERS; i++) {
		if(timer_nodes[i].next == &timer_nodes[timer_id]) {
			timer_nodes[i].next = timer_nodes[timer_id].next;
			break;
		}
	}
}

/**@brief Function to check for a timeout.
 *
 * @details	The function is periodically called by the timer-thread. 
 *			It checks if the queue-head timer-node expires and calls the timeout handler.
 *			Furthermore, it sets the queue-head to the next timer-node 
 *			and reinserts the expired timer if it is a repeated timer.
 *
 * @param[in]	ptr				Just a dummy parameter, needed by the pthread_create()-function.
 *
 * @retval		NULL			Needed by the pthread_create()-function.
 */
static void* timer_check_queue(void* ptr) {
	while(timer_running) {
		enter_critical_section();
		uint64_t cur_time = timer_get_microseconds_since_start();
		
		
		timer_timeout_handler_t timeout_handler = NULL;
		void *                 	timeout_context = NULL;
		
		
		if(queue_head != NULL) {			
			if(cur_time >= queue_head->microseconds_at_end) {				
				// Backup the current head-queue (needed for the reinsert, if timer-mode is REPEATED)
				volatile timer_node_t* prev_queue_head = queue_head;
				
				// Set the new queue head
				queue_head = (volatile timer_node_t*) queue_head->next;
				
				// Check if the element is running (not stopped)				
				if(prev_queue_head->is_running) {					
					prev_queue_head->is_running = 0;
					timeout_handler = prev_queue_head->p_timeout_handler;
					timeout_context	= prev_queue_head->p_context;
					
					if(prev_queue_head->mode == TIMER_MODE_REPEATED) {
						
						// Insert the former queue_head-node again (with a small correction)
						uint64_t delta_t = timer_get_microseconds_since_start() - cur_time;
						uint64_t microseconds_interval = (delta_t > prev_queue_head->microseconds_interval) ? 0 : (prev_queue_head->microseconds_interval - delta_t);
						timer_insert_node(prev_queue_head->timer_id, microseconds_interval);
					}
				}
			}
		}		
		exit_critical_section();
		
		// Call the handler outside the critical section (because probably the application timeout-handler could call timer_start_timer())
		if(timeout_handler != NULL) {
			timeout_handler(timeout_context);
		}	
				
	}
	return NULL;
}



void timer_init(void) {
	
	if(timer_running)
		return;
	pthread_mutex_init (&critical_section_mutex, NULL);
	
	enter_critical_section();
	timer_running = 1;
	queue_head = NULL;
	number_of_timers = 0;
	
	
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		
	pthread_create(&thread_handle, &attr, timer_check_queue, NULL);
	
	pthread_attr_destroy(&attr);
	
	exit_critical_section();
}

void timer_stop(void) {
	enter_critical_section();
	timer_running = 0;
	pthread_mutex_destroy (&critical_section_mutex);
	exit_critical_section();
}




uint8_t timer_create_timer(uint32_t* timer_id, timer_mode_t timer_mode, timer_timeout_handler_t timeout_handler, uint8_t timer_priority) {
	if(number_of_timers >= MAX_NUMBER_OF_TIMERS) {
		return 0;
	}
	
	*timer_id = number_of_timers;
	
	timer_nodes[*timer_id].timer_id					= *timer_id;
	timer_nodes[*timer_id].mode						= timer_mode;
	timer_nodes[*timer_id].timer_priority			= timer_priority;
	timer_nodes[*timer_id].is_running 				= 0;
	timer_nodes[*timer_id].next 					= NULL;
	timer_nodes[*timer_id].p_timeout_handler 		= timeout_handler;
	timer_nodes[*timer_id].p_context 				= NULL;
	
	timer_nodes[*timer_id].microseconds_at_start	= 0;
	timer_nodes[*timer_id].microseconds_at_end		= 0;
	timer_nodes[*timer_id].microseconds_interval	= 0;
	
	number_of_timers ++;
	
	return 1;
}





uint8_t timer_start_timer(uint32_t timer_id, uint64_t timeout_microseconds, void* p_context) {
	
	if(timer_nodes[timer_id].is_running)
		return 0;
	
	timer_nodes[timer_id].p_context = p_context;
	timer_nodes[timer_id].microseconds_interval = timeout_microseconds;
	
	enter_critical_section();
	timer_insert_node(timer_id, timer_nodes[timer_id].microseconds_interval);
	exit_critical_section();
	
	return 1;
	
}



void timer_stop_timer(uint32_t timer_id) {
	enter_critical_section();
	timer_remove_node(timer_id);
	exit_critical_section();
}










