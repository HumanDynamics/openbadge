#ifndef __STORAGE1_LIB_H
#define __STORAGE1_LIB_H

#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes


/** @brief Function for initializing storage1 (and the underlying flash-module)
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INTERNAL			If the underlying storage-module (here flash) could not correctly initialized.
 */
ret_code_t storage1_init(void);




/** @brief Function for storing bytes in the underlying storage-module (here flash).
 *
 * @details	The function is optimized for sequential writing. 
 *			It is optimized for handling STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE different
 *			addresses for sequential storing. It only erases pages if it is necessary and restores the data 
 *			on the first erased page that are in front of the current data address.
 *			If the application needs more than STORAGE1_LAST_STORED_ELEMENT_ADDRESSES_SIZE different sequential 
 *			addresses, this value has to be increased.
 *			The function converts the bytes to words and store them in the flash.
 *
 * @warning When storing data, all data that have been stored before on the same page but behind the new data, will be deleted.
 *			Furthermore, the erasing of pages before storing data to them could lead to inconsistent data, if the power supply is interrupted.
 *			The application has to take care of this (by using checksums for real critical data).
 * 
 * @param[in]	address					The address of the first byte to write.
 * @param[in]	data					Pointer to the bytes to store.
 * @param[in]	length_data				The number of bytes.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM		If data is NULL or specified address and length_data exceed the storage size.
 * @retval 		NRF_ERROR_BUSY				If the underlying storage-module (here flash) is busy.
 * @retval 		NRF_ERROR_INTERNAL			If the underlying storage-module (here flash) was not correctly initialized or the operation timed out (sth. went wrong).
 *											Or if something went wrong during the calculation of the backup-data size.
 */
ret_code_t storage1_store(uint32_t address, uint8_t* data, uint32_t length_data);


/** @brief Function for reading bytes from the underlying storage-module (here flash).
 *
 * 
 * @param[in]	address					The address of the first byte to read.
 * @param[in]	data					Pointer to memory where the bytes should be stored.
 * @param[in]	length_data				The number of bytes.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM		If data is NULL or specified address and length_data exceed the storage size or data is not in RAM section.
 */
ret_code_t storage1_read(uint32_t address, uint8_t* data, uint32_t length_data);


/** @brief Function for retrieving the unit size of the storage-module.
 *
 * @details The unit size of a storage-module describes the minimum size of data that should be reserved for 
 *			related data.
 *			Example: if you want to store two partitions of different data (that are not related). It is important
 *			to not store them on the same unit of the storage-module because it could happen that writing to 
 *			partition 1 might destroy/delete data of partition 2 on the same unit (here: unit = flash page).
 * 
 * @retval 		1024 or 4096	Unit size of a flash page: NRF51 or NRF52.
 */
uint32_t storage1_get_unit_size(void);


/**@brief   Function for reading the number of bytes in storage1.
 *
 * @retval  Number of available bytes in storage1.
 */
uint32_t storage1_get_size(void);

#endif 
