#include "battery_lib.h"


#include "callback_generator_lib.h"
#include "data_generator_lib.h"


ret_code_t battery_init(void) {
	return NRF_SUCCESS;
}

float battery_get_voltage(void) {
	return 3.0;
}



bool battery_selftest(void) {
	
	
	return 1;
}
