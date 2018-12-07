/** @file
 *
 *
 * @brief Filesystem optimized for different partitions of sequential store/read operations.
 *
 * @details The application can register partitions for storing and reading records/elements. 
 *			Each record in a partition contains a record/element-header followed by the actual data bytes.
 *			The element-header has a 2-Byte record-id that is incremented for each sequential record. 
 *			Optionally a 2-Byte CRC-value can be stored in the header to detect corrupted data.
 *			The element-header at the first address in each partition contains additional metadata about the partition.
 *			So the structure of a partition looks like this: (The number in the brackets represents the number of bytes needed.)
 *			| metadata (14) | element-header (2-6) | --- data --- | element-header (2-6) | --- data --- | element-header (2-6) | --- data --- | ...
 *
 * @details There are two different types of partitions: static and dynamic. 
 *			A static partition can be used for storing/reading a constant number of bytes for each entry/record.
 *			A dynamic partition can be used for storing/reading a variable number of bytes for each entry/record.
 *			The dynamic partition is organized as a XORed doubly linked list. Therefore another entry in the element-header
 *			is done to retrieve the address of the previous and next element/record.
 *
 * @details Because flash store operations can lead to inconsistent data if the power supply breaks down, 
 *			some safety countermeasurements are incorporated. A swap page for the first element-headers for each partition
 *			is implemented to backup the first element-header to prevent data loss. The swap-page is placed at the beginning 
 *			of the storage. It is advantageous to have the swap-page in a storage with byte-units like EEPROM.
 *
 * @details	For reading the sequential records/elements of a partition, an iterator should be used. 
 *			It could happen that the element the iterator is currently pointing to is overwritten by new data.
 *			In this case the iterator is invalidated, and the application needs to reinitialize the iterator.
 */

#ifndef __FILESYSTEM_LIB_H
#define __FILESYSTEM_LIB_H

#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes


#define MAX_NUMBER_OF_PARTITIONS 									20		/**< Maximal number of registerable partitions. */


#define PARTITION_METADATA_SIZE										14		/**< Number of bytes of the metadata. */
#define PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE						2		/**< Number of bytes of the record-id. */
#define PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE		2		/**< Number of bytes of previous_len_XOR_cur_len. */
#define PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE					2		/**< Number of bytes of the CRC-value. */


#define SWAP_PAGE_SIZE												(MAX_NUMBER_OF_PARTITIONS*(PARTITION_METADATA_SIZE + PARTITION_ELEMENT_HEADER_RECORD_ID_SIZE + PARTITION_ELEMENT_HEADER_PREVIOUS_LEN_XOR_CUR_LEN_SIZE + PARTITION_ELEMENT_HEADER_ELEMENT_CRC_SIZE)) /**< Size of the swap-page */






typedef struct {
	uint16_t header_crc;					/**< The CRC of the metadata and element-header of first element. Needed to validate the first element */
	uint16_t partition_id;					/**< The partition identifier. Highest bit --> dynamic(1) or static(0), second highest bit --> CRC enabled (1) or disabled (0) */
	uint32_t partition_size;				/**< Size of the partition. */
	uint16_t first_element_len;				/**< Length of first element. Static partition: length of every element. Dynamic partition: length of the first element. */
	uint32_t last_element_address;			/**< Element of last address in partition (only needed in dynamic partition). */
} partition_metadata_t;						/**< Metadata-struct that is stored in the storage (14 Bytes) */

typedef struct {
	uint16_t record_id;						/**< The record-id of the element/record. */
	uint16_t previous_len_XOR_cur_len;		/**< In dynamic partition, needed for XORed doubly linked list, to reference to next and previous element. */
	uint16_t element_crc;					/**< The CRC-value of the element-data. */
} partition_element_header_t;				/**< Element-header-struct that is stored in the storage (max. 6 Bytes, depends on configuration). */


