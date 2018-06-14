

// Don't forget gtest.h, which declares the testing framework.

#include "flash_lib.h"
#include "gtest/gtest.h"

#define FLASH_NUM_PAGES_TEST			30
#define FLASH_PAGE_SIZE_WORDS_TEST  	256

namespace {


// Test for init the module
TEST(FlashInitTest, ReturnValue) {
	ret_code_t ret = flash_init();
	ASSERT_EQ(ret, NRF_SUCCESS);
}


TEST(FlashInitTest, SizeCheck) {
	uint32_t num_pages = flash_get_page_number();
	ASSERT_EQ(num_pages, FLASH_NUM_PAGES_TEST);
	
	uint32_t page_size_words = flash_get_page_size_words();
	ASSERT_EQ(page_size_words, FLASH_PAGE_SIZE_WORDS_TEST);
}


TEST(FlashStoreTest, StartOfFlashTest) {
	char store_data[] = "Test data!!";
	int len = sizeof(store_data);
	ASSERT_EQ(len%4, 0);
	int word_num = len/sizeof(uint32_t);
	ret_code_t ret = flash_store(0, (uint32_t*) store_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	char read_data[len];
	ret = flash_read(0, (uint32_t*) read_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_STREQ(read_data, store_data);	
}


TEST(FlashStoreTest, EndOfFlashTest) {
	char store_data[] = "ABCDEFGHIJK";
	int len = sizeof(store_data);
	ASSERT_EQ(len%4, 0);
	int word_num = len/sizeof(uint32_t);
	int address = flash_get_page_number()*flash_get_page_size_words()-word_num;
	ret_code_t ret = flash_store(address, (uint32_t*) store_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	char read_data[len];
	ret = flash_read(address, (uint32_t*) read_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_STREQ(read_data, store_data);	
}



TEST(FlashEraseTest, StartOfFlash) {
	ret_code_t ret = flash_erase(0, 1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	char store_data[12];
	int len = sizeof(store_data);
	ASSERT_EQ(len%4, 0);
	memset(store_data, 0x01, len);
	int word_num = len/sizeof(uint32_t);
	int address = 0;	
	ret = flash_store(address, (uint32_t*) store_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	char read_data[len];
	ret = flash_read(address, (uint32_t*) read_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data, read_data, len)==0);	
	
	ret = flash_erase(0, 1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = flash_read(address, (uint32_t*) read_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	memset(store_data, 0xFF, len);
	EXPECT_TRUE(memcmp(store_data, read_data, len)==0);		
}


TEST(FlashEraseTest, EndOfFlash) {
	ret_code_t ret = flash_erase(flash_get_page_number()-1, 1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	char store_data[12];
	int len = sizeof(store_data);
	ASSERT_EQ(len%4, 0);
	memset(store_data, 0x11, len);
	int word_num = len/sizeof(uint32_t);
	int address = flash_get_page_number()*flash_get_page_size_words()-word_num;
	ret = flash_store(address, (uint32_t*) store_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	char read_data[len];
	ret = flash_read(address, (uint32_t*) read_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data, read_data, len)==0);	
	
	ret = flash_erase(flash_get_page_number()-1, 1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = flash_read(address, (uint32_t*) read_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	memset(store_data, 0xFF, len);
	EXPECT_TRUE(memcmp(store_data, read_data, len)==0);		
}


TEST(FlashStoreTest, WriteTwiceToStartOfFlashTest) {
	ret_code_t ret = flash_erase(0, 1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	uint32_t store_word = 0xDEADBEEF;
	int word_num = 1;
	int address = 0;
	ret = flash_store(address, (uint32_t*) &store_word, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	uint32_t read_word;
	ret = flash_read(address, (uint32_t*) &read_word, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(read_word, store_word);	
	
	
	store_word = 0xFFFFFFFF;
	ret = flash_store(address, (uint32_t*) &store_word, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = flash_read(address, (uint32_t*) &read_word, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_NE(read_word, store_word);	
}


TEST(FlashStoreTest, MultiPageStoreTest) {
	ret_code_t ret = flash_erase(1, 3);
	EXPECT_EQ(ret, NRF_SUCCESS);
	

	char store_data[flash_get_page_size_words()*sizeof(uint32_t)*2];
	int len = sizeof(store_data);
	ASSERT_EQ(len%4, 0);
	memset(store_data, 0x22, len);
	int word_num = len/sizeof(uint32_t);
	int address = flash_get_page_size_words() + 10;
	ret = flash_store(address, (uint32_t*) store_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	char read_data[len];
	ret = flash_read(address, (uint32_t*) read_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data, read_data, len)==0);	
	
}

TEST(FlashStoreTest, StoreAllOfFlashTest) {
	ret_code_t ret = flash_erase(0, flash_get_page_number());
	EXPECT_EQ(ret, NRF_SUCCESS);
	

	char store_data[flash_get_page_size_words()*flash_get_page_number()*sizeof(uint32_t)];
	int len = sizeof(store_data);
	ASSERT_EQ(len%4, 0);
	memset(store_data, 0x44, len);
	int word_num = len/sizeof(uint32_t);
	int address = 0;
	ret = flash_store(address, (uint32_t*) store_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	char read_data[len];
	ret = flash_read(address, (uint32_t*) read_data, word_num);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data, read_data, len)==0);	
	
}

TEST(FlashAddressTest, FalseAddressTest) {
	ret_code_t ret = flash_erase(0, flash_get_page_number());
	EXPECT_EQ(ret, NRF_SUCCESS);
	

	char store_data[12];
	int len = sizeof(store_data);
	ASSERT_EQ(len%4, 0);
	memset(store_data, 0x88, len);
	int word_num = len/sizeof(uint32_t);
	int address = flash_get_page_size_words()*flash_get_page_number()-word_num+1;
	ret = flash_store(address, (uint32_t*) store_data, word_num);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	char read_data[len];
	ret = flash_read(address, (uint32_t*) read_data, word_num);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
}


TEST(FlashOperationTest, NoOperationTest) {
	flash_operation_t flash_operation = flash_get_operation();
	EXPECT_EQ(flash_operation, FLASH_NO_OPERATION);
}




};
