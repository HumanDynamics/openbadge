#include "callback_generator_internal_lib.h"
#include "callback_generator_lib.h"



CALLBACK_GENERATOR_IMPLEMENTATION(ACCEL_INT1) {
	nrf_drv_gpiote_pin_t pin = 25;
	nrf_gpiote_polarity_t action = NRF_GPIOTE_POLARITY_LOTOHI;
	
	if(callback_generator_ACCEL_INT1_get_generator() != NULL) {
		callback_generator_ACCEL_INT1_get_generator()(&pin, &action);
	}
	
	if(callback_generator_ACCEL_INT1_get_handler() != NULL) {
		callback_generator_ACCEL_INT1_get_handler()(pin, action);
	}	
}








/*
#include "callback_generator_internal_lib.h"


#include "callback_generator_lib.h"

#include "stdint.h"
#include "timer_lib.h"
#include "stdio.h"
#include <vector>
#include "pthread.h"

static uint32_t 							TEST_timer_id;
static TEST_handler_t						TEST_handler = NULL;
static TEST_generator_t						TEST_generator = NULL;
static std::vector<std::vector<uint32_t> > 	TEST_trigger_vectors;


static volatile uint8_t 					TEST_pending_trigger = 0;		// If there is no trigger to process currently, but a new trigger command was there 
static volatile uint8_t 					TEST_processing_trigger = 0;		
static volatile uint32_t					TEST_trigger_index = 0;
static volatile uint32_t					TEST_timepoint_index = 0;

static pthread_mutex_t 						TEST_critical_section_mutex;				// Mutex for critical sections. 


static void callback_generator_TEST_internal_handler(void);


// This function is normally called from a critical-section safed function
static void callback_generator_TEST_stop_timer(void) {
	TEST_trigger_index++;	// First increment the index, before setting the processing-flag to 0.
	TEST_timepoint_index = 0;
	TEST_processing_trigger = 0;
	timer_stop_timer(TEST_timer_id);
	
}


// This function is normally called from a critical-section safed function
static void callback_generator_TEST_start_timer(void) {
	// Start the timer
	TEST_processing_trigger = 1;
	TEST_timepoint_index = 0;
	
	uint32_t trigger_len = (uint32_t) TEST_trigger_vectors.size();
	if(TEST_trigger_index < trigger_len) { // Only safety check (to not get a segfault)
		uint32_t timepoint_len = (uint32_t) TEST_trigger_vectors[TEST_trigger_index].size();
	
		if(TEST_timepoint_index < timepoint_len) {
			// Start the timer with the first timepoint
			uint64_t timeout_microseconds = ((uint64_t)(TEST_trigger_vectors[TEST_trigger_index][TEST_timepoint_index])) * ((uint64_t)1000);
			timer_start_timer(TEST_timer_id, timeout_microseconds, NULL);
		} else {
			callback_generator_TEST_stop_timer();
		}
		
	}
}


void callback_generator_TEST_internal_timeout_handler(void * p_context) {
	// Restart the timer if necessary
	// Call the handler that does the external generating and handler-calling
	
	pthread_mutex_lock(&TEST_critical_section_mutex);
	
	
	
	// Call the handler here (before incrementing the X_timepoint_index)
	callback_generator_TEST_internal_handler();
	
	
	
	TEST_timepoint_index++;
	
	uint32_t trigger_len = (uint32_t) TEST_trigger_vectors.size();
	if(TEST_trigger_index < trigger_len) { // Only safety check (to not get a segfault) --> should actually always be true
		uint32_t timepoint_len = (uint32_t) TEST_trigger_vectors[TEST_trigger_index].size();
	
		if(TEST_timepoint_index < timepoint_len) {
			// Restart the timer with the next timepoint
			uint64_t timeout_microseconds = ((uint64_t)(TEST_trigger_vectors[TEST_trigger_index][TEST_timepoint_index])) * ((uint64_t)1000);
			timer_start_timer(TEST_timer_id, timeout_microseconds, NULL);
		} else {
			callback_generator_TEST_stop_timer();
		}
	
	} else {
		callback_generator_TEST_stop_timer();
	}
		
	pthread_mutex_unlock(&TEST_critical_section_mutex);
}




void callback_generator_TEST_clear(void) {
	TEST_generator = NULL; 
	TEST_handler = NULL;			
	TEST_processing_trigger = 0;
	TEST_pending_trigger = 0;
	TEST_trigger_vectors.clear();
	TEST_trigger_index = 0;
	TEST_timepoint_index = 0;
}

uint8_t callback_generator_TEST_init(void) {
	static uint8_t init_done = 0;
	
	if(!init_done) {
		
		timer_init();
		uint8_t ret = timer_create_timer(&TEST_timer_id, TIMER_MODE_SINGLE_SHOT, callback_generator_TEST_internal_timeout_handler, 1);
		if(ret == 0)
			return 0;
		
		pthread_mutex_init (&TEST_critical_section_mutex, NULL);
	}	
	
	init_done = 1;
	
	return 1;
}

void callback_generator_TEST_set_handler(TEST_handler_t handler) {
	TEST_handler = handler;
}

TEST_handler_t callback_generator_TEST_get_handler(void) {
	return TEST_handler;
}

void callback_generator_TEST_set_generator(TEST_generator_t generator) {
	TEST_generator = generator;
}

TEST_generator_t callback_generator_TEST_get_generator(void) {
	return TEST_generator;
}


void callback_generator_TEST_trigger(void) {
	pthread_mutex_lock(&TEST_critical_section_mutex);
	uint32_t len = (uint32_t) TEST_trigger_vectors.size();
	
	// Check if there is sth to process:
	if(TEST_trigger_index < len) { // || TEST_processing_trigger) { // The (|| TEST_processing_trigger) is only for safety reasons.
		
		// Check that there is no ongoing trigger processing operation
		if(!TEST_processing_trigger) {
			TEST_processing_trigger = 1;
			TEST_pending_trigger = 0;
		
			// Start the callback generation
			callback_generator_TEST_start_timer();
		} else {
			printf("Trigger ignored\n");
		}
		
		
	} else { // If there is nothing to process (and nothing is currently processing, set the X_pending_trigger-flag)
		TEST_pending_trigger = 1;
		TEST_processing_trigger = 0;		// Implicit should this always be 0 in this case
	}
	
	pthread_mutex_unlock(&TEST_critical_section_mutex);
}

void callback_generator_TEST_add_trigger(uint32_t* trigger_array, uint32_t len) {
	if(len == 0)
		return;
	
	std::vector<uint32_t> trigger_vector;
	for(uint32_t i = 0; i < len; i++) {
		trigger_vector.push_back(trigger_array[i]);
	}

	
	pthread_mutex_lock(&TEST_critical_section_mutex);
	
	TEST_trigger_vectors.push_back(trigger_vector);
	
	if(TEST_pending_trigger) {
		TEST_processing_trigger = 1;
		TEST_pending_trigger = 0;		
		
		// Start the callback generation
		callback_generator_TEST_start_timer();
	}
	
	pthread_mutex_unlock(&TEST_critical_section_mutex);
}

uint8_t callback_generator_TEST_is_ready(void) {
	
	uint8_t ready = 0;
	
	pthread_mutex_lock(&TEST_critical_section_mutex);
	
	uint32_t trigger_len = (uint32_t) TEST_trigger_vectors.size();
	if(TEST_trigger_index >= trigger_len) {
		ready = 1;	
	}
	
	
	pthread_mutex_unlock(&TEST_critical_section_mutex);
	
	return ready;
}


static void callback_generator_TEST_internal_handler(void) {
	
	printf("Called: %u, %u\n", TEST_trigger_index, TEST_timepoint_index);
	
	uint8_t a = 0;
	if(callback_generator_TEST_get_generator() != NULL) {
		callback_generator_TEST_get_generator()(TEST_trigger_index, TEST_timepoint_index, &a);
	}
	
	if(callback_generator_TEST_get_handler() != NULL) {
		callback_generator_TEST_get_handler()(a);
	}
	
	
}

*/