typedef struct {
	partition_metadata_t 		metadata;					/**< Copy of the current metadata that must be backupt continiously. */		
	
	uint32_t 					first_element_address;		/**< The address of the first element/The partition start address. */	
	uint8_t						has_first_element;			/**< Flag if the partition has a first element or not. */		
	partition_element_header_t	first_element_header;		/**< Copy of the current first element header that must be backupt continiously. */		
	
	uint32_t 					latest_element_address;		/**< The address of the latest element to compute the next "free" address. */	
	uint16_t 					latest_element_record_id;	/**< The record-id of the latest element to compute the next element record-id address. */	
	uint16_t 					latest_element_len;			/**< The length of the latest element to compute the next element address. */	
	
} partition_t;												/**< Partition-struct to manage a partition. */


typedef struct {
	uint32_t 					cur_element_address;	/**< The address of the current element. */	
	uint16_t					cur_element_len;		/**< The length of the current element. */	
	partition_element_header_t 	cur_element_header;		/**< Copy of the current element header. */	
	uint8_t						iterator_valid;			/**< Specific numbers that represents if the iterator is valid or not. */	
} partition_iterator_t;									/**< Iterator-struct to manage a partition-iterator. */









/** @brief Function for initializing the filesystem.
 *
 * @details	It initializes the underlying storage module and resets the filesystem by calling filesystem_reset().
 * 
 * @retval 		NRF_SUCCESS					If operation was successful.
 * @retval 		NRF_ERROR_INTERNAL			If the underlying storage-module could not correctly initialized or there went something wrong while resetting the filesystem.
 * @retval 		NRF_ERROR_BUSY				If the operation could not be performed currently, because the underlying storage module is busy or it depends on another module that is busy.
 * @retval		NRF_ERROR_TIMEOUT			If the operation takes too long.
 */
ret_code_t 	filesystem_init(void);


/** @brief Function for resetting the filesystem.
 *
 * @details	The function resets the filesystem by setting the internal number_of_partitions to zero, and recomputing the next_free_address. 
 *			But it doesn't reinitialize the underlying storage module. After the reset, the partitions need to be registered again.
 * 
 * @retval 		NRF_SUCCESS					If operation was successful.
 * @retval 		NRF_ERROR_INTERNAL			If there was an invalid param to storage_get_unit_address_limits (e.g. the SWAP_PAGE_SIZE is larger than the storage) --> should actually not happen.
 */
ret_code_t 	filesystem_reset(void);

/** @brief Function for clearing the storage of the filesystem.
 *
 * @details	The function cleares the complete storage and resets the filesystem afterwords via filesystem_reset().
 * 
 * @retval 		NRF_SUCCESS					If operation was successful.
 * @retval 		NRF_ERROR_BUSY				If the underlying storage-module is busy.
 * @retval		NRF_ERROR_TIMEOUT			If the operation takes too long.
 */
ret_code_t filesystem_clear(void);

/** @brief Function for retrieving the available number of bytes in the filesystem.
 *
 * @retval Available bytes in the filesystem.
 */
uint32_t filesystem_get_available_size(void);

/** @brief Function for registering a partition.
 *
 * @details	The function registers a dynamic or static partition. It searches for an already existing partition in storage at the same address the
 *			new partition would be. If there is already a partition (with the same settings: partition_id, partition_size), the latest element address
 *			is searched, and new store operations will start beginning from this address.
 *
 * @param[out]		partition_id				Pointer to the identifier of the partition.
 * @param[in/out]	required_size				The required size of the partition. Could be smaller, equal or greater after registration.
 * @param[in]		is_dynamic					Flag if the partition should be dynamic (1) or static (0).
 * @param[in]		enable_crc					Flag if the crc for element data should be enabled (1) or not (0).
 * @param[in]		element_len					If the partition is static, this parameter represents the element length. 
 *												If the partition is dynamic, this parameter could be arbitrary (e.g. 0).
 * 
 * @retval 		NRF_SUCCESS					If the partition-registraton was successful.
 * @retval		NRF_ERROR_INVALID_PARAM		If the required-size == 0, or element_len == 0 in a static partition.
 * @retval		NRF_ERROR_NO_MEM			If there were already more than MAX_NUMBER_OF_PARTITIONS partition-registrations, or if the available size for the partition is too small.
 * @retval 		NRF_ERROR_INTERNAL			If there was an internal error (e.g. data couldn't be read because of busy).
 */
