#include "tinybuf.h"
#include <string.h>
#include "stdio.h"


/**@brief Function to retrieve the endianness of the system.
 *
 * @retval	TB_BIG_ENDIAN		If the system has big-endian format.
 * @retval	TB_LITTLE_ENDIAN	If the system has little-endian format.
 */
static tb_endian_t test_endianness(void) {
	union {
		uint16_t shortVar;   
		uint8_t  charVar[2];
	} test_endianness;
	
	test_endianness.shortVar = 0x8000; // das Most Significant Bit innerhalb von 16
	if (test_endianness.charVar[0] != 0) {
		return TB_BIG_ENDIAN;
	} else {
		return TB_LITTLE_ENDIAN;
	}
}
 


/**@brief Function to converts endiannes of input-data if necessary.
 *
 * @param[in]	data_in				Pointer to input-data.
 * @param[out]	data_out			Pointer to output-data.
 * @param[in]	data_size			Size of the data to convert.
 * @param[in] 	desired_endianness	The desired endianness of the output-data.
 */
static void convert_endianness(uint8_t* data_in, uint8_t* data_out, uint8_t data_size, tb_endian_t desired_endianness) {
	// First check the endiannes, to check whether we have to swap the entries or not
	static uint8_t endian_test_done = 0;
	static tb_endian_t endianness = TB_LITTLE_ENDIAN;
	if(endian_test_done == 0) {
		endian_test_done = 1;
		endianness = test_endianness();
	}
	
	if(endianness != desired_endianness) {
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
 * @param[in] 	output_endianness	The desired endianness of the output-data.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations.
 */
static uint8_t tb_write_to_ostream_big_endian(tb_ostream_t* ostream, uint8_t* data, uint8_t data_size, uint32_t len, tb_endian_t ouput_endianness) {
	uint8_t tmp[data_size];
	
	for(uint32_t i = 0; i < len; i++) {
		uint8_t* cur_data = data + (((uint32_t)data_size)*i);
		convert_endianness(cur_data, tmp, data_size, ouput_endianness);
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
 * @param[in] 	input_endianness	The endianness of the input-data.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations.
 */
static uint8_t tb_read_from_istream_little_endian(tb_istream_t* istream, uint8_t* data, uint8_t data_size, uint32_t len, tb_endian_t input_endianness) {
	uint8_t tmp[data_size];
	
	for(uint32_t i = 0; i < len; i++) {
		if(!tb_read_from_istream(istream, tmp, data_size))
			return 0;
		uint8_t* cur_data = data + (((uint32_t)data_size)*i);
		convert_endianness(tmp, cur_data, data_size, input_endianness);
		
	}
	return 1;
}


/**@brief Function to check if an oneof-which tag is valid.
 *
 * @details This function should only be called at the first field of the oneof-field.
 *
 * @param[in]	fields			Pointer to the array of structure-fields.
 * @param[in]	which_tag		The which_field to test.
 * @param[in]	field_index		The first field index of the oneof-field.
 
 *
 * @retval 		1			On success.
 * @retval		0			On failure, when the which_tag was not found in.
 */
static uint8_t check_oneof_which_tag_validity(const tb_field_t fields[], uint8_t which_tag, uint8_t field_index) {
	uint8_t i = field_index;
	do {
		// Check if the first found field_index has the which_tag:
		if(fields[i].oneof_tag == which_tag)
			return 1;
		i++;
	} while((fields[i].type & FIELD_TYPE_ONEOF) && (fields[i].oneof_first == 0));	// Search until the end of the Oneof-field, or until a new Oneof-field starts
		
	return 0;
}



/**@brief Function to serialize a structure into an output-stream.
 *
 * @param[in]	ostream		Pointer to output-stream structure.
 * @param[in]	fields		Pointer to the array of structure-fields.
 * @param[in]	src_struct	Pointer to structure that should be serialized.
 * @param[in] 	output_endianness	The desired endianness of the output-data.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations or invalid structure.
 */
uint8_t tb_encode(tb_ostream_t* ostream, const tb_field_t fields[], void* src_struct, tb_endian_t output_endianness) {
	uint8_t i = 0;
	
	while(fields[i].type != 0) {
		tb_field_t field = fields[i];
		// All these types are little endian
		if(field.type & DATA_TYPE_INT || field.type & DATA_TYPE_UINT || field.type & DATA_TYPE_FLOAT || field.type & DATA_TYPE_DOUBLE)  {
			void* data_ptr;
			data_ptr = ((uint8_t*)src_struct + field.data_offset);
			
			if(field.type & FIELD_TYPE_REQUIRED) {
				if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*) data_ptr, field.data_size, 1, output_endianness))
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
					if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*) data_ptr, field.data_size, 1, output_endianness))
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
				convert_endianness((uint8_t*)count_ptr, tmp, count_size, TB_BIG_ENDIAN);
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
				if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*)count_ptr, count_size, 1, output_endianness))
					return 0;
				
				// Now write the actual data of the array				
				if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*) data_ptr, field.data_size, count, output_endianness))
					return 0;
			} else if(field.type & FIELD_TYPE_FIXED_REPEATED) {
				// Write the actual data of the array				
				if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*) data_ptr, field.data_size, field.array_size, output_endianness))
					return 0;
			} else if (field.type & FIELD_TYPE_ONEOF) {
				void* which_ptr = ((uint8_t*) data_ptr + field.size_offset);
				uint8_t which = *((uint8_t*) which_ptr);
				
				// Only write if this is the first field of the oneof-field
				if(field.oneof_first) {
					// Check the validity of which-tag
					if(!check_oneof_which_tag_validity(fields, which, i))
						return 0;
					
					if(!tb_write_to_ostream(ostream, (uint8_t*) which_ptr, field.size_size))	// Could actually also write just 1 byte
						return 0;		
				}						
				
				if(which == field.oneof_tag) {
					// Here we assume to have a required-field type!
					if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*) data_ptr, field.data_size, 1, output_endianness))
						return 0;
				}				
			} else {
				//printf("Error no field type specified\n");
				return 0;
			}
		} else if (field.type & DATA_TYPE_MESSAGE) {
			
			
			void* struct_ptr;
			struct_ptr = ((uint8_t*)src_struct + field.data_offset);
			
			if(field.type & FIELD_TYPE_REQUIRED) {
				// Recursive call of encode function
				if(!tb_encode(ostream, (tb_field_t*) field.ptr, struct_ptr, output_endianness))
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
					if(!tb_encode(ostream, (tb_field_t*) field.ptr, struct_ptr, output_endianness))
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
				convert_endianness((uint8_t*)count_ptr, tmp, count_size, TB_BIG_ENDIAN);	
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
				
				if(!tb_write_to_ostream_big_endian(ostream, (uint8_t*)count_ptr, count_size, 1, output_endianness))
					return 0;
				
				
				for(uint32_t k = 0; k < count; k++) {
					struct_ptr = ((uint8_t*)src_struct + field.data_offset + k*field.data_size);
					// Recursive call of encode function
					if(!tb_encode(ostream, (tb_field_t*) field.ptr, struct_ptr, output_endianness))
						return 0;					
				}				
			} else if(field.type & FIELD_TYPE_FIXED_REPEATED) {
				for(uint32_t k = 0; k < field.array_size; k++) {
					struct_ptr = ((uint8_t*)src_struct + field.data_offset + k*field.data_size);
					// Recursive call of encode function
					if(!tb_encode(ostream, (tb_field_t*) field.ptr, struct_ptr, output_endianness))
						return 0;					
				}				
			} else if (field.type & FIELD_TYPE_ONEOF) {
				void* which_ptr = ((uint8_t*) struct_ptr + field.size_offset);
				uint8_t which = *((uint8_t*) which_ptr);
				
				// Only write if this is the first field of the oneof-field
				if(field.oneof_first) {
					// Check the validity of which-tag
					if(!check_oneof_which_tag_validity(fields, which, i))
						return 0;
					
					if(!tb_write_to_ostream(ostream, (uint8_t*) which_ptr, field.size_size))	// Could actually also write just 1 byte
						return 0;				
				}
				
				if(which == field.oneof_tag) {
					// Here we assume to have a required-field type!
					// Recursive call of encode function
					if(!tb_encode(ostream, (tb_field_t*) field.ptr, struct_ptr, output_endianness))
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
 * @param[in] 	input_endianness	The endianness of the input-data.
 *
 * @retval 		1			On success.
 * @retval		0			On failure, due to buffer limitations or invalid structure.
 */
uint8_t tb_decode(tb_istream_t* istream, const tb_field_t fields[], void* dst_struct, tb_endian_t input_endianness) {
	uint8_t i = 0;
	
	while(fields[i].type != 0) {		
		tb_field_t field = fields[i];
		// All these types are little endian
		if(field.type & DATA_TYPE_INT || field.type & DATA_TYPE_UINT || field.type & DATA_TYPE_FLOAT || field.type & DATA_TYPE_DOUBLE)  {
			void* data_ptr;
			data_ptr = ((uint8_t*)dst_struct + field.data_offset);
			
			if(field.type & FIELD_TYPE_REQUIRED) {
				if(!tb_read_from_istream_little_endian(istream, (uint8_t*) data_ptr, field.data_size, 1, input_endianness))
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
					if(!tb_read_from_istream_little_endian(istream, (uint8_t*) data_ptr, field.data_size, 1, input_endianness))
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
				if(!tb_read_from_istream_little_endian(istream, (uint8_t*) count_ptr, count_size, 1, input_endianness))
					return 0;
				
				uint8_t tmp[count_size];
				convert_endianness((uint8_t*)count_ptr, tmp, count_size, TB_BIG_ENDIAN);
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
				if(!tb_read_from_istream_little_endian(istream, (uint8_t*) data_ptr, field.data_size, count, input_endianness))
					return 0;
				
			} else if(field.type & FIELD_TYPE_FIXED_REPEATED) {
				// Write the actual data of the array		
				if(!tb_read_from_istream_little_endian(istream, (uint8_t*) data_ptr, field.data_size, field.array_size, input_endianness))
					return 0;
				
			} else if (field.type & FIELD_TYPE_ONEOF) {
				void* which_ptr = ((uint8_t*) data_ptr + field.size_offset);
				
				
				// Only read if this is the first field of the oneof-field
				if(field.oneof_first)
					if(!tb_read_from_istream(istream, (uint8_t*) which_ptr, field.size_size))	// Could actually also read just 1 byte
						return 0;
				
				// Here we can just read the which-entry from the structure (the field in the structure should have been written by the first-oneof field)
				uint8_t which = *((uint8_t*) which_ptr);
				
				if(field.oneof_first)
					// Check the validity of which-tag
					if(!check_oneof_which_tag_validity(fields, which, i))
						return 0;
				
				if(which == field.oneof_tag) {
					// Here we assume to have a required-field type!
					if(!tb_read_from_istream_little_endian(istream, (uint8_t*) data_ptr, field.data_size, 1, input_endianness))
						return 0;
				}				
			} else {
				//printf("Error no field type specified\n");
				return 0;
			}
		} else if (field.type & DATA_TYPE_MESSAGE) {
			
			void* struct_ptr;
			struct_ptr = ((uint8_t*)dst_struct + field.data_offset);
			
			if(field.type & FIELD_TYPE_REQUIRED) {
				// Recursive call of decode function
				if(!tb_decode(istream, (tb_field_t*) field.ptr, struct_ptr, input_endianness))
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
					if(!tb_decode(istream, (tb_field_t*) field.ptr, struct_ptr, input_endianness))
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
				if(!tb_read_from_istream_little_endian(istream, (uint8_t*) count_ptr, count_size, 1, input_endianness))
					return 0;
				
				uint8_t tmp[count_size];
				convert_endianness((uint8_t*)count_ptr, tmp, count_size, TB_BIG_ENDIAN);
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
					if(!tb_decode(istream, (tb_field_t*) field.ptr, struct_ptr, input_endianness))
						return 0;					
				}
				
			} else if(field.type & FIELD_TYPE_FIXED_REPEATED) {
				
				for(uint32_t k = 0; k < field.array_size; k++) {
					struct_ptr = ((uint8_t*)dst_struct + field.data_offset + k*field.data_size);
					// Recursive call of encode function
					if(!tb_decode(istream, (tb_field_t*) field.ptr, struct_ptr, input_endianness))
						return 0;					
				}
				
			} else if (field.type & FIELD_TYPE_ONEOF) {
				void* which_ptr = ((uint8_t*) struct_ptr + field.size_offset);
				
				// Only read if this is the first field of the oneof-field
				if(field.oneof_first)
					if(!tb_read_from_istream(istream, (uint8_t*) which_ptr, field.size_size))	// Could actually also read just 1 byte
						return 0;
				
				// Here we can just read the which-entry from the structure (the field in the structure should have been written by the first-oneof field)
				uint8_t which = *((uint8_t*) which_ptr);
				
				if(field.oneof_first)
					// Check the validity of which-tag
					if(!check_oneof_which_tag_validity(fields, which, i))
						return 0;
				
				if(which == field.oneof_tag) {
					// Here we assume to have a required-field type!
					// Recursive call of encode function
					if(!tb_decode(istream, (tb_field_t*) field.ptr, struct_ptr, input_endianness))
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




uint32_t tb_get_max_encoded_len(const tb_field_t fields[]) {
	uint8_t i = 0;
	uint32_t len = 0;
	
	while(fields[i].type != 0) {	
		tb_field_t field = fields[i];
		if(field.type & DATA_TYPE_INT || field.type & DATA_TYPE_UINT || field.type & DATA_TYPE_FLOAT || field.type & DATA_TYPE_DOUBLE)  {
			if(field.type & FIELD_TYPE_REQUIRED) {
				len += field.data_size;
			} else if(field.type & FIELD_TYPE_OPTIONAL) {
				len += field.size_size;
				len += field.data_size;
			} else if(field.type & FIELD_TYPE_REPEATED) {
				len += field.size_size;
				len += ((uint32_t) field.array_size) * field.data_size;
			} else if(field.type & FIELD_TYPE_FIXED_REPEATED) {
				len += ((uint32_t) field.array_size) * field.data_size;
			} else if (field.type & FIELD_TYPE_ONEOF) {
				// Here we need to search for the max
				len += field.size_size;	// The which field
				uint32_t tmp_len_max = 0;
				do {
					uint32_t tmp_len = 0;
					if(fields[i].type & DATA_TYPE_MESSAGE) {
						tmp_len = tb_get_max_encoded_len((tb_field_t*) fields[i].ptr);
					} else {
						tmp_len = fields[i].data_size;
					}
					if(tmp_len > tmp_len_max) {
						tmp_len_max = tmp_len;
					}
					i++;
				} while((fields[i].type & FIELD_TYPE_ONEOF) && (fields[i].oneof_first == 0));	// Search until the end of the Oneof-field, or until a new Oneof-field starts
				i--;	// Decrement one again, because we stepped one too far
				len += tmp_len_max;
			}
		} else if (field.type & DATA_TYPE_MESSAGE) {
			if(field.type & FIELD_TYPE_REQUIRED) {
				len += tb_get_max_encoded_len((tb_field_t*) field.ptr);
			} else if(field.type & FIELD_TYPE_OPTIONAL) {
				len += field.size_size;
				len += tb_get_max_encoded_len((tb_field_t*) field.ptr);				
			} else if(field.type & FIELD_TYPE_REPEATED) {
				len += field.size_size;
				len += ((uint32_t) field.array_size) * tb_get_max_encoded_len((tb_field_t*) field.ptr);				
			} else if(field.type & FIELD_TYPE_FIXED_REPEATED) {
				len += ((uint32_t) field.array_size) * tb_get_max_encoded_len((tb_field_t*) field.ptr);				
			} else if (field.type & FIELD_TYPE_ONEOF) {
				// Here we need to search for the max
				len += field.size_size;	// The which field
				uint32_t tmp_len_max = 0;
				do {
					uint32_t tmp_len = 0;
					if(field.type & DATA_TYPE_MESSAGE) {
						tmp_len = tb_get_max_encoded_len((tb_field_t*) field.ptr);
					} else {
						tmp_len = fields[i].data_size;
					}
					if(tmp_len > tmp_len_max) {
						tmp_len_max = tmp_len;
					}
					i++;
				} while((fields[i].type & FIELD_TYPE_ONEOF) && (fields[i].oneof_first == 0));	// Search until the end of the Oneof-field, or until a new Oneof-field starts
				i--;	// Decrement one again, because we stepped one too far
				len += tmp_len_max;
			}		
		} 	
		i++;
	}
	return len;
}


