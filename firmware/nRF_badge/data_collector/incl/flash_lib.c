#include "flash_lib.h"


#define NUM_PAGES 10	// define this by the linker scriptm with enough space for new program code!


#include "uart_lib.h"

extern uart_instance_t uart_instance;


typedef enum {
	FLASH_NO_OPERATION 					= 0,
	FLASH_ERASE_OPERATION 				= (1 << 0),	
	FLASH_STORE_OPERATION 				= (1 << 1),
	FLASH_ERASE_ERROR					= (1 << 2),
	FLASH_STORE_ERROR					= (1 << 3),
} flash_operation_t;


static volatile flash_operation_t flash_operation = FLASH_NO_OPERATION;



static void fs_evt_handler(fs_evt_t const * const evt, fs_ret_t result)
{
    if (result == FS_SUCCESS) {
		
		// Set operation to NO_OPERATION
		if(flash_operation == FLASH_ERASE_OPERATION || flash_operation == FLASH_STORE_OPERATION) {
			flash_operation = FLASH_NO_OPERATION;
		}
		
    } else {
		
		// Set the Error of the specific operation
		if(flash_operation == FLASH_ERASE_OPERATION) {
			flash_operation = FLASH_ERASE_ERROR;
		} else if(flash_operation == FLASH_STORE_OPERATION) {
			flash_operation = FLASH_STORE_ERROR;
		}		
	}
}



FS_REGISTER_CFG(fs_config_t fs_config) =
{
    .callback  = fs_evt_handler, // Function for event callbacks.
    .num_pages = NUM_PAGES,      // Number of physical flash pages required.
    .priority  = 0xFE            // Priority for flash usage.
};



/*
// Retrieve the address of a page.
static uint32_t const * address_of_page(uint16_t page_num)
{
    return fs_config.p_start_addr + (page_num * PAGE_SIZE_WORDS);
}
*/

// Retrieve the address of a word.
static uint32_t const * address_of_word(uint32_t word_num)
{
    return fs_config.p_start_addr + (word_num);
}



ret_code_t flash_init(void) {
	fs_ret_t status_init = fs_init();
	
	uart_printf(&uart_instance, "Init started: max_words: %d, max_retries: %d, queue_size: %d, fs_enabled: %d\n\r", FS_MAX_WRITE_SIZE_WORDS, FS_OP_MAX_RETRIES, FS_QUEUE_SIZE, FSTORAGE_ENABLED);
	
	
	ret_code_t ret = NRF_SUCCESS;
	if(status_init != FS_SUCCESS) {
		ret = NRF_ERROR_INTERNAL;
	}
	
	return ret;
}




ret_code_t flash_store(uint32_t word_num, const uint32_t* p_words, uint16_t length_words) {
	
	
	// Last operation timed out!
	if(flash_operation == FLASH_STORE_ERROR || flash_operation == FLASH_ERASE_ERROR) {
		return NRF_ERROR_TIMEOUT;
	}
	
	// We have already an ongoing operation!
	if(flash_operation == FLASH_STORE_OPERATION || flash_operation == FLASH_ERASE_OPERATION) {
		return NRF_ERROR_BUSY;
	}
	
	flash_operation = FLASH_STORE_OPERATION;
	
	fs_ret_t status_store = fs_store(&fs_config, address_of_word(word_num), p_words, length_words, NULL);

	
	ret_code_t ret = NRF_SUCCESS;
	
	if(status_store != FS_SUCCESS) {
		if(status_store == FS_ERR_QUEUE_FULL) {
			ret = NRF_ERROR_BUSY;
		} else {
			ret = NRF_ERROR_INTERNAL;
		}
	}
	
	return ret;
}

