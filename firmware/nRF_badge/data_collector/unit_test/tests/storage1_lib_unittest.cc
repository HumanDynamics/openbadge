

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
extern void 		storage1_compute_word_aligned_addresses(uint32_t address, uint32_t length_data, uint32_t* leading_num_bytes, uint32_t* intermediate_num_bytes, uint32_t* final_num_bytes);
extern ret_code_t 	storage1_store_uint8_as_uint32(uint32_t address, uint8_t* data, uint32_t length_data);
extern ret_code_t 	storage1_read_uint32_as_uint8(uint32_t address, uint8_t* data, uint32_t length_data);





namespace {
	
	
class Storage1Test : public ::testing::Test {
	virtual void SetUp() {
		storage1_init();
    }
};	

TEST_F(Storage1Test, GetSizeTest) {
	uint32_t size = storage1_get_size();
	ASSERT_EQ(size, STORAGE1_SIZE_TEST);
}

TEST_F(Storage1Test, GetUnitSizeTest) {
	uint32_t unit_size = storage1_get_unit_size();
	ASSERT_EQ(unit_size, FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t));
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
	EXPECT_EQ(ret, NRF_SUCCESS);
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



TEST_F(Storage1Test, ComputeWordAlignedAdressesTest) {
	
	// Some general Tests
	uint32_t address = 1, length_data = 7;
	uint32_t leading_num_bytes, intermediate_num_bytes, final_num_bytes;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 3);
	EXPECT_EQ(intermediate_num_bytes, 4);
	EXPECT_EQ(final_num_bytes, 0);
		
	address = 4;
	length_data = 7;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 0);
	EXPECT_EQ(intermediate_num_bytes, 4);
	EXPECT_EQ(final_num_bytes, 3);
	
	address = 0;
	length_data = 8;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 0);
	EXPECT_EQ(intermediate_num_bytes, 8);
	EXPECT_EQ(final_num_bytes, 0);
	
	address = 17;
	length_data = 8;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 3);
	EXPECT_EQ(intermediate_num_bytes, 4);
	EXPECT_EQ(final_num_bytes, 1);
	
	address = 19;
	length_data = 15;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 1);
	EXPECT_EQ(intermediate_num_bytes, 12);
	EXPECT_EQ(final_num_bytes, 2);
	
	
	// Edge cases with small data size
	address = 7;
	length_data = 1;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 1);
	EXPECT_EQ(intermediate_num_bytes, 0);
	EXPECT_EQ(final_num_bytes, 0);
	
	
	address = 5;
	length_data = 1;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 0);
	EXPECT_EQ(intermediate_num_bytes, 1);
	EXPECT_EQ(final_num_bytes, 0);
	
	address = 5;
	length_data = 2;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 0);
	EXPECT_EQ(intermediate_num_bytes, 2);
	EXPECT_EQ(final_num_bytes, 0);
	
	
	address = 4;
	length_data = 3;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 0);
	EXPECT_EQ(intermediate_num_bytes, 0);
	EXPECT_EQ(final_num_bytes, 3);
	
	address = 4;
	length_data = 4;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 0);
	EXPECT_EQ(intermediate_num_bytes, 4);
	EXPECT_EQ(final_num_bytes, 0);
	
	address = 8;
	length_data = 1;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 0);
	EXPECT_EQ(intermediate_num_bytes, 0);
	EXPECT_EQ(final_num_bytes, 1);
	
	
	address = 9;
	length_data = 0;
	storage1_compute_word_aligned_addresses(address, length_data, &leading_num_bytes, &intermediate_num_bytes, &final_num_bytes);
	EXPECT_EQ(leading_num_bytes, 0);
	EXPECT_EQ(intermediate_num_bytes, 0);
	EXPECT_EQ(final_num_bytes, 0);
	
	
	
}

