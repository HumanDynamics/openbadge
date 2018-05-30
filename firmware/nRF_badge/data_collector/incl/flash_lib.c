#include "flash_lib.h"


#include "fstorage.h"

#include "fstorage_internal_defs.h" // Needs to be included here, because of the "static/private" function declaration of fs_flash_page_end_addr(). 
									// Otherwise there will be a warning at compilation. Furthermore the function has to be called at least once (through FS_PAGE_END_ADDR), 
									// which is done in flash_get_page_size()



#include "system_event_lib.h"	// Needed to register an system event handler!


#define FLASH_ENABLE_STORE_CHECK 1


#include "uart_lib.h"
extern uart_instance_t uart_instance;

typedef struct {
	uint32_t word_num;
	const uint32_t* p_store_words;			// Pointer to the next data that has to be sent
	uint32_t length_words;				// Number of bytes that are pending to be sent 
} flash_store_operation_context_t;



static volatile flash_store_operation_context_t flash_store_operation_context;

static volatile flash_store_operation_t flash_store_operation = FLASH_STORE_NO_OPERATION;

static volatile flash_erase_operation_t flash_erase_operation = FLASH_ERASE_NO_OPERATION;


static uint32_t const * address_of_page(uint16_t page_num);
static uint32_t const * address_of_word(uint32_t word_num);



// The event handler for fstorage events.
static void fs_evt_handler(fs_evt_t const * const evt, fs_ret_t result)
{
    if (result == FS_SUCCESS) {	// Operation was successful!
		if(flash_erase_operation == FLASH_ERASE_OPERATION) {
			flash_erase_operation = FLASH_ERASE_NO_OPERATION;
		}
		
		if(flash_store_operation == FLASH_STORE_OPERATION) {
			#if FLASH_ENABLE_STORE_CHECK
			
			//uart_printf_bkgnd(&uart_instance, NULL, "Check if data are stored correctly: %p, %p, %d\n\r",flash_store_operation_context.p_store_words, address_of_word(flash_store_operation_context.word_num), flash_store_operation_context.length_words);
			
			// Check if the data are stored as expected!
			if(memcmp(flash_store_operation_context.p_store_words, address_of_word(flash_store_operation_context.word_num), flash_store_operation_context.length_words) == 0) {
			#endif
				
				flash_store_operation = FLASH_STORE_NO_OPERATION;
				
			#if FLASH_ENABLE_STORE_CHECK
			} else {
				flash_store_operation = FLASH_STORE_ERROR;
			}	
			#endif
			
			
		}
		
    } else {	// An error occurred during the operation!
	
		// Set the Error of the specific operation
		if(flash_erase_operation == FLASH_ERASE_OPERATION) {
			flash_erase_operation = FLASH_ERASE_ERROR;
		}
		
		if(flash_store_operation == FLASH_STORE_OPERATION) {
			flash_store_operation = FLASH_STORE_ERROR;
		}
	}
}





FS_REGISTER_CFG(fs_config_t fs_config) =
{
    .callback  = fs_evt_handler, // Function for event callbacks.
    .num_pages = NUM_PAGES,      // Number of physical flash pages required.
    .priority  = 0xFE            // Priority for flash usage.
};

ret_code_t flash_init(void) {
	
	// TODO: remove and put in main.c
	system_event_init();
	
	
	
	ret_code_t ret = NRF_SUCCESS;
	
	fs_ret_t status_init = fs_init();
	
	if(status_init != FS_SUCCESS) {
		ret = NRF_ERROR_INTERNAL;
	}
	
	// Register the system event handler for flash!
	system_event_register_handler(fs_sys_event_handler);
	
	
	return ret;
}


// Retrieve the address of a page.
static uint32_t const * address_of_page(uint16_t page_num)
{
    return fs_config.p_start_addr + (page_num * FS_PAGE_SIZE_WORDS);
}


// Retrieve the address of a word.
static uint32_t const * address_of_word(uint32_t word_num)
{
    return fs_config.p_start_addr + (word_num);
}







