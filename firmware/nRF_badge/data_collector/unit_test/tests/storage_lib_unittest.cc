

// Don't forget gtest.h, which declares the testing framework.

#include "storage_lib.h"
#include "gtest/gtest.h"


#define STORAGE1_SIZE_TEST				(30*256*sizeof(uint32_t))
#define STORAGE1_UNIT_SIZE_TEST			(256*sizeof(uint32_t))

#define STORAGE2_SIZE_TEST				(1024*256)
#define STORAGE2_UNIT_SIZE_TEST			(1)

#define STORAGE_SIZE_TEST				(STORAGE1_SIZE_TEST + STORAGE2_SIZE_TEST)



extern void storage_split_to_storage_modules(uint32_t address, uint8_t* data, uint32_t length_data, uint32_t splitted_address[], uint8_t* splitted_data[], uint32_t splitted_length_data[], uint32_t storage_sizes[], uint8_t number_of_storage_modules);





namespace {
	
	
class StorageTest : public ::testing::Test {
	virtual void SetUp() {
		storage_init();
    }
};	

TEST_F(StorageTest, GetSizeTest) {
	uint32_t size = storage_get_size();
	ASSERT_EQ(size, STORAGE_SIZE_TEST);
	
	
}

TEST_F(StorageTest, SplitToStorageModulesTest) {
	uint32_t splitted_address[3];
	uint8_t* splitted_data[3];
	uint32_t splitted_length_data[3];
	uint32_t storage_sizes[] = {10, 10, 10};
	
	
	// Only write in first storage-module
	uint32_t address = 0;
	uint8_t data[100];
	uint32_t length_data = 10;
	
	storage_split_to_storage_modules(address, data, length_data, splitted_address, splitted_data, splitted_length_data, storage_sizes, 3);
	EXPECT_EQ(splitted_address[0], 0);
	EXPECT_EQ(splitted_data[0], &data[0]);
	EXPECT_EQ(splitted_length_data[0], 10);
	
	EXPECT_EQ(splitted_address[1], 0);
	EXPECT_TRUE(splitted_data[1] == NULL);
	EXPECT_EQ(splitted_length_data[1], 0);
	
	EXPECT_EQ(splitted_address[2], 0);
	EXPECT_TRUE(splitted_data[2] == NULL);
	EXPECT_EQ(splitted_length_data[2], 0);
	
	
	// Write in first and second storage-module
	address = 5;
	length_data = 10;	
	storage_split_to_storage_modules(address, data, length_data, splitted_address, splitted_data, splitted_length_data, storage_sizes, 3);
	EXPECT_EQ(splitted_address[0], 5);
	EXPECT_EQ(splitted_data[0], &data[0]);
	EXPECT_EQ(splitted_length_data[0], 5);
	
	EXPECT_EQ(splitted_address[1], 0);
	EXPECT_EQ(splitted_data[1], &data[5]);
	EXPECT_EQ(splitted_length_data[1], 5);
	
	EXPECT_EQ(splitted_address[2], 0);
	EXPECT_TRUE(splitted_data[2] == NULL);
	EXPECT_EQ(splitted_length_data[2], 0);
	
	
	// Write in first, second and third storage-module
	address = 6;
	length_data = 20;	
	storage_split_to_storage_modules(address, data, length_data, splitted_address, splitted_data, splitted_length_data, storage_sizes, 3);
	EXPECT_EQ(splitted_address[0], 6);
	EXPECT_EQ(splitted_data[0], &data[0]);
	EXPECT_EQ(splitted_length_data[0], 4);
	
	EXPECT_EQ(splitted_address[1], 0);
	EXPECT_EQ(splitted_data[1], &data[4]);
	EXPECT_EQ(splitted_length_data[1], 10);
	
	EXPECT_EQ(splitted_address[2], 0);
	EXPECT_EQ(splitted_data[2], &data[14]);
	EXPECT_EQ(splitted_length_data[2], 6);
	
	
	// Write in second and third storage-module
	address = 10;
	length_data = 16;	
	storage_split_to_storage_modules(address, data, length_data, splitted_address, splitted_data, splitted_length_data, storage_sizes, 3);
	EXPECT_EQ(splitted_address[0], 0);
	EXPECT_TRUE(splitted_data[0] == NULL);
	EXPECT_EQ(splitted_length_data[0], 0);
	
	EXPECT_EQ(splitted_address[1], 0);
	EXPECT_EQ(splitted_data[1], &data[0]);
	EXPECT_EQ(splitted_length_data[1], 10);
	
	EXPECT_EQ(splitted_address[2], 0);
	EXPECT_EQ(splitted_data[2], &data[10]);
	EXPECT_EQ(splitted_length_data[2], 6);
	
	
	
	// Write 0 data
	address = 10;
	length_data = 0;	
	storage_split_to_storage_modules(address, data, length_data, splitted_address, splitted_data, splitted_length_data, storage_sizes, 3);
	EXPECT_EQ(splitted_address[0], 0);
	EXPECT_TRUE(splitted_data[0] == NULL);
	EXPECT_EQ(splitted_length_data[0], 0);
	
	EXPECT_EQ(splitted_address[1], 0);
	EXPECT_TRUE(splitted_data[1] == NULL);
	EXPECT_EQ(splitted_length_data[1], 0);
	
	EXPECT_EQ(splitted_address[2], 0);
	EXPECT_TRUE(splitted_data[2] == NULL);
	EXPECT_EQ(splitted_length_data[2], 0);
	
	
	// Write too large data
	address = 22;
	length_data = 10;
	storage_split_to_storage_modules(address, data, length_data, splitted_address, splitted_data, splitted_length_data, storage_sizes, 3);
	EXPECT_EQ(splitted_address[0], 0);
	EXPECT_TRUE(splitted_data[0] == NULL);
	EXPECT_EQ(splitted_length_data[0], 0);
	
	EXPECT_EQ(splitted_address[1], 0);
	EXPECT_TRUE(splitted_data[1] == NULL);
	EXPECT_EQ(splitted_length_data[1], 0);
	
	EXPECT_EQ(splitted_address[2], 2);
	EXPECT_EQ(splitted_data[2], &data[0]);
	EXPECT_EQ(splitted_length_data[2], 8);
	
}

TEST_F(StorageTest, GetUnitAddressLimits) {
	ret_code_t ret;
	uint32_t address = 0;
	uint32_t length_data = 10;
	uint32_t start_unit_address, end_unit_address;
	
	// Storage 1 (unit size: page size of flash)
	ret = storage_get_unit_address_limits(address, length_data, &start_unit_address, &end_unit_address);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(start_unit_address, 0);
	EXPECT_EQ(end_unit_address, STORAGE1_UNIT_SIZE_TEST - 1);
	
	address = 1;
	length_data = STORAGE1_UNIT_SIZE_TEST;
	ret = storage_get_unit_address_limits(address, length_data, &start_unit_address, &end_unit_address);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(start_unit_address, 0);
	EXPECT_EQ(end_unit_address, STORAGE1_UNIT_SIZE_TEST*2 - 1);
	
	// Storage 2 (unit size: byte)
	address = STORAGE1_SIZE_TEST;
	length_data = 100;
	ret = storage_get_unit_address_limits(address, length_data, &start_unit_address, &end_unit_address);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(start_unit_address, STORAGE1_SIZE_TEST);
	EXPECT_EQ(end_unit_address, STORAGE1_SIZE_TEST + 100 - 1);
	
	address = STORAGE1_SIZE_TEST + 101;
	length_data = 100;
	ret = storage_get_unit_address_limits(address, length_data, &start_unit_address, &end_unit_address);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(start_unit_address, STORAGE1_SIZE_TEST + 101);
	EXPECT_EQ(end_unit_address, STORAGE1_SIZE_TEST + 101 + 100 - 1);
	
	// Storage 1 and 2
	address = STORAGE1_SIZE_TEST - 23;
	length_data = 100;
	ret = storage_get_unit_address_limits(address, length_data, &start_unit_address, &end_unit_address);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(start_unit_address, STORAGE1_SIZE_TEST - STORAGE1_UNIT_SIZE_TEST);
	EXPECT_EQ(end_unit_address, STORAGE1_SIZE_TEST -23 + 100 - 1);
	
	ret = storage_get_unit_address_limits(0, STORAGE_SIZE_TEST, &start_unit_address, &end_unit_address);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(start_unit_address, 0);
	EXPECT_EQ(end_unit_address, STORAGE1_SIZE_TEST + STORAGE2_SIZE_TEST - 1);
	
	// Exceptions
	ret = storage_get_unit_address_limits(0, 0, &start_unit_address, &end_unit_address);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	ret = storage_get_unit_address_limits(0, STORAGE_SIZE_TEST + 1, &start_unit_address, &end_unit_address);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);	
}

