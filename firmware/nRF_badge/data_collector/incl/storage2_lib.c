#include "storage2_lib.h"

#include "eeprom_lib.h"

#include "stdio.h"	

#define STORAGE2_SIZE (EEPROM_SIZE)



ret_code_t storage2_init(void) {
	
	static uint8_t init_done = 0;
	
	// Directly return if the flash module was already initialized successfully (but only in normal operation, not in testing mode).
	#ifndef UNIT_TEST
	if(init_done) {
		return NRF_SUCCESS;
	}
	#else	// To not generate compiler warnings
	(void) init_done;
	#endif
	
	ret_code_t ret = eeprom_init();
	
	if(ret == NRF_SUCCESS) {
		init_done = 1;
	}
	
	return ret;	
}



ret_code_t storage2_store(uint32_t address, uint8_t* data, uint32_t length_data) {
	if(address + length_data > storage2_get_size() || data == NULL)
		return NRF_ERROR_INVALID_PARAM;
	if(length_data == 0)
		return NRF_SUCCESS;
	
	ret_code_t ret = eeprom_store(address, data, length_data);
	return ret;
}

ret_code_t storage2_read(uint32_t address, uint8_t* data, uint32_t length_data) {
	if(address + length_data > storage2_get_size() || data == NULL)
		return NRF_ERROR_INVALID_PARAM;
	if(length_data == 0)
		return NRF_SUCCESS;
	
	ret_code_t ret = eeprom_read(address, data, length_data);
	return ret;
}



uint32_t storage2_get_unit_size(void) {
	return 1;
}

uint32_t storage2_get_size(void) {
	return STORAGE2_SIZE;
}