ret_code_t filesystem_register_partition(uint16_t* partition_id, uint32_t* required_size, uint8_t is_dynamic, uint8_t enable_crc, uint16_t element_len);

/** @brief Function for clearing a partition.
 *
 * @details	The function clears a partition. It removes the first-element-header at the beginning of the partition and in the swap-page.
 *			Furthermore, it resets the state of the partition to "zero" (the default values after registering a partition).
 *			After clearing a partition no more elements could be found. The application needn't to register the partition again, but can just use it. 
 *			If there was already a first-element-header in the partition, it reads out its record_id and increments it, so that the record-id of the 
 *			new first-element-header is different. This is done, to not clear the (already existing) next element-headers all the time while storing.
 *
 *
 * @param[in]	partition_id			The identifier of the partition.
 * 
 * @retval 		NRF_SUCCESS					If the store operation was succesful.
 * @retval 		NRF_ERROR_INTERNAL			If there was an internal error (e.g. data couldn't be stored/read because of busy).
 */
ret_code_t filesystem_clear_partition(uint16_t partition_id);

/** @brief Function for storing an element (some bytes) to a partition.
 *
 * @details	The function computes the address where to store the next element and stores the element-data together with an element-header.
 *			Each time an element-header is stored, the function checks the correctness of the store operation by reading and compring the element-header.
 *
 *			If the element is stored to the first address in the partition, the procedure is the following:
 *			Before the first-element-header in storage is overwritten by the new first-element-header, the new first-element-header is backuped in the swap-page.
 *			This is done, because if the store operation fails (because of a breaked power supply), we would loose the reference to the last element (in a dynamic partition),
 *			and we wouldn't find it anymore if the whole first unit (in case of flash) is erased/corrupted.
 *			Furthermore, if there are repeated store operations in the first unit, 
 *			this backup is needed to keep a valid partition even if the first-element-header is corrupted during these store-operations.
 *
 *			Furthermore, before storing the data to storage, it is checked that the next-element header has not a consecutive record-id, to don't get confused.
 *			If it has a consecutive record-id the next-element header is cleared before writing the new data.
 *
 *			Another addition is, that it checks for a conflict with the iterator before storing the data (A conflict means, that the iterator is valid and the storer
 *			wants to overwrite the element the iterator is currently pointing to. This is not allowed.). When a conflict is present the function returns NRF_ERROR_INTERNAL.
 *			So it is very important to invalidate the iterator if it is not used anymore by filesystem_iterator_invalidate().
 *			
 * @note 	It is advantageous to keep the swap-page in a storage part with a unit size of one byte, because if different partitions alternately store in their first unit,
 *			the first-element-header must be backupt each time. If the addresses of the backupt first-element-headers are on the same unit (e.g. in flash), 
 *			it could happen that they delete each other backupt first-element-header. In this case the first-element-header has to be rewritten more often, 
 *			in worst case "#partitions x #store operations on first unit" times.
 *
 * @param[in]	partition_id			The identifier of the partition.
 * @param[in]	element_data			Pointer to the data that should be stored.
 * @param[in]	element_len				The length of the data to store (if the partition is static, this parameter could be 0 or the registered element_len).
 * 
 * @retval 		NRF_SUCCESS					If the store operation was succesful.
 * @retval		NRF_ERROR_INVALID_PARAM		If the partition is static and the element_len != 0 && element_len != registered element_len.
 * @retval		NRF_ERROR_NO_MEM			If the element is too big, to be stored in the partition.
 * @retval 		NRF_ERROR_INTERNAL			If there was an internal error (e.g. data couldn't be stored/read because of busy). 
 *											Or there is a conflict between storing and the iterator.
 */
