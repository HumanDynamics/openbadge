#include "tinybuf.h"
#include <string.h>
//#include "stdio.h"
typedef enum {
	BIG_ENDIAN = 0,
	LITTLE_ENDIAN = 1
} endian_t;


/**@brief Function to retrieve the endianness of the system.
 *
 * @retval	BIG_ENDIAN		If the system has big-endian format.
 * @retval	LITTLE_ENDIAN	If the system has little-endian format.
 */
static endian_t test_endianness(void) {
	union {
		uint16_t shortVar;   
		uint8_t  charVar[2];
	} test_endianness;
	
	test_endianness.shortVar = 0x8000; // das Most Significant Bit innerhalb von 16
	if (test_endianness.charVar[0] != 0) {
		return BIG_ENDIAN;
	} else {
		return LITTLE_ENDIAN;
	}
}
 


/**@brief Function to converts endiannes of input-data if necessary.
 *
 * @param[in]	data_in		Pointer to input-data.
 * @param[out]	data_out	Pointer to output-data.
 * @param[in]	data_size	Size of the data to convert.
 */
static void convert_endianness(uint8_t* data_in, uint8_t* data_out, uint8_t data_size) {
	// First check the endiannes, to check whether we have to swap the entries or not
	static uint8_t endian_test_done = 0;
	static endian_t endianness = LITTLE_ENDIAN;
	if(endian_test_done == 0) {
		endian_test_done = 1;
		endianness = test_endianness();
	}
	
	if(endianness == LITTLE_ENDIAN) {
		// Swap array entries to create array with correct endianness
		for(uint8_t k = 0; k < data_size; k++) {
			data_out[data_size-k-1] = data_in[k];
		}
	} else {
		for(uint8_t k = 0; k < data_size; k++) {
			data_out[k] = data_in[k];
		}
	}
}





/**@brief Function to create an output-stream from a buffer.
 *
 * @param[in]	buf				Pointer to the buffer that should be used by the output-stream.
 * @param[in]	buf_size		Maximal size of the buffer.
 *
 * @retval 		tb_ostream_t	Output-stream structure.
 */
tb_ostream_t tb_ostream_from_buffer(uint8_t* buf, uint32_t buf_size) {
	tb_ostream_t ostream = {buf, buf_size, 0};
	return ostream;
}

/**@brief Function to write data to ostream.
 *
 * @param[in]	ostream		Pointer to output-stream structure.
 * @param[in]	data		Pointer to data that should be written to the output-stream.
 * @param[in]	len			Size of data to be written to the output-stream.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations.
 */
static uint8_t tb_write_to_ostream(tb_ostream_t* ostream, uint8_t* data, uint32_t len) {
	if(ostream->bytes_written + len > ostream->buf_size)
		return 0;
	
	memcpy(&(ostream->buf[ostream->bytes_written]), data, len);
	/*
	printf("Written %u bytes to ostream: {", len);
	for(uint32_t i = ostream->bytes_written; i < ostream->bytes_written+len; i++)
		printf("0x%X, ", ostream->buf[i]);
	printf("}\n");
	*/
	ostream->bytes_written += len;	
	return 1;
}


/**@brief Function to write data to ostream in big-endian format.
 *
 * @param[in]	ostream		Pointer to output-stream structure.
 * @param[in]	data		Pointer to data that should be written in big endian format to the output-stream.
 * @param[in]	data_size	Size of one data entry to convert from little- to big-endian.
 * @param[in]	len			Number of data entries to convert from little- to big-endian and write to the output-stream.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations.
 */
static uint8_t tb_write_to_ostream_big_endian(tb_ostream_t* ostream, uint8_t* data, uint8_t data_size, uint32_t len) {
	uint8_t tmp[data_size];
	
	for(uint32_t i = 0; i < len; i++) {
		uint8_t* cur_data = data + (((uint32_t)data_size)*i);
		convert_endianness(cur_data, tmp, data_size);
		if(!tb_write_to_ostream(ostream, tmp, data_size))
			return 0;
	}
	return 1;
}



/**@brief Function to create an input-stream from a buffer.
 *
 * @param[in]	buf				Pointer to the buffer that should be used by the output-stream.
 * @param[in]	buf_size		Size of the input-buffer.
 *
 * @retval 		tb_istream_t	Input-stream structure.
 */