ret_code_t flash_erase(uint32_t page_num, uint16_t num_pages) {
	
	// We have already an ongoing operation!
	if(flash_store_operation == FLASH_STORE_OPERATION || flash_erase_operation == FLASH_ERASE_OPERATION) {
		return NRF_ERROR_BUSY;
	}
	
	flash_erase_operation = FLASH_ERASE_OPERATION;
	
	// Call the fstorage library for the actual erase operation.
	fs_ret_t status_erase = fs_erase(&fs_config, address_of_page(page_num), num_pages, NULL);
	
	
	// Prepare the return value:
	ret_code_t ret = NRF_SUCCESS;
	if(status_erase != FS_SUCCESS) {
		
		// Reset to no Operation
		flash_erase_operation = FLASH_ERASE_NO_OPERATION;
		
		if(status_erase == FS_ERR_QUEUE_FULL) {
			ret = NRF_ERROR_BUSY;
		} else if(status_erase == FS_ERR_NOT_INITIALIZED || status_erase == FS_ERR_INVALID_CFG){
			ret = NRF_ERROR_INTERNAL;
		} else { // FS_ERR_NULL_ARG || FS_ERR_INVALID_ARG || FS_ERR_INVALID_ADDR || FS_ERR_UNALIGNED_ADDR
			ret = NRF_ERROR_INVALID_PARAM;
		}
	}
	
	return ret;
}



ret_code_t flash_store(uint32_t word_num, const uint32_t* p_words, uint16_t length_words) {	
	
	// Check if there is already an ongoing operation!
	if(flash_store_operation == FLASH_STORE_OPERATION || flash_erase_operation == FLASH_ERASE_OPERATION) {
		return NRF_ERROR_BUSY;
	}
	
	if(p_words == NULL)
		return NRF_ERROR_INVALID_PARAM;
	
	
	flash_store_operation = FLASH_STORE_OPERATION;
	
	flash_store_operation_context.word_num = word_num;
	flash_store_operation_context.p_store_words = p_words;
	flash_store_operation_context.length_words = length_words;
	
	
	// Call the fstorage library for the actual storage operation.
	fs_ret_t status_store = fs_store(&fs_config, address_of_word(word_num), p_words, length_words, NULL);
	
	

	// Prepare the return value:
	ret_code_t ret = NRF_SUCCESS;
	if(status_store != FS_SUCCESS) {
		
		// Reset to no Operation
		flash_store_operation = FLASH_STORE_NO_OPERATION;
		
		if(status_store == FS_ERR_QUEUE_FULL) {
			ret = NRF_ERROR_BUSY;
		} else if(status_store == FS_ERR_NOT_INITIALIZED || status_store == FS_ERR_INVALID_CFG){
			ret = NRF_ERROR_INTERNAL;
		} else { // FS_ERR_NULL_ARG || FS_ERR_INVALID_ARG || FS_ERR_INVALID_ADDR || FS_ERR_UNALIGNED_ADDR
			ret = NRF_ERROR_INVALID_PARAM;
		}
	}
	
	return ret;
}




ret_code_t flash_read(uint32_t word_num, uint32_t* p_words, uint16_t length_words) {
	for(uint32_t i = 0; i < length_words; i++) {
		p_words[i] = *(address_of_word(word_num + i));		
	}
	return NRF_SUCCESS;
}



uint32_t flash_get_page_size_words(void) {
	// This has to be done, otherwise there will be a warning at compile time that the fs_flash_page_end_addr()-function is unused!
	// This call is done in flash_get_page_size(), because flash_get_page_size() is the only function that needs the fstorage_internal_defs.h File
	(void) FS_PAGE_END_ADDR;	
	
	
	return FS_PAGE_SIZE_WORDS;	
}

uint32_t flash_get_page_number(void) {
	return NUM_PAGES;
}



flash_store_operation_t flash_get_store_operation(void) {
	return flash_store_operation;
}

flash_erase_operation_t flash_get_erase_operation(void) {
	return flash_erase_operation;
}


