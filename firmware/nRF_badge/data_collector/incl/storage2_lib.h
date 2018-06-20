#ifndef __STORAGE2_LIB_H
#define __STORAGE2_LIB_H

#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes


/** @brief Function for initializing storage2 (and the underlying EEPROM-module)
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INTERNAL			If the underlying storage-module (here EEPROM) could not correctly initialized (here: bad spi configuration).
 * @retval 		NRF_ERROR_BUSY				If the underlying storage-module (here EEPROM) is busy (here: EEPROM unprotecting operation failed because of an ongoing spi operation).
 */
ret_code_t storage2_init(void);




/** @brief Function for storing bytes in the underlying storage-module (here EEPROM).
 *
 * @details	
 *
 * 
 * @param[in]	address					The address of the first byte to write.
 * @param[in]	data					Pointer to the bytes to store.
 * @param[in]	length_data				The number of bytes.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM		If data is NULL or specified address and length_data exceed the storage size or data is not in RAM section.
 * @retval 		NRF_ERROR_BUSY				If the underlying storage-module (here EEPROM or SPI) is busy.
 */
ret_code_t storage2_store(uint32_t address, uint8_t* data, uint32_t length_data);


/** @brief Function for reading bytes from the underlying storage-module (here EEPROM).
 *
 * 
 * @param[in]	address					The address of the first byte to read.
 * @param[in]	data					Pointer to memory where the bytes should be stored.
 * @param[in]	length_data				The number of bytes.
 *
 * @retval 		NRF_SUCCSS					If operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM		If data is NULL or specified address and length_data exceed the storage size or data is not in RAM section.
 * @retval 		NRF_ERROR_BUSY				If the underlying storage-module (here EEPROM or SPI) is busy.
 */
ret_code_t storage2_read(uint32_t address, uint8_t* data, uint32_t length_data);



/** @brief Function for retrieving the unit size of the storage-module.
 *
 * @details The unit size of a storage-module describes the minimum size of data that should be reserved for 
 *			related data.
 *			Example: if you want to store two partitions of different data (that are not related). It is important
 *			to not store them on the same unit of the storage-module because it could happen that writing to 
 *			partition 1 might destroy/delete data of partition 2 on the same unit (here: unit = byte).
 * 
 * @retval 		1	Unit size of a EEPROM.
 */
uint32_t storage2_get_unit_size(void);


/**@brief   Function for reading the number of bytes in storage2.
 *
 * @retval  Number of available bytes in storage2.
 */
uint32_t storage2_get_size(void);

#endif 
