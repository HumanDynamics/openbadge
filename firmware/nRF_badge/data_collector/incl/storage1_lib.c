#include "storage1_lib.h"
#include "flash_lib.h"

#include "stdio.h"

#define STORAGE1_SIZE	(FLASH_PAGE_SIZE_WORDS*FLASH_NUM_PAGES*sizeof(uint32_t)) /**< Size of storage1 in bytes */


#ifdef UNIT_TEST
	#define STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE	4	 /**< Number of addresses in storage1_last_stored_element_addresses-array, during unit testing. */
#else
	#define STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE	4	 /**< Number of addresses in storage1_last_stored_element_addresses-array, during normal operation. */
#endif

int32_t storage1_last_stored_element_addresses[STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE]; /**< Array to save the last/end addresses of stored elements */


uint8_t backup_data[FLASH_PAGE_SIZE_WORDS*sizeof(uint32_t)];					/**< Array to backup a whole flash page, needed for restoring bytes after a page erase */


/** @brief Retrieve the address of a page.
 * 
 * @param[in]	byte_address	The address of a byte.
 *
 * @retval 		Address of the page that contains the specified byte address. Returns 0 if byte_address >= STORAGE1_SIZE.
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
 * @retval 		NRF_ERROR_INVALID_PARAM		If specified address and length_data exceed the storage size.
 */
