#include "storage1_lib.h"
#include "flash_lib.h"

#include "stdio.h"

//#define STORAGE1_SIZE	(FLASH_PAGE_SIZE_WORDS*FLASH_NUM_PAGES*sizeof(uint32_t)) /**< Size of storage1 in bytes */


#define STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE	4
int32_t storage1_last_stored_element_addresses[STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE];



/** @brief Retrieve the address of a page.
 * 
 * @param[in]	byte_address	The address of a byte.
 *
 * @retval 		Address of the specified page. Returns 0 if byte_address >= STORAGE1_SIZE.
 */
uint32_t storage1_get_page_address(uint32_t byte_address) {
	if(byte_address >= storage1_get_size()) {
		return 0;
	}
	
	uint32_t page_size_bytes = flash_get_page_size_words()*sizeof(uint32_t);

	
	return byte_address/page_size_bytes;
}

/** @brief Retrieve the number of pages associated with the byte_address and byte_length.
 * 
 * @param[in]	byte_address	The address of the first byte.
 * @param[in]	byte_length		The number of bytes.
 *
 * @retval 		Number of pages where the data would be stored to. Returns 0 if byte_length == 0.
 */
uint32_t storage1_get_page_number(uint32_t byte_address, uint32_t byte_length) {
	if(byte_address + byte_length > storage1_get_size())
		return 0;
	
	if(byte_length == 0)
		return 0;
	
	uint32_t start_page_address = storage1_get_page_address(byte_address);
	// Get address of page where the last byte would be written to
	uint32_t end_page_address = storage1_get_page_address(byte_address + byte_length - 1);
	
	return ((end_page_address + 1) - start_page_address);
	
}

/** @brief Function to retrieve the number of elements in storage1_last_stored_element_addresses-array.
 * 
 *
 * @retval 		Number of elements in storage1_last_stored_element_addresses-array
 */
uint32_t storage1_get_last_stored_element_addresses_size(void) {
	return STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE;
}


/** @brief Retrieve the address and number of pages that should be erased before writing.
 *
 * @details	Calculates the address and number of pages that should be erased before writing in an optimal way.
 *			It tries to minimize the number of page erases by using the storage1_last_stored_element_addresses-array.
 *			The storage1_last_stored_element_addresses-array saves the addresses of the last written addresses 
 *			that have been calculated by this function before.
 * 
 * @param[in]	address						The address of the first byte to write.
 * @param[in]	length_data					The number of bytes.
 * @param[out]	erase_start_page_address	Pointer to erase page address.
 * @param[out]	erase_num_pages				Pointer to number of pages to erase.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM		If length_data == 0 or specified address and length_data exceed the storage size.
 */
ret_code_t storage1_compute_pages_to_erase(uint32_t address, uint32_t length_data, uint32_t* erase_start_page_address, uint32_t* erase_num_pages) {
	*erase_start_page_address = 0;
	*erase_num_pages = 0;
	
	
	if(length_data == 0 || address + length_data > storage1_get_size()) {
		return NRF_ERROR_INVALID_PARAM;
	}
	
	uint32_t start_page_address = storage1_get_page_address(address);
	uint32_t num_pages = storage1_get_page_number(address, length_data);
	
	*erase_start_page_address = start_page_address;
	*erase_num_pages = num_pages;
	
	
	
	// Iterate over all storage1_last_stored_element_addresses to reset addresses that would be overwritten or erased
	// and to adapt the erase_start_page if possible.
	for(uint32_t i = 0; i < STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE; i++) {
		
		if(storage1_last_stored_element_addresses[i] == -1)
			continue;
		
		// Get local copy
		uint32_t last_stored_element_page_address = storage1_get_page_address(storage1_last_stored_element_addresses[i]);
		
		if(last_stored_element_page_address == start_page_address) {
			if(storage1_last_stored_element_addresses[i] < (int32_t)address) {
				*erase_start_page_address = start_page_address + 1;	
				*erase_num_pages = num_pages - 1;
			}
			
			storage1_last_stored_element_addresses[i] = -1;		
		} else {
			// Check if storage1_last_stored_element_addresses[i] is on the page before, and equals "address-1" --> could be deleted
			if(storage1_last_stored_element_addresses[i] == (int32_t)address - 1) {
				storage1_last_stored_element_addresses[i] = -1;
			}
			
			// Check if storage1_last_stored_element_addresses[i] is in a page that will be written to
			if(last_stored_element_page_address > start_page_address && last_stored_element_page_address < start_page_address + num_pages) {
				storage1_last_stored_element_addresses[i] = -1;	
			}			
		}		
	}	
	
	// Search for the index, where to store the current address
	int32_t last_stored_element_address = (int32_t) (address + length_data - 1);
	uint32_t index = 0;
	int32_t min_diff = storage1_get_size();
	for(uint32_t i = 0; i < STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE; i++) {
		if(storage1_last_stored_element_addresses[i] == -1) {
			index = i;
			break;
		} else if(storage1_last_stored_element_addresses[i] < last_stored_element_address && last_stored_element_address - storage1_last_stored_element_addresses[i] < min_diff) {
			min_diff = last_stored_element_address - storage1_last_stored_element_addresses[i];
			index = i;
		}		
	}
	
	storage1_last_stored_element_addresses[index] = last_stored_element_address;
	
	
	return NRF_SUCCESS;
}


ret_code_t storage1_init(void) {
	for(uint32_t i = 0; i < STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE; i++)
		storage1_last_stored_element_addresses[i] = -1;
	
	return flash_init();
}




ret_code_t storage1_store(uint32_t address, uint8_t* data, uint32_t length_data) {
	return NRF_SUCCESS;
}


ret_code_t storage1_store_uint8_t(uint32_t address, uint8_t* data, uint32_t length_data) {
	
	if(address + length_data > flash_get_page_number()*flash_get_page_size_words()*sizeof(uint32_t))
		return NRF_ERROR_INVALID_PARAM;
	
	// Integer math truncation
	uint32_t num_words = length_data/sizeof(uint32_t);
	uint32_t remaining_num_bytes = length_data % sizeof(uint32_t);
	
	
	
	printf("Words: %u, remaining: %u\n", num_words, remaining_num_bytes);
	
	uint32_t* words = (uint32_t*) data;
	
	for(uint32_t i = 0; i < num_words; i++) {
		printf("Word[%u]: 0x%X, ", i, words[i]);
	}
	
	printf("\n");
	
	
	
	// Check if there are some remaining bytes that have to be stored separately
	if(remaining_num_bytes > 0) {
		uint8_t remaining_bytes[sizeof(uint32_t)];
		for(uint32_t i = 0; i < sizeof(uint32_t); i++) {
			if(i < remaining_num_bytes)
				remaining_bytes[i] = data[num_words*sizeof(uint32_t) + i];
			else
				remaining_bytes[i] = 0xFF;
		}
		
		uint32_t remaining_word = *((uint32_t*) remaining_bytes);
		printf("Remaining word: 0x%X\n", remaining_word);
	}
	
	return NRF_SUCCESS;
}


uint32_t storage1_get_size(void) {
	return flash_get_page_number()*flash_get_page_size_words()*sizeof(uint32_t);
}
