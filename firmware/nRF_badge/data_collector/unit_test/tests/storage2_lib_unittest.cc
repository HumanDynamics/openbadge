

// Don't forget gtest.h, which declares the testing framework.

#include "storage2_lib.h"
#include "gtest/gtest.h"



#define STORAGE2_SIZE_TEST				(1024*256)






namespace {
	
	
class Storage2Test : public ::testing::Test {
	virtual void SetUp() {
		storage2_init();
    }
};	

TEST_F(Storage2Test, GetSizeTest) {
	uint32_t size = storage2_get_size();
	ASSERT_EQ(size, STORAGE2_SIZE_TEST);
}

TEST_F(Storage2Test, GetUnitSizeTest) {
	uint32_t unit_size = storage2_get_unit_size();
	ASSERT_EQ(unit_size, 1);
}

TEST_F(Storage2Test, StoreAndReadTest) {
	uint8_t store_data[98];
	uint32_t len = sizeof(store_data);
	uint8_t read_data[len];
	memset(read_data, 0xFF, len);
	
	ret_code_t ret;
	for(uint32_t i = 0; i < len; i++) {
		store_data[i] = i;
	}
	
	// One simple write
	ret = storage2_store(11, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	memset(read_data, 0xFF, len);
	ret = storage2_read(11, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	
	// Write to same address
	for(uint32_t i = 0; i < len; i++) {
		store_data[i] = 255 - (i % 256);
	}
	ret = storage2_store(11, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	memset(read_data, 0xFF, len);
	ret = storage2_read(11, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data, read_data, len) == 0);
	
	
	
	// Write 0 length
	ret = storage2_store(0, store_data, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	memset(read_data, 0xFF, len);
	ret = storage2_read(0, read_data, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	// Null pointer test
	ret = storage2_store(0, NULL, 1);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	ret = storage2_read(0, NULL, 1);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	
	// Write big data
	uint8_t store_data1[4096];
	len = sizeof(store_data1);
	uint8_t read_data1[len];
	memset(read_data1, 0xFF, len);
	for(uint32_t i = 0; i < len; i++) {
		store_data1[i] = i % 256;
	}
	
	ret = storage2_store(0, store_data1, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = storage2_read(0, read_data1, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_TRUE(memcmp(store_data1, read_data1, len) == 0);
	
	
	// Large address test
	ret = storage2_store(1, store_data1, storage2_get_size());
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	ret = storage2_read(1, read_data1, storage2_get_size());
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	
	
}


TEST_F(Storage2Test, ClearTest) {
	
	ret_code_t ret;
	
	uint8_t store_data[2000];
	uint32_t len = sizeof(store_data);
	uint8_t read_data[len];
	
	for(uint32_t i = 0; i < len; i++) {
		store_data[i] = i % 256;
	}
	ret = storage2_store(0, store_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = storage2_read(0, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_ARRAY_EQ(store_data, read_data, len);
	
	ret = storage2_clear(len/4, len/2);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = storage2_read(0, read_data, len);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	for(uint32_t i = 0; i < len; i++) {
		if(i < len/4)
			store_data[i] = i % 256;
		else if(i < len/2 + len/4)
			store_data[i] = 0xFF;
		else
			store_data[i] = i % 256;
	}
	EXPECT_ARRAY_EQ(store_data, read_data, len);	
}

};  