tb_istream_t tb_istream_from_buffer(uint8_t* buf, uint32_t buf_size) {
	tb_istream_t istream = {buf, buf_size, 0};
	return istream;
}

/**@brief Function to read data from the input-stream.
 *
 * @param[in]	istream		Pointer to input-stream structure.
 * @param[in]	data		Pointer to data where the read data should be stored to.
 * @param[in]	len			Size of the data that should be read from the input-stream.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations.
 */
static uint8_t tb_read_from_istream(tb_istream_t* istream, uint8_t* data, uint32_t len) {
	if(istream->bytes_read + len > istream->buf_size)
		return 0;
	
	memcpy(data, &(istream->buf[istream->bytes_read]), len);
	/*
	printf("Read %u bytes from istream: {", len);
	for(uint32_t i = istream->bytes_read; i < istream->bytes_read+len; i++)
		printf("0x%X, ", istream->buf[i]);
	printf("}\n");
	*/
	istream->bytes_read += len;	
	return 1;
}

/**@brief Function to read data from istream in little-endian format.
 *
 * @param[in]	istream		Pointer to input-stream structure.
 * @param[in]	data		Pointer to data where the read data should be stored to (in little-endian format).
 * @param[in]	data_size	Size of one data entry to convert from big- to little-endian.
 * @param[in]	len			Number of data entries to convert from big- to little-endian.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations.
 */
static uint8_t tb_read_from_istream_little_endian(tb_istream_t* istream, uint8_t* data, uint8_t data_size, uint32_t len) {
	uint8_t tmp[data_size];
	
	for(uint32_t i = 0; i < len; i++) {
		if(!tb_read_from_istream(istream, tmp, data_size))
			return 0;
		uint8_t* cur_data = data + (((uint32_t)data_size)*i);
		convert_endianness(tmp, cur_data, data_size);
		
	}
	return 1;
}






/**@brief Function to serialize a structure into an output-stream.
 *
 * @param[in]	ostream		Pointer to output-stream structure.
 * @param[in]	fields		Pointer to the array of structure-fields.
 * @param[in]	src_struct	Pointer to structure that should be serialized.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations or invalid structure.
 */
