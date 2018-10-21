#ifndef __STORER_LIB_H
#define __STORER_LIB_H

#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes
#include "chunk_messages.h"

/**< The number of entries in each partition (can be adopted on the user's needs) */ 
#define STORER_BADGE_ASSIGNEMENT_NUMBER				1
#define STORER_BATTERY_DATA_NUMBER					100
#define STORER_MICROPHONE_DATA_NUMBER				1000
#define STORER_SCAN_DATA_NUMBER						1000
#define STORER_ACCELEROMETER_INTERRUPT_DATA_NUMBER	50
#define STORER_ACCELEROMETER_DATA_NUMBER			100


/**@brief Function to initialize the storer-module.
 * @details It initializes the filesystem and registers all the needed partitions. 
 *			You can change the above values for the number of chunks stored in the filesystem.
 *			If debug is enabled it should print out the number of available bytes after registration of all partitions.
 *
 * @retval NRF_SUCCESS				If everything was fine.
 * @retval 							Otherwise an error code is returned.
 *
 * @note All the storing/reading stuff must be done in main-context, because they share same buffer for serialization.
 *		 And the storage-modules allow only to be used in main-context and should not be interrupted.
 */
ret_code_t storer_init(void);


/**@brief Function to clear all partitions.
 * @details It does not erase all the memory but uses the filesystem_clear_partition-function to clear only the headers of the partitions.
 *
 * @retval NRF_SUCCESS				If everything was fine.
 * @retval NRF_ERROR_INTERNAL		Busy.
 */
ret_code_t storer_clear(void);


/**@brief Function to clear the badge-assignement in the filesystem.
 *
 * @retval NRF_SUCCESS				If everything was fine.
 * @retval NRF_ERROR_INTERNAL		Busy.
 */
ret_code_t storer_clear_badge_assignement(void);

/**@brief Function to store a badge-assignement in the filesystem.
 *
 * @retval NRF_ERROR_INTERNAL		Busy.
 * @retval NRF_ERROR_INVALID_DATA	If encoding fails.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_store_badge_assignement(BadgeAssignement* badge_assignement);

/**@brief Function to read the last stored badge-assignement.
 *
 * @retval NRF_ERROR_INTERNAL		Busy.
 * @retval NRF_ERROR_INVALID_STATE	Iterator invalidated/no data found.
 * @retval NRF_ERROR_INVALID_DATA	If the CRC does not match.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_read_badge_assignement(BadgeAssignement* badge_assignement);


/**@brief Function to invalidate all iterators of the chunk-partitions.
 * @note  This function has to be called when the application can't 
 * 		  call the get_next_..._chunk()-function anymore (e.g. because of disconnect),
 *		  to invalidate all the iterators, so that new chunks can overwrite the chunks,
 *		  the iterator is currently pointing to.
 */
void storer_invalidate_iterators(void);





/**@brief Function to store an accelerometer chunk in the accelerometer-partition.
 * @retval NRF_ERROR_NO_MEM			If the element is too big, to be stored in the partition.
 * @retval NRF_ERROR_INTERNAL		Busy or iterator is pointing to the same address we want to write to.
 * @retval NRF_ERROR_INVALID_DATA	If encoding fails.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_store_accelerometer_chunk(AccelerometerChunk* accelerometer_chunk);

/**@brief Function to find an accelerometer chunk from timestamp and set the iterator of the partition. (For detailed description: find_chunk_from_timestamp())
 * @retval NRF_ERROR_INTERNAL		Busy.
 * @retval NRF_ERROR_INVALID_STATE	Iterator invalidated/no data found.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_find_accelerometer_chunk_from_timestamp(Timestamp timestamp, AccelerometerChunk* accelerometer_chunk);

/**@brief Function to get the next accelerometer chunk from the iterator of the partition. (For detailed description: get_next_chunk())
 * @retval	NRF_SUCCESS					If an element was found and returned successfully.
 * @retval	NRF_ERROR_NOT_FOUND			If no more element in the partition.
 * @retval	NRF_ERROR_INVALID_STATE		If iterator not initialized or invalidated.
 * @retval	NRF_ERROR_INTERNAL			If busy.
 */
ret_code_t storer_get_next_accelerometer_chunk(AccelerometerChunk* accelerometer_chunk);






/**@brief Function to store an accelerometer-interrupt chunk in the accelerometer-interrupt-partition.
 * @retval NRF_ERROR_NO_MEM			If the element is too big, to be stored in the partition.
 * @retval NRF_ERROR_INTERNAL		Busy or iterator is pointing to the same address we want to write to.
 * @retval NRF_ERROR_INVALID_DATA	If encoding fails.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_store_accelerometer_interrupt_chunk(AccelerometerInterruptChunk* accelerometer_interrupt_chunk);

/**@brief Function to find an accelerometer-interrupt chunk from timestamp and set the iterator of the partition. (For detailed description: find_chunk_from_timestamp())
 * @retval NRF_ERROR_INTERNAL		Busy.
 * @retval NRF_ERROR_INVALID_STATE	Iterator invalidated/no data found.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_find_accelerometer_interrupt_chunk_from_timestamp(Timestamp timestamp, AccelerometerInterruptChunk* accelerometer_interrupt_chunk);

/**@brief Function to get the next accelerometer-interrupt chunk from the iterator of the partition. (For detailed description: get_next_chunk())
 * @retval	NRF_SUCCESS					If an element was found and returned successfully.
 * @retval	NRF_ERROR_NOT_FOUND			If no more element in the partition.
 * @retval	NRF_ERROR_INVALID_STATE		If iterator not initialized or invalidated.
 * @retval	NRF_ERROR_INTERNAL			If busy.
 */
