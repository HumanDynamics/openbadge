

// Don't forget gtest.h, which declares the testing framework.
#include "gtest/gtest.h"

#include "filesystem_lib.h"
#include "storage_lib.h"
#include "storage1_lib.h"
#include "storage2_lib.h"



#define STORAGE1_SIZE_TEST				(30*256*sizeof(uint32_t))
#define STORAGE1_UNIT_SIZE_TEST			(256*sizeof(uint32_t))

#define STORAGE2_SIZE_TEST				(1024*256)
#define STORAGE2_UNIT_SIZE_TEST			(1)


#define MAX_NUMBER_OF_PARTITIONS_TEST									20
#define	PARTITION_METADATA_SIZE_TEST									14
#define PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE_TEST					2
#define PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE_TEST		2
#define PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE_TEST					2



extern ret_code_t filesystem_compute_available_size(uint32_t partition_start_address, uint32_t required_size, uint32_t* available_size);

extern uint32_t filesystem_get_swap_page_address_of_partition(uint16_t partition_id);



extern uint32_t filesystem_compute_next_element_address(uint16_t partition_id, uint32_t cur_element_address, uint16_t cur_element_len, uint16_t next_element_len);

extern uint16_t filesystem_get_element_header_len(uint16_t partition_id);



extern partition_iterator_t	partition_iterators[];
extern partition_t partitions[];
extern uint32_t next_free_address;



extern void eeprom_write_to_file(const char* filename);
extern void flash_write_to_file(const char* filename);