ret_code_t filesystem_store_element(uint16_t partition_id, uint8_t* element_data, uint16_t element_len);



/** @brief Function for initializing the iterator for a partition.
 *
 * @details	The function initializes and validates the iterator to point to the latest stored element of a partition.
 *
 * @note	When an iterator was initialized, it needs to be invalidated (via filesystem_iterator_invalidate()) if it is not used anymore.
 *			Otherwise the store-function could not overwrite the element where the iterator is currently pointing to.
 * 
 * @param[in]	partition_id				The identifier of the partition.
 *
 * @retval 		NRF_SUCCESS					If operation was successful.
 * @retval		NRF_ERROR_INVALID_STATE		If the partition has no first element-header.
 * @retval     	NRF_ERROR_INTERNAL  		If there was an internal error (e.g. the data couldn't be read because of busy).
 */
ret_code_t filesystem_iterator_init(uint16_t partition_id);

/** @brief Function for invalidating the iterator of a partition.
 *
 * @details	The function invalidates the iterator of a partition, so that the store-function could overwrite the element 
 *			where the iterator was pointing to.
 * 
 * @param[in]	partition_id				The identifier of the partition.
 */
void filesystem_iterator_invalidate(uint16_t partition_id);

/** @brief Function to set the iterator pointing to the next element in a partition.
 *
 * @details	The iterator steps to the next element address and reads the record-id if it is consistent with the current record-id.
 * 
 * @param[in]	partition_id				The identifier of the partition.
 * 
 * @retval 		NRF_SUCCESS					If operation was successful.
 * @retval		NRF_ERROR_NOT_FOUND			If there is no next element in the partition.
 * @retval		NRF_ERROR_INVALID_STATE		If the iterator was invalidated.
 * @retval     	NRF_ERROR_INTERNAL  		If there was an internal error (e.g. the data couldn't be read because of busy).
 */
ret_code_t filesystem_iterator_next(uint16_t partition_id);

/** @brief Function to set the iterator pointing to the previous element in a partition.
 *
 * @details	The iterator steps to the previous element address and reads the record-id if it is consistent with the current record-id.
 * 
 * @param[in]	partition_id				The identifier of the partition.
 * 
 * @retval 		NRF_SUCCESS					If operation was successful.
 * @retval		NRF_ERROR_NOT_FOUND			If there is no previous element in the partition.
 * @retval		NRF_ERROR_INVALID_STATE		If the iterator was invalidated.
 * @retval     	NRF_ERROR_INTERNAL  		If there was an internal error (e.g. the data couldn't be read because of busy).
 */
ret_code_t filesystem_iterator_previous(uint16_t partition_id);


/** @brief Function to read the element the iterator is currently pointing to.
 *
 * @details	The function reads the element data from the storage and if CRC is enabled, the data is checked for integrity.
 * 
 * @param[in]	partition_id				The identifier of the partition.
 * @param[out]	element_data				Pointer to buffer where the data should be stored to.
 * @param[out]	element_len					Pointer to memory where the data length should stored to.
 * @param[out]	record_id					Pointer to memory where the record-id should stored to.
 * 
 * @retval 		NRF_SUCCESS					If operation was successful.
 * @retval		NRF_ERROR_INVALID_DATA		If CRC is enabled and the data are corrupted.
 * @retval		NRF_ERROR_INVALID_STATE		If the iterator was invalidated.
 * @retval     	NRF_ERROR_INTERNAL  		If there was an internal error (e.g. the data couldn't be read because of busy).
 */
ret_code_t filesystem_iterator_read_element(uint16_t partition_id, uint8_t* element_data, uint16_t* element_len, uint16_t* record_id);





#endif 