ret_code_t storer_get_next_accelerometer_interrupt_chunk(AccelerometerInterruptChunk* accelerometer_interrupt_chunk);






/**@brief Function to store a battery chunk in the battery-partition.
 * @retval NRF_ERROR_NO_MEM			If the element is too big, to be stored in the partition.
 * @retval NRF_ERROR_INTERNAL		Busy or iterator is pointing to the same address we want to write to.
 * @retval NRF_ERROR_INVALID_DATA	If encoding fails.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_store_battery_chunk(BatteryChunk* battery_chunk);

/**@brief Function to find a battery chunk from timestamp and set the iterator of the partition. (For detailed description: find_chunk_from_timestamp())
 * @retval NRF_ERROR_INTERNAL		Busy.
 * @retval NRF_ERROR_INVALID_STATE	Iterator invalidated/no data found.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_find_battery_chunk_from_timestamp(Timestamp timestamp, BatteryChunk* battery_chunk);

/**@brief Function to get the next battery chunk from the iterator of the partition. (For detailed description: get_next_chunk())
 * @retval	NRF_SUCCESS					If an element was found and returned successfully.
 * @retval	NRF_ERROR_NOT_FOUND			If no more element in the partition.
 * @retval	NRF_ERROR_INVALID_STATE		If iterator not initialized or invalidated.
 * @retval	NRF_ERROR_INTERNAL			If busy.
 */
ret_code_t storer_get_next_battery_chunk(BatteryChunk* battery_chunk);






/**@brief Function to store a scan chunk in the scan-partition.
 * @retval NRF_ERROR_NO_MEM			If the element is too big, to be stored in the partition.
 * @retval NRF_ERROR_INTERNAL		Busy or iterator is pointing to the same address we want to write to.
 * @retval NRF_ERROR_INVALID_DATA	If encoding fails.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_store_scan_chunk(ScanChunk* scan_chunk);

/**@brief Function to find a scan chunk from timestamp and set the iterator of the partition. (For detailed description: find_chunk_from_timestamp())
 * @retval NRF_ERROR_INTERNAL		Busy.
 * @retval NRF_ERROR_INVALID_STATE	Iterator invalidated/no data found.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_find_scan_chunk_from_timestamp(Timestamp timestamp, ScanChunk* scan_chunk);

/**@brief Function to get the next scan chunk from the iterator of the partition. (For detailed description: get_next_chunk())
 * @retval	NRF_SUCCESS					If an element was found and returned successfully.
 * @retval	NRF_ERROR_NOT_FOUND			If no more element in the partition.
 * @retval	NRF_ERROR_INVALID_STATE		If iterator not initialized or invalidated.
 * @retval	NRF_ERROR_INTERNAL			If busy.
 */
ret_code_t storer_get_next_scan_chunk(ScanChunk* scan_chunk);






/**@brief Function to store a microphone chunk in the microphone-partition.
 * @retval NRF_ERROR_NO_MEM			If the element is too big, to be stored in the partition.
 * @retval NRF_ERROR_INTERNAL		Busy or iterator is pointing to the same address we want to write to.
 * @retval NRF_ERROR_INVALID_DATA	If encoding fails.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_store_microphone_chunk(MicrophoneChunk* microphone_chunk);

/**@brief Function to find a microphone chunk from timestamp and set the iterator of the partition. (For detailed description: find_chunk_from_timestamp())
 * @retval NRF_ERROR_INTERNAL		Busy.
 * @retval NRF_ERROR_INVALID_STATE	Iterator invalidated/no data found.
 * @retval NRF_SUCCESS				If everything was fine.
 */
ret_code_t storer_find_microphone_chunk_from_timestamp(Timestamp timestamp, MicrophoneChunk* microphone_chunk);

/**@brief Function to get the next microphone chunk from the iterator of the partition. (For detailed description: get_next_chunk())
 * @retval	NRF_SUCCESS					If an element was found and returned successfully.
 * @retval	NRF_ERROR_NOT_FOUND			If no more element in the partition.
 * @retval	NRF_ERROR_INVALID_STATE		If iterator not initialized or invalidated.
 * @retval	NRF_ERROR_INTERNAL			If busy.
 */
ret_code_t storer_get_next_microphone_chunk(MicrophoneChunk* microphone_chunk);

#endif 

