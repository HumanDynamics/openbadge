#include "filesystem_lib.h"
#include "storage_lib.h"


#include "stdio.h"
#include "string.h"	// For memcpy


uint16_t number_of_partitions = 0;											/**< Number of registered partitions */
partition_t				partitions			[MAX_NUMBER_OF_PARTITIONS];		/**< Array of partitions (index is referenced through "partition_id & 0x3FFF") */
partition_iterator_t 	partition_iterators	[MAX_NUMBER_OF_PARTITIONS];		/**< Array of partition-iterators (index is referenced through "partition_id & 0x3FFF") */



uint32_t next_free_address = 0; 											/**< The next free address for a new partition */



/**@brief Function for calculating CRC-16 (CRC-16 CCITT) in blocks.
 *
 * Feed each consecutive data block into this function, along with the current value of p_crc as
 * returned by the previous call of this function. The first call of this function should pass NULL
 * as the initial value of the crc in p_crc.
 *
 * @param[in] p_data The input data block for computation.
 * @param[in] size   The size of the input data block in bytes.
 * @param[in] p_crc  The previous calculated CRC-16 value or NULL if first call.
 *
 * @retval The updated CRC-16 value, based on the input supplied.
 */
uint16_t crc16_compute(uint8_t const * p_data, uint32_t size, uint16_t const * p_crc) {
    uint16_t crc = (p_crc == NULL) ? 0xFFFF : *p_crc;

    for (uint32_t i = 0; i < size; i++)
    {
        crc  = (uint8_t)(crc >> 8) | (crc << 8);
        crc ^= p_data[i];
        crc ^= (uint8_t)(crc & 0xFF) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xFF) << 4) << 1;
    }

    return crc;
}



/** @brief Function for incrementing the record_id.
 *
 * @details	This function increments the record id and take some special values into consideration.
 *			The values 0xFFFF and 0 are special values, because the default value in flash/EEPROM is 0xFFFF or 0
 *			and a search for the next/previous valid record_id could behave incorrect.
 * 
 * @param[in]	record_id		The record_id that should be incremented.
 *
 * @retval 		Incremented record_id.
 */
uint16_t increment_record_id(uint16_t record_id) {

	return (record_id >= 0xFFFE) ? (1) : (record_id + 1);
}



/** @brief Function for decrementing the record_id.
 *
 * @details	This function decrements the record id and take some special values into consideration.
 *			The values 0xFFFF and 0 are special values, because the default value in flash/EEPROM is 0xFFFF or 0
 *			and a search for the next/previous valid record_id could behave incorrect.
 * 
 * @param[in]	record_id		The record_id that should be decremented.
 *
 * @retval 		Decremented record_id.
 */
uint16_t decrement_record_id(uint16_t record_id) {

	return (record_id <= 1) ? (0xFFFE) : (record_id - 1);
}







/** @brief Function to compute the number of available bytes in storage.
 *
 * @details	This function computes the number of available bytes in storage when the partition begins at partition_start_address.
 *			The available bytes that are returned, could be smaller (if less bytes are available), bigger (if the end address is rounded to the next unit boundary) or equal to the required bytes.
 *
 * 
 * @param[in]	partition_start_address		The start address of the partition
 * @param[in]	required_size				The number of required bytes for the partition.
 * @param[out]	available_size				Pointer to memory where the number of available bytes is stored.
 *
 * @retval 		NRF_SUCCSS					If operation was successful (available_size is smaller, bigger or equal required_size).
 * @retval 		NRF_ERROR_INVALID_PARAM		If the partition_start_address >= storage-size, the required_size == 0 or partition_start_address is not unit-aligned.
 * @retval		NRF_ERROR_INTERNAL			If there was an internal error while computing (should not happen).
 */
ret_code_t filesystem_compute_available_size(uint32_t partition_start_address, uint32_t required_size, uint32_t* available_size) {
	if(partition_start_address >= storage_get_size() || required_size == 0)
		return NRF_ERROR_INVALID_PARAM;
	
	
	*available_size = storage_get_size() - partition_start_address;
	if(*available_size > required_size) {
		*available_size = required_size;
	}
	
	uint32_t start_unit_address, end_unit_address;
	ret_code_t ret = storage_get_unit_address_limits(partition_start_address, *available_size, &start_unit_address, &end_unit_address);
	if(ret != NRF_SUCCESS)	// Should actually not happen
		return NRF_ERROR_INTERNAL;
	
		
	// Check if the partition_start_address == start_unit_address. Should actually always be true, because we try always to place partition_start_address to the beginning of a unit.
	if(partition_start_address != start_unit_address)
		return NRF_ERROR_INVALID_PARAM;
	
	// Now set require_size to the unit_address boundaries
	*available_size = end_unit_address - start_unit_address + 1;

	return NRF_SUCCESS;
}


/** @brief Function to retrieve the corresponding first_element_header-address in the swap page.
 *
 * @param[in]	partition_id		The identifier of the partition.
 *
 * @retval	Address of the first_element_header-address of the specified partition in the swap page.
 */
uint32_t filesystem_get_swap_page_address_of_partition(uint16_t partition_id) {
	uint32_t address = 0;
	// Clear all the MSBs
	uint16_t index = partition_id & (0x3FFF);
	
	address = index * (PARTITION_METADATA_SIZE + PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE + PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE + PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE);
	
	return address;
}


/** @brief Function to backup a first element-header in the swap-page.
 *
 * @details This function backups the first-element header in the swap-page. To minimze the number of store operations,
 *			the header is only stored if it differs from the currently header in the swap-page. 
 *			The function also checks if the store operation was successful by reading and comparing the stored data.
 *
 * @param[in]	first_element_address_swap_page		The address where to store the first element-header in the swap page.
 * @param[in]	serialized_first_element_header		The serialized bytes of the first element-header.
 * @param[in]	first_element_header_len			The number of bytes of the first element-header.
 *
 * @retval     NRF_SUCCESS        		If the backup-operation was successfully.
 * @retval     NRF_ERROR_INVALID_PARAM  If the specified address is outside the swap-page.
 * @retval     NRF_ERROR_INTERNAL   	If there was an internal error (e.g. the data couldn't be stored because of busy, or the data weren't stored correctly).
 */
