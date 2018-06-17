

// Don't forget gtest.h, which declares the testing framework.

#include "storage1_lib.h"
#include "gtest/gtest.h"

#define FLASH_NUM_PAGES_TEST			30
#define FLASH_PAGE_SIZE_WORDS_TEST  	256

#define STORAGE1_SIZE_TEST				(FLASH_NUM_PAGES_TEST*FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t))


// Some external functions and variables
extern uint32_t 	storage1_get_page_number(uint32_t byte_address, uint32_t byte_length);
extern uint32_t 	storage1_get_page_address(uint32_t byte_address);
extern uint32_t 	storage1_get_last_stored_element_addresses_size(void);
extern ret_code_t 	storage1_compute_pages_to_erase(uint32_t address, uint32_t length_data, uint32_t* erase_start_page_address, uint32_t* erase_num_pages);
extern int32_t 		storage1_last_stored_element_addresses[];




namespace {
	
	
class Storage1Test : public ::testing::Test {
	virtual void SetUp() {
		storage1_init();
    }
};	

TEST_F(Storage1Test, SizeCheck) {
	uint32_t size = storage1_get_size();
	ASSERT_EQ(size, STORAGE1_SIZE_TEST);
}

TEST_F(Storage1Test, GetPageAddressTest) {
	uint32_t page_address = storage1_get_page_address(0);
	EXPECT_EQ(page_address, 0);
	
	page_address = storage1_get_page_address(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)-1);
	EXPECT_EQ(page_address, 0);

	page_address = storage1_get_page_address(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2);
	EXPECT_EQ(page_address, 2);
	
	page_address = storage1_get_page_address(STORAGE1_SIZE_TEST-FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t));
	EXPECT_EQ(page_address, FLASH_NUM_PAGES_TEST-1);
	
	page_address = storage1_get_page_address(STORAGE1_SIZE_TEST-1);
	EXPECT_EQ(page_address, FLASH_NUM_PAGES_TEST-1);
	
	page_address = storage1_get_page_address(STORAGE1_SIZE_TEST);
	EXPECT_EQ(page_address, 0);
	
}


TEST_F(Storage1Test, GetPageNumberTest) {
	uint32_t num_pages = storage1_get_page_number(0, 0);
	EXPECT_EQ(num_pages, 0);
	
	num_pages = storage1_get_page_number(0, 1);
	EXPECT_EQ(num_pages, 1);
	
	num_pages = storage1_get_page_number(1, FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t));
	EXPECT_EQ(num_pages, 2);
	
	num_pages = storage1_get_page_number(2, FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2-2);
	EXPECT_EQ(num_pages, 2);
	
	num_pages = storage1_get_page_number(3, FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2-2);
	EXPECT_EQ(num_pages, 3);
	
	num_pages = storage1_get_page_number(0, STORAGE1_SIZE_TEST-FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t));
	EXPECT_EQ(num_pages, FLASH_NUM_PAGES_TEST-1);
		
	num_pages = storage1_get_page_number(0, STORAGE1_SIZE_TEST);
	EXPECT_EQ(num_pages, FLASH_NUM_PAGES_TEST);
	
	num_pages = storage1_get_page_number(1, STORAGE1_SIZE_TEST);
	EXPECT_EQ(num_pages, 0);
}