namespace {

class FilesystemTest : public ::testing::Test {
	virtual void SetUp() {
		filesystem_init();
    }
};


TEST_F(FilesystemTest, InitTest) {
	ret_code_t ret = filesystem_init();
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	ASSERT_EQ(PARTITION_METADATA_SIZE_TEST, PARTITION_METADATA_SIZE);
	
	ASSERT_EQ(PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE_TEST, PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE);
	ASSERT_EQ(PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE_TEST, PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE);
	ASSERT_EQ(PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE_TEST, PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE);		
	
	
	ASSERT_EQ(MAX_NUMBER_OF_PARTITIONS_TEST, MAX_NUMBER_OF_PARTITIONS);
	
	ASSERT_EQ(STORAGE1_SIZE_TEST, storage1_get_size());
	ASSERT_EQ(STORAGE1_UNIT_SIZE_TEST, storage1_get_unit_size());
	ASSERT_EQ(STORAGE2_SIZE_TEST, storage2_get_size());
	ASSERT_EQ(STORAGE2_UNIT_SIZE_TEST, storage2_get_unit_size());
}

TEST_F(FilesystemTest, NextFreeAdressTest) {
	EXPECT_EQ(SWAP_PAGE_SIZE, (MAX_NUMBER_OF_PARTITIONS_TEST*(PARTITION_METADATA_SIZE_TEST + PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE_TEST + PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE_TEST + PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE_TEST))); 
	
	// The next free address should be after the Swap page
	EXPECT_EQ(next_free_address, 1024);
}

TEST_F(FilesystemTest, ComputeAvailableSizeTest) {
	uint32_t partition_start_address = 0;
	uint32_t required_size = 100;
	uint32_t available_size = 0;
	ret_code_t ret;
	
	// Storage 1 start
	ret = filesystem_compute_available_size(partition_start_address, required_size, &available_size);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(available_size, STORAGE1_UNIT_SIZE_TEST);
	
	// Storage 1 end
	partition_start_address = STORAGE1_SIZE_TEST - STORAGE1_UNIT_SIZE_TEST;
	required_size = 100;	
	ret = filesystem_compute_available_size(partition_start_address, required_size, &available_size);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(available_size, STORAGE1_UNIT_SIZE_TEST);

	// Storage 2 start
	partition_start_address = STORAGE1_SIZE_TEST;
	required_size = 100;
	ret = filesystem_compute_available_size(partition_start_address, required_size, &available_size);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(available_size, 100);
	
	// Storage 2 end
	partition_start_address = STORAGE1_SIZE_TEST + STORAGE2_SIZE_TEST - 100;
	required_size = 100;	
	ret = filesystem_compute_available_size(partition_start_address, required_size, &available_size);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(available_size, 100);

	// Storage 1 and 2
	partition_start_address = STORAGE1_SIZE_TEST - STORAGE1_UNIT_SIZE_TEST;
	required_size = STORAGE1_UNIT_SIZE_TEST + 100;	
	ret = filesystem_compute_available_size(partition_start_address, required_size, &available_size);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(available_size, STORAGE1_UNIT_SIZE_TEST + 100);
	
	// Exceptions
	// Too large data --> clip to available data
	partition_start_address = STORAGE1_SIZE_TEST + STORAGE2_SIZE_TEST - 3;
	required_size = STORAGE1_UNIT_SIZE_TEST + 100;	
	ret = filesystem_compute_available_size(partition_start_address, required_size, &available_size);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(available_size, 3);
	
	
	// Storage 1 and 2 false start address (not unit-aligned)
	partition_start_address = STORAGE1_SIZE_TEST - 3;
	required_size = 100;	
	ret = filesystem_compute_available_size(partition_start_address, required_size, &available_size);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	// Too large address
	partition_start_address = STORAGE1_SIZE_TEST + STORAGE2_SIZE_TEST;
	required_size = 100;	
	ret = filesystem_compute_available_size(partition_start_address, required_size, &available_size);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	// Zero length required size
	partition_start_address = 0;
	required_size = 0;
	ret = filesystem_compute_available_size(partition_start_address, required_size, &available_size);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
}


TEST_F(FilesystemTest, GetSwapPageAddressTest) {
	uint32_t header_len = (PARTITION_METADATA_SIZE_TEST + PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE_TEST + PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE_TEST + PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE_TEST);
	
	// Static partition
	uint32_t swap_page_address = filesystem_get_swap_page_address_of_partition(0);
	EXPECT_EQ(swap_page_address, 0);
	
	// Static partition with crc
	swap_page_address = filesystem_get_swap_page_address_of_partition(0x4001);
	EXPECT_EQ(swap_page_address, 1*header_len);
	
	// Dynamic partition
	swap_page_address = filesystem_get_swap_page_address_of_partition(0x8000);
	EXPECT_EQ(swap_page_address, 0);
	
	// Dynamic partition with crc
	swap_page_address = filesystem_get_swap_page_address_of_partition(0xC005);
	EXPECT_EQ(swap_page_address, 5*header_len);	
}


TEST_F(FilesystemTest, GetHeaderLenTest) {
	
	
	uint16_t partition_id = 0x8000;
	uint16_t header_len = filesystem_get_element_header_len(partition_id);
	EXPECT_EQ(header_len, PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE_TEST + PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE_TEST);
	
	partition_id = 0xC000;
	header_len = filesystem_get_element_header_len(partition_id);
	EXPECT_EQ(header_len, PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE_TEST + PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE_TEST + PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE_TEST);
	
	partition_id = 0x0001;
	header_len = filesystem_get_element_header_len(partition_id);
	EXPECT_EQ(header_len, PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE_TEST);
	
}


TEST_F(FilesystemTest, RegisterPartitionTest) {
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = 1000;
	
	// Registering a dynamic partition with crc
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(partition_id, 0x8000 | 0x4000);
	EXPECT_EQ(required_size, 1024);	
	EXPECT_EQ(next_free_address, 1024 + 1024);	
	
	EXPECT_EQ(partitions[0].first_element_address, 1024);
	EXPECT_EQ(partitions[0].has_first_element, 0);
	EXPECT_EQ(partitions[0].latest_element_address, 1024);
	EXPECT_EQ(partitions[0].latest_element_record_id, 1);
	EXPECT_EQ(partitions[0].latest_element_len, 0);
	
	EXPECT_EQ(partitions[0].metadata.partition_id, 0x8000 | 0x4000);
	EXPECT_EQ(partitions[0].metadata.partition_size, 1024);
	EXPECT_EQ(partitions[0].metadata.first_element_len, 0);
	EXPECT_EQ(partitions[0].metadata.last_element_address, 1024);
	
	
	
	// Registering a static partition with crc
	ret = filesystem_register_partition(&partition_id, &required_size, 0, 1, 100);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(partition_id, 0x4001);
	EXPECT_EQ(required_size, 1024);	
	EXPECT_EQ(next_free_address, 1024 + 1024*2);	
	
	EXPECT_EQ(partitions[1].first_element_address, 1024*2);
	EXPECT_EQ(partitions[1].has_first_element, 0);
	EXPECT_EQ(partitions[1].latest_element_address, 1024*2);
	EXPECT_EQ(partitions[1].latest_element_record_id, 1);
	EXPECT_EQ(partitions[1].latest_element_len, 0);
	
	EXPECT_EQ(partitions[1].metadata.partition_id, 0x4001);
	EXPECT_EQ(partitions[1].metadata.partition_size, 1024);
	EXPECT_EQ(partitions[1].metadata.first_element_len, 100);
	EXPECT_EQ(partitions[1].metadata.last_element_address, 1024*2);
	
	
	
	// Registering a static partition without crc
	ret = filesystem_register_partition(&partition_id, &required_size, 0, 0, 200);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(partition_id, 0x2);
	EXPECT_EQ(required_size, 1024);	
	EXPECT_EQ(next_free_address, 1024 + 1024*3);	
	
	EXPECT_EQ(partitions[2].first_element_address, 1024*3);
	EXPECT_EQ(partitions[2].has_first_element, 0);
	EXPECT_EQ(partitions[2].latest_element_address, 1024*3);
	EXPECT_EQ(partitions[2].latest_element_record_id, 1);
	EXPECT_EQ(partitions[2].latest_element_len, 0);
	
	EXPECT_EQ(partitions[2].metadata.partition_id, 0x2);
	EXPECT_EQ(partitions[2].metadata.partition_size, 1024);
	EXPECT_EQ(partitions[2].metadata.first_element_len, 200);
	EXPECT_EQ(partitions[2].metadata.last_element_address, 1024*3);
	
	
	
	// Registering a dynamic partition without crc
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 0, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(partition_id, 0x8003);
	EXPECT_EQ(required_size, 1024);	
	EXPECT_EQ(next_free_address, 1024 + 1024*4);	
	
	EXPECT_EQ(partitions[3].first_element_address, 1024*4);
	EXPECT_EQ(partitions[3].has_first_element, 0);
	EXPECT_EQ(partitions[3].latest_element_address, 1024*4);
	EXPECT_EQ(partitions[3].latest_element_record_id, 1);
	EXPECT_EQ(partitions[3].latest_element_len, 0);
	
	EXPECT_EQ(partitions[3].metadata.partition_id, 0x8003);
	EXPECT_EQ(partitions[3].metadata.partition_size, 1024);
	EXPECT_EQ(partitions[3].metadata.first_element_len, 0);
	EXPECT_EQ(partitions[3].metadata.last_element_address, 1024*4);
}


TEST_F(FilesystemTest, StoreElementTest) {
	

	uint8_t data[1000];
	for(uint16_t i =0; i < 1000; i++)
		data[i] = i % 256;
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = 1000;
	
	// Registering a dynamic partition with crc
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	
	
	ret = filesystem_store_element(partition_id, data, 105);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	
	EXPECT_EQ(partitions[0].first_element_address, 1024);
	EXPECT_EQ(partitions[0].has_first_element, 1);
	EXPECT_EQ(partitions[0].latest_element_address, 1024);
	EXPECT_EQ(partitions[0].latest_element_record_id, 1);
	EXPECT_EQ(partitions[0].latest_element_len, 105);
	
	EXPECT_EQ(partitions[0].metadata.partition_id, 0x8000 | 0x4000);
	EXPECT_EQ(partitions[0].metadata.partition_size, 1024);
	EXPECT_EQ(partitions[0].metadata.first_element_len, 105);
	EXPECT_EQ(partitions[0].metadata.last_element_address, 1024);
		
	
	// Read the raw data from storage
	uint8_t read_data[sizeof(data)];
	ret = storage_read(1024, read_data, sizeof(read_data));
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	uint8_t tmp[PARTITION_METADATA_SIZE_TEST + filesystem_get_element_header_len(partition_id)];
	tmp[2] = 0x80 | 0x40; 	// partition_id msb
	tmp[3] = 0x00; 			// partition_id lsb
	tmp[4] = 0x00;			// partition_size msb
	tmp[5] = 0x00;			// partition_size 
	tmp[6] = 0x04;			// partition_size 
	tmp[7] = 0x00;			// partition_size lsb
	tmp[8] = 0x00; 			// first_element_len msb
	tmp[9] = 0x69; 			// first_element_len lsb
	tmp[10] = 0x00; 		// last_element_address msb
	tmp[11] = 0x00; 		// last_element_address
	tmp[12] = 0x04; 		// last_element_address
	tmp[13] = 0x00; 		// last_element_address lsb
	tmp[14] = 0x00; 		// record_id msb
	tmp[15] = 0x01; 		// record_id lsb
	tmp[16] = 0x00; 		// previous_len_XOR_cur_len msb
	tmp[17] = 0x00; 		// previous_len_XOR_cur_len lsb


	// Check if the metadata and element header is correct (except header-crc and element-crc)
	EXPECT_TRUE(memcmp(&tmp[2], &read_data[2], 16) == 0);	
	
	// Check if the data are in storage after the metadata and element header
	EXPECT_TRUE(memcmp(data, &read_data[PARTITION_METADATA_SIZE_TEST + filesystem_get_element_header_len(partition_id)], 105) == 0);
	
	
	
	// Registering a static partition with crc
	ret = filesystem_register_partition(&partition_id, &required_size, 0, 1, 100);
	ASSERT_EQ(ret, NRF_SUCCESS); 
	
	
	ret = filesystem_store_element(partition_id, data, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = storage_read(1024 + 1024, read_data, sizeof(read_data));
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	uint8_t tmp1[PARTITION_METADATA_SIZE_TEST + filesystem_get_element_header_len(partition_id)];
	tmp1[2] = 0x40; 		// partition_id msb
	tmp1[3] = 0x01; 		// partition_id lsb
	tmp1[4] = 0x00;			// partition_size msb
	tmp1[5] = 0x00;			// partition_size 
	tmp1[6] = 0x04;			// partition_size 
	tmp1[7] = 0x00;			// partition_size lsb
	tmp1[8] = 0x00; 		// first_element_len msb
	tmp1[9] = 0x64; 		// first_element_len lsb
	tmp1[10] = 0x00; 		// last_element_address msb
	tmp1[11] = 0x00; 		// last_element_address
	tmp1[12] = 0x08; 		// last_element_address
	tmp1[13] = 0x00; 		// last_element_address lsb
	tmp1[14] = 0x00; 		// record_id msb
	tmp1[15] = 0x01; 		// record_id lsb
	
	
	
	
	
	// Check if the metadata and element header is correct (except header-crc and element-crc)
	EXPECT_TRUE(memcmp(&tmp1[2], &read_data[2], 14) == 0);	
	
	// Check if the data are in storage after the metadata and element header
	EXPECT_TRUE(memcmp(data, &read_data[PARTITION_METADATA_SIZE_TEST + filesystem_get_element_header_len(partition_id)], 100) == 0);
	
	
	
	
	
	
	// Exception because element_len != 0 and != 100
	ret = filesystem_store_element(partition_id, data, 105);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
}


TEST_F(FilesystemTest, RegisterAfterStoreTest) {
	

	uint8_t data[1000];
	for(uint16_t i =0; i < 1000; i++)
		data[i] = i % 256;
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = 2000;
	
	// Registering a dynamic partition with crc
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(partitions[0].first_element_address, 1024);
	EXPECT_EQ(partitions[0].has_first_element, 0);
	EXPECT_EQ(partitions[0].latest_element_address, 1024);
	EXPECT_EQ(partitions[0].latest_element_record_id, 1);
	EXPECT_EQ(partitions[0].latest_element_len, 0);
	
	EXPECT_EQ(partitions[0].metadata.partition_id, 0x8000 | 0x4000);
	EXPECT_EQ(partitions[0].metadata.partition_size, 2048);
	EXPECT_EQ(partitions[0].metadata.first_element_len, 0);
	EXPECT_EQ(partitions[0].metadata.last_element_address, 1024);
	
	
	
	// TODO: store some more data (2 pages) and overwrite the first element header.
	
	ret = filesystem_store_element(partition_id, data, 500);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = filesystem_store_element(partition_id, data, 500);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = filesystem_store_element(partition_id, data, 500);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = filesystem_store_element(partition_id, data, 500);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = filesystem_store_element(partition_id, data, 500);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = filesystem_store_element(partition_id, data, 500);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	// Reset the filesystem
	filesystem_reset();
	
	// Try to find the former created filesystem
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	
	
	EXPECT_EQ(partitions[0].first_element_address, 1024);
	EXPECT_EQ(partitions[0].has_first_element, 1);
	EXPECT_EQ(partitions[0].latest_element_address, 1024 + PARTITION_METADATA_SIZE_TEST + 1*(filesystem_get_element_header_len(partition_id) + 500));
	EXPECT_EQ(partitions[0].latest_element_record_id, 6);
	EXPECT_EQ(partitions[0].latest_element_len, 500);
	
	EXPECT_EQ(partitions[0].metadata.partition_id, 0x8000 | 0x4000);
	EXPECT_EQ(partitions[0].metadata.partition_size, 2048);
	EXPECT_EQ(partitions[0].metadata.first_element_len, 500);
	EXPECT_EQ(partitions[0].metadata.last_element_address, 1024 + PARTITION_METADATA_SIZE_TEST + 3*(filesystem_get_element_header_len(partition_id) + 500));
	
	
	
	// Corrupt the first_element_header, so the entry in swap page should be used.
	uint8_t tmp[10];
	memset(tmp, 0xAB, 10);
	storage_store(1024, tmp, 2);
	
	
	
	// Reset the filesystem
	filesystem_reset();
	
	// Try to find the former created filesystem
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	
	EXPECT_EQ(partitions[0].first_element_address, 1024);
	EXPECT_EQ(partitions[0].has_first_element, 1);
	EXPECT_EQ(partitions[0].latest_element_address, 1024); // Because the other elements on same page have been deleted
	EXPECT_EQ(partitions[0].latest_element_record_id, 5);  // Because the other elements on same page have been deleted
	EXPECT_EQ(partitions[0].latest_element_len, 500);
	
	EXPECT_EQ(partitions[0].metadata.partition_id, 0x8000 | 0x4000);
	EXPECT_EQ(partitions[0].metadata.partition_size, 2048);
	EXPECT_EQ(partitions[0].metadata.first_element_len, 500);
	EXPECT_EQ(partitions[0].metadata.last_element_address, 1024 + PARTITION_METADATA_SIZE_TEST + 3*(filesystem_get_element_header_len(partition_id) + 500));
	
}


TEST_F(FilesystemTest, StoreLargeElementTest) {
	uint8_t data[5000];
	for(uint16_t i =0; i < 5000; i++)
		data[i] = i % 256;
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = 2000;
	
	// Registering a dynamic partition with crc
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(partitions[0].first_element_address, 1024);
	EXPECT_EQ(partitions[0].has_first_element, 0);
	EXPECT_EQ(partitions[0].latest_element_address, 1024);
	EXPECT_EQ(partitions[0].latest_element_record_id, 1);
	EXPECT_EQ(partitions[0].latest_element_len, 0);
	
	EXPECT_EQ(partitions[0].metadata.partition_id, 0x8000 | 0x4000);
	EXPECT_EQ(partitions[0].metadata.partition_size, 2048);
	EXPECT_EQ(partitions[0].metadata.first_element_len, 0);
	EXPECT_EQ(partitions[0].metadata.last_element_address, 1024);
	
	// Store a large element
	ret = filesystem_store_element(partition_id, data, 2000);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(partitions[0].first_element_address, 1024);
	EXPECT_EQ(partitions[0].has_first_element, 1);
	EXPECT_EQ(partitions[0].latest_element_address, 1024);
	EXPECT_EQ(partitions[0].latest_element_record_id, 1);
	EXPECT_EQ(partitions[0].latest_element_len, 2000);
	
	EXPECT_EQ(partitions[0].metadata.partition_id, 0x8000 | 0x4000);
	EXPECT_EQ(partitions[0].metadata.partition_size, 2048);
	EXPECT_EQ(partitions[0].metadata.first_element_len, 2000);
	EXPECT_EQ(partitions[0].metadata.last_element_address, 1024);
	
	// Store another large element
	ret = filesystem_store_element(partition_id, data, 1999);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(partitions[0].first_element_address, 1024);
	EXPECT_EQ(partitions[0].has_first_element, 1);
	EXPECT_EQ(partitions[0].latest_element_address, 1024);
	EXPECT_EQ(partitions[0].latest_element_record_id, 2);
	EXPECT_EQ(partitions[0].latest_element_len, 1999);
	
	EXPECT_EQ(partitions[0].metadata.partition_id, 0x8000 | 0x4000);
	EXPECT_EQ(partitions[0].metadata.partition_size, 2048);
	EXPECT_EQ(partitions[0].metadata.first_element_len, 1999);
	EXPECT_EQ(partitions[0].metadata.last_element_address, 1024);
	
	// Store a too large element
	ret = filesystem_store_element(partition_id, data, 2029);
	EXPECT_EQ(ret, NRF_ERROR_NO_MEM);
	
	
	
} 

TEST_F(FilesystemTest, IteratorInitTest) {
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = STORAGE1_SIZE_TEST;
	
	// Register partition
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	ret = filesystem_iterator_init(partition_id);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_STATE);
	
		
	// Create 3 elements in partition
	uint8_t data[3000];	
	for(uint32_t j = 0; j < 3; j++) {
		
		for(uint16_t i =0; i < 1000; i++)
			data[i] = i % 256;
	
		ret_code_t ret = filesystem_store_element(partition_id, data, (j+1)*1000);
		EXPECT_EQ(ret, NRF_SUCCESS);		
	}
	
	
	
	ret = filesystem_iterator_init(partition_id);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(partition_iterators[0].iterator_valid, 0xA5);
	
}


TEST_F(FilesystemTest, DynamicIteratorReadTest) {
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = STORAGE1_SIZE_TEST;
	
	// Register partition
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
		
	// Create elements in partition
	uint8_t data[1000];	
	for(uint32_t j = 0; j < 1000; j++) {
		
		for(uint16_t i =0; i < 1000; i++)
			data[i] = i % 256;
	
		ret_code_t ret = filesystem_store_element(partition_id, data, j);
		EXPECT_EQ(ret, NRF_SUCCESS);		
	}
	
	
	// Reset the filesystem
	filesystem_reset();
	
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	ret = filesystem_iterator_init(partition_id);
	ASSERT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(partition_iterators[0].iterator_valid, 0xA5);
	
	
	uint8_t read_data[1000];
	uint32_t j = 999;
	while(ret == NRF_SUCCESS) {
		uint16_t element_len, record_id;
	
		ret = filesystem_iterator_read_element(partition_id, read_data, &element_len, &record_id);
		EXPECT_EQ(ret, NRF_SUCCESS);
		EXPECT_EQ(element_len, j);
		EXPECT_EQ(record_id, j + 1);

		uint8_t data[1000];
		for(uint16_t i =0; i < 1000; i++)
				data[i] = i % 256;
		
		
		EXPECT_TRUE(memcmp(read_data, data, element_len) == 0);
		j--;
		
		ret = filesystem_iterator_previous(partition_id);
	}
	EXPECT_EQ(ret, NRF_ERROR_NOT_FOUND);
	
	
	
	
	ret = NRF_SUCCESS;
	j++;
	while(ret == NRF_SUCCESS) {
		uint16_t element_len, record_id;
	
		ret = filesystem_iterator_read_element(partition_id, read_data, &element_len, &record_id);
		EXPECT_EQ(ret, NRF_SUCCESS);
		EXPECT_EQ(element_len, j);
		EXPECT_EQ(record_id, j + 1);

		uint8_t data[1000];
		for(uint16_t i =0; i < 1000; i++)
				data[i] = i % 256;
		
		
		EXPECT_TRUE(memcmp(read_data, data, element_len) == 0);
		j++;
		
		ret = filesystem_iterator_next(partition_id);
	}
	EXPECT_EQ(ret, NRF_ERROR_NOT_FOUND);
	
}


TEST_F(FilesystemTest, StaticIteratorReadTest) {
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = STORAGE1_SIZE_TEST;
	
	// Register partition
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 0, 0, 500);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
		
