#include "microphone_lib.h"


#include "callback_generator_lib.h"
#include "data_generator_lib.h"

void microphone_init(void) {
}

ret_code_t microphone_read(uint8_t* value) {
	*value = 99;
	return NRF_SUCCESS;;
}


bool microphone_selftest(void) {
	return 1;
	
}



