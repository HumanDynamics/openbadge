

// Don't forget gtest.h, which declares the testing framework.


#include "gtest/gtest.h"
#include "chunk_fifo_lib.h"

/**< Struct that contains the actual chunk-data. */ 
typedef struct {
	uint8_t buf[100];
} data_t;

/**< Struct that contains the additional-infos for a chunk. */ 
typedef struct {
	uint8_t buf[4];
} additional_info_t;
	
namespace {


TEST(ChunkFifoTest, InitTest) {
	chunk_fifo_t chunk_fifo;
	ret_code_t ret;
	CHUNK_FIFO_INIT(ret, chunk_fifo, 2, 100, 4);	
	EXPECT_EQ(ret, NRF_SUCCESS);
}

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

};  
