

// Don't forget gtest.h, which declares the testing framework.

#include "eeprom_lib.h"
#include "gtest/gtest.h"

#define EEPROM_SIZE_TEST	(256*1024)

namespace {


// Test for init the module
TEST(EEPROMInitTest, ReturnValue) {
	ret_code_t ret = eeprom_init();
	ASSERT_EQ(ret, NRF_SUCCESS);
}

TEST(EEPROMInitTest, SizeCheck) {
	uint32_t size = eeprom_get_size();
	ASSERT_EQ(size, EEPROM_SIZE_TEST);
}

TEST(EEPROMStoreTest, StartOfEEPROMTest) {
	char store_data[] = "Test data!";
	int len = sizeof(store_data);
	ret_code_t ret = eeprom_store(0, (uint8_t*) store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	char read_data[len];
	ret = eeprom_read(0, (uint8_t*) read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_STREQ(read_data, store_data);	
}




TEST(EEPROMStoreTest, EndOfEEPROMTest) {
	char store_data[] = "Test data!";
	int len = sizeof(store_data);
	ret_code_t ret = eeprom_store(EEPROM_SIZE_TEST-len, (uint8_t*) store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	char read_data[len];
	ret = eeprom_read(EEPROM_SIZE_TEST-len, (uint8_t*) read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_STREQ(read_data, store_data);	
}

TEST(EEPROMStoreTest, StoreAllOfEEPROMTest) {
	char store_data[EEPROM_SIZE_TEST];
	int len = sizeof(store_data);
	memset((uint8_t*) store_data, 0xAB, len);
	
	ret_code_t ret = eeprom_store(0, (uint8_t*) store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	char read_data[len];
	ret = eeprom_read(0, (uint8_t*) read_data, len);
	
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(read_data, store_data, len) == 0);	
}

TEST(EEPROMAddressTest, FalseAddressTest) {
	char store_data[20];
	int len = sizeof(store_data);
	memset((uint8_t*) store_data, 0xDE, len);
	
	ret_code_t ret = eeprom_store(EEPROM_SIZE_TEST-len+1, (uint8_t*) store_data, len);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	char read_data[len];
	ret = eeprom_read(EEPROM_SIZE_TEST-len+1, (uint8_t*) read_data, len);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
}

TEST(EEPROMNullPointerTest, ReturnValueTest) {


	ret_code_t ret = eeprom_store(0, NULL, 1);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	ret = eeprom_read(0, NULL, 1);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
}


};  
