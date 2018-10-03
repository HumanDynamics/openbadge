#include "battery_lib.h"


#include "callback_generator_lib.h"
#include "data_generator_lib.h"


void battery_init(void) {
	
}

ret_code_t battery_read_voltage(float* voltage) {
	*voltage = 3.0;
	
	return NRF_SUCCESS;
}



bool battery_selftest(void) {
	
	
	return 1;
}
