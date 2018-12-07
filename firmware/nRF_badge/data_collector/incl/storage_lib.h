#ifndef __STORAGE_LIB_H
#define __STORAGE_LIB_H

#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes


/** @brief Function for initializing 
 *
 * @details The function initializes all registered storage-modules by calling their init-functions.
 *			The init-functions of the storage-modules should tolerate a multiple init-call, because
 *			it might happen that one storage-module could not be initialized currently. And when retrying
 *			the other modules should be able to handle multiple init-calls.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INTERNAL			If the underlying storage-module could not correctly initialized.
 * @retval 		NRF_ERROR_BUSY				If the operation could not be performed currently, because the underlying storage module is busy or it depends on another module that is busy.
 * @retval		NRF_ERROR_TIMEOUT			If the operation takes too long.
 */
ret_code_t storage_init(void);




/** @brief Function for storing bytes to the storage.
 *
 * @details	This function splits the data to store (if necessary) into the different storage-modules and 
 *			calls the store-function of the different storage-modules (if there is sth to store).
 *
 * 
 * @param[in]	address					The address of the first byte to write.
 * @param[in]	data					Pointer to the bytes to store.
 * @param[in]	length_data				The number of bytes.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM		If data is NULL or specified address and length_data exceed the storage size or data is not in RAM section.
 * @retval 		NRF_ERROR_BUSY				If the underlying storage-module is busy.
 * @retval 		NRF_ERROR_INTERNAL			If the underlying storage-module was not correctly initialized or if something went wrong during the execution of the function.
 * @retval		NRF_ERROR_TIMEOUT			If the operation timed out (sth. went wrong) or the operation takes too long.
 */
ret_code_t storage_store(uint32_t address, uint8_t* data, uint32_t length_data);


/** @brief Function for reading bytes from the storage.
 *
 * @details	This function splits the data to read (if necessary) into the different storage-modules and 
 *			calls the read-function of the different storage-modules (if there is sth to read from).
 *
 * 
 * @param[in]	address					The address of the first byte to read.
 * @param[in]	data					Pointer to memory where the bytes should be stored.
 * @param[in]	length_data				The number of bytes.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM		If data is NULL or specified address and length_data exceed the storage size or data is not in RAM section.
 * @retval 		NRF_ERROR_BUSY				If the underlying storage-module is busy.
 * @retval		NRF_ERROR_TIMEOUT			If the operation takes too long.
 */
ret_code_t storage_read(uint32_t address, uint8_t* data, uint32_t length_data);


/** @brief Function for retrieving the storage-unit boundaries of an address-area.
 *
 * @details	The function computes an area in storage where the specified address-area (address --> address + length_data - 1)
 *			is included with respect to the unit-sizes of the different modules.
 *			This function could be used to check whether some new data are stored to the same unit as some other data.
 *			Furthermore, it could be used to calculate a partition-size, because partitions should always be unit-aligned.
 *
 * 
 * @param[in]	address					The address of the first byte to read.
 * @param[in]	length_data				The number of bytes.
 * @param[out]	start_unit_address		Pointer to memory where the start address of the affected unit(s) should be stored.
 * @param[out]	end_unit_address		Pointer to memory where the end address of the affected unit(s) should be stored.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM		If specified address and length_data exceed the storage size or length_data == 0.
 */
ret_code_t storage_get_unit_address_limits(uint32_t address, uint32_t length_data, uint32_t* start_unit_address, uint32_t* end_unit_address);


/**@brief   Function for reading the number of bytes in storage.
 *
 * @retval  Number of available bytes in storage.
 */
uint32_t storage_get_size(void);


/** @brief Function to clear/erase a defined address range.
 * 
 * @param[in]	address			The address of the first byte to clear.
 * @param[in]	length			The number of bytes to clear.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM		If specified address and length exceed the storage size.
 * @retval 		NRF_ERROR_BUSY				If the underlying storage-module is busy.
 * @retval		NRF_ERROR_TIMEOUT			If the operation takes too long.
 */
ret_code_t storage_clear(uint32_t address, uint32_t length);

#endif 