ret_code_t filesystem_backup_first_element_header(uint32_t first_element_address_swap_page, uint8_t* serialized_first_element_header, uint16_t first_element_header_len) {
	
	ret_code_t ret;
	
	uint8_t tmp[first_element_header_len];
	
	// Check if address is in swap page
	if(first_element_address_swap_page + first_element_header_len > SWAP_PAGE_SIZE)
		return NRF_ERROR_INVALID_PARAM;
	
	
	// Read metadata and element-header from swap page
	ret = storage_read(first_element_address_swap_page, &tmp[0], first_element_header_len);
	if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
	
	
	// Compare
	if(memcmp(serialized_first_element_header, tmp, first_element_header_len) != 0) {
		// The entries in swap page and storage mismatch --> backup
		
		// Check which unit size is in the swap page (to determine if swap page is in flash or eeprom)
		uint32_t first_element_swap_page_start_unit_address, first_element_swap_page_end_unit_address;
		ret = storage_get_unit_address_limits(first_element_address_swap_page, first_element_header_len, &first_element_swap_page_start_unit_address, &first_element_swap_page_end_unit_address);
		if(ret != NRF_SUCCESS)	return NRF_ERROR_INTERNAL;
		
		// Compute the unit size
		uint32_t unit_size = first_element_swap_page_end_unit_address + 1 - first_element_swap_page_start_unit_address;
		
		uint8_t tmp_swap_page[unit_size];
		
		// Read the unit from the swap page
		ret = storage_read(first_element_swap_page_start_unit_address, &tmp_swap_page[0], unit_size);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		
		// Replace the entry
		memcpy(&tmp_swap_page[first_element_address_swap_page - first_element_swap_page_start_unit_address], serialized_first_element_header, first_element_header_len);
			
		// Store the unit to the swap page again
		ret = storage_store(first_element_swap_page_start_unit_address, tmp_swap_page, unit_size);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		
		// Check if write was successful
		uint8_t tmp_read[sizeof(tmp_swap_page)];
		ret = storage_read(first_element_swap_page_start_unit_address, &tmp_read[0], sizeof(tmp_swap_page));
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;				
		if(memcmp(tmp_swap_page, tmp_read, sizeof(tmp_swap_page)) != 0)	return NRF_ERROR_INTERNAL;
		
	}
	return NRF_SUCCESS;
}


/** @brief Function to retrieve the element-header length.
 *
 * @details	The length of the element-header depends on whether the partition is dynamic or static
 *			and whether CRC should be used or not.
 *
 * @param[in]	partition_id		The identifier of the partition.
 *
 * @retval	The length of the element-header.
 */
uint16_t filesystem_get_element_header_len(uint16_t partition_id) {
	
	uint8_t is_dynamic		= (partition_id & 0x8000) ? 1 : 0;
	uint8_t has_element_crc = (partition_id & 0x4000) ? 1 : 0;
	
	
	
	uint16_t element_header_len = PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE;
	
	if(is_dynamic)
		element_header_len += PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE;
	
	if(has_element_crc)
		element_header_len += PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE;
	
	return element_header_len;
}




/** @brief Function to efficiently serialize a metadata-struct into bytes.
 *
 * @warning The output buffer "serialized" must have a size of at least PARTITION_METADATA_SIZE bytes.
 *
 * @param[in]	metadata		Pointer to the metadata that should be serialized.
 * @param[out]	serialized		Pointer to buffer where the serialized metadata should be stored to.
 */
void filesystem_serialize_metadata(const partition_metadata_t* metadata, uint8_t* serialized) {
	uint8_t tmp[PARTITION_METADATA_SIZE];
	tmp[0] = (metadata->header_crc >> 8) & 0xFF;
	tmp[1] = (metadata->header_crc) & 0xFF;
	tmp[2] = (metadata->partition_id >> 8) & 0xFF;
	tmp[3] = (metadata->partition_id) & 0xFF;
	tmp[4] = (metadata->partition_size >> 24) & 0xFF;
	tmp[5] = (metadata->partition_size >> 16) & 0xFF;
	tmp[6] = (metadata->partition_size >> 8) & 0xFF;
	tmp[7] = (metadata->partition_size) & 0xFF;
	tmp[8] = (metadata->first_element_len >> 8) & 0xFF;
	tmp[9] = (metadata->first_element_len) & 0xFF;
	tmp[10] = (metadata->last_element_address >> 24) & 0xFF;
	tmp[11] = (metadata->last_element_address >> 16) & 0xFF;
	tmp[12] = (metadata->last_element_address >> 8) & 0xFF;
	tmp[13] = (metadata->last_element_address) & 0xFF;	
	memcpy(serialized, (uint8_t*) tmp, sizeof(tmp));
}

/** @brief Function to deserialize bytes into a metadata-struct.
 *
 * @param[in]	serialized		Pointer to buffer that contains the serialized metadata.
 * @param[out]	metadata		Pointer to a metadata-struct that should be filled.
 */
void filesystem_deserialize_metadata(const uint8_t* serialized, partition_metadata_t* metadata) {
	uint8_t tmp[PARTITION_METADATA_SIZE];
	memcpy((uint8_t*) tmp, serialized, sizeof(tmp));
	
	metadata->header_crc = (((uint16_t)tmp[0]) << 8) | tmp[1];
	metadata->partition_id = (((uint16_t)tmp[2]) << 8) | tmp[3];
	metadata->partition_size = (((uint32_t)tmp[4]) << 24) | (((uint32_t)tmp[5]) << 16)  | (((uint32_t)tmp[6]) << 8) | tmp[7];
	metadata->first_element_len = (((uint16_t)tmp[8]) << 8) | tmp[9];
	metadata->last_element_address = (((uint32_t)tmp[10]) << 24) | (((uint32_t)tmp[11]) << 16)  | (((uint32_t)tmp[12]) << 8) | tmp[13];
}


/** @brief Function to efficiently serialize an element-header-struct into bytes.
 *
 * @warning The output buffer "serialized" must have a size of at least "filesystem_get_element_header_len(partition_id)" bytes.
 *
 * @param[in]	element_header	Pointer to the metadata that should be serialized.
 * @param[out]	serialized		Pointer to buffer where the serialized metadata should be stored to.
 */
void filesystem_serialize_element_header(uint16_t partition_id, const partition_element_header_t* element_header, uint8_t* serialized) {
	uint16_t header_len = filesystem_get_element_header_len(partition_id);
	
	uint8_t is_dynamic		= (partition_id & 0x8000) ? 1 : 0;
	uint8_t has_element_crc = (partition_id & 0x4000) ? 1 : 0;
	
	uint8_t tmp[header_len];
	
	tmp[0] = (element_header->record_id >> 8) & 0xFF;
	tmp[1] = (element_header->record_id) & 0xFF;
	
	if(is_dynamic) {
		tmp[2] = (element_header->previous_len_XOR_cur_len >> 8) & 0xFF;
		tmp[3] = (element_header->previous_len_XOR_cur_len) & 0xFF;
	}
	
	if(has_element_crc) {
		tmp[2 + is_dynamic*2] = (element_header->element_crc >> 8) & 0xFF;
		tmp[3 + is_dynamic*2] = (element_header->element_crc) & 0xFF;
	}
	
	memcpy(serialized, (uint8_t*) tmp, sizeof(tmp));
}

