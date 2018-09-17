#include "data_generator_internal_lib.h"
#include "data_generator_lib.h"
#include <string.h>


DATA_GENERATOR_IMPLEMENTATION(accel_read_acceleration, ret_code_t, int16_t* accel_x, int16_t* accel_y, int16_t* accel_z, uint8_t* num_samples, uint32_t max_num_samples) {
	
	if(data_generator_accel_read_acceleration_get_generator() != NULL) {
		return data_generator_accel_read_acceleration_get_generator() (accel_x, accel_y, accel_z, num_samples, max_num_samples);
	}
	
	*accel_x = 0;
	*accel_y = 0;
	*accel_z = 0;
	*num_samples = (max_num_samples == 0) ? 0 : 1;
	return NRF_SUCCESS;
}


DATA_GENERATOR_IMPLEMENTATION(ble_get_MAC_address, void, uint8_t* MAC_address, uint8_t len) {
	
	memset(MAC_address, 0, len);
	if(data_generator_ble_get_MAC_address_get_generator() != NULL) {
		data_generator_ble_get_MAC_address_get_generator()(MAC_address, len);
	}

}