TEST_F(Storage1Test, ComputePagesToEraseTest) {
	uint32_t erase_start_page_address, erase_num_pages;
	ASSERT_EQ(storage1_get_last_stored_element_addresses_size(), 4);
	
	ret_code_t ret;
	
	
	
	// Error value tests:
	// Zero length
	ret = storage1_compute_pages_to_erase(0, 0, &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	EXPECT_EQ(erase_start_page_address, 0);
	EXPECT_EQ(erase_num_pages, 0);
	
	// Data length to large
	ret = storage1_compute_pages_to_erase(1, storage1_get_size(), &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	EXPECT_EQ(erase_start_page_address, 0);
	EXPECT_EQ(erase_num_pages, 0);
	
	
	
	
	// Fill the internal state with some data
	ret	= storage1_compute_pages_to_erase(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*5, FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*3, &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(erase_start_page_address, 5);
	EXPECT_EQ(erase_num_pages, 3);
	ret = storage1_compute_pages_to_erase(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*4, 30, &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(erase_start_page_address, 4);
	EXPECT_EQ(erase_num_pages, 1);
	ret = storage1_compute_pages_to_erase(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2, 20, &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(erase_start_page_address, 2);
	EXPECT_EQ(erase_num_pages, 1);
	ret = storage1_compute_pages_to_erase(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1, 10, &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(erase_start_page_address, 1);
	EXPECT_EQ(erase_num_pages, 1);
	EXPECT_EQ(storage1_last_stored_element_addresses[0], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*5 + FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*3 - 1);
	EXPECT_EQ(storage1_last_stored_element_addresses[1], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*4 + 29);
	EXPECT_EQ(storage1_last_stored_element_addresses[2], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2 + 19);
	EXPECT_EQ(storage1_last_stored_element_addresses[3], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1 + 9);
	
	
	
	// Now write to page 0 --> it should replace storage1_last_stored_element_addresses at index 0 (because could not find minimum, and no entry is -1)
	ret	= storage1_compute_pages_to_erase(0, 10, &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(erase_start_page_address, 0);
	EXPECT_EQ(erase_num_pages, 1);
	EXPECT_EQ(storage1_last_stored_element_addresses[0], 9);
	EXPECT_EQ(storage1_last_stored_element_addresses[1], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*4 + 29);
	EXPECT_EQ(storage1_last_stored_element_addresses[2], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2 + 19);
	EXPECT_EQ(storage1_last_stored_element_addresses[3], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1 + 9);
	
	// Now write again to page 0 with higher address (and full page write) --> it should replace storage1_last_stored_element_addresses at index 0, and should erase no pages
	ret	= storage1_compute_pages_to_erase(20, FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1 -20, &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(erase_start_page_address, 1);
	EXPECT_EQ(erase_num_pages, 0);
	EXPECT_EQ(storage1_last_stored_element_addresses[0], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1 - 1);
	EXPECT_EQ(storage1_last_stored_element_addresses[1], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*4 + 29);
	EXPECT_EQ(storage1_last_stored_element_addresses[2], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2 + 19);
	EXPECT_EQ(storage1_last_stored_element_addresses[3], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1 + 9);
	
	// Now write to page 1 --> it should replace storage1_last_stored_element_addresses at index 0 (because == address-1) and should set storage1_last_stored_element_addresses[3] to -1
	ret	= storage1_compute_pages_to_erase(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1, 10, &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(erase_start_page_address, 1);
	EXPECT_EQ(erase_num_pages, 1);
	EXPECT_EQ(storage1_last_stored_element_addresses[0], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1 + 9);
	EXPECT_EQ(storage1_last_stored_element_addresses[1], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*4 + 29);
	EXPECT_EQ(storage1_last_stored_element_addresses[2], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2 + 19);
	EXPECT_EQ(storage1_last_stored_element_addresses[3], -1);
	
	
	// Now write again to page 0 --> it should replace the storage1_last_stored_element_addresses[3] (because it is -1)
	ret	= storage1_compute_pages_to_erase(10, 10, &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(erase_start_page_address, 0);
	EXPECT_EQ(erase_num_pages, 1);
	EXPECT_EQ(storage1_last_stored_element_addresses[0], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1 + 9);
	EXPECT_EQ(storage1_last_stored_element_addresses[1], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*4 + 29);
	EXPECT_EQ(storage1_last_stored_element_addresses[2], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2 + 19);
	EXPECT_EQ(storage1_last_stored_element_addresses[3], 19);
	
	
	// Now write to page 3 --> it should replace storage1_last_stored_element_addresses at index 2 (because of min difference)
	ret	= storage1_compute_pages_to_erase(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*3, 10, &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(erase_start_page_address, 3);
	EXPECT_EQ(erase_num_pages, 1);
	EXPECT_EQ(storage1_last_stored_element_addresses[0], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1 + 9);
	EXPECT_EQ(storage1_last_stored_element_addresses[1], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*4 + 29);
	EXPECT_EQ(storage1_last_stored_element_addresses[2], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*3 + 9);
	EXPECT_EQ(storage1_last_stored_element_addresses[3], 19);
	
	// Now write again to page 0 with large data --> it should set [0] and [3] to -1, erase page 1 (not 0 because larger than former address in page 0) and should insert to [0]
	ret	= storage1_compute_pages_to_erase(20, FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1, &erase_start_page_address, &erase_num_pages);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(erase_start_page_address, 1);
	EXPECT_EQ(erase_num_pages, 1);
	EXPECT_EQ(storage1_last_stored_element_addresses[0], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*1 + 19);
	EXPECT_EQ(storage1_last_stored_element_addresses[1], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*4 + 29);
	EXPECT_EQ(storage1_last_stored_element_addresses[2], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*3 + 9);
	EXPECT_EQ(storage1_last_stored_element_addresses[3], -1);
	
}







};  