ret_code_t storage1_compute_pages_to_erase(uint32_t address, uint32_t length_data, uint32_t* erase_start_page_address, uint32_t* erase_num_pages) {
	*erase_start_page_address = 0;
	*erase_num_pages = 0;
	
	
	if(address + length_data > storage1_get_size()) {
		return NRF_ERROR_INVALID_PARAM;
	}
	if(length_data == 0) {
		return NRF_SUCCESS;
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


/** @brief Calculate the number of leading_num_bytes and final_num_bytes so that intermediate_num_bytes is word aligned.
 *
 * @details Example of clipping: The "|" is the word alignment separator, X is one byte: XX | XXXX | XXX
 *			leads to leading_num_bytes = 2, intermediate_num_bytes = 4, final_num_bytes = 3.
 *			If the data are within one single word, only intermediate_num_bytes is set to length_data.
 * 
 * @param[in]	address						The address of the first byte to write.
 * @param[in]	length_data					The number of bytes.
 * @param[out]	leading_num_bytes			Pointer to leading number of bytes (0 if address is word aligned).
 * @param[out]	intermediate_num_bytes		Pointer to intermediate number of bytes. Clipped to be word aligned.
 * @param[out]	final_num_bytes				Pointer to final number of bytes (0 if address+length is word aligned).
 */
void storage1_compute_word_aligned_addresses(uint32_t address, uint32_t length_data, uint32_t* leading_num_bytes, uint32_t* intermediate_num_bytes, uint32_t* final_num_bytes) {
	
	
	*leading_num_bytes = 0;
	*final_num_bytes = 0;
	*intermediate_num_bytes = 0;
	
	if(length_data == 0)
		return;
	
	uint32_t round_off_start_address = address/sizeof(uint32_t);
	uint32_t round_off_end_address = (address+length_data-1)/sizeof(uint32_t);
	
	// Check if the data range is just within one word
	if((round_off_start_address == round_off_end_address) && (address % sizeof(uint32_t) != 0) && ((address + length_data) % sizeof(uint32_t) != 0)) {
		*intermediate_num_bytes = length_data;		
	} else {	
		// Integer math truncation: round start address to next word address, subtract address, modulo word size
		*leading_num_bytes = ((address / sizeof(uint32_t) + 1)*sizeof(uint32_t) - address) % sizeof(uint32_t);
		
		// Integer math truncation: round end address to previous word address, subtract it from the end address, modulo word size
		*final_num_bytes = ((address + length_data)  -  (((address + length_data) / sizeof(uint32_t))*sizeof(uint32_t))) % sizeof(uint32_t);
		
		if(length_data >=  (*final_num_bytes + *leading_num_bytes))	// only for safety reasons
			*intermediate_num_bytes = length_data - (*final_num_bytes + *leading_num_bytes);
	}
}


/** @brief Function for storing uint8-data in the 32bit-flash.
 *
 * @details	The function splits the bytes into words (fills them if necessary with 0xFF) and stores them to flash.
 * 
 * @param[in]	address					The address of the first byte to write.
 * @param[in]	data					Pointer to the bytes to store.
 * @param[in]	length_data				The number of bytes.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM		If specified address and length_data exceed the storage size.
 * @retval 		NRF_ERROR_BUSY				If the underlying storage-module (here flash) is busy.
 * @retval 		NRF_ERROR_INTERNAL			If the underlying storage-module (here flash) was not correctly initialized or the operation timed out (sth. went wrong).
 */
ret_code_t storage1_store_uint8_as_uint32(uint32_t address, uint8_t* data, uint32_t length_data) {
	
	if(address + length_data > storage1_get_size())
		return NRF_ERROR_INVALID_PARAM;
	
	if(length_data == 0)
		return NRF_SUCCESS;
	

	ret_code_t ret = NRF_SUCCESS;
	
	// Compute the word aligned addresses
	uint32_t leading_num_bytes, intermediate_num_bytes, final_num_bytes;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	
	uint32_t * words;
	uint32_t word_address_aligned;
	uint32_t length_words;
	
	
	// first write the intermediate bytes as words to flash
	if(intermediate_num_bytes > 0) {
		
		if(intermediate_num_bytes < sizeof(uint32_t)) { // in this case leading_num_bytes and final_num_bytes should be 0.
		
			uint8_t tmp[sizeof(uint32_t)];		
			for(uint8_t i = 0; i < sizeof(uint32_t); i++) 
				tmp[i] = 0xFF;
			uint8_t start_index = (uint8_t)(address % sizeof(uint32_t));
			uint8_t end_index = (start_index + intermediate_num_bytes)% sizeof(uint32_t); // % sizeof(uint32_t) just for safety reasons
			
			for(uint8_t i = start_index; i < end_index; i++) {
				tmp[i] = data[i - start_index];
			}
			
			// Cast to word pointer
			words = (uint32_t*) tmp;
			// Calculate aligned word address with integer truncation
			word_address_aligned = (address/sizeof(uint32_t));
			// Set the number of words to 1
			length_words = 1;
			
		} else {
			// Cast to word pointer
			words = (uint32_t*) (&data[leading_num_bytes]);
			// Calculate aligned word address with integer truncation
			word_address_aligned = (address + leading_num_bytes)/sizeof(uint32_t);
			// Calculate the number of words with integer truncation
			length_words = intermediate_num_bytes/sizeof(uint32_t);	
		}
		
		// Store the words to flash
		ret = flash_store(word_address_aligned, words, length_words);
		if(ret != NRF_SUCCESS) {	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INTERNAL, NRF_ERROR_INVALID_PARAM, NRF_ERROR_TIMEOUT
			if(ret == NRF_ERROR_INTERNAL || ret == NRF_ERROR_TIMEOUT)
				ret = NRF_ERROR_INTERNAL;	
			// ret could be NRF_ERROR_BUSY, NRF_ERROR_INVALID_PARAM, NRF_ERROR_INTERNAL
			return ret;
		}
	}
	
	
	// then write the leading bytes as words to flash
	if(leading_num_bytes > 0) {
		// Generate a word that is 0xFF where no data byte is.
		uint8_t tmp[sizeof(uint32_t)];		
		uint8_t start_index = sizeof(uint32_t) - leading_num_bytes;
		for(uint8_t i = 0; i < sizeof(uint32_t); i++) {
			if(i < start_index) {
				tmp[i] = 0xFF;
			} else {
				tmp[i] = data[i - start_index];
			}
		}	
		
		// Cast to word pointer
		words = (uint32_t*) tmp;
		// Calculate aligned word address with integer truncation
		word_address_aligned = (address/sizeof(uint32_t));
		// Set the number of words to 1
		length_words = 1;
		// Store the words to flash
		ret = flash_store(word_address_aligned, words, length_words);
		if(ret != NRF_SUCCESS) {	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INTERNAL, NRF_ERROR_INVALID_PARAM, NRF_ERROR_TIMEOUT
			if(ret == NRF_ERROR_INTERNAL || ret == NRF_ERROR_TIMEOUT)
				ret = NRF_ERROR_INTERNAL;	
			// ret could be NRF_ERROR_BUSY, NRF_ERROR_INVALID_PARAM, NRF_ERROR_INTERNAL
			return ret;
		}
	}
	
	
	
	
	
	
	// eventually, write the final bytes as words to flash
	if(final_num_bytes > 0) {
		// Generate a word that is 0xFF where no data byte is.
		uint8_t tmp[sizeof(uint32_t)];		
		for(uint8_t i = 0; i < sizeof(uint32_t); i++) {
			if(i < final_num_bytes) {
				tmp[i] = data[i + leading_num_bytes + intermediate_num_bytes];
			} else {
				tmp[i] = 0xFF;
			}
		}
		// Cast to word pointer
		words = (uint32_t*) tmp;
		// Calculate aligned word address with integer truncation
		word_address_aligned = (address + leading_num_bytes + intermediate_num_bytes)/sizeof(uint32_t);
		// Set the number of words to 1
		length_words = 1;
		// Store the words to flash
		ret = flash_store(word_address_aligned, words, length_words);
		if(ret != NRF_SUCCESS) {	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INTERNAL, NRF_ERROR_INVALID_PARAM, NRF_ERROR_TIMEOUT
			if(ret == NRF_ERROR_INTERNAL || ret == NRF_ERROR_TIMEOUT)
				ret = NRF_ERROR_INTERNAL;	
			// ret could be NRF_ERROR_BUSY, NRF_ERROR_INVALID_PARAM, NRF_ERROR_INTERNAL
			return ret;
		}
	}
	
	
	return NRF_SUCCESS;
}


/** @brief Function for reading uint8-data from the 32bit-flash.
 *
 * @details	The function reads the words from flash and filters out the needed bytes.
 * 
 * @param[in]	address					The address of the first byte to read.
 * @param[in]	data					Pointer to memory where the bytes should be stored.
 * @param[in]	length_data				The number of bytes.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM		If specified address and length_data exceed the storage size.
 */
ret_code_t storage1_read_uint32_as_uint8(uint32_t address, uint8_t* data, uint32_t length_data) {
	
	if(address + length_data > storage1_get_size())
		return NRF_ERROR_INVALID_PARAM;
	
	if(length_data == 0)
		return NRF_SUCCESS;
	
	ret_code_t ret = NRF_SUCCESS;
	
	// Compute the word aligned addresses
	uint32_t leading_num_bytes, intermediate_num_bytes, final_num_bytes;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	
	uint32_t word;
	uint32_t word_address_aligned;
	uint32_t length_words;
	
	
	// first read the intermediate bytes from flash
	if(intermediate_num_bytes > 0) {
		if(intermediate_num_bytes < sizeof(uint32_t)) { // in this case leading_num_bytes and final_num_bytes should be 0.	
			
			// Calculate aligned word address with integer truncation
			word_address_aligned = (address/sizeof(uint32_t));
			// Set the number of words to 1
			length_words = 1;
			
			// Read the word from flash
			ret = flash_read(word_address_aligned, &word, length_words);
			if(ret != NRF_SUCCESS) { // ret could be NRF_SUCCESS, NRF_ERROR_INVALID_PARAM
				return ret;
			}
			
		
			uint8_t * tmp = (uint8_t*) &word;	
			uint8_t start_index = (uint8_t)(address % sizeof(uint32_t));
			uint8_t end_index = (start_index + intermediate_num_bytes)% sizeof(uint32_t); // % sizeof(uint32_t) just for safety reasons
			
			for(uint8_t i = start_index; i < end_index; i++) {
				data[i - start_index] = tmp[i];
			}
		} else {
			// Calculate aligned word address with integer truncation
			word_address_aligned = (address + leading_num_bytes)/sizeof(uint32_t);
			// Calculate the number of words with integer truncation
			length_words = intermediate_num_bytes/sizeof(uint32_t);
			
			// Read the words/bytes from flash
			ret = flash_read(word_address_aligned, (uint32_t*) (&data[leading_num_bytes]), length_words);
			if(ret != NRF_SUCCESS) { // ret could be NRF_SUCCESS, NRF_ERROR_INVALID_PARAM
				return ret;
			}
		}
	}
	
	
	// then read the leading bytes from flash
	if(leading_num_bytes > 0) {
		// Calculate aligned word address with integer truncation
		word_address_aligned = (address/sizeof(uint32_t));
		// Set the number of words to 1
		length_words = 1;
		
		// Read the word from flash
		ret = flash_read(word_address_aligned, &word, length_words);
		if(ret != NRF_SUCCESS) { // ret could be NRF_SUCCESS, NRF_ERROR_INVALID_PARAM
			return ret;
		}
		
		uint8_t * tmp = (uint8_t*) &word;
		uint8_t start_index = sizeof(uint32_t) - leading_num_bytes;
		for(uint8_t i = start_index; i < sizeof(uint32_t); i++) {
			data[i - start_index] = tmp[i];
		}
	}
	
	
	
	// eventually, read the final bytes from flash
	if(final_num_bytes > 0) {
		

		// Calculate aligned word address with integer truncation
		word_address_aligned = (address + leading_num_bytes + intermediate_num_bytes)/sizeof(uint32_t);
		// Set the number of words to 1
		length_words = 1;
		
		// Read the word from flash
		ret = flash_read(word_address_aligned, &word, length_words);
		if(ret != NRF_SUCCESS) { // ret could be NRF_SUCCESS, NRF_ERROR_INVALID_PARAM
			return ret;
		}
		
		uint8_t * tmp = (uint8_t*) &word;
		
		for(uint8_t i = 0; i < final_num_bytes; i++) {
			data[i + leading_num_bytes + intermediate_num_bytes] = tmp[i];
		}
	}
	return NRF_SUCCESS;	
}






ret_code_t storage1_init(void) {
	
	
	
	for(uint32_t i = 0; i < STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE; i++)
		storage1_last_stored_element_addresses[i] = -1;
	
	// Flag if the initialization has already be done and was successful
	static uint8_t init_done = 0;
	
	// Directly return if the flash module was already initialized successfully (but only in normal operation, not in testing mode).
	#ifndef UNIT_TEST
	if(init_done) {
		return NRF_SUCCESS;
	}
	#else	// To not generate compiler warnings
	(void) init_done;
	#endif
	
	ret_code_t ret = flash_init();
	
	if(ret == NRF_SUCCESS) {
		init_done = 1;
	}
	
	return ret;
}




ret_code_t storage1_store(uint32_t address, uint8_t* data, uint32_t length_data) {
	
	
	if(address + length_data > storage1_get_size() || data == NULL)
		return NRF_ERROR_INVALID_PARAM;
	
	if(length_data == 0)
		return NRF_SUCCESS;
	
	ret_code_t ret;
	
	// Save the old data on the same page
	uint32_t start_page_address = storage1_get_page_address(address);
	uint32_t start_page_first_byte_address = start_page_address*flash_get_page_size_words()*sizeof(uint32_t);
	uint32_t backup_data_length = address - start_page_first_byte_address;
	if(backup_data_length > sizeof(backup_data))	// just for security reasons
		return NRF_ERROR_INTERNAL;
	
	if(backup_data_length > 0) {
		ret = storage1_read_uint32_as_uint8(start_page_first_byte_address, backup_data, backup_data_length);
		if(ret != NRF_SUCCESS) { // ret could be NRF_SUCCESS, NRF_ERROR_INVALID_PARAM
			return ret;
		}
	}
	
	
	// Compute the pages to erase
	uint32_t erase_start_page_address, erase_num_pages;
	ret = storage1_compute_pages_to_erase(address, length_data, &erase_start_page_address, &erase_num_pages);
	if(ret != NRF_SUCCESS) { // ret could be NRF_SUCCESS, NRF_ERROR_INVALID_PARAM
		return ret;
	}	
	// Erase the pages
	if(erase_num_pages > 0) {
		ret = flash_erase(erase_start_page_address, erase_num_pages);
		if(ret != NRF_SUCCESS) { // ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INTERNAL, NRF_ERROR_INVALID_PARAM, NRF_ERROR_TIMEOUT
			if(ret == NRF_ERROR_INTERNAL || ret == NRF_ERROR_TIMEOUT)
				ret = NRF_ERROR_INTERNAL;	
			// ret could be NRF_ERROR_BUSY, NRF_ERROR_INVALID_PARAM, NRF_ERROR_INTERNAL
			return ret;
		}
	}
	
	// Restore the backup data, but only if the first page was erased
	if(backup_data_length > 0 && erase_start_page_address == start_page_address) {
		ret = storage1_store_uint8_as_uint32(start_page_first_byte_address, backup_data, backup_data_length);
		if(ret != NRF_SUCCESS) {  // ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INTERNAL, NRF_ERROR_INVALID_PARAM
			return ret;
		}
	}
	
	// Finally store the data to flash
	ret = storage1_store_uint8_as_uint32(address, data, length_data);
	if(ret != NRF_SUCCESS) {  // ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INTERNAL, NRF_ERROR_INVALID_PARAM
		return ret;
	}
	
	
	
	
	return NRF_SUCCESS;
}



ret_code_t storage1_read(uint32_t address, uint8_t* data, uint32_t length_data) {
	if(address + length_data > storage1_get_size() || data == NULL)
		return NRF_ERROR_INVALID_PARAM;
	
	if(length_data == 0)
		return NRF_SUCCESS;
	
	ret_code_t ret = storage1_read_uint32_as_uint8(address, data, length_data);
	return ret;	
}


uint32_t storage1_get_unit_size(void) {
	return flash_get_page_size_words()*sizeof(uint32_t);
}

uint32_t storage1_get_size(void) {
	return STORAGE1_SIZE;
}
