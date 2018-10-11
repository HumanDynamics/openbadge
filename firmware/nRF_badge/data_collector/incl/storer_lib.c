#include "storer_lib.h"
#include "filesystem_lib.h"
#include "debug_lib.h"
#include "chunk_messages.h"
#include "tinybuf.h"
#include "string.h"	// For memset-function




#define STORER_SERIALIZED_BUFFER_SIZE				512




static uint8_t serialized_buf[STORER_SERIALIZED_BUFFER_SIZE];

static uint16_t partition_id_badge_assignement;
static uint16_t partition_id_battery_chunks;
static uint16_t partition_id_microphone_chunks;
static uint16_t partition_id_scan_chunks;
static uint16_t partition_id_accelerometer_interrupt_chunks;
static uint16_t partition_id_accelerometer_chunks;


static uint8_t microphone_chunks_found_timestamp = 0;
static uint8_t scan_chunks_found_timestamp = 0;
static uint8_t battery_chunks_found_timestamp = 0;
static uint8_t accelerometer_interrupt_chunks_found_timestamp = 0;
static uint8_t accelerometer_chunks_found_timestamp = 0;
/*


static uint16_t partition_id_accelerometer_data;
static uint16_t partition_id_accelerometer_interrupts;
*/


/**@brief Function that registers all needed partitions to the filesystem.
 *
 * @retval		NRF_SUCCESS 				If operation was successful.
 * @retval		NRF_ERROR_INVALID_PARAM		If the required-size == 0, or element_len == 0 in a static partition.
 * @retval		NRF_ERROR_NO_MEM			If there were already more than MAX_NUMBER_OF_PARTITIONS partition-registrations, or if the available size for the partition is too small.
 * @retval 		NRF_ERROR_INTERNAL			If there was an internal error (e.g. data couldn't be read because of busy).
 */
