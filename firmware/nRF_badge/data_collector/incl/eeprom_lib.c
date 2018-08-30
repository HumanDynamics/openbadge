#include "eeprom_lib.h"

#include "spi_lib.h"
#include "nrf_drv_common.h"

#include "debug_lib.h"

#include "systick_lib.h"		// Needed for the timeout-checks



#define CMD_WREN	0b00000110	/**< Write enable command code */
#define CMD_RDSR	0b00000101	/**< Read status register command code */
#define CMD_WRSR	0b00000001	/**< Write status register command code */
#define CMD_WRITE	0b00000010	/**< Write to memory array command code */
#define CMD_READ	0b00000011	/**< Read from memory array command code */

#define EEPROM_OPERATION_TIMEOUT_MS		100	/**< The time in milliseconds to wait for an operation to finish. */


static ret_code_t eeprom_read_status(uint8_t* eeprom_status);
static bool eeprom_is_busy(void);
static ret_code_t eeprom_write_enable(void);
static ret_code_t eeprom_global_unprotect(void);





typedef struct {
	uint8_t 	first_data[4];
	uint8_t*	p_data;
} eeprom_finish_operation_t;



static spi_instance_t spi_instance;	/**< The spi instance used to communicate with the EEPROM-IC via SPI */

static volatile eeprom_operation_t eeprom_operation = EEPROM_NO_OPERATION; /**< The current EEPROM operation */

static volatile eeprom_finish_operation_t eeprom_finish_operation;




// It is a little bit more complicate than flash, because there is no callback when the actual write operation is done in EEPROM!
void spi_event_handler(spi_evt_t const * p_event) {
	if(p_event->type == SPI_TRANSFER_DONE) {
		// Just copy the replaced 4 bytes at the beginning back to the data-array (rx_data or tx_data)
		memcpy((uint8_t*) eeprom_finish_operation.p_data, (uint8_t*) eeprom_finish_operation.first_data, 4);
	}
}





ret_code_t eeprom_init(void) {
	
	// TODO: retrieve Pin-numbers from the custom_board-file!
	
	spi_instance.spi_peripheral = 0;
	spi_instance.nrf_drv_spi_config.frequency 		= NRF_DRV_SPI_FREQ_8M;
	spi_instance.nrf_drv_spi_config.bit_order 		= NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;
	spi_instance.nrf_drv_spi_config.mode			= NRF_DRV_SPI_MODE_3;
	spi_instance.nrf_drv_spi_config.orc				= 0;
	spi_instance.nrf_drv_spi_config.irq_priority	= 1; //APP_IRQ_PRIORITY_MID;	
	spi_instance.nrf_drv_spi_config.ss_pin 			= 0;
	spi_instance.nrf_drv_spi_config.miso_pin		= 1;
	spi_instance.nrf_drv_spi_config.mosi_pin		= 4;
	spi_instance.nrf_drv_spi_config.sck_pin			= 3;
	
	ret_code_t ret = spi_init(&spi_instance);
	
	// ret could be NRF_SUCCESS or NRF_ERROR_INVALID_PARAM
	if(ret != NRF_SUCCESS)
		return NRF_ERROR_INTERNAL;
	
	// Directly unprotect the EEPROM in the initialization step
	ret = eeprom_global_unprotect();
	
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY	or NRF_ERROR_TIMEOUT
	
	return ret;
}





/**@brief   Function for reading the status of the EEPROM.		
 *
 * @param[in,out]	eeprom_status		Pointer to memory where to save current eeprom status.
 *
 * @retval  		NRF_SUCCESS    		If the spi operation was succesful.
 * @retval  		NRF_ERROR_BUSY  	If there is already an SPI operation ongoing on the same spi peripheral.
 */