	// Create elements in partition
	uint8_t data[1000];	
	for(uint32_t j = 0; j < 1000; j++) {
		
		for(uint16_t i =0; i < 1000; i++)
			data[i] = i % 256;
	
		ret_code_t ret = filesystem_store_element(partition_id, data, 0);
		EXPECT_EQ(ret, NRF_SUCCESS);		
	}
	
	
	// Reset the filesystem
	filesystem_reset();
	
	ret = filesystem_register_partition(&partition_id, &required_size, 0, 0, 500);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	ret = filesystem_iterator_init(partition_id);
	ASSERT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(partition_iterators[0].iterator_valid, 0xA5);
	
	
	uint8_t read_data[1000];
	uint32_t j = 999;
	while(ret == NRF_SUCCESS) {
		uint16_t element_len, record_id;
	
		ret = filesystem_iterator_read_element(partition_id, read_data, &element_len, &record_id);
		EXPECT_EQ(ret, NRF_SUCCESS);
		EXPECT_EQ(element_len, 500);
		EXPECT_EQ(record_id, j + 1);

		uint8_t data[1000];
		for(uint16_t i =0; i < 1000; i++)
				data[i] = i % 256;
		
		
		EXPECT_TRUE(memcmp(read_data, data, element_len) == 0);
		j--;
		
		ret = filesystem_iterator_previous(partition_id);
	}
	EXPECT_EQ(ret, NRF_ERROR_NOT_FOUND);
	
	
	
	
	ret = NRF_SUCCESS;
	j++;
	while(ret == NRF_SUCCESS) {
		uint16_t element_len, record_id;
	
		ret = filesystem_iterator_read_element(partition_id, read_data, &element_len, &record_id);
		EXPECT_EQ(ret, NRF_SUCCESS);
		EXPECT_EQ(element_len, 500);
		EXPECT_EQ(record_id, j + 1);

		uint8_t data[1000];
		for(uint16_t i =0; i < 1000; i++)
				data[i] = i % 256;
		
		
		EXPECT_TRUE(memcmp(read_data, data, element_len) == 0);
		j++;
		
		ret = filesystem_iterator_next(partition_id);
	}
	EXPECT_EQ(ret, NRF_ERROR_NOT_FOUND);

}