TEST_F(StorageTest, StoreAndReadTest) {
	ret_code_t ret;
	uint8_t store_data[1000];
	uint32_t len = sizeof(store_data);
	uint8_t read_data[len];
	for(uint32_t i = 0; i < len; i++)
		store_data[i] = i % 256;
	memset(read_data, 0xFF, len);
	uint32_t address;


	// Store in storage 1
	address = 0;
	ret = storage_store(address, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = storage_read(address, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	
	
	// Store in storage 2
	address = STORAGE1_SIZE_TEST;
	ret = storage_store(address, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	memset(read_data, 0xFF, len);
	ret = storage_read(address, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	
	
	// Store in storage 1 and 2
	address = STORAGE1_SIZE_TEST - len/2;
	ret = storage_store(address, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	memset(read_data, 0xFF, len);
	ret = storage_read(address, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	
	// Writing small amount of data not word aligned
	address = STORAGE1_SIZE_TEST - 3;
	ret = storage_store(address, store_data, 2);
	EXPECT_EQ(ret, NRF_SUCCESS);
	memset(read_data, 0xFF, len);
	ret = storage_read(address, read_data, 2);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data, read_data, 2) == 0);
	
	// Writing small amount of data not word aligned
	address = STORAGE1_SIZE_TEST - 1;
	ret = storage_store(address, store_data, 3);
	EXPECT_EQ(ret, NRF_SUCCESS);
	memset(read_data, 0xFF, len);
	ret = storage_read(address, read_data, 3);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data, read_data, 3) == 0);
	
	
	// Expceptions
	// NULL Pointer
	address = 0;
	ret = storage_store(address, NULL, 3);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	ret = storage_read(address, NULL, 3);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	// Too large data
	address = 1;
	ret = storage_store(address, store_data, STORAGE_SIZE_TEST);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	ret = storage_read(address, read_data, STORAGE_SIZE_TEST);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	// Zero length data
	address = 1;
	ret = storage_store(address, store_data, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = storage_read(address, read_data, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);	
}

};  
