#ifndef __EEPROM_LIB_H
#define __EEPROM_LIB_H


/** @file
 *
 * @brief EEPROM-IC abstraction library.
 *
 * @details It enables to store and read data to and from the EEPROM via SPI in blocking or in non-blocking mode.
 *			It uses a cool mechanism to not allocate big internal buffers for the SPI transfers.
 *			For the SPI operations the spi_lib-module is used.
 *
 * @note   It is important that the tx_data-buffer (for store operations) is in the Data RAM section (not in read-only section).
 */
 
#include <stdbool.h>
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes


#define EEPROM_SIZE	(1024*256)	/**< Size of the external EEPROM in bytes */
 
/**@brief The different EEPROM operations. These operations will be used to set the peripheral busy or not. */
 typedef enum {
	EEPROM_NO_OPERATION 			= 0,			/**< Currently no operation ongoing. */
	EEPROM_STORE_OPERATION 			= (1 << 0),		/**< Currently there is an ongoing store operation. */
	EEPROM_READ_OPERATION			= (1 << 1),		/**< Currently there is an ongoing read operation. */
} eeprom_operation_t;
 
 
/**@brief   Function for initializing the eeprom module.
 *
 * @details This functions initializes the underlying spi-module.
 *			The spi peripheral has be enabled in the config-file: sdk_config.h.
 *			Furthermore, it unprotects the EEPROM (to be able to write to any EEPROM block).
 *			
 *
 * @retval  NRF_SUCCESS    		If the module was successfully initialized.
 * @retval  NRF_ERROR_INTERNAL  If there was an error while initializing the spi-module (e.g. bad configuration)
 * @retval 	NRF_ERROR_BUSY		If the unprotecting operation failed (because of an ongoing spi operation).
 */
ret_code_t eeprom_init(void);




/**@brief   Function for storing data in asynchronous/non-blocking/background mode in EEPROM via SPI.
 *
 * @details This is a non-blocking function. If there is already an ongoing SPI or EEPROM operation it returns NRF_ERROR_BUSY.
 *			It achieves the data storing to EEPROM without internal buffers for transmitting the data via SPI.
 *			Therefore the tx_data-buffer is modified during the function, so the "write-header" for the EEPROM could be placed
 *			in this buffer. 
 *			This operation is splitted in three parts. At the beginning the first 4 data bytes (or less) are transmitted in blocking mode via an internal allocated buffer.
 *			After that the remaining bytes are transmitted in non-blocking mode (with the Write-header in the first 4 tx_data-buffer bytes).
 *			In the end if the spi transmit operation terminated and the internal spi-handler is called, the first 4 data bytes are restored to the tx_data-buffer.
 *	
 * @warning The store data must be kept in memory until the operation has terminated.
 *
 * @param[in]   address			Address in EEPROM where to store the data. 	   	
 * @param[in]   tx_data			Pointer to the data to store. This is not const because it is internally modified. It must be in Data RAM region.
 * @param[in]   length_tx_data	Length of the data to store.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If the SPI interface or the EEPROM is busy.
 * @retval 	NRF_ERROR_INVALID_PARAM   	If the address is to big or the provided tx_data-buffer is not placed in the Data RAM region.
 */
ret_code_t eeprom_store_bkgnd(uint32_t address, uint8_t* tx_data, uint32_t length_tx_data);


/**@brief   Function for storing data in blocking mode in EEPROM via SPI.
 *
 * @details This function uses internally eeprom_store_bkgnd() to store the data and eeprom_get_operation()
 *			to wait until the operation (spi transfer and EEPROM internal write) has terminated. 
 *	
 *
 * @param[in]   address			Address in EEPROM where to store the data. 	   	
 * @param[in]   tx_data			Pointer to the data to store. This is not const because it is internally modified. It must be in Data RAM region.
 * @param[in]   length_tx_data	Length of the data to store.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If the SPI interface or the EEPROM is busy.
 * @retval 	NRF_ERROR_INVALID_PARAM   	If the address is to big or the provided tx_data-buffer is not placed in the Data RAM region.
 */
