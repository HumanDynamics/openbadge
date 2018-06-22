#include "storage_lib.h"

#include "stdio.h"

#include "storage1_lib.h"
#include "storage2_lib.h"


typedef ret_code_t 	(*storage_init_function_t)(void);
typedef ret_code_t 	(*storage_read_function_t)(uint32_t address, uint8_t* data, uint32_t length_data);
typedef ret_code_t 	(*storage_store_function_t)(uint32_t address, uint8_t* data, uint32_t length_data);
typedef uint32_t 	(*storage_get_size_function_t)(void);
typedef uint32_t 	(*storage_get_unit_size_function_t)(void);



storage_init_function_t 			storage_init_functions[] 			= {storage1_init, 			storage2_init};
storage_read_function_t 			storage_read_functions[] 			= {storage1_read, 			storage2_read};
storage_store_function_t 			storage_store_functions[]			= {storage1_store, 			storage2_store};
storage_get_size_function_t 		storage_get_size_functions[] 		= {storage1_get_size, 		storage2_get_size};
storage_get_unit_size_function_t 	storage_get_unit_size_functions[] 	= {storage1_get_unit_size, 	storage2_get_unit_size};


#define NUMBER_OF_STORAGE_MODULES (uint8_t) (sizeof(storage_init_functions)/sizeof(storage_init_function_t))	/**< The number of different storage modules */
static uint32_t storage_sizes[NUMBER_OF_STORAGE_MODULES];														/**< Contains the sizes of the storage modules (set during init-function) */

ret_code_t storage_init(void) {

	ret_code_t ret;
	
	for(uint8_t i = 0; i < NUMBER_OF_STORAGE_MODULES; i++) {
		
		// Set the storage_sizes to the sizes of the different storage-modules
		storage_sizes[i] = storage_get_size_functions[i]();
		
		ret = storage_init_functions[i]();
		if(ret != NRF_SUCCESS) {
			ret = NRF_ERROR_INTERNAL;
			return ret;
		}
		
	}	
	return NRF_SUCCESS;
}

void storage_split_to_storage_modules(uint32_t address, uint8_t* data, uint32_t length_data, uint32_t splitted_address[], uint8_t* splitted_data[], uint32_t splitted_length_data[], uint32_t storage_sizes[], uint8_t number_of_storage_modules) {
	uint32_t tmp_address = address;
	uint32_t tmp_length_data = length_data;
	uint32_t cumulated_size = 0;
	for(uint8_t i = 0; i < number_of_storage_modules; i++) {
		uint32_t size = storage_sizes[i];

		// Check if the address is within the i-th storage-module, and we have some data left
		if(tmp_address >= cumulated_size &&  tmp_address < cumulated_size + size && tmp_length_data > 0) {
			
			splitted_address[i] = tmp_address - cumulated_size;
			splitted_data[i] 	= &data[tmp_address - address];
			if(tmp_address + tmp_length_data <= cumulated_size + size) {
				splitted_length_data[i] = tmp_length_data;
			} else {
				splitted_length_data[i] = (cumulated_size + size) - tmp_address;
			}
			tmp_address += splitted_length_data[i];	
			tmp_length_data -= splitted_length_data[i];
		} else { // if we have nothing in the i-th storage module, set everything to NULL
			splitted_address[i] = 0;
			splitted_data[i] = NULL;
			splitted_length_data[i] = 0;
		}	
		
		cumulated_size += size;		
		
	}	
}