/** @brief Function to deserialize bytes into a element-header-struct.
 *
 * @param[in]	serialized		Pointer to buffer that contains the serialized element-header.
 * @param[out]	element_header	Pointer to a element-header-struct that should be filled.
 */
void filesystem_deserialize_element_header(uint16_t partition_id, const uint8_t* serialized, partition_element_header_t* element_header) {
	uint16_t header_len = filesystem_get_element_header_len(partition_id);
	
	uint8_t is_dynamic		= (partition_id & 0x8000) ? 1 : 0;
	uint8_t has_element_crc = (partition_id & 0x4000) ? 1 : 0;
	
	uint8_t tmp[header_len];
	
	memcpy((uint8_t*) tmp, serialized, sizeof(tmp));
	element_header->record_id = ((uint16_t)tmp[0]) << 8 | tmp[1];
	
	if(is_dynamic) {
		element_header->previous_len_XOR_cur_len = ((uint16_t)tmp[2]) << 8 | tmp[3];
	} else {
		element_header->previous_len_XOR_cur_len = 0;
	}
	
	if(has_element_crc) {
		element_header->element_crc = ((uint16_t)tmp[2 + is_dynamic*2]) << 8 | tmp[3 + is_dynamic*2];
	} else {
		element_header->element_crc = 0;
	}
}



/** @brief Function to read a element-header from storage.
 *
 * @details The function reads a element-header (record_id, element_crc, previous_len_XOR_cur_len) from storage at a given address.
 *			If the header at the first address of the partition should be read, the function tries to read the header from the first address. 
 *			If the metadata + element-header is not valid (checked by header-crc), the function tries to read the first element-header from the swap-page.
 *			If the entry in the swap-page is valid, it is copied to the storage (to the first address of the partition).
 *			Otherwise if the entry in the swap-page is not valid, NRF_ERROR_NOT_FOUND is returned.
 *			
 *
 * @param[in]	partition_id				The identifier of the partition.
 * @param[in]	element_address				The address of the element-header.
 * @param[out]	record_id					The record-id of the element.
 * @param[out]	element_crc					The CRC of the element-data.
 * @param[out]	previous_len_XOR_cur_len	The XOR of the previous- and the current-element length.
 *
 * @retval     NRF_SUCCESS        	If the element-header read operation was successfully.
 * @retval     NRF_ERROR_NOT_FOUND  If the function couldn't find any valid first-element-header.
 * @retval     NRF_ERROR_INTERNAL   If there was an internal error (e.g. the data couldn't be read because of busy).
 */
ret_code_t filesystem_read_element_header(uint16_t partition_id, uint32_t element_address, uint16_t* record_id, uint16_t* element_crc, uint16_t* previous_len_XOR_cur_len) {
	uint16_t index = partition_id & 0x3FFF;	// Clear the MSBs
	uint8_t is_dynamic		= (partition_id & 0x8000) ? 1 : 0;
	
	ret_code_t ret;

	
	uint32_t partition_start_address = partitions[index].first_element_address;
	
	partition_element_header_t 	element_header;
	uint16_t element_header_len = filesystem_get_element_header_len(partition_id);
	
	// Check if the application tries to read the element-header of the first element or a normal element-header
	if(element_address == partition_start_address) {
		
		partition_metadata_t metadata;
		
		
		uint8_t tmp[PARTITION_METADATA_SIZE + element_header_len];
		
		// Read metadata
		ret = storage_read(element_address, &tmp[0], PARTITION_METADATA_SIZE);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		filesystem_deserialize_metadata(&tmp[0], &metadata);
		
		// Read element header
		ret = storage_read(element_address + PARTITION_METADATA_SIZE, &tmp[PARTITION_METADATA_SIZE], element_header_len);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		filesystem_deserialize_element_header(partition_id, &tmp[PARTITION_METADATA_SIZE], &element_header);
		
		// Check the header crc
		uint16_t header_crc = crc16_compute(&tmp[2], PARTITION_METADATA_SIZE + element_header_len - 2, NULL);
		if(header_crc == metadata.header_crc) {
			// Check if the metadata are consistent with the application metadata for the partition (if static --> element-length must be equal)
			if(metadata.partition_id == partitions[index].metadata.partition_id && metadata.partition_size == partitions[index].metadata.partition_size && (is_dynamic || metadata.first_element_len == partitions[index].metadata.first_element_len)) {
				partitions[index].metadata = metadata;
				*record_id = element_header.record_id;
				*element_crc = element_header.element_crc;
				*previous_len_XOR_cur_len = element_header.previous_len_XOR_cur_len;
				return NRF_SUCCESS;
			}
			return NRF_ERROR_NOT_FOUND;
		}
		
		// The header/metadata in storage is not correct --> check in swap page
		uint32_t first_element_address_swap_page	=  filesystem_get_swap_page_address_of_partition(partition_id);
		
		// Read metadata from swap page
		ret = storage_read(first_element_address_swap_page, &tmp[0], PARTITION_METADATA_SIZE);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		filesystem_deserialize_metadata(&tmp[0], &metadata);
		
		// Read element header from swap page
		ret = storage_read(first_element_address_swap_page + PARTITION_METADATA_SIZE, &tmp[PARTITION_METADATA_SIZE], element_header_len);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		filesystem_deserialize_element_header(partition_id, &tmp[PARTITION_METADATA_SIZE], &element_header);
		
		// Check the header crc of swap page
		header_crc = crc16_compute(&tmp[2], PARTITION_METADATA_SIZE + element_header_len - 2, NULL);
		if(header_crc == metadata.header_crc) {
			// Check if the metadata are consistent with the application metadata for the partition
			if(metadata.partition_id == partitions[index].metadata.partition_id && metadata.partition_size == partitions[index].metadata.partition_size && (is_dynamic || metadata.first_element_len == partitions[index].metadata.first_element_len)) {
				
				// Copy the metadata and element header to the storage
				ret = storage_store(element_address, &tmp[0], PARTITION_METADATA_SIZE + element_header_len);
				if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
				
				// Check if the metadata + header was written successful
				uint8_t tmp_read[sizeof(tmp)];
				ret = storage_read(element_address, &tmp_read[0], PARTITION_METADATA_SIZE + element_header_len);
				if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;				
				if(memcmp(tmp, tmp_read, sizeof(tmp)) != 0)	return NRF_ERROR_INTERNAL;
				
				partitions[index].metadata = metadata;
				*record_id = element_header.record_id;
				*element_crc = element_header.element_crc;
				*previous_len_XOR_cur_len = element_header.previous_len_XOR_cur_len;
				return NRF_SUCCESS;
			}
			return NRF_ERROR_NOT_FOUND;		
		}
		return NRF_ERROR_NOT_FOUND;
	} else {
		
		// It is a normal element
		uint8_t tmp[element_header_len];
		
		// Read element header
		ret = storage_read(element_address, &tmp[0], element_header_len);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		filesystem_deserialize_element_header(partition_id, &tmp[0], &element_header);
		
		*record_id = element_header.record_id;
		*element_crc = element_header.element_crc;
		*previous_len_XOR_cur_len = element_header.previous_len_XOR_cur_len;
		return NRF_SUCCESS;
	}	
}