ret_code_t storer_register_partitions(void) {
	ret_code_t ret;
	/******************* BADGE ASSIGNEMENT **********************/
	uint32_t serialized_badge_assignement_len = tb_get_max_encoded_len(BadgeAssignement_fields);
	// Required size for badge_assignment
	uint32_t required_size = PARTITION_METADATA_SIZE + STORER_BADGE_ASSIGNEMENT_NUMBER*(serialized_badge_assignement_len + PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE + PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE);
	// Register a static partition with CRC for the badge-assignement	
	ret = filesystem_register_partition(&partition_id_badge_assignement, &required_size, 0, 1, serialized_badge_assignement_len);
	if(ret != NRF_SUCCESS) return ret;	
	//debug_log("Available size: %u\n", filesystem_get_available_size());
	
	/****************** BATTERY *****************************/
	uint32_t serialized_battery_data_len = tb_get_max_encoded_len(BatteryChunk_fields);
	// Required size for battery_data
	required_size = PARTITION_METADATA_SIZE + STORER_BATTERY_DATA_NUMBER*(serialized_battery_data_len + PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE);
	// Register a static partition without CRC for the battery-data	
	ret = filesystem_register_partition(&partition_id_battery_chunks, &required_size, 0, 0, serialized_battery_data_len);
	if(ret != NRF_SUCCESS) return ret;	
	//debug_log("Available size: %u\n", filesystem_get_available_size());
	
	/****************** MICROPHONE ******************************/
	uint32_t serialized_microphone_data_len = tb_get_max_encoded_len(MicrophoneChunk_fields);
	// Required size for microphone data
	required_size = PARTITION_METADATA_SIZE + STORER_MICROPHONE_DATA_NUMBER * (serialized_microphone_data_len + PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE + PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE);
	// Register a static partition with CRC for the microphone-data	
	ret = filesystem_register_partition(&partition_id_microphone_chunks, &required_size, 0, 1, serialized_microphone_data_len);
	if(ret != NRF_SUCCESS) return ret;
	//debug_log("Available size: %u\n", filesystem_get_available_size());
	
	/******************* SCAN *********************************/
	uint32_t max_serialized_scan_data_len = tb_get_max_encoded_len(ScanChunk_fields);
	// Required size for scan data
	required_size = PARTITION_METADATA_SIZE + STORER_SCAN_DATA_NUMBER * (max_serialized_scan_data_len + PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE + PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE + PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE);
	// Register a dynamic partition with CRC for the scan-data	
	ret = filesystem_register_partition(&partition_id_scan_chunks, &required_size, 1, 1, 0);
	if(ret != NRF_SUCCESS) return ret;	
	//debug_log("Available size: %u\n", filesystem_get_available_size());
	
	/****************** ACCELEROMETER INTERRUPT *****************/
	uint32_t serialized_accelerometer_interrupt_data_len = tb_get_max_encoded_len(AccelerometerInterruptChunk_fields);
	// Required size for accelerometer interrupt-data
	required_size = PARTITION_METADATA_SIZE + STORER_ACCELEROMETER_INTERRUPT_DATA_NUMBER * (serialized_accelerometer_interrupt_data_len + PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE + PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE);
	// Register a static partition with CRC for the accelerometer interrupt-data	
	ret = filesystem_register_partition(&partition_id_accelerometer_interrupt_chunks, &required_size, 0, 1, serialized_accelerometer_interrupt_data_len);
	if(ret != NRF_SUCCESS) return ret;
	//debug_log("Available size: %u\n", filesystem_get_available_size());
	
	/********************** ACCELEROMETER ***********************/
	uint32_t serialized_accelerometer_data_len = tb_get_max_encoded_len(AccelerometerChunk_fields);
	// Required size for accelerometer data
	required_size = PARTITION_METADATA_SIZE + STORER_ACCELEROMETER_DATA_NUMBER * (serialized_accelerometer_data_len + PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE + PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE);
	// Register a static partition with CRC for the accelerometer-data	
	ret = filesystem_register_partition(&partition_id_accelerometer_chunks, &required_size, 0, 1, serialized_accelerometer_data_len);
	if(ret != NRF_SUCCESS) return ret;
	debug_log("Available size: %u\n", filesystem_get_available_size());
	
	
	return NRF_SUCCESS;
}

ret_code_t storer_init(void) {
	ret_code_t ret;
	ret = filesystem_init();
	if(ret != NRF_SUCCESS) return ret;
	
	ret = storer_register_partitions();
	if(ret != NRF_SUCCESS) return ret;
	
	return NRF_SUCCESS;
}

ret_code_t storer_clear(void) {
	ret_code_t ret = filesystem_clear_partition(partition_id_badge_assignement);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = filesystem_clear_partition(partition_id_battery_chunks);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = filesystem_clear_partition(partition_id_microphone_chunks);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = filesystem_clear_partition(partition_id_scan_chunks);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = filesystem_clear_partition(partition_id_accelerometer_interrupt_chunks);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = filesystem_clear_partition(partition_id_accelerometer_chunks);
	if(ret != NRF_SUCCESS) return ret;	
	
	return ret;
}

/**@brief Function to compare two timestamps. 
 *
 * @retval	-1	timestamp1 > timestamp2
 * @retval  0	timestamp1 == timestamp2
 * @retval  1	timestamp1 < timestamp2
 */
int8_t storer_compare_timestamps(Timestamp timestamp1, Timestamp timestamp2) {
	uint64_t t1 = ((uint64_t)timestamp1.seconds)*1000 + timestamp1.ms;
	uint64_t t2 = ((uint64_t)timestamp2.seconds)*1000 + timestamp2.ms;
	return (t1 > t2) ? -1 : ((t2 > t1) ? 1 : 0);
} 