TEST_F(FilesystemTest, CorruptedDataTest) {
	

	
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = STORAGE1_SIZE_TEST;
	
	uint8_t data[1000];
	for(uint16_t i =0; i < 1000; i++)
		data[i] = i % 256;
	
	
	// Register dummy partition (so our actual partition is in EEPROM --> manipulate bytes)
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	
	// Register a dynamic partition with CRC
	required_size = 3000;
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
		
	// Create 2 elements in partition	
	ret = filesystem_store_element(partition_id, data, 1000);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	ret = filesystem_store_element(partition_id, data, 1000);
	EXPECT_EQ(ret, NRF_SUCCESS);	

	
	
	ret = filesystem_iterator_init(partition_id);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	uint8_t read_data[1000];
	uint16_t element_len, record_id;
	ret = filesystem_iterator_read_element(partition_id, read_data, &element_len, &record_id);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(element_len, 1000);
	EXPECT_EQ(record_id, 2);
	
	
	
	
	uint32_t element_address = partitions[1].latest_element_address;
	uint8_t tmp = 0xAB;
	uint8_t tmp_read;
	ret = storage_read(element_address + filesystem_get_element_header_len(partition_id) + 1, &tmp_read, 1);
	
	// Manipulate one byte in element data
	ret = storage_store(element_address + filesystem_get_element_header_len(partition_id) + 1, &tmp, 1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = filesystem_iterator_read_element(partition_id, read_data, &element_len, &record_id);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_DATA);
	
	// Restore the entry
	ret = storage_store(element_address + filesystem_get_element_header_len(partition_id) + 1, &tmp_read, 1);
	
	// Manipulate one byte in element header
	ret = storage_store(element_address + filesystem_get_element_header_len(partition_id) - 1, &tmp, 1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = filesystem_iterator_read_element(partition_id, read_data, &element_len, &record_id);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_STATE);
	
	// The iterator should stay in invalid state
	ret = filesystem_iterator_previous(partition_id);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_STATE);
	
	
	
	// Another test with a static partition without CRC
	
	
	// Register a static partition without CRC
	required_size = 3000;
	ret = filesystem_register_partition(&partition_id, &required_size, 0, 0, 100);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	// Create 2 elements in partition	
	ret = filesystem_store_element(partition_id, data, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	ret = filesystem_store_element(partition_id, data, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	// Initialize the iterator
	ret = filesystem_iterator_init(partition_id);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	// Read current (latest element)
	ret = filesystem_iterator_read_element(partition_id, read_data, &element_len, &record_id);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(element_len, 100);
	EXPECT_EQ(record_id, 2);
	
	
	// Now manipulate some bytes
	element_address = partitions[2].latest_element_address;
	tmp = 0xAB;
	
	ret = storage_read(element_address + filesystem_get_element_header_len(partition_id) + 1, &tmp_read, 1);
	
	// Manipulate one byte in element data
	ret = storage_store(element_address + filesystem_get_element_header_len(partition_id) + 1, &tmp, 1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = filesystem_iterator_read_element(partition_id, read_data, &element_len, &record_id);
	EXPECT_EQ(ret, NRF_SUCCESS);	// Because we don't use CRC here, we could not detect corrupted data
	
	// Restore the entry
	ret = storage_store(element_address + filesystem_get_element_header_len(partition_id) + 1, &tmp_read, 1);
	
	// Manipulate one byte in element header
	ret = storage_store(element_address + filesystem_get_element_header_len(partition_id) - 1, &tmp, 1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = filesystem_iterator_read_element(partition_id, read_data, &element_len, &record_id);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_STATE);
	
	// The iterator should stay in invalid state
	ret = filesystem_iterator_previous(partition_id);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_STATE);
	
}


TEST_F(FilesystemTest, ClearTest) {
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = STORAGE1_SIZE_TEST;
	
	// Register partition
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	ret = filesystem_iterator_init(partition_id);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_STATE);
	
		
	// Create 3 elements in partition
	uint8_t data[3000];	
	for(uint32_t j = 0; j < 3; j++) {
		
		for(uint16_t i =0; i < 3000; i++)
			data[i] = i % 256;
	
		ret_code_t ret = filesystem_store_element(partition_id, data, (j+1)*1000);
		EXPECT_EQ(ret, NRF_SUCCESS);		
	}
	
	
	
	ret = filesystem_iterator_init(partition_id);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(partition_iterators[0].iterator_valid, 0xA5);
	
	// If we only reset the filesystem, the partition should be still there and we can create an iterator for it
	ret = filesystem_reset();
	EXPECT_EQ(ret, NRF_SUCCESS);
	// Register partition
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = filesystem_iterator_init(partition_id);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_EQ(partition_iterators[0].iterator_valid, 0xA5);
	
	
	
	// If we clear the filesystem completely, we should not find a valid iterator
	ret = filesystem_clear();
	EXPECT_EQ(ret, NRF_SUCCESS);
	// Register partition
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = filesystem_iterator_init(partition_id);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_STATE);
	
}