static ret_code_t eeprom_read_status(uint8_t* p_eeprom_status) {
	uint8_t tx_buf[1] = {CMD_RDSR};
    uint8_t rx_buf[2] = {0, 0x01};
					 
	ret_code_t ret = spi_transmit_receive(&spi_instance, tx_buf, 1, rx_buf, 2);
	
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INVALID_ADDR (the last one can't happen here)
    if(ret != NRF_SUCCESS)
		ret = NRF_ERROR_BUSY;
	
	
	*p_eeprom_status = rx_buf[1];
	
	return ret;
}

/**@brief   Function for checking if the EEPROM is busy.		
 *
 * @retval  0    		If the EEPROM and the spi interface is not busy.
 * @retval  1  			If the EEPROM or the spi interface is busy.
 */
static bool eeprom_is_busy(void) {
	uint8_t eeprom_status = 0x01;
	ret_code_t ret = eeprom_read_status(&eeprom_status);
	
	// Check if the SPI is busy
	if(ret != NRF_SUCCESS)
		return 1;
	
	// Check if busy flag of the EEPROM status is set
	if(eeprom_status & 0x01) {
		return 1;
	}
	return 0;
}



/**@brief   Function for enabling a write operation to the EEPROM.		
 *
 * @retval  NRF_SUCCESS    		If the spi transmit operation was succesful.
 * @retval  NRF_ERROR_BUSY  	If there is already an SPI operation ongoing on the same spi peripheral.
 * @retval	NRF_ERROR_TIMEOUT	If the operation takes too long.
 */
static ret_code_t eeprom_write_enable(void) {
	// Generating one-byte buffer for writing the "write enable"-command
	uint8_t tx_buf[1] = {CMD_WREN}; 
	
	// SPI transmit in blocking mode
	ret_code_t ret = spi_transmit(&spi_instance, tx_buf, 1);
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INVALID_ADDR (the last one can't happen here)
	if(ret != NRF_SUCCESS) {
		return NRF_ERROR_BUSY;
	}
	

	uint64_t end_ms = systick_get_millis() + EEPROM_OPERATION_TIMEOUT_MS;
	while(eeprom_get_operation() != EEPROM_NO_OPERATION && systick_get_millis() < end_ms);
	if(eeprom_get_operation() != EEPROM_NO_OPERATION) {
		// Reset the eeprom operation
		eeprom_operation = EEPROM_NO_OPERATION;
		return NRF_ERROR_TIMEOUT;
	}

	return NRF_SUCCESS;
}

/**@brief   Function for unprotecting all EEPROM blocks for writing to them.		
 *
 * @retval  NRF_SUCCESS    		If the spi transmit operation was succesful.
 * @retval  NRF_ERROR_BUSY  	If there is already an SPI operation ongoing on the same spi peripheral.
 * @retval	NRF_ERROR_TIMEOUT	If the operation takes too long.
 */
static ret_code_t eeprom_global_unprotect(void) {
	
	// Enable writing
	ret_code_t ret = eeprom_write_enable();
	if(ret != NRF_SUCCESS)
		return ret;
	
	// Generating two-byte buffer for writing the "write status register"-command and the new value of the status register
	// Status register format (Datasheet p.20, Table 6)  | SRWD | 0 | 0 | 0 | BP1 | BP0 | WEL | WIP |
	// Only the Bits SRWD (Status Register Write Protect), BP1 and BP0 (Block Protect) can be manipulated
	uint8_t tx_buf[2] = {CMD_WRSR, 0x00};

	// SPI transmit in blocking mode
	ret = spi_transmit(&spi_instance, tx_buf, 2);
	
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INVALID_ADDR (the last one can't happen here)
    if(ret != NRF_SUCCESS)
		ret = NRF_ERROR_BUSY;
	
	uint64_t end_ms = systick_get_millis() + EEPROM_OPERATION_TIMEOUT_MS;
	while(eeprom_get_operation() != EEPROM_NO_OPERATION && systick_get_millis() < end_ms);
	if(eeprom_get_operation() != EEPROM_NO_OPERATION) {
		// Reset the eeprom operation
		eeprom_operation = EEPROM_NO_OPERATION;
		return NRF_ERROR_TIMEOUT;
	}
	
	
	return ret;	
}







ret_code_t eeprom_store_bkgnd(uint32_t address, uint8_t* tx_data, uint32_t length_tx_data) {
	
	// Check if the specified address is valid. The EEPROM has 256kByte of memory.
	// Furthermore check if the data pointer is in RAM, not in read only-Memory
	if((address + length_tx_data > (EEPROM_SIZE)) ||  !nrf_drv_is_in_RAM(tx_data))
		return NRF_ERROR_INVALID_PARAM;
	
	
	if(tx_data == NULL)
		return NRF_ERROR_INVALID_PARAM;
	
	// Check if the EEPROM has already an ongoing operation.
	if(eeprom_get_operation() != EEPROM_NO_OPERATION) {
		return NRF_ERROR_BUSY;
	}
	
	// Set the eeprom operation to store
	eeprom_operation = EEPROM_STORE_OPERATION;
	
	
	// Enable writing
	ret_code_t ret = eeprom_write_enable();
	
	// ret could be NRF_SUCCESS or NRF_ERROR_BUSY	
	if(ret != NRF_SUCCESS) {
		// Reset the eeprom operation
		eeprom_operation = EEPROM_NO_OPERATION;
		return ret;
	}
	
	
	
	
	// The header for writing {CMD_WRITE, address24_16, address15_8, address7_0}
	uint8_t tx_header[4] = {CMD_WRITE, ((address>>16)&0xFF), ((address>>8)&0xFF), ((address>>0)&0xFF)};
	
	
	// At the beginning the first 4 bytes (or less) are stored
	uint32_t first_length_tx_data = 0;
	uint32_t remaining_length_tx_data = 0;
	
	if(length_tx_data < 4) {
		first_length_tx_data = length_tx_data;
		remaining_length_tx_data = 0;
	} else {
		first_length_tx_data = 4;
		remaining_length_tx_data = length_tx_data - 4;
	}
	
	// Temporary array for transmitting the first 4 data bytes.
	uint32_t len = 4 + first_length_tx_data;
	uint8_t first_transmit[len];
	// Copy the header into the first_transmit-array
	memcpy(&first_transmit[0], tx_header, 4);
	memcpy(&first_transmit[4], tx_data, first_length_tx_data);
	
	
	// Start a blocking spi transmission
	ret = spi_transmit(&spi_instance, first_transmit, len);
	
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INVALID_ADDR
	if(ret == NRF_ERROR_INVALID_ADDR)
		ret = NRF_ERROR_INVALID_PARAM;
	if(ret != NRF_SUCCESS) {		
		// Reset the eeprom operation
		eeprom_operation = EEPROM_NO_OPERATION;
		return ret;
	}
	
	// Wait until the first 4 data bytes are stored into the EEPROM (important to not just check if they have been transmitted via spi, because the EEPROM needs time to store it internally)
	uint64_t end_ms = systick_get_millis() + EEPROM_OPERATION_TIMEOUT_MS;
	while(eeprom_get_operation() != EEPROM_NO_OPERATION && systick_get_millis() < end_ms);
	if(eeprom_get_operation() != EEPROM_NO_OPERATION) {
		// Reset the eeprom operation
		eeprom_operation = EEPROM_NO_OPERATION;
		return NRF_ERROR_TIMEOUT;
	}
	

		
	// Now reschedule the store operation of the remaining bytes (if length_tx_data is > 4) in real background mode!
	if(remaining_length_tx_data > 0) {
		
		// Set the eeprom operation to store again
		eeprom_operation = EEPROM_STORE_OPERATION;
		
		ret = eeprom_write_enable();
	
		// ret could be NRF_SUCCESS or NRF_ERROR_BUSY	
		if(ret != NRF_SUCCESS) {
			// Reset the eeprom operation
			eeprom_operation = EEPROM_NO_OPERATION;
			return ret;
		}
		
		
		// Save the first 4 data bytes into the eeprom_finish_operation-type for resetting the first 4 bytes of tx_data in the spi-interrupt handler when the transmission is done.
		eeprom_finish_operation.p_data = tx_data;
		memcpy((uint8_t*) eeprom_finish_operation.first_data, tx_data, 4);
		
		// replace the first 4 Bytes of the tx_data with the new header
		address = address + 4;
		tx_header[1] = ((address>>16)&0xFF);
		tx_header[2] = ((address>>8)&0xFF);
		tx_header[3] = ((address>>0)&0xFF);
		
		memcpy(tx_data, tx_header, 4);
		len = length_tx_data;
		
		ret = spi_transmit_bkgnd(&spi_instance, spi_event_handler, tx_data, len);
		
		// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INVALID_ADDR
		if(ret == NRF_ERROR_INVALID_ADDR)
			ret = NRF_ERROR_INVALID_PARAM;
		if(ret != NRF_SUCCESS) {
			// Reset the eeprom operation
			eeprom_operation = EEPROM_NO_OPERATION;
			return ret;
		}
		
	}
	
	return NRF_SUCCESS;
	
}


ret_code_t eeprom_store(uint32_t address, uint8_t* tx_data, uint32_t length_tx_data) {
	// Start a background store operation
	ret_code_t ret = eeprom_store_bkgnd(address, tx_data, length_tx_data);
	if(ret != NRF_SUCCESS) {
		return ret;
	}
	
	// Wait until the operation terminates
	uint64_t end_ms = systick_get_millis() + EEPROM_OPERATION_TIMEOUT_MS;
	while(eeprom_get_operation() == EEPROM_STORE_OPERATION && systick_get_millis() < end_ms);
	if(eeprom_get_operation() == EEPROM_STORE_OPERATION) {
		// Reset the eeprom operation
		eeprom_operation = EEPROM_NO_OPERATION;
		return NRF_ERROR_TIMEOUT;
	}
	
	return NRF_SUCCESS;
}







ret_code_t eeprom_read_bkgnd(uint32_t address, uint8_t* rx_data, uint32_t length_rx_data) {
	
	// Check if the specified address is valid. The EEPROM has 256kByte of memory.
	// Furthermore check if the data pointer is in RAM, not in read only-Memory
	if((address + length_rx_data > (EEPROM_SIZE) ) ||  !nrf_drv_is_in_RAM(rx_data))
		return NRF_ERROR_INVALID_PARAM;

	if(rx_data == NULL)
		return NRF_ERROR_INVALID_PARAM;
	
	// Check if the EEPROM has already an ongoing operation.
	if(eeprom_get_operation() != EEPROM_NO_OPERATION) {
		return NRF_ERROR_BUSY;
	}
	
	
	
	// Set the eeprom operation to read
	eeprom_operation = EEPROM_READ_OPERATION;
	
	
	// The header for reading data via spi from the EEPROM
	uint8_t tx_header[4] = {CMD_READ,  ((address>>16)&0xFF), ((address>>8)&0xFF), (address&0xFF)};
	
	
	// At the beginning the first 4 bytes (or less) are read
	uint32_t first_length_rx_data = 0;
	uint32_t remaining_length_rx_data = 0;
	if(length_rx_data < 4) {
		first_length_rx_data = length_rx_data;
		remaining_length_rx_data = 0;
	} else {
		first_length_rx_data = 4;
		remaining_length_rx_data = length_rx_data - 4;
	}
	
	
	uint32_t len = 4 + first_length_rx_data;
	uint8_t first_receive[len];
	
	
	// Do a blocking transmit receive operation
	ret_code_t ret = spi_transmit_receive(&spi_instance, tx_header, 4, first_receive, len);
	
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INVALID_ADDR
	if(ret == NRF_ERROR_INVALID_ADDR)
		ret = NRF_ERROR_INVALID_PARAM;
	if(ret != NRF_SUCCESS) {		
		// Reset the eeprom operation
		eeprom_operation = EEPROM_NO_OPERATION;
		return ret;
	}
	
	// Wait until the operation terminates
	uint64_t end_ms = systick_get_millis() + EEPROM_OPERATION_TIMEOUT_MS;
	while(eeprom_get_operation() != EEPROM_NO_OPERATION && systick_get_millis() < end_ms);
	if(eeprom_get_operation() != EEPROM_NO_OPERATION) {
		// Reset the eeprom operation
		eeprom_operation = EEPROM_NO_OPERATION;
		return NRF_ERROR_TIMEOUT;
	}
	
	
	
	// copy the first data to the output buffer
	memcpy(rx_data, &first_receive[4], first_length_rx_data);
	
	
	
	// Now reschedule the read operation of the remaining bytes (if length_rx_data is > 4) in real background mode!
	if(remaining_length_rx_data > 0) {
		
		// Set the eeprom operation to read again
		eeprom_operation = EEPROM_READ_OPERATION;
	
		
		// Save the first 4 data bytes into the eeprom_finish_operation-type for resetting the first 4 bytes of rx_data in the spi-interrupt handler when the transmission is done.
		eeprom_finish_operation.p_data = rx_data;
		memcpy((uint8_t*) eeprom_finish_operation.first_data, &first_receive[4], 4);
		
		
		// recompute the header
		address = address + 4;
		tx_header[1] = ((address>>16)&0xFF);
		tx_header[2] = ((address>>8)&0xFF);
		tx_header[3] = ((address>>0)&0xFF);
		
	
		len = 4 + remaining_length_rx_data;
		
	
		// Start the background spi transmit receive operation
		ret = spi_transmit_receive_bkgnd(&spi_instance, spi_event_handler, tx_header, 4, rx_data, len);


		// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INVALID_ADDR
		if(ret == NRF_ERROR_INVALID_ADDR)
			ret = NRF_ERROR_INVALID_PARAM;
		if(ret != NRF_SUCCESS) {
			// Reset the eeprom operation
			eeprom_operation = EEPROM_NO_OPERATION;
			return ret;
		}
	}
	
	
	return NRF_SUCCESS;
	
}

ret_code_t eeprom_read(uint32_t address, uint8_t* rx_data, uint32_t length_rx_data) {
	// Start a background read operation
	ret_code_t ret = eeprom_read_bkgnd(address, rx_data, length_rx_data);
	if(ret != NRF_SUCCESS) {
		return ret;
	}
	
	// Wait until the operation terminates
	uint64_t end_ms = systick_get_millis() + EEPROM_OPERATION_TIMEOUT_MS;
	while(eeprom_get_operation() == EEPROM_READ_OPERATION && systick_get_millis() < end_ms);
	if(eeprom_get_operation() == EEPROM_READ_OPERATION) {
		// Reset the eeprom operation
		eeprom_operation = EEPROM_NO_OPERATION;
		return NRF_ERROR_TIMEOUT;
	}
	
	return NRF_SUCCESS;
	
}




eeprom_operation_t eeprom_get_operation(void) {	
	
	// Reset the eeprom operation if the EEPROM and the spi interface is not busy (this could not be done in an interrupt handler,
	// because there is no interrupt from the EEPROM only from the SPI, but this is not representative for internal EEPROM operation)
	if(eeprom_is_busy()) {		
		// If no operation is set, but the EEPROM seems to be busy, there should be an store operation ongoing (e.g. unprotect-function)
		if(eeprom_operation == EEPROM_NO_OPERATION)
			eeprom_operation = EEPROM_STORE_OPERATION;
	} else {
		eeprom_operation = EEPROM_NO_OPERATION;
		
	}
	
	return eeprom_operation;
	
}



uint32_t eeprom_get_size(void) {
	return EEPROM_SIZE;
}

bool eeprom_selftest(void) {
	
	#define EEPROM_TEST_DATA_LEN	40
	#define EEPROM_TEST_ADDRESS		100
	
	uint8_t data[EEPROM_TEST_DATA_LEN];
	uint8_t rx_data[EEPROM_TEST_DATA_LEN+1];
	
	for(uint32_t i = 0; i < EEPROM_TEST_DATA_LEN; i++) {
		data[i] = (i%26)+65;
	}
	
	
	
	debug_log("Started EEPROM selftest...\n\r");
	
	
	
//********************** Read and write Tests ***********************
	ret_code_t ret = eeprom_store(EEPROM_TEST_ADDRESS, (uint8_t*) data, EEPROM_TEST_DATA_LEN);
	debug_log("Test store, Ret: %d\n\r", ret);
	if(ret != NRF_SUCCESS) {
		debug_log("Store failed!\n\r");
		return 0;
	}
	
	
	
	ret = eeprom_read(EEPROM_TEST_ADDRESS, rx_data, EEPROM_TEST_DATA_LEN);
	rx_data[EEPROM_TEST_DATA_LEN] = 0;
	
	debug_log("Test read, Ret: %d, Read: %s\n\r", ret, rx_data);
	if(ret != NRF_SUCCESS) {
		debug_log("Read failed!\n\r");
		return 0;
	}
	
	
	if(memcmp(data, rx_data, EEPROM_TEST_DATA_LEN) != 0) {
		debug_log("Read data don't match written data!");
		return 0;
	}
	
//********************* No RAM memory tests *************************
	char* test = "HELLO";
	ret = eeprom_store(EEPROM_TEST_ADDRESS, (uint8_t*) test, 5);
	
	debug_log("Test store non RAM data, Ret: %d\n\r", ret);
	if(ret != NRF_ERROR_INVALID_PARAM) {
		debug_log("Test store non RAM data failed!\n\r");
		return 0;
	}
	
	
	ret = eeprom_read(EEPROM_TEST_ADDRESS, (uint8_t*) test, 5);
	
	debug_log("Test read to non RAM data, Ret: %d\n\r", ret);
	if(ret != NRF_ERROR_INVALID_PARAM) {
		debug_log("Test read to non RAM data failed!\n\r");
		return 0;
	}

//******************* False address test **************************
	ret = eeprom_store(EEPROM_SIZE - 2, (uint8_t*) data, EEPROM_TEST_DATA_LEN);
	
	debug_log("Test invalid address, Ret: %d\n\r", ret);
	if(ret != NRF_ERROR_INVALID_PARAM) {
		debug_log("Test invalid address failed!\n\r");
		return 0;
	}
	
	debug_log("EEPROM test successful!!\n\r");	
	
	
	return 1;
}





