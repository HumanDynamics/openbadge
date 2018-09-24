#include "eeprom_lib.h"


#include "debug_lib.h"


#include "string.h"		// For memset

#include "storage_file_lib.h"


static uint8_t eeprom_data[EEPROM_SIZE];	/**< Simulator of the external EEPROM data bytes */


static volatile eeprom_operation_t eeprom_operation = EEPROM_NO_OPERATION; /**< The current EEPROM operation */



/**@brief   Function for initializing the in the simulated EEPROM module.
 *
 * @details If there is no EEPROM file there this function creates one and initializes the file with 0xFF.
 *			
 *
 * @retval  NRF_SUCCESS    		If the module was successfully initialized.
 */
ret_code_t eeprom_init(void) {
	
	
	memset(eeprom_data, 0xFF, EEPROM_SIZE);	
	
	//debug_log("EEPROM initialized\n");	
	return  NRF_SUCCESS;
}


/**@brief   Function for storing bytes to simulated EEPROM (it is actually not non-blocking).
 *
 * @details Tries to store the bytes in the simulated EEPROM. 
 * 			It updates the file and the internal RAM-array accordingly.	
 *			
 *
 * @retval  NRF_SUCCESS    				If the store operation was successfully.
 * @retval  NRF_ERROR_BUSY  			If there is already an ongoing EEPROM operation.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 */
ret_code_t eeprom_store_bkgnd(uint32_t address, const uint8_t* tx_data, uint32_t length_tx_data) {
	
	
	
	// Check if the specified address is valid. The EEPROM has 256kByte of memory.
	if((address + length_tx_data > (EEPROM_SIZE)))
		return NRF_ERROR_INVALID_PARAM;
	
	if(tx_data == NULL) 
		return NRF_ERROR_INVALID_PARAM;
	
	
	// Check if the EEPROM has already an ongoing operation.
	if(eeprom_get_operation() != EEPROM_NO_OPERATION) {
		return NRF_ERROR_BUSY;
	}
	
	eeprom_operation = EEPROM_STORE_OPERATION;
	
	// Set data also in the RAM array (to read from it)
	memcpy(&eeprom_data[address], tx_data, length_tx_data);
		
	
	eeprom_operation = EEPROM_NO_OPERATION;
	
	
	return NRF_SUCCESS;
	
}

/**@brief   Function for storing bytes to simulated EEPROM (it is actually the same as eeprom_store_bkgnd()).
 *
 * @retval  NRF_SUCCESS    				If the store operation was successfully.
 * @retval  NRF_ERROR_BUSY  			If there is already an ongoing EEPROM operation.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 */
ret_code_t eeprom_store(uint32_t address, const uint8_t* tx_data, uint32_t length_tx_data) {
	// Start a background store operation
	ret_code_t ret = eeprom_store_bkgnd(address, tx_data, length_tx_data);
	if(ret != NRF_SUCCESS) {
		return ret;
	}
	
	// Wait until the operation terminates
	while(eeprom_get_operation() == EEPROM_STORE_OPERATION);
	
	return NRF_SUCCESS;
}






/**@brief   Function for reading words from simulated EEPROM (it is actually not non-blocking).
 *
 *  
 * @details Reads the words from the internal RAM-array.
 * 
 * @retval  NRF_SUCCESS    				If the read operation was successfully.
 * @retval  NRF_ERROR_BUSY  			If there is already an ongoing EEPROM operation.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 */
ret_code_t eeprom_read_bkgnd(uint32_t address, uint8_t* rx_data, uint32_t length_rx_data) {
	// Check if the specified address is valid. The EEPROM has 256kByte of memory.
	if((address + length_rx_data > (EEPROM_SIZE)))
		return NRF_ERROR_INVALID_PARAM;
	
	if(rx_data == NULL)
		return NRF_ERROR_INVALID_PARAM;
	
	// Check if the EEPROM has already an ongoing operation.
	if(eeprom_get_operation() != EEPROM_NO_OPERATION) {
		return NRF_ERROR_BUSY;
	}
	
	eeprom_operation = EEPROM_READ_OPERATION;
	
	// Set data also in array
	memcpy(rx_data, &eeprom_data[address], length_rx_data);
	
	
	eeprom_operation = EEPROM_NO_OPERATION;
	
	return NRF_SUCCESS;
	
}

/**@brief   Function for reading words from simulated EEPROM (it is actually the same as eeprom_read_bkgnd()).
 *
 * @retval  NRF_SUCCESS    				If the read operation was successfully.
 * @retval  NRF_ERROR_BUSY  			If there is already an ongoing EEPROM operation.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 */
ret_code_t eeprom_read(uint32_t address, uint8_t* rx_data, uint32_t length_rx_data) {
	// Start a background read operation
	ret_code_t ret = eeprom_read_bkgnd(address, rx_data, length_rx_data);
	if(ret != NRF_SUCCESS) {
		return ret;
	}
	
	// Wait until the operation terminates
	while(eeprom_get_operation() == EEPROM_READ_OPERATION);
	
	return NRF_SUCCESS;
	
}




eeprom_operation_t eeprom_get_operation(void) {	
	
	return eeprom_operation;
	
}



uint32_t eeprom_get_size(void) {
	return EEPROM_SIZE;
}


void eeprom_write_to_file(const char* filename) {
	uint8_t* bytes = (uint8_t*) eeprom_data;
	uint32_t len = EEPROM_SIZE;
	storage_file_write_to_file(filename, bytes, len);
}