/** @brief Function to read the next element-header from storage.
 *
 * @details The function computes the address of the next element and tries to read the element-header at this address.
 *			If the record-id of the next element-header is not consistent with the record-id of the current element-header, NRF_ERROR_NOT_FOUND is returned.
 *			
 *
 * @param[in]	partition_id							The identifier of the partition.
 * @param[in]	cur_element_address						The address of the current element-header.
 * @param[in]	cur_element_record_id					The record-id of the current element.
 * @param[out]	next_element_address					The address of the next element-header.
 * @param[out]	next_element_record_id					The record-id of the next element.
 * @param[out]	next_element_crc						The CRC of the next element-data.
 * @param[out]	next_element_previous_len_XOR_cur_len	The XOR of next previous- and the current-element length.
 *
 * @retval     NRF_SUCCESS        	If the element-header read operation was successfully.
 * @retval     NRF_ERROR_NOT_FOUND  If the function couldn't find a valid next element-header.
 * @retval     NRF_ERROR_INTERNAL   If there was an internal error (e.g. the data couldn't be read because of busy).
 */
ret_code_t filesystem_get_next_element_header(uint16_t partition_id, uint32_t cur_element_address, uint16_t cur_element_record_id, uint16_t cur_element_len, uint32_t* next_element_address, uint16_t* next_element_record_id, uint16_t* next_element_crc, uint16_t* next_element_previous_len_XOR_cur_len) {
	uint16_t index = partition_id & 0x3FFF;	// Clear the MSBs
	uint8_t is_dynamic = (partition_id & 0x8000) ? 1 : 0;
	
	
	ret_code_t ret;
	
	
	uint32_t partition_start_address 	= partitions[index].first_element_address;
	uint32_t partition_size				= partitions[index].metadata.partition_size;
	uint32_t latest_element_record_id	= partitions[index].latest_element_record_id;	// The record id of the latest element. To stop when there are probably some old entries that are consecutive..
	
	// Check if we have already reached the latest element --> return
	if(cur_element_record_id == latest_element_record_id) {
		return NRF_ERROR_NOT_FOUND;
	}
	
	
	uint16_t element_header_len = filesystem_get_element_header_len(partition_id);
	
	
	uint32_t cur_header_len 	= (cur_element_address == partition_start_address) ? (PARTITION_METADATA_SIZE + element_header_len) : (element_header_len);

	
	*next_element_address = cur_element_address + cur_header_len + cur_element_len;
	
	uint16_t record_id, element_crc, previous_len_XOR_cur_len;
	
	
	ret = filesystem_read_element_header(partition_id, *next_element_address, &record_id, &element_crc, &previous_len_XOR_cur_len);
	if(ret != NRF_SUCCESS)
		return ret;
	
		
	// Check the record id of the "next element"
	if(record_id == increment_record_id(cur_element_record_id)) {
		
		*next_element_record_id = record_id;
		*next_element_crc = element_crc;
		*next_element_previous_len_XOR_cur_len = previous_len_XOR_cur_len;
		
		uint32_t next_element_len 			= 0;
		if(is_dynamic)
			next_element_len = cur_element_len ^ previous_len_XOR_cur_len; // Compute the next element length by XOR
		else
			next_element_len = partitions[index].metadata.first_element_len;
		
		// Check if the next element is in the partition boundaries
		if(*next_element_address + element_header_len + next_element_len <= partition_start_address + partition_size) {
			return NRF_SUCCESS;
		}
		
	}
	
	// Otherwise check if the first element is the next element:
	*next_element_address 	= partition_start_address;
	
	ret = filesystem_read_element_header(partition_id, *next_element_address, &record_id, &element_crc, &previous_len_XOR_cur_len);
	if(ret != NRF_SUCCESS)
		return ret;
	
	// Check the record id	--> if only one element in partition, this won't be true
	if(record_id == increment_record_id(cur_element_record_id)) {
		
		*next_element_record_id = record_id;
		*next_element_crc = element_crc;
		*next_element_previous_len_XOR_cur_len = previous_len_XOR_cur_len;
		
		return NRF_SUCCESS;
	} 
	return NRF_ERROR_NOT_FOUND;
}



/** @brief Function to read the previous element-header from storage.
 *
 * @details The function computes the address of the previous element and tries to read the element-header at this address.
 *			If the record-id of the previous element-header is not consistent with the record-id of the current element-header, NRF_ERROR_NOT_FOUND is returned.
 *			
 *
 * @param[in]	partition_id								The identifier of the partition.
 * @param[in]	cur_element_address							The address of the current element-header.
 * @param[in]	cur_element_record_id						The record-id of the current element.
 * @param[out]	previous_element_address					The address of the previous element-header.
 * @param[out]	previous_element_record_id					The record-id of the previous element.
 * @param[out]	previous_element_crc						The CRC of the previous element-data.
 * @param[out]	previous_element_previous_len_XOR_cur_len	The XOR of previous previous- and the current-element length.
 *
 * @retval     NRF_SUCCESS        	If the element-header read operation was successfully.
 * @retval     NRF_ERROR_NOT_FOUND  If the function couldn't find a valid previous element-header.
 * @retval     NRF_ERROR_INTERNAL   If there was an internal error (e.g. the data couldn't be read because of busy).
 */