ret_code_t storer_store_badge_assignement(BadgeAssignement* badge_assignement) {
	tb_ostream_t ostream = tb_ostream_from_buffer(serialized_buf, sizeof(serialized_buf));
	uint8_t encode_status = tb_encode(&ostream, BadgeAssignement_fields, badge_assignement, TB_LITTLE_ENDIAN);
	if(!encode_status) return NRF_ERROR_INVALID_DATA;
	
	return filesystem_store_element(partition_id_badge_assignement, serialized_buf, ostream.bytes_written);
}


ret_code_t storer_read_badge_assignement(BadgeAssignement* badge_assignement) {
	memset(badge_assignement, 0, sizeof(BadgeAssignement));
	ret_code_t ret = filesystem_iterator_init(partition_id_badge_assignement); // Get the latest stored assignement
	if(ret != NRF_SUCCESS) {
		filesystem_iterator_invalidate(partition_id_badge_assignement);
		return ret;
	}
	uint16_t element_len, record_id;
	ret = filesystem_iterator_read_element(partition_id_badge_assignement, serialized_buf, &element_len, &record_id);
	filesystem_iterator_invalidate(partition_id_badge_assignement);
	
	if(ret != NRF_SUCCESS) return ret;
	
	tb_istream_t istream = tb_istream_from_buffer(serialized_buf, element_len);
	uint8_t decode_status = tb_decode(&istream, BadgeAssignement_fields, badge_assignement, TB_LITTLE_ENDIAN);
	if(!decode_status) return NRF_ERROR_INVALID_DATA;

	return NRF_SUCCESS;
}



/**@brief Function to store a chunk of data in a partition.
 *
 * @param[in]	partition_id	The partition_id where to store the chunk.
 * @param[in]	message_fields	The message fields need to encode the message-chunk with tinybuf.
 * @param[in]	message			Pointer to the message-chunk that should be encoded and stored.
 * 
 * @retval NRF_ERROR_NO_MEM			If the element is too big, to be stored in the partition.
 * @retval NRF_ERROR_INTERNAL		Busy or iterator is pointing to the same address we want to write to.
 * @retval NRF_ERROR_INVALID_DATA	If encoding fails.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t store_chunk(uint16_t partition_id, const tb_field_t message_fields[], void* message) {
	tb_ostream_t ostream = tb_ostream_from_buffer(serialized_buf, sizeof(serialized_buf));
	uint8_t encode_status = tb_encode(&ostream, message_fields, message, TB_LITTLE_ENDIAN);
	if(!encode_status) return NRF_ERROR_INVALID_DATA;

	return filesystem_store_element(partition_id, serialized_buf, ostream.bytes_written);
}


/**@brief Function to find a chunk in the partition based on its timestamp.
 *
 * @details This function is normally used in connection with get_next_chunk().
 *			It tries to step back in the partition until it finds the oldest chunk 
 *			that is still greater than the timestamp. It uses the iterator of the partition
 *			to step back.
 *
 * @param[in]	timestamp			The timestamp since when the data should be requested.
 * @param[in]	partition_id		The partition_id where to search the chunk.
 * @param[in]	message_fields		The message fields need to decode the message-chunk with tinybuf.
 * @param[out]	message				Pointer to a message-chunk (needed for internal decoding).
 * @param[out]	message_timestamp	Pointer to the timestamp entry in the message-chunk.
 * @param[out]	found_timestamp		Pointer to a flag-variable that expresses, if an "old" element with a greater timestamp was found.
 * 
 * @retval NRF_ERROR_INTERNAL		Busy
 * @retval NRF_ERROR_INVALID_STATE	Iterator invalidated/no data found.
 * @retval NRF_SUCCESS				If everything was fine.
 */
