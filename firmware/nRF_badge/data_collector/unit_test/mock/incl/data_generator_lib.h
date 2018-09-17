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

/** Accelerometer defines */
DATA_GENERATOR_FUNCTION_DECLARATION(accel_read_acceleration, ret_code_t, int16_t* accel_x, int16_t* accel_y, int16_t* accel_z, uint8_t* num_samples, uint32_t max_num_samples);
DATA_GENERATOR_DECLARATION(accel_read_acceleration, ret_code_t, int16_t* accel_x, int16_t* accel_y, int16_t* accel_z, uint8_t* num_samples, uint32_t max_num_samples);


/** BLE defines */
DATA_GENERATOR_FUNCTION_DECLARATION(ble_get_MAC_address, void, uint8_t* MAC_address, uint8_t len);
DATA_GENERATOR_DECLARATION(ble_get_MAC_address, void, uint8_t* MAC_address, uint8_t len);

#endif
