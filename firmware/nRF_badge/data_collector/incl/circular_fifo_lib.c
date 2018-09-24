#include "circular_fifo_lib.h"

#include "stdlib.h" // Needed for NULL definition



ret_code_t circular_fifo_init(circular_fifo_t * p_fifo, uint8_t * p_buf, uint16_t buf_size) {
	if(p_buf == NULL)
		return NRF_ERROR_NULL;
	
	
	p_fifo->p_buf 		= p_buf;
	p_fifo->buf_size	= buf_size;
	p_fifo->read_pos    = 0;
    p_fifo->write_pos   = 0;

    return NRF_SUCCESS;
}


void circular_fifo_flush(circular_fifo_t * p_fifo) {
	p_fifo->read_pos = p_fifo->write_pos;
	
}

ret_code_t circular_fifo_get(circular_fifo_t * p_fifo, uint8_t* byte) {
	if(p_fifo->read_pos != p_fifo->write_pos) {
		*byte = p_fifo->p_buf[p_fifo->read_pos];
		p_fifo->read_pos = (p_fifo->read_pos + 1) % (p_fifo->buf_size + 1);		
		return NRF_SUCCESS;
	}
	return NRF_ERROR_NOT_FOUND;
}

void circular_fifo_put(circular_fifo_t * p_fifo, uint8_t byte) {
		p_fifo->p_buf[p_fifo->write_pos] = byte;
		p_fifo->write_pos = (p_fifo->write_pos + 1) % (p_fifo->buf_size + 1);		
}

void circular_fifo_read(circular_fifo_t * p_fifo, uint8_t * p_byte_array, uint32_t * p_size) {
	uint32_t index = 0;	
	while(index < *p_size) {
		if(circular_fifo_get(p_fifo, &(p_byte_array[index])) != NRF_SUCCESS) {
			break;
		}
		index++;
	}
	*p_size = index;	
}

void circular_fifo_write(circular_fifo_t * p_fifo, uint8_t const * p_byte_array, uint32_t size) {
	uint32_t index = 0;	
	while(index < size) {
		circular_fifo_put(p_fifo, p_byte_array[index]);
		index++;
	}
}


uint32_t circular_fifo_get_size(circular_fifo_t * p_fifo) {
	uint32_t available_len = 0;
	if(p_fifo->write_pos >= p_fifo->read_pos) {
		available_len = p_fifo->write_pos - p_fifo->read_pos;
	} else {
		available_len = p_fifo->buf_size + 1 - p_fifo->read_pos + p_fifo->write_pos;
	}
	return available_len;
}