ret_code_t storage_store(uint32_t address, uint8_t* data, uint32_t length_data) {
	
	if(address + length_data > storage_get_size() || data == NULL) {
		return NRF_ERROR_INVALID_PARAM;
	}
	if(length_data == 0)
		return NRF_SUCCESS;
	
	
	uint32_t splitted_address[NUMBER_OF_STORAGE_MODULES];
	uint8_t* splitted_data[NUMBER_OF_STORAGE_MODULES];
	uint32_t splitted_length_data[NUMBER_OF_STORAGE_MODULES];
	
	storage_split_to_storage_modules(address, data, length_data, splitted_address, splitted_data, splitted_length_data, storage_sizes, NUMBER_OF_STORAGE_MODULES);
	
	
	for(uint8_t i = 0; i < NUMBER_OF_STORAGE_MODULES; i++)  {
		if(splitted_data[i] == NULL || splitted_length_data[i] == 0) continue;
		
		ret_code_t ret = storage_store_functions[i](splitted_address[i], splitted_data[i], splitted_length_data[i]);
		if(ret != NRF_SUCCESS) 
			return ret;
	}
	
	return NRF_SUCCESS;
}

ret_code_t storage_read(uint32_t address, uint8_t* data, uint32_t length_data) {
	
	if(address + length_data > storage_get_size() || data == NULL) {
		return NRF_ERROR_INVALID_PARAM;
	}
	if(length_data == 0)
		return NRF_SUCCESS;
	
	
	uint32_t splitted_address[NUMBER_OF_STORAGE_MODULES];
	uint8_t* splitted_data[NUMBER_OF_STORAGE_MODULES];
	uint32_t splitted_length_data[NUMBER_OF_STORAGE_MODULES];

	storage_split_to_storage_modules(address, data, length_data, splitted_address, splitted_data, splitted_length_data, storage_sizes, NUMBER_OF_STORAGE_MODULES);
	
	
	for(uint8_t i = 0; i < NUMBER_OF_STORAGE_MODULES; i++)  {
		
		
		if(splitted_data[i] == NULL || splitted_length_data[i] == 0) continue;
		ret_code_t ret = storage_read_functions[i](splitted_address[i], splitted_data[i], splitted_length_data[i]);
		if(ret != NRF_SUCCESS) 
			return ret;
	}
	
	return NRF_SUCCESS;
}

ret_code_t storage_get_unit_address_limits(uint32_t address, uint32_t length_data, uint32_t* start_unit_address, uint32_t* end_unit_address) {
	if(address + length_data > storage_get_size()) {
		return NRF_ERROR_INVALID_PARAM;
	}
	if(length_data == 0)
		return NRF_ERROR_INVALID_PARAM;
	
	uint32_t cumulated_size = 0;
	for(uint8_t i = 0; i < NUMBER_OF_STORAGE_MODULES; i++) {
		uint32_t size = storage_sizes[i];
		
		
		
		// Check if address is in the i-th storage module
		if(address >= cumulated_size && address < cumulated_size + size) {
			// The address relative to start address of the i-th storage module
			uint32_t tmp_address = address - cumulated_size;
			uint32_t unit_size = storage_get_unit_size_functions[i]();
			
			// Compute start_unit_address by integer truncation (round off)
			*start_unit_address = (tmp_address/unit_size)*unit_size + cumulated_size;
		}
		
		// Check if end-address is in the i-th storage module
		if(address + length_data - 1 >= cumulated_size && address + length_data - 1  < cumulated_size + size) {
			uint32_t tmp_address = address + length_data - 1 - cumulated_size;
			uint32_t unit_size = storage_get_unit_size_functions[i]();
			
			// Compute end_unit_address by integer truncation (round up)
			*end_unit_address = (tmp_address/unit_size + 1)*unit_size - 1 + cumulated_size;
			
			// If the end_unit_address is computed, the function can return --> leave for-loop
			break;
		}		
		cumulated_size += size;
	}
	
	return NRF_SUCCESS;	
}

uint32_t storage_get_size(void) {
	static uint32_t size = 0;
	if(size == 0) {	// Only compute the size once.
		for(uint8_t i = 0; i < sizeof(storage_get_size_functions)/sizeof(storage_get_size_function_t); i++) {
			size += storage_get_size_functions[i]();
		}
	}
	
	return size;
}