ret_code_t filesystem_get_previous_element_header(uint16_t partition_id, uint32_t cur_element_address, uint16_t cur_element_record_id, uint16_t cur_element_len, uint32_t* previous_element_address, uint16_t* previous_element_record_id, uint16_t* previous_element_crc, uint16_t* previous_element_previous_len_XOR_cur_len) {
	uint16_t index = partition_id & 0x3FFF;
	uint8_t is_dynamic = (partition_id & 0x8000) ? 1 : 0;
	
	ret_code_t ret;
	
	
	uint32_t partition_start_address = partitions[index].first_element_address;
	uint32_t last_element_address	 = partitions[index].metadata.last_element_address;
	
	
	uint16_t element_header_len = filesystem_get_element_header_len(partition_id);
	
	
	uint16_t record_id, element_crc, previous_len_XOR_cur_len;
	
	// Check if the cur-element is the first element
	if(cur_element_address <= partition_start_address) {
		// Check if there is a valid last element address
		if(last_element_address <= partition_start_address) {
			return NRF_ERROR_NOT_FOUND;
		}
		
		*previous_element_address = last_element_address;
		
		// Read the previous (here: last) element
		ret = filesystem_read_element_header(partition_id, *previous_element_address, &record_id, &element_crc, &previous_len_XOR_cur_len);
		if(ret != NRF_SUCCESS)
			return ret;
		
		// Check the record id	
		if(record_id == decrement_record_id(cur_element_record_id)) {
			*previous_element_record_id = record_id;
			*previous_element_crc = element_crc;
			*previous_element_previous_len_XOR_cur_len = previous_len_XOR_cur_len;
			return NRF_SUCCESS;
		}
		return NRF_ERROR_NOT_FOUND;
	}
	
	// Otherwise the previous element is not the last element
	
	
	// Read the current element to compute the previous length
	ret = filesystem_read_element_header(partition_id, cur_element_address, &record_id, &element_crc, &previous_len_XOR_cur_len);
	if(ret != NRF_SUCCESS)
		return ret;
	
	uint32_t previous_element_len = 0; 
	if(is_dynamic)
		previous_element_len = cur_element_len ^ previous_len_XOR_cur_len; // Compute the previous element length by XOR
	else
		previous_element_len = partitions[index].metadata.first_element_len;
	
	// Compute the previous_element_address
	if(cur_element_address <= partition_start_address + (PARTITION_METADATA_SIZE + element_header_len) + previous_element_len) {
		*previous_element_address = partition_start_address;
	} else {	// It is a normal element (not first element)
		*previous_element_address = cur_element_address - previous_element_len - element_header_len;
	}
		
	// Read the previous_element_address
	ret = filesystem_read_element_header(partition_id, *previous_element_address, &record_id, &element_crc, &previous_len_XOR_cur_len);
	if(ret != NRF_SUCCESS)
		return ret;
	
	
	// Check the record id	
	if(record_id == decrement_record_id(cur_element_record_id)) {
		
		*previous_element_record_id = record_id;
		*previous_element_crc = element_crc;
		*previous_element_previous_len_XOR_cur_len = previous_len_XOR_cur_len;
		
		return NRF_SUCCESS;
	}

	return NRF_ERROR_NOT_FOUND;
}

/** @brief Function to find the address of the latest element of a partition.
 *
 * @details The function tries to find the latest stored element by stepping through the next element-header and checking the assigned record-ids.
 *			
 *
 * @param[in]	partition_id				The identifier of the partition.
 * @param[out]	latest_element_address		The address of the latest element-header.
 * @param[out]	latest_element_record_id	The record-id of the latest element-header.
 * @param[out]	latest_element_len			The length of the latest element.
 *
 * @retval     NRF_SUCCESS        	If the latest element find operation was successfully.
 * @retval     NRF_ERROR_NOT_FOUND  If the function couldn't find any valid element-header.
 * @retval     NRF_ERROR_INTERNAL   If there was an internal error (e.g. the data couldn't be read because of busy).
 */
ret_code_t filesystem_find_latest_element_address(uint16_t partition_id, uint32_t* latest_element_address, uint16_t* latest_element_record_id, uint16_t* latest_element_len) {

	uint16_t index = partition_id & 0x3FFF;
	uint8_t is_dynamic = (partition_id & 0x8000) ? 1 : 0;
	
	
	ret_code_t ret;
	
	uint32_t partition_start_address 	= partitions[index].first_element_address;
	uint32_t first_element_len			= partitions[index].metadata.first_element_len;

	*latest_element_address = partition_start_address;
	
	
	uint16_t record_id, element_crc, previous_len_XOR_cur_len;
	uint32_t cur_element_address = partition_start_address;
	// Read the first element, for the initial record_id
	ret = filesystem_read_element_header(partition_id, cur_element_address, &record_id, &element_crc, &previous_len_XOR_cur_len);
	if(ret != NRF_SUCCESS)	// ret could be NRF_SUCCESS, NRF_ERROR_NOT_FOUND, NRF_ERROR_INTERNAL 
		return ret;
	
	
	uint32_t cur_element_len = first_element_len;

	
	
	uint32_t next_element_address;
	uint16_t next_element_record_id;
	ret = filesystem_get_next_element_header(partition_id, cur_element_address, record_id, cur_element_len, &next_element_address, &next_element_record_id, &element_crc, &previous_len_XOR_cur_len);
	
	// Search until there is no element
	while(ret == NRF_SUCCESS) {
		if(next_element_address <= cur_element_address)
			break;		
		
		if(is_dynamic)
			cur_element_len = cur_element_len ^ previous_len_XOR_cur_len;
		else
			cur_element_len = first_element_len;
		
		cur_element_address = next_element_address;
		record_id = next_element_record_id;
		
		
		ret = filesystem_get_next_element_header(partition_id, cur_element_address, record_id, cur_element_len, &next_element_address, &next_element_record_id, &element_crc, &previous_len_XOR_cur_len);
	}
	*latest_element_address = cur_element_address;
	*latest_element_record_id = record_id;
	*latest_element_len = cur_element_len;
	
	if(ret != NRF_SUCCESS && ret != NRF_ERROR_NOT_FOUND)
		return ret;
	
	return NRF_SUCCESS;
}







ret_code_t filesystem_init(void) {
	ret_code_t ret = storage_init();
	
	if(ret != NRF_SUCCESS)
		return ret;
	
	ret = filesystem_reset();
	
	
	return ret;
}

ret_code_t filesystem_reset(void) {
	number_of_partitions = 0;
	// Compute the address area for the swap page
	uint32_t start_unit_address, end_unit_address;
	ret_code_t ret = storage_get_unit_address_limits(0, SWAP_PAGE_SIZE, &start_unit_address, &end_unit_address);
	if(ret != NRF_SUCCESS)	// Should actually not happen
		return NRF_ERROR_INTERNAL;
	
	// Set the next free address to the address after the swap page
	next_free_address = end_unit_address + 1;

	return NRF_SUCCESS;
} 


ret_code_t filesystem_clear(void) {
	
	uint32_t length = storage_get_size();
	ret_code_t ret = storage_clear(0, length);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = filesystem_reset();
	return ret;
}

uint32_t filesystem_get_available_size(void) {
	uint32_t storage_size = storage_get_size();
	if(storage_size >= next_free_address) {
		return storage_size - next_free_address;
	}
	return 0;
}