TEST_F(FilesystemTest, ClearPartitionTest) {
	
	
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = STORAGE1_SIZE_TEST;
	
	// Register partition to fill flash:
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	// Now in EEPROM
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
	ret = filesystem_iterator_init(partition_id);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_STATE);
	
		
	// Create 3 elements in partition
	uint8_t data[3000];	
	for(uint32_t j = 0; j < 3; j++) {
		
		for(uint16_t i =0; i < 3000; i++)
			data[i] = i % 256;
	
		ret_code_t ret = filesystem_store_element(partition_id, data, (j+1)*1000);
		EXPECT_EQ(ret, NRF_SUCCESS);		
	}
	
	
	
	ret = filesystem_iterator_init(partition_id);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(partition_iterators[1].iterator_valid, 0xA5);
	
	// If we only reset the filesystem, the partition should be still there and we can create an iterator for it
	ret = filesystem_reset();
	EXPECT_EQ(ret, NRF_SUCCESS);
	// Register partition
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	// Now in EEPROM
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = filesystem_iterator_init(partition_id);
	EXPECT_EQ(ret, NRF_SUCCESS);	
	EXPECT_EQ(partition_iterators[1].iterator_valid, 0xA5);
	
	// Check if number of elements is 3:
	ret = filesystem_iterator_previous(partition_id);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = filesystem_iterator_previous(partition_id);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = filesystem_iterator_previous(partition_id);
	EXPECT_EQ(ret, NRF_ERROR_NOT_FOUND);

	
	
	// Now clear the partition:
	ret = filesystem_clear_partition(partition_id);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	
	// If we clear the partition, we should not find a valid iterator (because there are no elements anymore)
	ret = filesystem_iterator_init(partition_id);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_STATE);
	
	
	// Write just one element, the same size like the first element before:
	for(uint32_t j = 0; j < 1; j++) {		
		for(uint16_t i =0; i < 1000; i++)
			data[i] = i % 256;
	
		ret_code_t ret = filesystem_store_element(partition_id, data, (j+1)*1000);
		EXPECT_EQ(ret, NRF_SUCCESS);		
	}
	
	

	// Now reset the filesystem, and reregister the partition again.
	ret = filesystem_reset();
	EXPECT_EQ(ret, NRF_SUCCESS);
	// Fill again flash partition
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	// Now in EEPROM
	ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
		
	// Now we should find an iterator
	ret = filesystem_iterator_init(partition_id);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(partition_iterators[1].iterator_valid, 0xA5);
	
	// Actually now there should only be one element!!!
	
	// Check if number of elements is 1:
	ret = filesystem_iterator_previous(partition_id);
	EXPECT_EQ(ret, NRF_ERROR_NOT_FOUND);
	
}

