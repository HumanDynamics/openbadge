/** @file
 *
 *
 * @brief 	This module provides an easy to use data-generating capability for creating data (and return values) for "read"-functions (e.g. accel_read_acceleration).
 *
 * @details	The advantage of this abstraction is the independence of the test-suite, because in the data-generating functions the default data could be generated, 
 *			and only if it is needed the test-suite can set its own data-generating function.
 *			The module provides macros to define the needed functions and data-types.
 *			Example: 
 *			- DATA_GENERATOR_FUNCTION_DECLARATION(TEST, uint8_t, uint8_t* x, uint8_t* y);
 *				Declaring the generator-function type
 *			- DATA_GENERATOR_DECLARATION(TEST, uint8_t, uint8_t* x, uint8_t* y);
 *				Declaring all the needed data-generating functions
 * 			- DATA_GENERATOR_IMPLEMENTATION(TEST, uint8_t, uint8_t* x, uint8_t* y) {
 *					if(data_generator_TEST_get_generator() != NULL)
 *					return data_generator_TEST_get_generator()(x, y);
 *				
 *				*x = 0;
 *				*y = 0;
 *				return 0;
 *			  }
 *				Implementation of the default data-generator function
 *
 *			The following functions are provided to control the data-generating module (NAME = X):
 *			- void data_generator_X_set_generator(X_data_generator_t)
 *				Function to set a external generator-function (could also be NULL) with the signature defined with the "DATA_GENERATOR_FUNCTION_DECLARATION" macro.
 *				@param[in] X_data_generator_t	The generator function that should be called to generate the data.
 *			- X_data_generator_t data_generator_X_get_generator(void)
 *				Function to retrieve the external generator-function (could also be NULL).
 *				@retval 	The current external data-generator function.
 *			- void data_generator_X_reset(void)
 *				Function to reset the data-generator module. It only set the external generator-function to NULL.
 *			- RET data_generator_X(...)
 *				The function that is called by the actual "read"-function. This function has to be implemented by the application to provide default values, 
 *				if no generator-function is set, and to call the generator-function if it is set.
 *				@params[in/out]	The parameter-sequence that are defined by the macros.
 *				@retval RET 	The return value of the function setted by the macro.
 */

#ifndef DATA_GENERATOR_INTERNAL_LIB_H
#define DATA_GENERATOR_INTERNAL_LIB_H


#include "stdint.h"
#include "stdio.h"


#define DATA_GENERATOR_FUNCTION_DECLARATION(NAME, RET, ...) \
typedef RET (* NAME##_data_generator_t) (__VA_ARGS__)

#define DATA_GENERATOR_DECLARATION(NAME, RET, ...) \
void data_generator_##NAME##_reset(void); \
NAME##_data_generator_t data_generator_##NAME##_get_generator(void); \
void data_generator_##NAME##_set_generator(NAME##_data_generator_t data_generator); \
RET data_generator_##NAME(__VA_ARGS__); 



#define DATA_GENERATOR_IMPLEMENTATION(NAME, RET, ...) \
static NAME##_data_generator_t	NAME##_data_generator = NULL; \
void data_generator_##NAME##_reset(void){ \
	NAME##_data_generator = NULL; \
} \
NAME##_data_generator_t data_generator_##NAME##_get_generator(void) { \
	return NAME##_data_generator; \
} \
void data_generator_##NAME##_set_generator(NAME##_data_generator_t data_generator) { \
	NAME##_data_generator = data_generator; \
} \
RET data_generator_##NAME(__VA_ARGS__)

#endif
