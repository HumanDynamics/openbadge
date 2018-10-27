#include "flash_lib.h"

#include "debug_lib.h"


#include "string.h"		// For memset

#include "storage_file_lib.h"






#define FLASH_SIZE			(FLASH_PAGE_SIZE_WORDS*FLASH_NUM_PAGES*sizeof(uint32_t))	/**< Flash size in bytes */
#define FLASH_NUM_WORDS		(FLASH_PAGE_SIZE_WORDS*FLASH_NUM_PAGES)					/**< Flash size in words */


static uint32_t flash_words[FLASH_NUM_WORDS];	/**< Simulator of the internal flash words */




static volatile flash_operation_t flash_operation = FLASH_NO_OPERATION;	/**< The current flash operation ((in simulation it is actually not really used/set) */



/**@brief   Function for initializing the in the simulated flash module.
 *
 * @details If there is no flash file there this function creates one and initializes the file with 0xFF.
 *			
 *
 * @retval  NRF_SUCCESS    		If the module was successfully initialized.
 */
ret_code_t flash_init(void) {
	
	
	
	memset(flash_words, 0xFF, FLASH_SIZE);
	
	
	//debug_log("Flash initialized\n");
	return NRF_SUCCESS;
}


/**@brief   Function for erasing a page in the simulated flash (it is actually not non-blocking).
 *
 * @details Erases the desired pages by setting the bytes in the pages to 0xFF. 
 * 			It updates the file and the internal RAM-array.	
 *			
 *
 * @retval  NRF_SUCCESS    				If the erased the pages successfully.
 * @retval  NRF_ERROR_BUSY  			If there is already an ongoing Flash operation.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 */
ret_code_t flash_erase_bkgnd(uint32_t page_num, uint16_t num_pages) {
	
	// Check if we have already an ongoing operation
	if((flash_get_operation() & FLASH_STORE_OPERATION) || (flash_get_operation() & FLASH_ERASE_OPERATION)) {
		return NRF_ERROR_BUSY;
	}
	
	if(page_num + num_pages > flash_get_page_number())
		return NRF_ERROR_INVALID_PARAM;
	
	flash_operation = (flash_operation_t) (flash_operation | FLASH_ERASE_OPERATION);	
	// Reset the Error flag (if it exists)
	flash_operation = (flash_operation_t) (flash_operation & ~FLASH_ERASE_ERROR);

	
	
	uint32_t start_word_address = page_num*flash_get_page_size_words();
	uint32_t number_of_words = num_pages*flash_get_page_size_words();
	
	
	memset(&flash_words[start_word_address], 0xFF, number_of_words*sizeof(uint32_t));
	
	
	flash_operation = (flash_operation_t) (flash_operation & ~FLASH_ERASE_OPERATION);
	
	return NRF_SUCCESS;
}


/**@brief   Function for erasing a page in the simulated flash (it is actually the same as flash_erase_bkgnd()).
 *
 * @retval  NRF_SUCCESS    				If the erased the pages successfully.
 * @retval  NRF_ERROR_BUSY  			If there is already an ongoing Flash operation.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 * @retval  NRF_ERROR_TIMEOUT  			If the operation timed out (should not happen).
 */
ret_code_t flash_erase(uint32_t page_num, uint16_t num_pages) {
	ret_code_t ret = flash_erase_bkgnd(page_num, num_pages);
	
	// Return if erase operation was not started successfully
	if(ret != NRF_SUCCESS) {
		return ret;
	}
	
	// Wait for the erase operation to terminate.
	while(flash_get_operation() & FLASH_ERASE_OPERATION);
	
	// Return an error if the erase operation was not successful.
	if(flash_get_operation() & FLASH_ERASE_ERROR) {
		return NRF_ERROR_TIMEOUT; 
	}
	
	return NRF_SUCCESS;
	
}


/**@brief   Function for storing words to simulated flash (it is actually not non-blocking).
 *
 * @details Tries to store the words in the simulated flash. It mimics the dame behaviour flash has:
 *			Write from 1 to 0 is ok. From 0 to 1 is not valid/will not work.
 * 			It updates the file and the internal RAM-array accordingly.	
 *			
 *
 * @retval  NRF_SUCCESS    				If the store operation was successfully.
 * @retval  NRF_ERROR_BUSY  			If there is already an ongoing Flash operation.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 */