TEST_F(FilesystemTest, AvailableSizeTest) {
	uint32_t available_size = 0;
	available_size = filesystem_get_available_size();	
	EXPECT_EQ(available_size, STORAGE1_SIZE_TEST + STORAGE2_SIZE_TEST - STORAGE1_UNIT_SIZE_TEST);
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = 1222;	
	// Register partition
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	available_size = filesystem_get_available_size();	
	EXPECT_EQ(available_size, STORAGE1_SIZE_TEST + STORAGE2_SIZE_TEST - STORAGE1_UNIT_SIZE_TEST - required_size);

}

TEST_F(FilesystemTest, IteratorConflictTest) {
	
	uint16_t partition_id = 0xFFFF;
	uint32_t required_size = 2048;
	
	// Register partition
	ret_code_t ret = filesystem_register_partition(&partition_id, &required_size, 1, 1, 0);
	ASSERT_EQ(ret, NRF_SUCCESS);
	
		
	// Create 3 elements in partition
	uint8_t data[1000];	
	for(uint32_t j = 0; j < 3; j++) {
		
		for(uint16_t i =0; i < 1000; i++)
			data[i] = i % 256;
	
		ret_code_t ret = filesystem_store_element(partition_id, data, j);
		EXPECT_EQ(ret, NRF_SUCCESS);		
	}
	
	
	ret = filesystem_iterator_init(partition_id);
	ASSERT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(partition_iterators[0].iterator_valid, 0xA5);
	
	// Create some more elements in partition
	for(uint32_t j = 3; j < 58; j++) {
		
		for(uint16_t i =0; i < 1000; i++)
			data[i] = i % 256;
	
		ret_code_t ret = filesystem_store_element(partition_id, data, j);
		EXPECT_EQ(ret, NRF_SUCCESS);
	}
	// At element 59 we have a conflict now..
	ret = filesystem_store_element(partition_id, data, 59);
	EXPECT_EQ(ret, NRF_ERROR_INTERNAL);	
}

};