static ret_code_t find_chunk_from_timestamp(Timestamp timestamp, uint16_t partition_id, const tb_field_t message_fields[], void* message, Timestamp* message_timestamp, uint8_t* found_timestamp) {
	
	*found_timestamp = 0;
	
	ret_code_t ret = filesystem_iterator_init(partition_id);
	// If there are no data in partition --> directly return
	if(ret != NRF_SUCCESS) {
		filesystem_iterator_invalidate(partition_id);
		return ret;
	}

	
	while(1) {		
		uint16_t element_len, record_id;
		ret = filesystem_iterator_read_element(partition_id, serialized_buf, &element_len, &record_id);
		// ret could be NRF_SUCCESS, NRF_ERROR_INVALID_DATA, NRF_ERROR_INVALID_STATE, NRF_ERROR_INTERNAL
		if(!(ret == NRF_ERROR_INVALID_DATA || ret == NRF_SUCCESS)) {
			filesystem_iterator_invalidate(partition_id);
			return ret;
		}

		if(ret == NRF_SUCCESS) { // Only try to decode when the data are not corrupted (NRF_ERROR_INVALID_DATA)

			// Decode the current element
			tb_istream_t istream = tb_istream_from_buffer(serialized_buf, element_len);
			uint8_t decode_status = tb_decode(&istream, message_fields, message, TB_LITTLE_ENDIAN);
			if(decode_status) {	
				if(storer_compare_timestamps(*message_timestamp, timestamp) == 1) {
					// We have found the timestamp --> we need to go to the next again
					ret = filesystem_iterator_next(partition_id);
					// ret could be NRF_SUCCESS, NRF_ERROR_NOT_FOUND, NRF_ERROR_INVALID_STATE, NRF_ERROR_INTERNAL
					if(!(ret == NRF_ERROR_NOT_FOUND || ret == NRF_SUCCESS)) {
						filesystem_iterator_invalidate(partition_id);
						return ret;
					}
					// ret could be NRF_SUCCESS, NRF_ERROR_NOT_FOUND	
					if(ret == NRF_SUCCESS) {
						*found_timestamp = 1;
					} else { // If we have not found a "next" element (because the current one is the latest), we haven't a valid timestamp
						// So if the storer_get_next_..._data()-function is called,
						// it tries directly to move to the next-element and it will return NRF_ERROR_NOT_FOUND (except new data has been written since then)
						*found_timestamp = 0;
					}
					// But we want to return NRF_SUCCESS when we have just not found the next-element
					ret = (ret == NRF_ERROR_NOT_FOUND) ? NRF_SUCCESS : ret;					
					break;
				}
			}
		}
		// Otherwise go to the previous except there is no previous element any more
		ret = filesystem_iterator_previous(partition_id);
		// ret could be NRF_SUCCESS, NRF_ERROR_NOT_FOUND, NRF_ERROR_INVALID_STATE, NRF_ERROR_INTERNAL
		if(!(ret == NRF_ERROR_NOT_FOUND || ret == NRF_SUCCESS)) {
			filesystem_iterator_invalidate(partition_id);
			return ret;
		}
		// ret could be NRF_SUCCESS, NRF_ERROR_NOT_FOUND
		if(ret == NRF_ERROR_NOT_FOUND) {
			// We have reached the end of the partition --> stop searching
			*found_timestamp = 1;
			ret = NRF_SUCCESS;
			break;
		}
	}	
	return ret;	// ret should be NRF_SUCCESS
}


/**@brief Function to get the next chunk in the partition based on the current status of the partition-iterator.
 *
 * @details This function is normally called after find_chunk_from_timestamp(). It tries to get the next element stored in partition
 *			and decodes it with the message_fields.
 *			
 *
 * @param[in]	partition_id		The partition_id where to search the chunk.
 * @param[in]	message_fields		The message fields need to decode the message-chunk with tinybuf.
 * @param[out]	message				Pointer to a message-chunk where to store the read chunk.
 * @param[in]	found_timestamp		Pointer to a flag-variable that expresses, if an "old" element with a greater timestamp was found.
 * 
 * @retval	NRF_SUCCESS					If an element was found and returned successfully.
 * @retval	NRF_ERROR_NOT_FOUND			If no more element in the partition.
 * @retval	NRF_ERROR_INVALID_STATE		If iterator not initialized or invalidated.
 * @retval	NRF_ERROR_INTERNAL			If busy.
 *
 * @note 	The application needs to invalidate the iterator, if the function is not used until no more element is found (because it invalidates it then automatically).
 */