ret_code_t eeprom_store(uint32_t address, uint8_t* tx_data, uint32_t length_tx_data);


/**@brief   Function for reading data in asynchronous/non-blocking/background mode from EEPROM via SPI.
 *
 * @details This is a non-blocking function. If there is already an ongoing SPI or EEPROM operation it returns NRF_ERROR_BUSY.
 *			It achieves the data reading from EEPROM without internal buffers for receiving the data via SPI.
 *			Therefore the rx_data-buffer is modified during the function, so the first 4 dummy bytes during the SPI transfer could be placed
 *			in this buffer and be overwritten by the actual 4 data bytes after transmission.
 *			This operation is splitted in three parts. At the beginning the first 4 data bytes (or less) are read in blocking mode via an internal allocated buffer.
 *			After that the remaining bytes are read in non-blocking mode (with the 4 dummy bytes in the first 4 rx_data-buffer bytes).
 *			In the end if the spi transmit operation terminated and the internal spi-handler is called, the first 4 data bytes are restored to the rx_data-buffer.
 *	
 * @warning The rx_data-buffer must be kept in memory until the operation has terminated.
 *
 * @param[in]   address			Address of the data to be read from EEPROM. 	   	
 * @param[in]   tx_data			Pointer to the buffer where to store the read data. It must be in Data RAM region.
 * @param[in]   length_tx_data	Length of the data to read.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If the SPI interface or the EEPROM is busy.
 * @retval 	NRF_ERROR_INVALID_PARAM   	If the address is to big or the provided rx_data-buffer is not placed in the Data RAM region.
 */
ret_code_t eeprom_read_bkgnd(uint32_t address, uint8_t* rx_data, uint32_t length_rx_data);


/**@brief   Function for reading data in blocking mode from EEPROM via SPI.
 *
 * @details This function uses internally eeprom_read_bkgnd() to read the data and eeprom_get_operation()
 *			to wait until the operation (spi transfer) has terminated. 
 *	
 *
 * @param[in]   address			Address of the data to be read from EEPROM.  	   	
 * @param[in]   rx_data			Pointer to the buffer where to store the read data. It must be in Data RAM region.
 * @param[in]   length_rx_data	Length of the data to read.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If the SPI interface or the EEPROM is busy.
 * @retval 	NRF_ERROR_INVALID_PARAM   	If the address is to big or the provided rx_data-buffer is not placed in the Data RAM region.
 */
ret_code_t eeprom_read(uint32_t address, uint8_t* rx_data, uint32_t length_rx_data);


/**@brief   Function for retrieving the current operation.
 *
 * @details This function actually only return the current eeprom operation. 
 *			Furthermore, it checks if the SPI or EEPROM is busy. If this is the case and if the current
 *			operation is EEPROM_NO_OPERATION, it sets the operation to EEPROM_STORE_OPERATION.
 *			If the SPI and EEPROM is not busy, it resets the current operation to EEPROM_NO_OPERATION.
 *	
 *
 * @retval  EEPROM_NO_OPERATION         If there is currently no EEPROM (and SPI) operation is ongoing.
 * @retval  EEPROM_STORE_OPERATION		If there is currently a store-operation ongoing.
 * @retval 	EEPROM_READ_OPERATION   	If there is currently a read-operation ongoing.
 */
eeprom_operation_t eeprom_get_operation(void);


/**@brief   Function for reading the number of bytes in the EEPROM.
 *
 * @retval  Number of bytes in the EEPROM.
 */
uint32_t eeprom_get_size(void);

/**@brief   Function for testing the eeprom module.
 *
 * @retval  0	If selftest failed.
 * @retval  1	If selftest passed.
 */
bool eeprom_selftest(void);


#endif