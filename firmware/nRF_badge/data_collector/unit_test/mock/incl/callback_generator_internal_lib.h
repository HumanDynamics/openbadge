/** @file
 *
 *
 * @brief 	This module provides an easy to use callback-generating capability for creating asynchronous events after triggering.
 *	
 * @details	The module provides macros to define the needed functions and data-types.
 *			The application can use this module to create asynchronous events at specific timepoints after the application triggers the events.
 *			Therefore the application has to add a timepoint-array for each trigger. When the trigger()-function is called, this module
 *			generates interrupts at the timepoints of this timepoint-array (in milliseconds). 
 *			Example: The application wants to simulate 5 interrupts after the accelerometer has been initialized:
 *						- The application (or the test-suite) has to add the timepoints for the trigger: here [100, 100, 100, 100, 100].
 *						- The init-function of the accelerometer is the "trigger", so it calls the trigger()-function of this module to start the interrupt-creating process.
 *			Each time an interrupt/callback is generated an internal callback-handler function is called. This function is not implemented, and has to be implemented by the application.
 *			Normally, this internal callback-handler calls a "generator" and a "handler"-function. The generator-function generates data for the actual handler-function that should be called afterwords. 
 *			Example: To embed the interrupt/callback-creating process into the application like the accelerometer module, the ISR of the accelerometer has e.g. a parameter "polarity" of the pin.
 *						- That means the callback-creating process needs to generate a "polarity" (by the generator-function) and has to call the ISR (handler-function) with this "polarity".
 *						- 	CALLBACK_IMPLEMENTATION(ACCEL) {
 *								uint8_t polarity;
 *								callback_generator_ACCEL_get_generator()(&polarity);
 *								callback_generator_ACCEL_get_handler()(polarity);
 *						  	}
 *						- The generator-function has to be implemented by the test-suite. The application (or test-suite) can also get the current state of the callback-creating process by calling the functions:
 *						  callback_generator_ACCEL_get_trigger_index() or callback_generator_ACCEL_get_timepoint_index()	
 *			
 *			The macros get a NAME parameter. This parameter represents the individual-name for one callback-creating process (in the following the NAME is X).
 *			- CALLBACK_HANDLER_DECLARATION(NAME, RET, ...) declares the handler-function type (NAME_handler_t) that should be called after a callback is created and the data are generated
 *				Example: 	CALLBACK_HANDLER_DECLARATION(X, void, uint8_t x)
 *			- CALLBACK_GENERATOR_DECLARATION(NAME, RET, ...) declares the generator-function type (NAME_generator_t) that should be called after a callback is created to generate the data for this callback
 *				Example: 	CALLBACK_HANDLER_DECLARATION(X, void, uint8_t* x)
 *			- CALLBACK_DECLARATION(NAME) declares all the functions that are needed to configure and start the callback-creating process
 *				Example: CALLBACK_DECLARATION(X)
 *          - CALLBACK_IMPLEMENTATION(NAME) implements all the functions that are needed to configure and start the callback-creating process. 
 *			  Furthermore it creates a function that is called when a callback is created. This function is not implementation, so the application needs to do this
 *				Example: 	CALLBACK_IMPLEMENTATION(X) {
 * 								uint8_t polarity;
 *								callback_generator_ACCEL_get_generator()(&polarity);
 *								callback_generator_ACCEL_get_handler()(polarity);
 *							}
 *									
 *			The following functions are used to configure the callback-creating process. They can be devided in functions the test-suite calls/implements and functions the application calls/implements:
 *
 *			- uint8_t callback_generator_X_init(void) 	
 *				Initializes the callback-creating process for X. 
 *				@retval 1 on success
 *				@retval 0 on failure
 *				This should be called by the application (e.g. in accel_init()).
 *			- void callback_generator_X_reset(void)	
 *				Clears the internal structures and states to have a clean callback-creating process again. 
 *				This should be called by the test-suite or application.
 *			- void callback_generator_X_set_handler(X_handler_t)
 *				Sets the ISR/Callback-handler that should be called after the callback and the data for the callback is generated. 
 *				@param[in] 	X_handler_t			The handler that should be called after the callback and the data for the callback is generated (could also be NULL --> there will be no callback for the application)
 *				This should be set by the application.
 *			- void callback_generator_X_set_generator(X_generator_t)	
 *				Sets the generator-function that should be called to generate the data for a callback. 
 *				@param[in] 	X_generator_t		The generator-function that generated the data for a callback (could also be NULL --> the default implementation should be taken).
 *				This should be set by the test-suite.
 *			- void callback_generator_X_trigger(void)
 *				Triggers the callback-creating process for one added timepoint-array. 
 *				This should be called by the application (e.g. accel_init() or accel_reset_interrupt()).
 *			- void callback_generator_X_add_trigger_timepoints(uint32_t* timepoint_array, uint32_t len)
 *				Adds callback-timepoints in milliseconds for one trigger (when the trigger()-function is called, the callback will be generated at these timepoints).
 *				@param[in] 	timepoint_array		Array of timepoints in milliseconds when the interrupt/callback should be generated after a trigger()-call.
 *				@param[in]	len					Number of timepoints in the timepoint_array.
 *				This should be called by the test-suite.
 *			- X_handler_t callback_generator_X_get_handler(void)
 *				Function to retrieve the current handler-function.
 *				@retval		The handler-function.
 *				This could be called by the test-suite.
 *			- X_generator_t callback_generator_X_get_generator(void)
 *				Function to retrieve the current generator-function.
 *				@retval		The generator-function.
 *				This could be called by the test-suite.
 *			- uint32_t callback_generator_X_get_trigger_index(void)
 *				Function to retrieve the trigger index that is currently processed.
 *				@retval		The current trigger-index that is currently processed.
 *				This could be called by the test-suite.
 *			- uint32_t callback_generator_X_get_timepoint_index(void)
 *				Function to retrieve the timepoint index (within the current trigger) that is currently processed.
 *				@retval		The current timpoint-index (within the current trigger) that is currently processed.
 *				This could be called by the test-suite.
 *			
 *
 *
 * @note	It is not allowed to call the trigger()-function before the init()-function. All the other functions can be called beforehand without problems.
 * 			If the trigger()-function is called before some timepoint(s) has been added for this trigger, it will be delayed, until new timepoint(s) has been added.
 *			If the trigger()-function is called while another trigger is processed, it will be ignored.
 */