uint8_t tb_encode(tb_ostream_t* ostream, const tb_field_t fields[], void* src_struct) {
	uint8_t i = 0;
	
	while(fields[i].type != 0) {
		
		tb_field_t field = fields[i];
		// All these types are little endian
		if(field.type & DATA_TYPE_INT || field.type & DATA_TYPE_UINT || field.type & DATA_TYPE_FLOAT || field.type & DATA_TYPE_DOUBLE)  {
			void* data_ptr;
			data_ptr = ((uint8_t*)src_struct + field.data_offset);
			
			if(field.type & FIELD_TYPE_REQUIRED) {
				if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*) data_ptr, field.data_size, 1))
					return 0;
			} else if(field.type & FIELD_TYPE_OPTIONAL) {
				
				// Check the has ptr-flag
				void* has_ptr = ((uint8_t*)data_ptr + field.size_offset);
				uint8_t has_flag = *((uint8_t*) has_ptr);
				if(!tb_write_to_ostream(ostream, (uint8_t*) has_ptr, field.size_size))	// Could actually also write just 1 byte
					return 0;
				
				//printf("Has flag is: %u!\n", has_flag);
				
				// Only write the data if has_flag is true
				if(has_flag) {
					if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*) data_ptr, field.data_size, 1))
						return 0;
				}				
			} else if(field.type & FIELD_TYPE_REPEATED) {
				// Read the number of elements:
				void* count_ptr = ((uint8_t*)data_ptr + field.size_offset);
				uint32_t 	count_size = field.size_size;
				uint32_t 	count = 0;
				if(count_size > sizeof(count)) {
					//printf("Error count_size is too big for uint32_t!\n");
					return 0;
				}
				uint8_t tmp[count_size];
				convert_endianness((uint8_t*)count_ptr, tmp, count_size);	// Convert endianness (only if endianness is little-endian)
				// Create the actual count
				for(uint8_t k = 0; k < count_size; k++) {
					count |= (tmp[count_size-1-k]) << (8*k);
				}
				//printf("Count: %u\n", count);
				
				// Check the count:
				if(count > field.array_size) {
					//printf("Count exceeds array size!\n");
					return 0;				
				}				
				if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*)count_ptr, count_size, 1))
					return 0;
				
				// Now write the actual data of the array				
				if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*) data_ptr, field.data_size, count))
					return 0;
				
			} else {
				//printf("Error no field type specified\n");
				return 0;
			}
		} else if (field.type & DATA_TYPE_MESSAGE) {
			
			
			void* struct_ptr;
			struct_ptr = ((uint8_t*)src_struct + field.data_offset);
			
			if(field.type & FIELD_TYPE_REQUIRED) {
				// Recursive call of encode function
				if(!tb_encode(ostream, (tb_field_t*) field.ptr, struct_ptr))
					return 0;
			} else if(field.type & FIELD_TYPE_OPTIONAL) {
				// Check the has ptr-flag
				void* has_ptr = ((uint8_t*)struct_ptr + field.size_offset);
				uint8_t has_flag = *((uint8_t*) has_ptr);
				if(!tb_write_to_ostream(ostream, (uint8_t*) has_ptr, field.size_size)) // Could actually also write just 1 byte
					return 0;
				
				//printf("Has flag is: %u!\n", has_flag);
				
				// Only write the data if has_flag is true
				if(has_flag) {
					// Recursive call of encode function
					if(!tb_encode(ostream, (tb_field_t*) field.ptr, struct_ptr))
						return 0;
				}
				
				
			} else if(field.type & FIELD_TYPE_REPEATED) {
				
				// Read the number of elements:
				void* count_ptr = ((uint8_t*)struct_ptr + field.size_offset);
				uint32_t 	count_size = field.size_size;
				uint32_t 	count = 0;
				if(count_size > sizeof(count)) {
					//printf("Error count_size is too big for uint32_t!\n");
					return 0;
				}
				uint8_t tmp[count_size];
				convert_endianness((uint8_t*)count_ptr, tmp, count_size);	// Convert endianness (only if endianness is little-endian)
				// Create the actual count
				for(uint8_t k = 0; k < count_size; k++) {
					count |= (tmp[count_size-1-k]) << (8*k);
				}
				//printf("Count: %u\n", count);				
				// Check the count:
				if(count > field.array_size) {
					//printf("Count exceeds array size!\n");
					return 0;				
				}
				
				if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*)count_ptr, count_size, 1))
					return 0;
				
				
				for(uint32_t k = 0; k < count; k++) {
					struct_ptr = ((uint8_t*)src_struct + field.data_offset + k*field.data_size);
					// Recursive call of encode function
					if(!tb_encode(ostream, (tb_field_t*) field.ptr, struct_ptr))
						return 0;					
				}				
			} else {
				//printf("Error no field type specified\n");
				return 0;
			}		
		} else {
			//printf("Error no data type specified\n");
			return 0;
		}
				
		i++;
	}
	return 1;
}




/**@brief Function to deserialize an input-stream to a structure.
 *
 * @param[in]	istream		Pointer to input-stream structure.
 * @param[in]	fields		Pointer to the array of structure-fields.
 * @param[in]	src_struct	Pointer to structure where the deserialized data should be stored to.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations or invalid structure.
 */