static ret_code_t get_next_chunk(uint16_t partition_id, const tb_field_t message_fields[], void* message, uint8_t* found_timestamp) {
	ret_code_t ret;
	uint16_t element_len, record_id;
	// do-while-loop to ignore invalid data
	do {
		// Skip step to the next iterator element if found_timestamp is true
		if(!(*found_timestamp)) {
			ret = filesystem_iterator_next(partition_id);
			// ret could be NRF_SUCCESS, NRF_ERROR_NOT_FOUND, NRF_ERROR_INVALID_STATE, NRF_ERROR_INTERNAL
			if(ret != NRF_SUCCESS) {
				filesystem_iterator_invalidate(partition_id);
				return ret;
			}
		} 
		*found_timestamp = 0;	
		
		// TODO: What happens if read failed, but we have already done a next-step successfully?
		ret = filesystem_iterator_read_element(partition_id, serialized_buf, &element_len, &record_id);
		// ret could be NRF_SUCCESS, NRF_ERROR_INVALID_DATA, NRF_ERROR_INVALID_STATE, NRF_ERROR_INTERNAL
		if(!(ret == NRF_ERROR_INVALID_DATA || ret == NRF_SUCCESS)) {
			filesystem_iterator_invalidate(partition_id);
			return ret;
		}
		// ret could be NRF_SUCCESS, NRF_ERROR_INVALID_DATA
		if(ret == NRF_SUCCESS) { // Only decode if no invalid data
			// Now decode it
			tb_istream_t istream = tb_istream_from_buffer(serialized_buf, element_len);
			uint8_t decode_status = tb_decode(&istream, message_fields, message, TB_LITTLE_ENDIAN);
			if(!decode_status) ret = NRF_ERROR_INVALID_DATA;	// Set to invalid data, to proceed in the while-loop
		}
	} while(ret == NRF_ERROR_INVALID_DATA);
	
	return ret; // Should actually always be NRF_SUCCESS
}



void storer_invalidate_iterators(void) {
	filesystem_iterator_invalidate(partition_id_accelerometer_chunks);
	filesystem_iterator_invalidate(partition_id_accelerometer_interrupt_chunks);
	filesystem_iterator_invalidate(partition_id_battery_chunks);
	filesystem_iterator_invalidate(partition_id_scan_chunks);
	filesystem_iterator_invalidate(partition_id_microphone_chunks);
}


ret_code_t storer_store_accelerometer_chunk(AccelerometerChunk* accelerometer_chunk) {
	return store_chunk(partition_id_accelerometer_chunks, AccelerometerChunk_fields, accelerometer_chunk);
}

ret_code_t storer_find_accelerometer_chunk_from_timestamp(Timestamp timestamp, AccelerometerChunk* accelerometer_chunk) {
	memset(accelerometer_chunk, 0, sizeof(AccelerometerChunk));
	return find_chunk_from_timestamp(timestamp, partition_id_accelerometer_chunks, AccelerometerChunk_fields, accelerometer_chunk, &(accelerometer_chunk->timestamp), &accelerometer_chunks_found_timestamp);
}

ret_code_t storer_get_next_accelerometer_chunk(AccelerometerChunk* accelerometer_chunk) {
	memset(accelerometer_chunk, 0, sizeof(AccelerometerChunk));
	return get_next_chunk(partition_id_accelerometer_chunks, AccelerometerChunk_fields, accelerometer_chunk, &accelerometer_chunks_found_timestamp);
}



ret_code_t storer_store_accelerometer_interrupt_chunk(AccelerometerInterruptChunk* accelerometer_interrupt_chunk) {
	return store_chunk(partition_id_accelerometer_interrupt_chunks, AccelerometerInterruptChunk_fields, accelerometer_interrupt_chunk);
}