ret_code_t filesystem_register_partition(uint16_t* partition_id, uint32_t* required_size, uint8_t is_dynamic, uint8_t enable_crc, uint16_t element_len) {
	if(number_of_partitions >= MAX_NUMBER_OF_PARTITIONS) 
		return NRF_ERROR_NO_MEM;
	
	// Check if we have element length > 0 in static partitions
	if(!is_dynamic && element_len == 0) 
		return NRF_ERROR_INVALID_PARAM;
	
	if(is_dynamic)	
		element_len = 0;
	
	uint32_t partition_start_address = next_free_address; 
	
	// Compute the size that is available in storage
	uint32_t available_size = 0;
	ret_code_t ret = filesystem_compute_available_size(partition_start_address, *required_size, &available_size);
	if(ret != NRF_SUCCESS) return NRF_ERROR_NO_MEM;

	*partition_id = number_of_partitions;
	if(is_dynamic)
		*partition_id |= 0x8000;	
	if(enable_crc)
		*partition_id |= 0x4000;	
	
	
	partitions[number_of_partitions].has_first_element 			= 0;
	partitions[number_of_partitions].first_element_address		= partition_start_address;
	partitions[number_of_partitions].latest_element_address		= partition_start_address;
	partitions[number_of_partitions].latest_element_record_id	= 1;
	partitions[number_of_partitions].latest_element_len			= 0;
	
	partitions[number_of_partitions].metadata.partition_id			= *partition_id;
	partitions[number_of_partitions].metadata.partition_size		= available_size;
	partitions[number_of_partitions].metadata.first_element_len		= element_len;
	partitions[number_of_partitions].metadata.last_element_address	= partition_start_address;
	
	
	// Set require_size to the available bytes
	*required_size = available_size;
	
	uint16_t element_header_len = filesystem_get_element_header_len(*partition_id);
	
	
	
	// Check if we have enough space for at least the first element header and data (in dynamic case, this check will only check if we can store the first-element header).
	if(available_size < (uint32_t)(PARTITION_METADATA_SIZE + element_header_len + element_len) )
		return NRF_ERROR_NO_MEM;
	
	
	uint16_t record_id, element_crc, previous_len_XOR_cur_len;
	ret = filesystem_read_element_header(*partition_id, partition_start_address, &record_id, &element_crc, &previous_len_XOR_cur_len);
	if(ret != NRF_SUCCESS && ret != NRF_ERROR_NOT_FOUND)
		return NRF_ERROR_INTERNAL;	// --> retry
	
	
	if(ret == NRF_ERROR_NOT_FOUND) { // No first element header found in storage and swap page --> create one the next time a store operation is performed
		
		partitions[number_of_partitions].has_first_element			= 0;
			
	} else if(ret == NRF_SUCCESS) { // Find the latest element!
	
		ret = filesystem_find_latest_element_address(*partition_id, &(partitions[number_of_partitions].latest_element_address), &(partitions[number_of_partitions].latest_element_record_id), &(partitions[number_of_partitions].latest_element_len));
		if(ret != NRF_SUCCESS) return ret;
		
		partitions[number_of_partitions].has_first_element			= 1;
	}
	
	
	
	next_free_address = partition_start_address + *required_size;	
	number_of_partitions ++;
	
	return NRF_SUCCESS;
}


ret_code_t filesystem_clear_partition(uint16_t partition_id) {
	// We just need to clear the first element-header and the header in the SWAP-PAGE to clear a whole partition.
	
	uint16_t index = partition_id & 0x3FFF;
	
	
	ret_code_t ret;
	
	
	uint32_t partition_start_address 	= partitions[index].first_element_address;
	
	uint16_t element_header_len = filesystem_get_element_header_len(partition_id);
	
	
	// Clear the backup page:	
	uint32_t first_element_address_swap_page = filesystem_get_swap_page_address_of_partition(partition_id);
	uint8_t tmp[PARTITION_METADATA_SIZE + element_header_len];
	memset(tmp, 0xFF, sizeof(tmp));		// Set to an bad metadata + first_element_header
	ret = filesystem_backup_first_element_header(first_element_address_swap_page, tmp, PARTITION_METADATA_SIZE + element_header_len);
	if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
	
	// CLear the first element-header:
	memset(tmp, 0xFF, sizeof(tmp));		// Set to an bad metadata + first_element_header
	// Write metadata + header to storage
	ret = storage_store(partition_start_address, &tmp[0], PARTITION_METADATA_SIZE + element_header_len);
	if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;	
	// Check if the metadata + header was written successfully
	uint8_t tmp_read[sizeof(tmp)];
	ret = storage_read(partition_start_address, &tmp_read[0], PARTITION_METADATA_SIZE + element_header_len);
	if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;				
	if(memcmp(tmp, tmp_read, sizeof(tmp)) != 0)	return NRF_ERROR_INTERNAL;
	
	
	// Now reset the state of the partition:
	partitions[index].has_first_element 		= 0;
	partitions[index].first_element_address		= partition_start_address;
	partitions[index].latest_element_address	= partition_start_address;
	partitions[index].latest_element_record_id	= 1;
	partitions[index].latest_element_len		= 0;

	return NRF_SUCCESS;
}

