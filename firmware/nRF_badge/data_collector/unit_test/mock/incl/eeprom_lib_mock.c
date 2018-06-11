#include "eeprom_lib.h"


#include "debug_lib.h"

#include "stdio.h"		// For reading and writing to file
#include "sys/stat.h"	// For checking size of the file
#include "string.h"		// For memset


#define EEPROM_SIZE				(1024*256)				/**< Size of the external EEPROM in bytes */
#define	EEPROM_FILE_PATH		"../mock/incl/_build/EEPROM.txt"	/**< Path to the EEPROM file, relative to build directory */





static uint8_t eeprom_data[EEPROM_SIZE];	/**< Simulator of the external EEPROM data bytes */


static FILE* file_descriptor;												/**< File descriptor of the EEPROM file */
static volatile eeprom_operation_t eeprom_operation = EEPROM_NO_OPERATION; /**< The current EEPROM operation */



static uint8_t init_done = 0;

/**@brief   Function for initializing the in the simulated EEPROM module.
 *
 * @details If there is no EEPROM file there this function creates one and initializes the file with 0xFF.
 *			
 *
 * @retval  NRF_SUCCESS    		If the module was successfully initialized.
 * @retval  NRF_ERROR_INTERNAL  If there was an error while creating, writing or reading the file.
 */
ret_code_t eeprom_init(void) {
	
	init_done = 1;
	
	// https://stackoverflow.com/questions/30133210/check-if-file-is-empty-or-not-in-c?rq=1&utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
	struct stat fileStat;
	int ret_stat = stat(EEPROM_FILE_PATH, &fileStat);
	
	// Check if EEPROM-File does not exist or has the false size.
	if(ret_stat != 0 || fileStat.st_size != EEPROM_SIZE) {	
		debug_log("EEPROM file does not exist or has the false size. Try creating one..\n");
		memset(eeprom_data, 0xFF, EEPROM_SIZE);
		file_descriptor= fopen(EEPROM_FILE_PATH, "wb");
		if(file_descriptor == NULL) {	// if we can not generate the file, return an error.
			debug_log("Could not generate EEPROM-file: %s\n", EEPROM_FILE_PATH);
			return NRF_ERROR_INTERNAL;
		}
		fwrite(eeprom_data, sizeof(uint8_t), sizeof(eeprom_data), file_descriptor);
		fclose(file_descriptor);
	}
	
	// Read data in from the file
	file_descriptor= fopen(EEPROM_FILE_PATH, "rb");
	if(file_descriptor == NULL) {	// if we can not generate the file, return an error.
		debug_log("Could not read from EEPROM-file: %s\n", EEPROM_FILE_PATH);
		return NRF_ERROR_INTERNAL;
	}
	size_t result = fread(eeprom_data, sizeof(uint8_t), sizeof(eeprom_data), file_descriptor);
	if(result != sizeof(eeprom_data)) {
		debug_log("Could not read %u bytes from EEPROM-file: %s\n", sizeof(eeprom_data), EEPROM_FILE_PATH);
		fclose(file_descriptor);
		return NRF_ERROR_INTERNAL;
	}
	fclose(file_descriptor);
	
	
	
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
 * @retval  NRF_ERROR_BUSY  			If there was an error with opening the file or there is already an ongoing EEPROM operation.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 */
ret_code_t eeprom_store_bkgnd(uint32_t address, uint8_t* tx_data, uint32_t length_tx_data) {
	
	
	//return NRF_ERROR_INTERNAL;
	
	if(init_done == 0) {
		return NRF_ERROR_INTERNAL;
	}
		
	
	
	// Check if the specified address is valid. The EEPROM has 256kByte of memory.
	if((address + length_tx_data > (EEPROM_SIZE)))
		return NRF_ERROR_INVALID_PARAM;
	
	// Check if the EEPROM has already an ongoing operation.
	if(eeprom_get_operation() != EEPROM_NO_OPERATION) {
		return NRF_ERROR_BUSY;
	}
	
	eeprom_operation = EEPROM_STORE_OPERATION;
	
	
	
	
	// Write the changes to file
	if((file_descriptor = fopen(EEPROM_FILE_PATH, "r+b")) == NULL) //open the file for updating
		return NRF_ERROR_BUSY;	// TODO, probably other error code?
		
	fseek(file_descriptor, address, SEEK_SET);	//set the stream pointer address bytes from the start.
	fwrite(tx_data, sizeof(uint8_t), length_tx_data, file_descriptor);
	fclose(file_descriptor);
	
	// Set data also in the RAM array (to read from it)
	memcpy(&eeprom_data[address], tx_data, length_tx_data);
	
	
	eeprom_operation = EEPROM_NO_OPERATION;
	
	
	return NRF_SUCCESS;
	
}

/**@brief   Function for storing bytes to simulated EEPROM (it is actually the same as eeprom_store_bkgnd()).
 *
 * @retval  NRF_SUCCESS    				If the store operation was successfully.
 * @retval  NRF_ERROR_BUSY  			If there was an error with opening the file or there is already an ongoing Flash operation.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 */
ret_code_t eeprom_store(uint32_t address, uint8_t* tx_data, uint32_t length_tx_data) {
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
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified parameters are bad.
 */
ret_code_t eeprom_read_bkgnd(uint32_t address, uint8_t* rx_data, uint32_t length_rx_data) {
	// Check if the specified address is valid. The EEPROM has 256kByte of memory.
	if((address + length_rx_data > (EEPROM_SIZE)))
		return NRF_ERROR_INVALID_PARAM;
	
	if(rx_data == NULL)
		return NRF_ERROR_INVALID_PARAM;
	
	eeprom_operation = EEPROM_READ_OPERATION;
	
	// Set data also in array
	memcpy(rx_data, &eeprom_data[address], length_rx_data);
	
	
	eeprom_operation = EEPROM_NO_OPERATION;
	
	return NRF_SUCCESS;
	
}

/**@brief   Function for reading words from simulated EEPROM (it is actually the same as eeprom_read_bkgnd()).
 *
 * @retval  NRF_SUCCESS    				If the read operation was successfully.
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

bool eeprom_selftest(void) {
	
	
	return 1;
}