ret_code_t storer_find_accelerometer_interrupt_chunk_from_timestamp(Timestamp timestamp, AccelerometerInterruptChunk* accelerometer_interrupt_chunk) {
	memset(accelerometer_interrupt_chunk, 0, sizeof(AccelerometerInterruptChunk));
	return find_chunk_from_timestamp(timestamp, partition_id_accelerometer_interrupt_chunks, AccelerometerInterruptChunk_fields, accelerometer_interrupt_chunk, &(accelerometer_interrupt_chunk->timestamp), &accelerometer_interrupt_chunks_found_timestamp);
}

ret_code_t storer_get_next_accelerometer_interrupt_chunk(AccelerometerInterruptChunk* accelerometer_interrupt_chunk) {
	memset(accelerometer_interrupt_chunk, 0, sizeof(AccelerometerInterruptChunk));
	return get_next_chunk(partition_id_accelerometer_interrupt_chunks, AccelerometerInterruptChunk_fields, accelerometer_interrupt_chunk, &accelerometer_interrupt_chunks_found_timestamp);
}







ret_code_t storer_store_battery_chunk(BatteryChunk* battery_chunk) {
	return store_chunk(partition_id_battery_chunks, BatteryChunk_fields, battery_chunk);
}

ret_code_t storer_find_battery_chunk_from_timestamp(Timestamp timestamp, BatteryChunk* battery_chunk) {
	memset(battery_chunk, 0, sizeof(BatteryChunk));
	return find_chunk_from_timestamp(timestamp, partition_id_battery_chunks, BatteryChunk_fields, battery_chunk, &(battery_chunk->timestamp), &battery_chunks_found_timestamp);
}

ret_code_t storer_get_next_battery_chunk(BatteryChunk* battery_chunk) {
	memset(battery_chunk, 0, sizeof(BatteryChunk));
	return get_next_chunk(partition_id_battery_chunks, BatteryChunk_fields, battery_chunk, &battery_chunks_found_timestamp);
}





ret_code_t storer_store_scan_chunk(ScanChunk* scan_chunk) {
	return store_chunk(partition_id_scan_chunks, ScanChunk_fields, scan_chunk);
}

ret_code_t storer_find_scan_chunk_from_timestamp(Timestamp timestamp, ScanChunk* scan_chunk) {
	memset(scan_chunk, 0, sizeof(ScanChunk));
	return find_chunk_from_timestamp(timestamp, partition_id_scan_chunks, ScanChunk_fields, scan_chunk, &(scan_chunk->timestamp), &scan_chunks_found_timestamp);
}

ret_code_t storer_get_next_scan_chunk(ScanChunk* scan_chunk) {
	memset(scan_chunk, 0, sizeof(ScanChunk));
	return get_next_chunk(partition_id_scan_chunks, ScanChunk_fields, scan_chunk, &scan_chunks_found_timestamp);
}



ret_code_t storer_store_microphone_chunk(MicrophoneChunk* microphone_chunk) {
	return store_chunk(partition_id_microphone_chunks, MicrophoneChunk_fields, microphone_chunk);
}

ret_code_t storer_find_microphone_chunk_from_timestamp(Timestamp timestamp, MicrophoneChunk* microphone_chunk) {
	memset(microphone_chunk, 0, sizeof(MicrophoneChunk));
	return find_chunk_from_timestamp(timestamp, partition_id_microphone_chunks, MicrophoneChunk_fields, microphone_chunk, &(microphone_chunk->timestamp), &microphone_chunks_found_timestamp);
}

ret_code_t storer_get_next_microphone_chunk(MicrophoneChunk* microphone_chunk) {
	memset(microphone_chunk, 0, sizeof(MicrophoneChunk));
	return get_next_chunk(partition_id_microphone_chunks, MicrophoneChunk_fields, microphone_chunk, &microphone_chunks_found_timestamp);
}