ret_code_t filesystem_store_element(uint16_t partition_id, uint8_t* element_data, uint16_t element_len) {
	uint16_t index = partition_id & 0x3FFF;
	uint8_t is_dynamic = (partition_id & 0x8000) ? 1 : 0;
	
	// If partition is static, we check the element length for correctness (except element_len == 0, then we will set it to the initialized value)
	if(!is_dynamic) {
		if(element_len == 0)
			element_len = partitions[index].metadata.first_element_len;
		else if(element_len != partitions[index].metadata.first_element_len)
			return NRF_ERROR_INVALID_PARAM;
		// Else the element_len == partitions[index].metadata.first_element_len!
	} 
	
	ret_code_t ret;
	
	
	uint32_t partition_start_address 	= partitions[index].first_element_address;
	uint32_t partition_size			 	= partitions[index].metadata.partition_size;
	
	uint16_t element_header_len = filesystem_get_element_header_len(partition_id);
	
	
	
	// Compute the record id and the address where to store the element
	uint32_t element_address;
	uint16_t record_id, previous_len_XOR_cur_len;	
	if(partitions[index].has_first_element == 0) {
		
		element_address = partition_start_address;
		record_id = 1;
		previous_len_XOR_cur_len = 0;
	} else {
		uint32_t latest_element_address 	= partitions[index].latest_element_address;
		uint16_t latest_element_record_id 	= partitions[index].latest_element_record_id;
		uint16_t latest_element_len		 	= partitions[index].latest_element_len;
		
		// Compute the (next) element address:
		
		
		uint32_t latest_header_len = (latest_element_address == partition_start_address) ? (PARTITION_METADATA_SIZE + element_header_len) : (element_header_len);
		uint32_t next_element_address = latest_element_address + latest_header_len + latest_element_len;
	
		// Check if the next element would be in partition boundaries
		if(next_element_address + element_header_len + element_len <= partition_start_address + partition_size) {
			element_address = next_element_address;
		} else {
			element_address = partition_start_address;
		}
		
		record_id	= increment_record_id(latest_element_record_id);
		previous_len_XOR_cur_len = latest_element_len ^ element_len;
	}
	
	
	// Compute the length of the element header
	uint16_t header_len = (element_address == partition_start_address) ? (PARTITION_METADATA_SIZE + element_header_len) : (element_header_len);
	
	
	// Check if there is enough space in partition to store the element
	if(element_address + header_len + element_len > partition_start_address + partition_size)
		return NRF_ERROR_NO_MEM;
	
	
	// Compute the crc of the element
	uint16_t element_crc = crc16_compute(element_data, element_len, NULL);
	
	partition_element_header_t 	element_header;
	element_header.record_id = record_id;
	element_header.element_crc = element_crc;
	element_header.previous_len_XOR_cur_len = previous_len_XOR_cur_len;
	
	
	
	if(element_address == partition_start_address) {
		uint8_t tmp[PARTITION_METADATA_SIZE + element_header_len];
		
	
		// Write the new metadata and element-header to storage		
		partition_metadata_t 		metadata;
		
		metadata.header_crc			= 0;// Init with 0
		metadata.partition_id 		= partition_id;
		metadata.partition_size 	= partitions[index].metadata.partition_size;
		metadata.first_element_len 	= element_len;
		
		
		// Set the last element address and length
		if(element_address + PARTITION_METADATA_SIZE + element_header_len + element_len >= partitions[index].latest_element_address) {
			// The new element would overwrite the former last element --> replace the last element with the new element
			metadata.last_element_address 	= element_address;
		} else {
			// The former last element won't be overwritten by the new element --> set to latest element
			metadata.last_element_address 	= partitions[index].latest_element_address;
		}
		
		
		filesystem_serialize_metadata(&metadata, &tmp[0]);
		filesystem_serialize_element_header(partition_id, &element_header, &tmp[PARTITION_METADATA_SIZE]);
		
		// Compute the metadata header-crc
		metadata.header_crc = crc16_compute(&tmp[2], PARTITION_METADATA_SIZE + element_header_len - 2, NULL);
	
		
		// Update the metadata and first_element_header in RAM
		partitions[index].metadata = metadata;		
		partitions[index].first_element_header = element_header;
	}
	
	// Before storing the new first-element-header to storage, store it in the backup page!! (Not the old header, because if first partition address
	// is erased, the old header doesn't use anything. So we need to backup the current every time we write on the same unit as first element 
	// --> therefore we need the partitions[index].first_element_header...
	
	
	// Check if the element_address is on the same unit as the first-element, if yes backup the first element header in swap page
	uint32_t element_start_unit_address, element_end_unit_address;
	// It doesn't matter if element_header_len or element_header_len+PARTITION_METADATA_SIZE, because we only need element_start_unit_address
	ret = storage_get_unit_address_limits(element_address, element_header_len + element_len, &element_start_unit_address, &element_end_unit_address);
	if(ret != NRF_SUCCESS)	return NRF_ERROR_INTERNAL;
	if(element_start_unit_address == partition_start_address) {
		// Backup the first element header, if necessary		
		uint32_t first_element_address_swap_page = filesystem_get_swap_page_address_of_partition(partition_id);
		
		uint8_t tmp[PARTITION_METADATA_SIZE + element_header_len];
		
		filesystem_serialize_metadata(&partitions[index].metadata, &tmp[0]);
		
		filesystem_serialize_element_header(partition_id, &partitions[index].first_element_header, &tmp[PARTITION_METADATA_SIZE]);
	
	
		ret = filesystem_backup_first_element_header(first_element_address_swap_page, tmp, PARTITION_METADATA_SIZE + element_header_len);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
	} 
		
		
	
		
		
		
	// Now write to the actual storage.
	if(element_address == partition_start_address) {
		
		uint8_t tmp[PARTITION_METADATA_SIZE + element_header_len];
		filesystem_serialize_metadata(&partitions[index].metadata, &tmp[0]);
		filesystem_serialize_element_header(partition_id, &element_header, &tmp[PARTITION_METADATA_SIZE]);
		
		// Write metadata + header to storage
		ret = storage_store(element_address, &tmp[0], PARTITION_METADATA_SIZE + element_header_len);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		
		// Check if the metadata + header was written successfully
		uint8_t tmp_read[sizeof(tmp)];
		ret = storage_read(element_address, &tmp_read[0], PARTITION_METADATA_SIZE + element_header_len);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;				
		if(memcmp(tmp, tmp_read, sizeof(tmp)) != 0)	return NRF_ERROR_INTERNAL;
		
		// Write the data
		ret = storage_store(element_address + PARTITION_METADATA_SIZE + element_header_len, element_data, element_len);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;	
		
		
		
	} else {
		uint8_t tmp[element_header_len];
		filesystem_serialize_element_header(partition_id, &element_header, &tmp[0]);
		
		// Write header to storage
		ret = storage_store(element_address, &tmp[0], element_header_len);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		
		// Check if the header was written successful
		uint8_t tmp_read[sizeof(tmp)];
		ret = storage_read(element_address, &tmp_read[0], element_header_len);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;				
		if(memcmp(tmp, tmp_read, sizeof(tmp)) != 0)	return NRF_ERROR_INTERNAL;
		
		// Write the data
		ret = storage_store(element_address + element_header_len, element_data, element_len);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		
	}
	
	
	// Set the latest element address
	partitions[index].latest_element_address  	= element_address;
	partitions[index].latest_element_record_id	= record_id;
	partitions[index].latest_element_len 		= element_len;
	
	partitions[index].has_first_element = 1;

	
	return NRF_SUCCESS;
	
	
}








/** @brief Function to check the validity of a iterator.
 *
 * @details The function checks the validity of a iterator by reading the current element-header again and comparing if it has changed.
 *			If the element-header has changed the iterator is invalidated.
 *			
 *
 * @param[in]	partition_id			The identifier of the partition.
 *
 * @retval     	NRF_SUCCESS        		If the iterator is valid.
 * @retval     	NRF_ERROR_INVALID_STATE If the iterator is invalid.
 * @retval     	NRF_ERROR_INTERNAL  	If there was an internal error (e.g. the data couldn't be read because of busy).
 */