TEST_F(Storage1Test, StoreAndReadUint8Uint32Test) {
	uint8_t store_data[98];
	uint32_t len = sizeof(store_data);
	uint8_t read_data[len];
	ret_code_t ret;
	for(uint32_t i = 0; i < len; i++) {
		store_data[i] = i;
	}
	
	ret = storage1_store_uint8_as_uint32(0, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	
	ret = storage1_read_uint32_as_uint8(0, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	
	// Write again to next address
	ret = storage1_store_uint8_as_uint32(len, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = storage1_read_uint32_as_uint8(len, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	
	// Check if the former written bytes are still correct
	ret = storage1_read_uint32_as_uint8(0, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	
	// Check when addresses are not aligned
	ret = storage1_store_uint8_as_uint32(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)+1, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	uint8_t read_data1[len + 2];
	
	ret = storage1_read_uint32_as_uint8(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t), read_data1, len+2);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	
	EXPECT_EQ(read_data1[0], 0xFF);
	EXPECT_TRUE(memcmp(store_data, &read_data1[1], len) == 0);
	EXPECT_EQ(read_data1[len+1], 0xFF);
	
	
	
	// Write small data
	ret = storage1_store_uint8_as_uint32(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2+1, store_data, 1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = storage1_read_uint32_as_uint8(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2+1, read_data, 1);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_TRUE(memcmp(store_data, read_data, 1) == 0);
	
	ret = storage1_store_uint8_as_uint32(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2+6, store_data, 3);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = storage1_read_uint32_as_uint8(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2+6, read_data, 3);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_TRUE(memcmp(store_data, read_data, 3) == 0);
	
	// Write 0 data
	ret = storage1_store_uint8_as_uint32(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2+22, store_data, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = storage1_read_uint32_as_uint8(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*2+22, read_data, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	
	// Write to large address
	ret = storage1_store_uint8_as_uint32(storage1_get_size()-10, store_data, 11);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	ret = storage1_read_uint32_as_uint8(storage1_get_size()-10, read_data, 11);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);	
	
	
	
	
}


TEST_F(Storage1Test, StoreAndReadTest) {
	
	ASSERT_EQ(storage1_get_last_stored_element_addresses_size(), 4);
	
	uint8_t store_data[98];
	uint32_t len = sizeof(store_data);
	uint8_t read_data[len];
	
	ret_code_t ret;
	for(uint32_t i = 0; i < len; i++) {
		store_data[i] = i;
	}
	
	// Erase page 0 and store data
	ret = storage1_store(0, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(storage1_last_stored_element_addresses[0], len-1);
	EXPECT_EQ(storage1_last_stored_element_addresses[1], -1);
	EXPECT_EQ(storage1_last_stored_element_addresses[2], -1);
	EXPECT_EQ(storage1_last_stored_element_addresses[3], -1);	
	ret = storage1_read(0, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	
	
	// Write again to page 0 directly behind the former data (page should not be erased)
	ret = storage1_store(len, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(storage1_last_stored_element_addresses[0], len*2-1);
	EXPECT_EQ(storage1_last_stored_element_addresses[1], -1);
	EXPECT_EQ(storage1_last_stored_element_addresses[2], -1);
	EXPECT_EQ(storage1_last_stored_element_addresses[3], -1);
	ret = storage1_read(len, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	// Check if the former data are still there
	ret = storage1_read(0, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	
	
	
	// Write again to page 0 to the same address like before (page should be erased, and the former data should be restored)
	ret = storage1_store(len, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(storage1_last_stored_element_addresses[0], len*2-1);
	EXPECT_EQ(storage1_last_stored_element_addresses[1], -1);
	EXPECT_EQ(storage1_last_stored_element_addresses[2], -1);
	EXPECT_EQ(storage1_last_stored_element_addresses[3], -1);
	ret = storage1_read(len, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);	
	// Check if the former data are still there
	ret = storage1_read(0, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	
	
	// Initialize large data set
	uint8_t store_data1[FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t)*5];
	uint32_t len1 = sizeof(store_data1);
	uint8_t read_data1[len1];
	for(uint32_t i = 0; i < len1; i++) {
		store_data1[i] = (uint8_t) (i % 256);
	}
	
	
	// Write to multiple pages (should erase the pages, and restore data on the first page that was erased)
	ret = storage1_store(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t) + 1, store_data1, len1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(storage1_last_stored_element_addresses[0], len*2-1);
	EXPECT_EQ(storage1_last_stored_element_addresses[1], FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t) + 1 + len1 - 1);
	EXPECT_EQ(storage1_last_stored_element_addresses[2], -1);
	EXPECT_EQ(storage1_last_stored_element_addresses[3], -1);
	ret = storage1_read(FLASH_PAGE_SIZE_WORDS_TEST*sizeof(uint32_t) + 1, read_data1, len1);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_TRUE(memcmp(store_data1, read_data1, len1) == 0);
	
	
	// Finally test some exception parameters
	ret = storage1_store(0, NULL, 1);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	ret = storage1_store(0, store_data1, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = storage1_store(1, store_data1, storage1_get_size());
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	ret = storage1_read(0, NULL, 1);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	ret = storage1_read(0, read_data1, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = storage1_read(1, read_data1, storage1_get_size());
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	
	
	
	
}

};  