#ifndef CALLBACK_GENERATOR_INTERNAL_LIB_H
#define CALLBACK_GENERATOR_INTERNAL_LIB_H


#include "stdint.h"
#include "timer_lib.h"
#include "stdio.h"
#include <vector>
#include "pthread.h"



#define CALLBACK_HANDLER_FUNCTION_DECLARATION(NAME, RET, ...) \
typedef RET (* NAME##_handler_t) (__VA_ARGS__)

#define CALLBACK_GENERATOR_FUNCTION_DECLARATION(NAME, RET, ...) \
typedef RET (* NAME##_generator_t) (__VA_ARGS__)

#define CALLBACK_GENERATOR_DECLARATION(NAME) \
uint8_t callback_generator_##NAME##_init(void); \
void callback_generator_##NAME##_reset(void); \
void callback_generator_##NAME##_set_handler(NAME##_handler_t handler); \
NAME##_handler_t callback_generator_##NAME##_get_handler(void); \
void callback_generator_##NAME##_set_generator(NAME##_generator_t generator); \
NAME##_generator_t callback_generator_##NAME##_get_generator(void); \
void callback_generator_##NAME##_trigger(void) ; \
void callback_generator_##NAME##_add_trigger_timepoints(uint32_t* timepoint_array, uint32_t len); \
uint8_t callback_generator_##NAME##_is_ready(void); \
uint32_t callback_generator_##NAME##_get_trigger_index(void); \
uint32_t callback_generator_##NAME##_get_timepoint_index(void);




#define CALLBACK_GENERATOR_IMPLEMENTATION(NAME) \
static NAME##_handler_t						NAME##_handler = NULL; \
static NAME##_generator_t					NAME##_generator = NULL; \
static std::vector<std::vector<uint32_t> > 	NAME##_trigger_vectors; \
static volatile uint8_t 					NAME##_pending_trigger = 0; \
static volatile uint8_t 					NAME##_processing_trigger = 0; \
static volatile uint32_t					NAME##_trigger_index = 0; \
static volatile uint32_t					NAME##_timepoint_index = 0; \
static pthread_mutex_t 						NAME##_critical_section_mutex; \
static volatile uint8_t 					NAME##_start_timer = 0; \
static volatile uint64_t 					NAME##_timer_end_timepoint_microseconds = 0; \
static pthread_t 							NAME##_thread_handle; \
static void callback_generator_##NAME##_internal_handler(void);	\
static void callback_generator_##NAME##_stop_timer(void) { \
	NAME##_trigger_index++; \
	NAME##_timepoint_index = 0; \
	NAME##_processing_trigger = 0; \
	NAME##_start_timer = 0; \
} \
static void callback_generator_##NAME##_start_timer(void) { \
	NAME##_processing_trigger = 1; \
	NAME##_timepoint_index = 0; \
	uint32_t trigger_len = (uint32_t) NAME##_trigger_vectors.size(); \
	if(NAME##_trigger_index < trigger_len) { \
		uint32_t timepoint_len = (uint32_t) NAME##_trigger_vectors[NAME##_trigger_index].size(); \
		if(NAME##_timepoint_index < timepoint_len) { \
			uint64_t timeout_microseconds = ((uint64_t)(NAME##_trigger_vectors[NAME##_trigger_index][NAME##_timepoint_index])) * ((uint64_t)1000); \
			NAME##_timer_end_timepoint_microseconds = timer_get_microseconds_since_start() + timeout_microseconds; \
			NAME##_start_timer = 1; \
		} else { \
			callback_generator_##NAME##_stop_timer(); \
		} \
	} \
} \
static void* callback_generator_##NAME##_thread_handler(void* ptr) { \
	while(1) { \
		pthread_mutex_lock(&NAME##_critical_section_mutex); \
		if(NAME##_start_timer && timer_get_microseconds_since_start() >= NAME##_timer_end_timepoint_microseconds) { \
			pthread_mutex_unlock(&NAME##_critical_section_mutex); \
			callback_generator_##NAME##_internal_handler(); \
			pthread_mutex_lock(&NAME##_critical_section_mutex); \
			NAME##_timepoint_index++; \
			uint32_t trigger_len = (uint32_t) NAME##_trigger_vectors.size(); \
			if(NAME##_trigger_index < trigger_len) { \
				uint32_t timepoint_len = (uint32_t) NAME##_trigger_vectors[NAME##_trigger_index].size(); \
				if(NAME##_timepoint_index < timepoint_len) { \
					uint64_t timeout_microseconds = ((uint64_t)(NAME##_trigger_vectors[NAME##_trigger_index][NAME##_timepoint_index])) * ((uint64_t)1000); \
					NAME##_timer_end_timepoint_microseconds = timer_get_microseconds_since_start() + timeout_microseconds; \
					NAME##_start_timer = 1; \
				} else { \
					callback_generator_##NAME##_stop_timer(); \
				} \
			} else { \
				callback_generator_##NAME##_stop_timer(); \
			} \
		} \
		pthread_mutex_unlock(&NAME##_critical_section_mutex); \
	} \
	return NULL; \
} \
void callback_generator_##NAME##_reset(void) { \
	NAME##_generator = NULL; \
	NAME##_handler = NULL; \
	NAME##_processing_trigger = 0; \
	NAME##_pending_trigger = 0; \
	NAME##_trigger_vectors.clear(); \
	NAME##_trigger_index = 0; \
	NAME##_timepoint_index = 0; \
	NAME##_start_timer = 0; \
} \
uint8_t callback_generator_##NAME##_init(void) { \
	static uint8_t init_done = 0; \
	if(!init_done) { \
		pthread_mutex_init (&NAME##_critical_section_mutex, NULL); \
		pthread_attr_t attr; \
		pthread_attr_init(&attr); \
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); \
		pthread_create(&NAME##_thread_handle, &attr, callback_generator_##NAME##_thread_handler, NULL); \
		pthread_attr_destroy(&attr); \
	} \
	init_done = 1; \
	return 1; \
} \
void callback_generator_##NAME##_set_handler(NAME##_handler_t handler) { \
	NAME##_handler = handler; \
} \
NAME##_handler_t callback_generator_##NAME##_get_handler(void) { \
	return NAME##_handler; \
} \
void callback_generator_##NAME##_set_generator(NAME##_generator_t generator) { \
	NAME##_generator = generator; \
} \
NAME##_generator_t callback_generator_##NAME##_get_generator(void) { \
	return NAME##_generator; \
} \
void callback_generator_##NAME##_trigger(void) { \
	pthread_mutex_lock(&NAME##_critical_section_mutex); \
	uint32_t len = (uint32_t) NAME##_trigger_vectors.size(); \
	if(NAME##_trigger_index < len) { \
		if(!NAME##_processing_trigger) { \
			NAME##_processing_trigger = 1; \
			NAME##_pending_trigger = 0; \
			callback_generator_##NAME##_start_timer(); \
		} \
	} else { \
		NAME##_pending_trigger = 1; \
		NAME##_processing_trigger = 0; \
	} \
	pthread_mutex_unlock(&NAME##_critical_section_mutex); \
} \
void callback_generator_##NAME##_add_trigger_timepoints(uint32_t* timepoint_array, uint32_t len) { \
	if(len == 0) \
		return; \
	std::vector<uint32_t> timepoint_vector; \
	for(uint32_t i = 0; i < len; i++) { \
		timepoint_vector.push_back(timepoint_array[i]); \
	} \
	pthread_mutex_lock(&NAME##_critical_section_mutex); \
	NAME##_trigger_vectors.push_back(timepoint_vector); \
	if(NAME##_pending_trigger) { \
		NAME##_processing_trigger = 1; \
		NAME##_pending_trigger = 0; \
		callback_generator_##NAME##_start_timer(); \
	} \
	pthread_mutex_unlock(&NAME##_critical_section_mutex); \
} \
uint8_t callback_generator_##NAME##_is_ready(void) { \
	uint8_t ready = 0; \
	pthread_mutex_lock(&NAME##_critical_section_mutex);  \
	uint32_t trigger_len = (uint32_t) NAME##_trigger_vectors.size();  \
	if(NAME##_trigger_index >= trigger_len) {  \
		ready = 1;	 \
	} \
	pthread_mutex_unlock(&NAME##_critical_section_mutex); \
	return ready; \
}  \
uint32_t callback_generator_##NAME##_get_trigger_index(void) { \
	return NAME##_trigger_index; \
} \
uint32_t callback_generator_##NAME##_get_timepoint_index(void) { \
	return NAME##_timepoint_index; \
} \
static void callback_generator_##NAME##_internal_handler(void)


/*
#define CALLBACK_GENERATOR_IMPLEMENTATION(NAME) \
static uint32_t 							NAME##_timer_id; \
static NAME##_handler_t						NAME##_handler = NULL; \
static NAME##_generator_t					NAME##_generator = NULL; \
static std::vector<std::vector<uint32_t> > 	NAME##_trigger_vectors; \
static volatile uint8_t 					NAME##_pending_trigger = 0; \
static volatile uint8_t 					NAME##_processing_trigger = 0; \
static volatile uint32_t					NAME##_trigger_index = 0; \
static volatile uint32_t					NAME##_timepoint_index = 0; \
static pthread_mutex_t 						NAME##_critical_section_mutex; \
static void callback_generator_##NAME##_internal_handler(void);	\
static void callback_generator_##NAME##_stop_timer(void) { \
	NAME##_trigger_index++; \
	NAME##_timepoint_index = 0; \
	NAME##_processing_trigger = 0; \
	timer_stop_timer(NAME##_timer_id); \
} \
static void callback_generator_##NAME##_start_timer(void) { \
	NAME##_processing_trigger = 1; \
	NAME##_timepoint_index = 0; \
	uint32_t trigger_len = (uint32_t) NAME##_trigger_vectors.size(); \
	if(NAME##_trigger_index < trigger_len) { \
		uint32_t timepoint_len = (uint32_t) NAME##_trigger_vectors[NAME##_trigger_index].size(); \
		if(NAME##_timepoint_index < timepoint_len) { \
			uint64_t timeout_microseconds = ((uint64_t)(NAME##_trigger_vectors[NAME##_trigger_index][NAME##_timepoint_index])) * ((uint64_t)1000); \
			timer_start_timer(NAME##_timer_id, timeout_microseconds, NULL); \
		} else { \
			callback_generator_##NAME##_stop_timer(); \
		} \
	} \
} \
void callback_generator_##NAME##_internal_timeout_handler(void * p_context) { \
	callback_generator_##NAME##_internal_handler(); \
	pthread_mutex_lock(&NAME##_critical_section_mutex); \
	NAME##_timepoint_index++; \
	uint32_t trigger_len = (uint32_t) NAME##_trigger_vectors.size(); \
	if(NAME##_trigger_index < trigger_len) { \
		uint32_t timepoint_len = (uint32_t) NAME##_trigger_vectors[NAME##_trigger_index].size(); \
		if(NAME##_timepoint_index < timepoint_len) { \
			uint64_t timeout_microseconds = ((uint64_t)(NAME##_trigger_vectors[NAME##_trigger_index][NAME##_timepoint_index])) * ((uint64_t)1000); \
			timer_start_timer(NAME##_timer_id, timeout_microseconds, NULL); \
		} else { \
			callback_generator_##NAME##_stop_timer(); \
		} \
	} else { \
		callback_generator_##NAME##_stop_timer(); \
	} \
	pthread_mutex_unlock(&NAME##_critical_section_mutex); \
} \
void callback_generator_##NAME##_reset(void) { \
	NAME##_generator = NULL; \
	NAME##_handler = NULL; \
	NAME##_processing_trigger = 0; \
	NAME##_pending_trigger = 0; \
	NAME##_trigger_vectors.clear(); \
	NAME##_trigger_index = 0; \
	NAME##_timepoint_index = 0; \
} \
uint8_t callback_generator_##NAME##_init(void) { \
	static uint8_t init_done = 0; \
	if(!init_done) { \
		timer_init(); \
		uint8_t ret = timer_create_timer(&NAME##_timer_id, TIMER_MODE_SINGLE_SHOT, callback_generator_##NAME##_internal_timeout_handler, 1); \
		if(ret == 0) return 0; \
		pthread_mutex_init (&NAME##_critical_section_mutex, NULL); \
	} \
	init_done = 1; \
	return 1; \
} \
void callback_generator_##NAME##_set_handler(NAME##_handler_t handler) { \
	NAME##_handler = handler; \
} \
NAME##_handler_t callback_generator_##NAME##_get_handler(void) { \
	return NAME##_handler; \
} \
void callback_generator_##NAME##_set_generator(NAME##_generator_t generator) { \
	NAME##_generator = generator; \
} \
NAME##_generator_t callback_generator_##NAME##_get_generator(void) { \
	return NAME##_generator; \
} \
void callback_generator_##NAME##_trigger(void) { \
	pthread_mutex_lock(&NAME##_critical_section_mutex); \
	uint32_t len = (uint32_t) NAME##_trigger_vectors.size(); \
	if(NAME##_trigger_index < len) { \
		if(!NAME##_processing_trigger) { \
			NAME##_processing_trigger = 1; \
			NAME##_pending_trigger = 0; \
			callback_generator_##NAME##_start_timer(); \
		} \
	} else { \
		NAME##_pending_trigger = 1; \
		NAME##_processing_trigger = 0; \
	} \
	pthread_mutex_unlock(&NAME##_critical_section_mutex); \
} \
void callback_generator_##NAME##_add_trigger_timepoints(uint32_t* timepoint_array, uint32_t len) { \
	if(len == 0) \
		return; \
	std::vector<uint32_t> timepoint_vector; \
	for(uint32_t i = 0; i < len; i++) { \
		timepoint_vector.push_back(timepoint_array[i]); \
	} \
	pthread_mutex_lock(&NAME##_critical_section_mutex); \
	NAME##_trigger_vectors.push_back(timepoint_vector); \
	if(NAME##_pending_trigger) { \
		NAME##_processing_trigger = 1; \
		NAME##_pending_trigger = 0; \
		callback_generator_##NAME##_start_timer(); \
	} \
	pthread_mutex_unlock(&NAME##_critical_section_mutex); \
} \
uint8_t callback_generator_##NAME##_is_ready(void) { \
	uint8_t ready = 0; \
	pthread_mutex_lock(&NAME##_critical_section_mutex);  \
	uint32_t trigger_len = (uint32_t) NAME##_trigger_vectors.size();  \
	if(NAME##_trigger_index >= trigger_len) {  \
		ready = 1;	 \
	} \
	pthread_mutex_unlock(&NAME##_critical_section_mutex); \
	return ready; \
}  \
uint32_t callback_generator_##NAME##_get_trigger_index(void) { \
	return NAME##_trigger_index; \
} \
uint32_t callback_generator_##NAME##_get_timepoint_index(void) { \
	return NAME##_timepoint_index; \
} \
static void callback_generator_##NAME##_internal_handler(void)
*/


#endif
