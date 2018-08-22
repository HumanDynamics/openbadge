#include "chunk_fifo_lib.h"

#include "stdlib.h" // Needed for NULL definition

ret_code_t chunk_fifo_init(chunk_fifo_t* chunk_fifo, uint8_t chunk_num, uint32_t chunk_size, uint32_t additional_info_size, uint8_t* p_chunk_fifo_buf) {
	if(chunk_num == 0 || p_chunk_fifo_buf == NULL)
		return NRF_ERROR_INVALID_PARAM;
	
	chunk_fifo->chunk_num = chunk_num;
	chunk_fifo->chunk_size = chunk_size;
	chunk_fifo->additional_info_size = additional_info_size;
	chunk_fifo->p_chunk_fifo_buf = p_chunk_fifo_buf;
	chunk_fifo->chunk_read_pos = 0;
	chunk_fifo->chunk_write_pos = 0;

	return NRF_SUCCESS;
}


ret_code_t chunk_fifo_read_open(chunk_fifo_t* chunk_fifo, void** p_chunk, void** p_additional_info) {
	
	if(chunk_fifo->chunk_read_pos != chunk_fifo->chunk_write_pos) {
		// New chunks are available
		*p_chunk = &(chunk_fifo->p_chunk_fifo_buf[(chunk_fifo->chunk_read_pos)*(chunk_fifo->chunk_size + chunk_fifo->additional_info_size)]);
		*p_additional_info = &(chunk_fifo->p_chunk_fifo_buf[chunk_fifo->chunk_size + (chunk_fifo->chunk_read_pos)*(chunk_fifo->chunk_size + chunk_fifo->additional_info_size)]);
		return NRF_SUCCESS;
	} else {
		return NRF_ERROR_NOT_FOUND;
	}
}

void chunk_fifo_read_close(chunk_fifo_t* chunk_fifo) {
	if(chunk_fifo->chunk_read_pos == chunk_fifo->chunk_write_pos)
		return;
	
	chunk_fifo->chunk_read_pos = (chunk_fifo->chunk_read_pos + 1) % (chunk_fifo->chunk_num + 1); 
}

void chunk_fifo_write_open(chunk_fifo_t* chunk_fifo, void** p_chunk, void** p_additional_info) {
	
	*p_chunk = &(chunk_fifo->p_chunk_fifo_buf[(chunk_fifo->chunk_write_pos)*(chunk_fifo->chunk_size + chunk_fifo->additional_info_size)]);
	*p_additional_info = &(chunk_fifo->p_chunk_fifo_buf[chunk_fifo->chunk_size + (chunk_fifo->chunk_write_pos)*(chunk_fifo->chunk_size + chunk_fifo->additional_info_size)]);
	
}

void chunk_fifo_write_close(chunk_fifo_t* chunk_fifo) {
	
	if((chunk_fifo->chunk_write_pos + 1) % (chunk_fifo->chunk_num + 1) != chunk_fifo->chunk_read_pos) {
		chunk_fifo->chunk_write_pos = (chunk_fifo->chunk_write_pos + 1) % (chunk_fifo->chunk_num + 1);
	}
	
}

uint8_t chunk_fifo_get_number_of_chunks(chunk_fifo_t* chunk_fifo) {
	if(chunk_fifo->chunk_write_pos >= chunk_fifo->chunk_read_pos) {
		return chunk_fifo->chunk_write_pos - chunk_fifo->chunk_read_pos;
	} else {
		return chunk_fifo->chunk_read_pos - chunk_fifo->chunk_write_pos;
	}
}