ret_code_t flash_store_bkgnd(uint32_t word_num, const uint32_t* p_words, uint16_t length_words) {	
	
	// Check if we have already an ongoing operation
	if((flash_get_operation() & FLASH_STORE_OPERATION) || (flash_get_operation() & FLASH_ERASE_OPERATION)) {
		return NRF_ERROR_BUSY;
	}
	
	if(p_words == NULL)
		return NRF_ERROR_INVALID_PARAM;
	
	if(((uintptr_t)p_words) % sizeof(uint32_t) != 0)
		return NRF_ERROR_INVALID_PARAM;

	if(word_num + length_words > flash_get_page_number()*flash_get_page_size_words())
		return NRF_ERROR_INVALID_PARAM;
	

	flash_operation = (flash_operation_t) (flash_operation | FLASH_STORE_OPERATION);
	// Reset the Error flag (if it exists)
	flash_operation = (flash_operation_t) (flash_operation & ~FLASH_STORE_ERROR);
	
	
	uint32_t start_word_address = word_num;
	uint32_t number_of_words = length_words;
	
	// Simulate the "flash behaviour" of setting bits to zero, but not to one again --> logical & 
	for(uint16_t i = 0; i < number_of_words; i++) {
		flash_words[start_word_address + i] = flash_words[start_word_address + i] & p_words[i];
	}
	
	// Reset the store operation
	flash_operation = (flash_operation_t) (flash_operation & ~FLASH_STORE_OPERATION);	
	
	return NRF_SUCCESS;
}


/**@brief   Function for storing words to simulated flash (it is actually the same as flash_store_bkgnd()).
 *
 * @retval  NRF_SUCCESS    				If the store operation was successfully.
 * @retval  NRF_ERROR_BUSY  			If there is already an ongoing Flash operation.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 * @retval  NRF_ERROR_TIMEOUT  			If the operation timed out (should not happen).
 */
ret_code_t flash_store(uint32_t word_num, const uint32_t* p_words, uint16_t length_words) {
	// Call the non-blocking flash store function.
	ret_code_t ret = flash_store_bkgnd(word_num, p_words, length_words);
	
	// Return if store operation was not started successfully
	if(ret != NRF_SUCCESS) {
		return ret;
	}
	
	// Wait for the store operation to terminate.
	while(flash_get_operation() & FLASH_STORE_OPERATION);
	
	// Return an error if the store operation was not successful.
	if(flash_get_operation() & FLASH_STORE_ERROR) {
		return NRF_ERROR_TIMEOUT; 
	}
	
	return NRF_SUCCESS;
	
}


flash_operation_t flash_get_operation(void) {
	return flash_operation;
}

/**@brief   Function for reading words from simulated flash.
 *
 * @details Reads the words from the internal RAM-array.
 *
 * @retval  NRF_SUCCESS    				If the read operation was successfully.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 */
ret_code_t flash_read(uint32_t word_num, uint32_t* p_words, uint16_t length_words) {
	
	
	if(word_num + length_words > (flash_get_page_size_words()*flash_get_page_number()))
		return NRF_ERROR_INVALID_PARAM;
	
	if(p_words == NULL)
		return NRF_ERROR_INVALID_PARAM;
	
	if(((uintptr_t)p_words) % sizeof(uint32_t) != 0)
		return NRF_ERROR_INVALID_PARAM;
	
	memcpy(p_words, &flash_words[word_num],  length_words*sizeof(uint32_t));
	
	return NRF_SUCCESS;
}


uint32_t flash_get_page_size_words(void) {
	
	return FLASH_PAGE_SIZE_WORDS;	
}

uint32_t flash_get_page_number(void) {
	return FLASH_NUM_PAGES;
}

void flash_write_to_file(const char* filename) {
	uint8_t* bytes = (uint8_t*) flash_words;
	uint32_t len = FLASH_NUM_WORDS*sizeof(uint32_t);
	storage_file_write_to_file(filename, bytes, len);
}