uint8_t tb_decode(tb_istream_t* istream, const tb_field_t fields[], void* dst_struct) {
	uint8_t i = 0;
	
	while(fields[i].type != 0) {
		
		tb_field_t field = fields[i];
		// All these types are little endian
		if(field.type & DATA_TYPE_INT || field.type & DATA_TYPE_UINT || field.type & DATA_TYPE_FLOAT || field.type & DATA_TYPE_DOUBLE)  {
			void* data_ptr;
			data_ptr = ((uint8_t*)dst_struct + field.data_offset);
			
			if(field.type & FIELD_TYPE_REQUIRED) {
				if(!tb_read_from_istream_little_endian(istream, (uint8_t*) data_ptr, field.data_size, 1))
					return 0;
			} else if(field.type & FIELD_TYPE_OPTIONAL) {
				
				// Check the has ptr-flag
				void* has_ptr = ((uint8_t*)data_ptr + field.size_offset);
				if(!tb_read_from_istream(istream, (uint8_t*) has_ptr, field.size_size))	// Could actually also read just 1 byte
					return 0;
				
				uint8_t has_flag = *((uint8_t*) has_ptr);
				//printf("Has flag is: %u!\n", has_flag);
				
				// Only write the data if has_flag is true
				if(has_flag) {
					if(!tb_read_from_istream_little_endian(istream, (uint8_t*) data_ptr, field.data_size, 1))
						return 0;
				}				
			} else if(field.type & FIELD_TYPE_REPEATED) {
				// Read the number of elements:
				void* count_ptr = ((uint8_t*)data_ptr + field.size_offset);
				uint32_t 	count_size = field.size_size;
				uint32_t 	count = 0;
				if(count_size > sizeof(count)) {
					//printf("Error count_size is too big for uint32_t!\n");
					return 0;
				}
				if(!tb_read_from_istream_little_endian(istream, (uint8_t*) count_ptr, count_size, 1))
					return 0;
				
				uint8_t tmp[count_size];
				convert_endianness((uint8_t*)count_ptr, tmp, count_size);	// Convert endianness (only if endianness is little-endian)
				// Create the actual count
				for(uint8_t k = 0; k < count_size; k++) {
					count |= (tmp[count_size-1-k]) << (8*k);
				}
				//printf("Count: %u\n", count);
				// Check the count:
				if(count > field.array_size) {
					//printf("Count exceeds array size!\n");
					return 0;				
				}
				
				// Now read the actual data of the array		
				if(!tb_read_from_istream_little_endian(istream, (uint8_t*) data_ptr, field.data_size, count))
					return 0;
				
			} else {
				//printf("Error no field type specified\n");
				return 0;
			}
		} else if (field.type & DATA_TYPE_MESSAGE) {
			
			void* struct_ptr;
			struct_ptr = ((uint8_t*)dst_struct + field.data_offset);
			
			if(field.type & FIELD_TYPE_REQUIRED) {
				// Recursive call of decode function
				if(!tb_decode(istream, (tb_field_t*) field.ptr, struct_ptr))
					return 0;
			} else if(field.type & FIELD_TYPE_OPTIONAL) {
				
				// Check the has ptr-flag
				void* has_ptr = ((uint8_t*)struct_ptr + field.size_offset);
				if(!tb_read_from_istream(istream, (uint8_t*) has_ptr, field.size_size)) // Could actually also read just 1 byte
					return 0;
				
				uint8_t has_flag = *((uint8_t*) has_ptr);
				//printf("Has flag is: %u!\n", has_flag);
				
				// Only read the data if has_flag is true
				if(has_flag) {
					// Recursive call of decode function
					if(!tb_decode(istream, (tb_field_t*) field.ptr, struct_ptr))
						return 0;
				}
				
				
			} else if(field.type & FIELD_TYPE_REPEATED) {
				
				// Read the number of elements:
				void* count_ptr = ((uint8_t*)struct_ptr + field.size_offset);
				uint32_t 	count_size = field.size_size;
				uint32_t 	count = 0;
				if(count_size > sizeof(count)) {
					//printf("Error count_size is too big for uint32_t!\n");
					return 0;
				}
				tb_read_from_istream_little_endian(istream, (uint8_t*) count_ptr, count_size, 1);				
				uint8_t tmp[count_size];
				convert_endianness((uint8_t*)count_ptr, tmp, count_size);	// Convert endianness (only if endianness is little-endian)
				// Create the actual count
				for(uint8_t k = 0; k < count_size; k++) {
					count |= (tmp[count_size-1-k]) << (8*k);
				}
				//printf("Count: %u\n", count);
				// Check the count:
				if(count > field.array_size) {
					//printf("Count exceeds array size!\n");
					return 0;				
				}
			
				for(uint32_t k = 0; k < count; k++) {
					struct_ptr = ((uint8_t*)dst_struct + field.data_offset + k*field.data_size);
					// Recursive call of encode function
					if(!tb_decode(istream, (tb_field_t*) field.ptr, struct_ptr))
						return 0;					
				}				
			} else {
				//printf("Error no field type specified\n");
				return 0;
			}
			
		} else {
			//printf("Error no data type specified\n");
			return 0;
		}
		i++;		
	}
	return 1;
}

