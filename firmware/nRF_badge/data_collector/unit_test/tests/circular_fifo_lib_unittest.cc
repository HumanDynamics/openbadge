

// Don't forget gtest.h, which declares the testing framework.


#include "gtest/gtest.h"
#include "circular_fifo_lib.h"


namespace {


TEST(CircularFifoTest, InitTest) {
	circular_fifo_t circular_fifo;
	ret_code_t ret;
	CIRCULAR_FIFO_INIT(ret, circular_fifo, 100);	
	EXPECT_EQ(ret, NRF_SUCCESS);
}


TEST(CircularFifoTest, InitErrorTest) {
	circular_fifo_t circular_fifo;
	ret_code_t ret;
	ret = circular_fifo_init(&circular_fifo, NULL, 100);	
	EXPECT_EQ(ret, NRF_ERROR_NULL);
}

TEST(CircularFifoTest, ReadWriteTest) {
	circular_fifo_t circular_fifo;
	ret_code_t ret;
	CIRCULAR_FIFO_INIT(ret, circular_fifo, 200);	
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	
	uint8_t write_data[100];
	uint8_t read_data[100];
	memset(read_data, 0, sizeof(read_data));
	for(uint32_t i = 0; i < sizeof(write_data); i++) {
		write_data[i] = i % 256;
	}
	
	uint32_t size = circular_fifo_get_size(&circular_fifo);
	EXPECT_EQ(size, 0);
	
	circular_fifo_write(&circular_fifo, write_data, sizeof(write_data));
	size = circular_fifo_get_size(&circular_fifo);
	EXPECT_EQ(size, sizeof(write_data));
	
	uint32_t read_size = sizeof(read_data);
	circular_fifo_read(&circular_fifo, read_data, &read_size);
	EXPECT_EQ(read_size, sizeof(read_data));
	EXPECT_ARRAY_EQ(write_data, read_data, read_size);
	
	size = circular_fifo_get_size(&circular_fifo);
	EXPECT_EQ(size, 0);	
	
	read_size = sizeof(read_data);
	circular_fifo_read(&circular_fifo, read_data, &read_size);
	EXPECT_EQ(read_size, 0);
}



TEST(CircularFifoTest, MultipleReadWriteTest) {
	circular_fifo_t circular_fifo;
	ret_code_t ret;
	CIRCULAR_FIFO_INIT(ret, circular_fifo, 200);	
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	for(uint32_t k = 0; k  < 10; k++) {
		uint8_t write_data[150];
		uint8_t read_data[150];
		memset(read_data, 0, sizeof(read_data));
		for(uint32_t i = 0; i < sizeof(write_data); i++) {
			write_data[i] = i % 256;
		}
		
		uint32_t size = circular_fifo_get_size(&circular_fifo);
		EXPECT_EQ(size, 0);
		
		circular_fifo_write(&circular_fifo, write_data, sizeof(write_data));
		size = circular_fifo_get_size(&circular_fifo);
		EXPECT_EQ(size, sizeof(write_data));
		
		uint32_t read_size = sizeof(read_data);
		circular_fifo_read(&circular_fifo, read_data, &read_size);
		EXPECT_EQ(read_size, sizeof(read_data));
		EXPECT_ARRAY_EQ(write_data, read_data, read_size);
		
		size = circular_fifo_get_size(&circular_fifo);
		EXPECT_EQ(size, 0);
	}
}

TEST(CircularFifoTest, FlushTest) {
	circular_fifo_t circular_fifo;
	ret_code_t ret;
	CIRCULAR_FIFO_INIT(ret, circular_fifo, 200);	
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	
	uint8_t write_data[150];
	uint8_t read_data[150];
	memset(read_data, 0, sizeof(read_data));
	for(uint32_t i = 0; i < sizeof(write_data); i++) {
		write_data[i] = i % 256;
	}
	
	uint32_t size = circular_fifo_get_size(&circular_fifo);
	EXPECT_EQ(size, 0);
	
	circular_fifo_write(&circular_fifo, write_data, sizeof(write_data));
	size = circular_fifo_get_size(&circular_fifo);
	EXPECT_EQ(size, sizeof(write_data));
	
	circular_fifo_flush(&circular_fifo);
	
	size = circular_fifo_get_size(&circular_fifo);
	EXPECT_EQ(size, 0);
	
	uint32_t read_size = sizeof(read_data);
	circular_fifo_read(&circular_fifo, read_data, &read_size);
	EXPECT_EQ(read_size, 0);
	
	
}


/*

TEST(ChunkFifoTest, InitErrorTest) {
	chunk_fifo_t chunk_fifo;
	ret_code_t ret;
	CHUNK_FIFO_INIT(ret, chunk_fifo, 0, 100, 4);	
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);

	// Initialize the chunk-fifo with NULL as buffer
	ret = chunk_fifo_init(&chunk_fifo, 1, 100, 4, NULL);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);	
}

TEST(ChunkFifoTest, OneElementTest) {
	chunk_fifo_t chunk_fifo;
	ret_code_t ret;
	CHUNK_FIFO_INIT(ret, chunk_fifo, 1, 100, 4);
	
	
	
	uint8_t num = chunk_fifo_get_number_of_chunks(&chunk_fifo);
	EXPECT_EQ(num, 0);
	
	// Close the chunk-fifo without opening it --> nothing should happen
	chunk_fifo_write_close(&chunk_fifo);
	
	// Open the chunk-fifo for writing one element
	data_t*				data = NULL;
	additional_info_t* 	additional_info = NULL;
	chunk_fifo_write_open(&chunk_fifo, (void**)&data, (void**)&additional_info);
	
	// Put some data into the chunk-fifo
	memset(data->buf, 0xAB, 100);
	memset(additional_info->buf, 0xCD, 4);
	
	// Num should still be 0, because not closed the write yet
	num = chunk_fifo_get_number_of_chunks(&chunk_fifo);
	EXPECT_EQ(num, 0);
	
	// There should be no chunk to read:
	data_t* data_read;
	additional_info_t* additional_info_read;
	ret = chunk_fifo_read_open(&chunk_fifo, (void**) &data_read, (void**) &additional_info_read);
	EXPECT_EQ(ret, NRF_ERROR_NOT_FOUND);
	chunk_fifo_read_close(&chunk_fifo);
	
	// Close the chunk-fifo
	chunk_fifo_write_close(&chunk_fifo);
	
	// Now the number of chunks should be 1
	num = chunk_fifo_get_number_of_chunks(&chunk_fifo);
	EXPECT_EQ(num, 1);
		
	
	ret = chunk_fifo_read_open(&chunk_fifo, (void**) &data_read, (void**) &additional_info_read);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	// Test the equality of the data
	EXPECT_ARRAY_EQ(data->buf, data_read->buf, 100);
	EXPECT_ARRAY_EQ(additional_info->buf, additional_info_read->buf, 4);
	
	chunk_fifo_read_close(&chunk_fifo);
	
	// Now the number of chunks should be 0
	num = chunk_fifo_get_number_of_chunks(&chunk_fifo);
	EXPECT_EQ(num, 0);
	
	// And there should be no chunk to read:
	ret = chunk_fifo_read_open(&chunk_fifo, (void**) &data_read, (void**) &additional_info_read);
	EXPECT_EQ(ret, NRF_ERROR_NOT_FOUND);
	
	
}


TEST(ChunkFifoTest, MultipleElementsTest) {
	chunk_fifo_t chunk_fifo;
	ret_code_t ret;
	CHUNK_FIFO_INIT(ret, chunk_fifo, 10, 100, 4);
	
	
	
	uint8_t num = chunk_fifo_get_number_of_chunks(&chunk_fifo);
	EXPECT_EQ(num, 0);
	
	// Open the chunk-fifo for writing one element
	data_t*				data = NULL;
	additional_info_t* 	additional_info = NULL;
	data_t* data_read;
	additional_info_t* additional_info_read;
	
	for(uint8_t i = 0; i < 20; i++) {
		chunk_fifo_write_open(&chunk_fifo, (void**)&data, (void**)&additional_info);
		chunk_fifo_write_close(&chunk_fifo);
	}
	// Number of elements should be maximum 10 (although 20 written chunks). But the latest chunks will be ignored.
	num = chunk_fifo_get_number_of_chunks(&chunk_fifo);
	EXPECT_EQ(num, 10);
	

	
	ret = chunk_fifo_read_open(&chunk_fifo, (void**) &data_read, (void**) &additional_info_read);
	EXPECT_EQ(ret, NRF_SUCCESS);
	chunk_fifo_read_close(&chunk_fifo);	
	
	num = chunk_fifo_get_number_of_chunks(&chunk_fifo);
	EXPECT_EQ(num, 9);
	
	
	chunk_fifo_write_open(&chunk_fifo, (void**)&data, (void**)&additional_info);
	chunk_fifo_write_close(&chunk_fifo);
	
	num = chunk_fifo_get_number_of_chunks(&chunk_fifo);
	EXPECT_EQ(num, 10);
	
	ret = chunk_fifo_read_open(&chunk_fifo, (void**) &data_read, (void**) &additional_info_read);
	EXPECT_EQ(ret, NRF_SUCCESS);
	chunk_fifo_read_close(&chunk_fifo);	
	
	num = chunk_fifo_get_number_of_chunks(&chunk_fifo);
	EXPECT_EQ(num, 9);
	
	
}
*/
};  