bool flash_selftest(void) {
	
	uart_printf(&uart_instance, "Started flash selftest...\n\r");
	
	uart_printf(&uart_instance, "Page size words: %u, Number of pages: %u\n\r", flash_get_page_size_words(), flash_get_page_number());
	
	
	// Just a dummy write to test erasing..
	uint32_t tmp = 0xABCD1234;		
	flash_store(0, &tmp, 1);

	while(flash_get_store_operation() == FLASH_STORE_OPERATION);
	
	
//******************** Test erasing 2 pages *************************	
	ret_code_t ret = flash_erase(0, 2);
	uart_printf(&uart_instance, "Started erasing: Ret %d\n\r", ret);
	if(ret != NRF_SUCCESS) {
		uart_printf(&uart_instance, "Start erasing failed!\n\r");
		return 0;
	}
	
	flash_erase_operation_t erase_operation = flash_get_erase_operation();
	//TODO: Check with timeout!
	while(erase_operation == FLASH_ERASE_OPERATION) {
		erase_operation = flash_get_erase_operation();
	}
	if(erase_operation == FLASH_ERASE_ERROR) {
		uart_printf(&uart_instance, "Erasing error!\n\r");
		return 0;
	}
	uart_printf(&uart_instance, "Erasing success!\n\r");
	
	
	
//******************** Test write a word *************************	
	uint32_t write_word = 0x1234ABCD;	// Should be != 0xFFFFFFFF --> next test will assume this!
	
	ret = flash_store(0, &write_word, 1);
	uart_printf(&uart_instance, "Started storing to flash at word 0: 0x%X, Ret: %d\n\r", write_word, ret);
	if(ret != NRF_SUCCESS) {
		uart_printf(&uart_instance, "Start storing word failed!\n\r");
		return 0;
	}
	
	flash_store_operation_t store_operation = flash_get_store_operation();
	//TODO: Check with timeout!
	while(store_operation == FLASH_STORE_OPERATION) {
		store_operation = flash_get_store_operation();
	}
	if(store_operation == FLASH_STORE_ERROR) {
		uart_printf(&uart_instance, "Storing error!\n\r");
		return 0;
	}
	
	uint32_t read_word;
	// Check if the stored word is the correct word!
	flash_read(0, &read_word, 1);
	if(read_word != write_word) {
		uart_printf(&uart_instance, "Stored word is not the right word!\n\r");
		return 0;
	}
	
	
	uart_printf(&uart_instance, "Storing success!\n\r");

//******************** Test writing to the same position another value *************************
	write_word = 0xFFFFFFFF;	
	
	ret = flash_store(0, &write_word, 1);
	uart_printf(&uart_instance, "Started storing to flash at word 0: 0x%X, Ret: %d\n\r", write_word, ret);
	if(ret != NRF_SUCCESS) {
		uart_printf(&uart_instance, "Start storing word failed!\n\r");
		return 0;
	}
	
	store_operation = flash_get_store_operation();
	//TODO: Check with timeout!
	while(store_operation == FLASH_STORE_OPERATION) {
		store_operation = flash_get_store_operation();
	}
	
	// Although the value is not stored correctly, there is no error. So it is very important to erase the page first!
	
	
	
	// Check if the stored word is the correct word!
	flash_read(0, &read_word, 1);
	
#if FLASH_ENABLE_STORE_CHECK	
	if(store_operation == FLASH_STORE_ERROR) {
		uart_printf(&uart_instance, "Storing error as expected, because tried to write on the same position another word!\n\r");
	} else {
		if(read_word != write_word) {
			uart_printf(&uart_instance, "Store failed!  Written: 0x%X, Read: 0x%X\n\r", write_word, read_word);
			return 0;
		}
	}
#else
		
	if(store_operation == FLASH_STORE_ERROR) {
		uart_printf(&uart_instance, "Storing error!\n\r");
		return 0;
	}
	
	if(read_word == write_word) {
		uart_printf(&uart_instance, "Store word should actually fail!! Written: 0x%X, Read: 0x%X\n\r", write_word, read_word);
		return 0;
	}
#endif
	
	
	uart_printf(&uart_instance, "Store different word at same position behaves as expected!\n\r");
	
	
//******************** Test a overflowing write operation over 2 pages *************************#
	#define WORD_NUMBER	3100
	uint32_t write_address = (flash_get_page_size_words())-1;
	uint32_t write_words[WORD_NUMBER];
	ret = flash_store(write_address, write_words, WORD_NUMBER);
	uart_printf(&uart_instance, "Started storing words at address: %u, Ret: %d\n\r",write_address, ret);
	if(ret != NRF_SUCCESS) {
		uart_printf(&uart_instance, "Start storing words failed!\n\r");
		return 0;
	}
	
	store_operation = flash_get_store_operation();
	//TODO: Check with timeout!
	while(store_operation == FLASH_STORE_OPERATION) {
		store_operation = flash_get_store_operation();
	}
	if(store_operation == FLASH_STORE_ERROR) {
		uart_printf(&uart_instance, "Storing words error!\n\r");
		return 0;
	}
	// Check if the stored words are correct!
	uint32_t read_words[WORD_NUMBER];
	flash_read(write_address, read_words, WORD_NUMBER);
	//uart_printf(&uart_instance, "Written words:  0x%X, 0x%X, Read out words: 0x%X, 0x%X\n\r", write_words[0], write_words[1], read_words[0], read_words[1]);
	
	if(memcmp((uint8_t*)&write_words[0], (uint8_t*)&read_words[0], sizeof(uint32_t)*WORD_NUMBER) != 0) {
		uart_printf(&uart_instance, "Stored words are not the right words!\n\r");
		return 0;
	}
	
	uart_printf(&uart_instance, "Storing words success!\n\r");	
	
	uart_printf(&uart_instance, "Flash test successful!!\n\r");	
	
	
	return 1;
}