ret_code_t filesystem_iterator_check_validity(uint16_t partition_id) {
	uint16_t index = partition_id & 0x3FFF;	// Clear the MSBs
	uint8_t is_dynamic 		= (partition_id & 0x8000) ? 1 : 0;
	uint8_t has_element_crc = (partition_id & 0x4000) ? 1 : 0;
	
	
	if(partition_iterators[index].iterator_valid != 0xA5) {
		return NRF_ERROR_INVALID_STATE;
	}
	
	
	if(!partitions[index].has_first_element) {
		partition_iterators[index].iterator_valid = 0;
		return NRF_ERROR_INVALID_STATE;
	}
	
	uint32_t 					cur_element_address	= partition_iterators[index].cur_element_address;
	partition_element_header_t 	cur_element_header 	= partition_iterators[index].cur_element_header;
	
	uint16_t record_id, element_crc, previous_len_XOR_cur_len;
	ret_code_t ret = filesystem_read_element_header(partition_id, cur_element_address, &record_id, &element_crc, &previous_len_XOR_cur_len);
	if(ret == NRF_ERROR_NOT_FOUND) {	// Only if read of the first element header failed (because deleted in storage and swap page..)
		partition_iterators[index].iterator_valid = 0; 
		return NRF_ERROR_INVALID_STATE;
	}
	
	if(ret != NRF_SUCCESS)	// ret could be NRF_SUCCESS or NRF_ERROR_INTERNAL
		return ret;	
	
	// Check the validity (equality of record_id's, element_crc's and previous_len_XOR_cur_len)
	if(record_id != cur_element_header.record_id || (has_element_crc && element_crc != cur_element_header.element_crc) || (is_dynamic && previous_len_XOR_cur_len != cur_element_header.previous_len_XOR_cur_len)) {
		partition_iterators[index].iterator_valid = 0; 
		return NRF_ERROR_INVALID_STATE;
	}
	
	return NRF_SUCCESS;
}



ret_code_t filesystem_iterator_init(uint16_t partition_id) {
	uint16_t index = partition_id & 0x3FFF;	// Clear the MSBs
	
	
	partition_iterators[index].iterator_valid = 0;
	
	if(!partitions[index].has_first_element) {
		return NRF_ERROR_INVALID_STATE;
	}
	
	// Set the current element address and length of the iterator to the latest element of the partition
	partition_iterators[index].cur_element_address =	partitions[index].latest_element_address;
	partition_iterators[index].cur_element_len 	= 		partitions[index].latest_element_len;
	
	
	// Read the current element header (record_id, element_crc and previous_len_XOR_cur_len)
	ret_code_t ret = filesystem_read_element_header(partition_id, partition_iterators[index].cur_element_address, &(partition_iterators[index].cur_element_header.record_id), &(partition_iterators[index].cur_element_header.element_crc), &(partition_iterators[index].cur_element_header.previous_len_XOR_cur_len));
	if(ret == NRF_ERROR_NOT_FOUND) { // Should actually not happen, because we should have a first element
		return NRF_ERROR_INVALID_STATE;
	}
	
	if(ret != NRF_SUCCESS)
		return ret;
	
	
	// Set iterator to valid (unlikely bitpattern 10100101 = 0xA5)
	partition_iterators[index].iterator_valid = 0xA5;
	
	return NRF_SUCCESS;
}

ret_code_t filesystem_iterator_next(uint16_t partition_id) {
	ret_code_t ret = filesystem_iterator_check_validity(partition_id);
	if(ret != NRF_SUCCESS)
		return ret;
	uint16_t index = partition_id & 0x3FFF;	// Clear the MSBs
	uint8_t is_dynamic = (partition_id & 0x8000) ? 1 : 0;
	
	
	uint32_t cur_element_address 					= partition_iterators[index].cur_element_address;
	uint16_t cur_element_len 						= partition_iterators[index].cur_element_len;
	partition_element_header_t cur_element_header 	= partition_iterators[index].cur_element_header;
	
	
	uint32_t next_element_address;
	uint16_t next_element_len;
	partition_element_header_t next_element_header;
	
	
	ret = filesystem_get_next_element_header(partition_id, cur_element_address, cur_element_header.record_id, cur_element_len, &next_element_address, &(next_element_header.record_id), &(next_element_header.element_crc), &(next_element_header.previous_len_XOR_cur_len));
	if(ret != NRF_SUCCESS)
		return ret;
	
	if(is_dynamic)
		next_element_len = cur_element_len ^ next_element_header.previous_len_XOR_cur_len;
	else
		next_element_len = partitions[index].metadata.first_element_len;
	
		
	partition_iterators[index].cur_element_address 	= next_element_address;
	partition_iterators[index].cur_element_len	 	= next_element_len;
	partition_iterators[index].cur_element_header 	= next_element_header;
	
	return NRF_SUCCESS;
}

ret_code_t filesystem_iterator_previous(uint16_t partition_id) {
	ret_code_t ret = filesystem_iterator_check_validity(partition_id);
	if(ret != NRF_SUCCESS)
		return ret;
	
	uint16_t index = partition_id & 0x3FFF;	// Clear the MSBs
	uint8_t is_dynamic = (partition_id & 0x8000) ? 1 : 0;
	
	uint32_t cur_element_address 					= partition_iterators[index].cur_element_address;
	uint16_t cur_element_len 						= partition_iterators[index].cur_element_len;
	partition_element_header_t cur_element_header 	= partition_iterators[index].cur_element_header;
	
	
	uint32_t previous_element_address;
	uint16_t previous_element_len;
	partition_element_header_t previous_element_header;
	
	ret = filesystem_get_previous_element_header(partition_id, cur_element_address, cur_element_header.record_id, cur_element_len, &previous_element_address, &(previous_element_header.record_id), &(previous_element_header.element_crc), &(previous_element_header.previous_len_XOR_cur_len));
	if(ret != NRF_SUCCESS)
		return ret;
	if(is_dynamic)
		previous_element_len = cur_element_len ^ cur_element_header.previous_len_XOR_cur_len;
	else
		previous_element_len = partitions[index].metadata.first_element_len;
	
	partition_iterators[index].cur_element_address 	= previous_element_address;
	partition_iterators[index].cur_element_len	 	= previous_element_len;
	partition_iterators[index].cur_element_header 	= previous_element_header;
	
	return NRF_SUCCESS;
}

ret_code_t filesystem_iterator_read_element(uint16_t partition_id, uint8_t* element_data, uint16_t* element_len, uint16_t* record_id) {
	ret_code_t ret = filesystem_iterator_check_validity(partition_id);
	if(ret != NRF_SUCCESS)
		return ret;
	
	uint16_t index = partition_id & 0x3FFF;	// Clear the MSBs
	uint8_t has_element_crc = (partition_id & 0x4000) ? 1 : 0;
	
	
	uint32_t cur_element_address 					= partition_iterators[index].cur_element_address;
	*element_len 									= partition_iterators[index].cur_element_len;
	partition_element_header_t cur_element_header 	= partition_iterators[index].cur_element_header;
	
	
	uint32_t header_len = (cur_element_address == partitions[index].first_element_address) ? (PARTITION_METADATA_SIZE + filesystem_get_element_header_len(partition_id)) : (filesystem_get_element_header_len(partition_id));
		
	ret = storage_read(cur_element_address + header_len, element_data, *element_len);
	if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
	
	if(has_element_crc) {		
		// Check the element-crc
		uint16_t element_crc = crc16_compute(element_data, *element_len, NULL);
		if(element_crc != cur_element_header.element_crc) {
			return NRF_ERROR_INVALID_DATA;
		}
	}
	
	*record_id = cur_element_header.record_id;
	
	return NRF_SUCCESS;
}

