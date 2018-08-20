/** @file
 *
 *
 * @brief 
 */

#ifndef DATA_GENERATOR_LIB_H
#define DATA_GENERATOR_LIB_H

#include "data_generator_internal_lib.h"
#include "stdint.h"
#include "sdk_errors.h"		// Needed for the definition of ret_code_t and the error-codes

DATA_GENERATOR_FUNCTION_DECLARATION(accel_read_acceleration, ret_code_t, int16_t* accel_x, int16_t* accel_y, int16_t* accel_z, uint8_t* num_samples);
DATA_GENERATOR_DECLARATION(accel_read_acceleration, ret_code_t, int16_t* accel_x, int16_t* accel_y, int16_t* accel_z, uint8_t* num_samples);

#endif
