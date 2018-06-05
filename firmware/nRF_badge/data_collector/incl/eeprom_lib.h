#ifndef __EEPROM_LIB_H
#define __EEPROM_LIB_H


/** @file
 *
 * @brief Atmel AT25M02 EEPROM-IC abstraction library.
 *
 * @details 
 *
 * @note   
 */
 
#include <stdbool.h>
#include "sdk_common.h"	// Needed for the definition of ret_code_t and the error-codes
 
 
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
 * @retval  NRF_ERROR_INTERNAL  If there was an error while initializing the spi-module.
 * @retval 	NRF_ERROR_BUSY		If the unprotecting operation failed (because of an ongoing spi operation).
 */
ret_code_t eeprom_init(void);



ret_code_t eeprom_store_bkgnd(uint32_t address, uint8_t* tx_data, uint32_t length_tx_data);

ret_code_t eeprom_store(uint32_t address, uint8_t* tx_data, uint32_t length_tx_data);

ret_code_t eeprom_read_bkgnd(uint32_t address, uint8_t* rx_data, uint32_t length_rx_data);

ret_code_t eeprom_read(uint32_t address, uint8_t* rx_data, uint32_t length_rx_data);


eeprom_operation_t eeprom_get_operation(void);

bool eeprom_selftest(void);


#endif