#include "uart_lib.h"



#include <stdarg.h>		// Needed for the printf-function
#include <string.h>		// Needed for the printf-function
#include <stdio.h>		// Needed for the vsnprintf-function



#define UART_PERIPHERAL_NUMBER 			UART_COUNT	/**< Number of activated uart peripherals in sdk_config.h. */


#define MAX_BYTES_PER_TRANSMIT_OPERATION	255		/**< Maximum number of bytes transmitted in each transmission step (large packets are split). */



static volatile uart_operation_t  	uart_operations[UART_PERIPHERAL_NUMBER] 	= {0};	/**< Array to save the current uart_operations (needed to check if there is an ongoing uart operation on the peripheral) */
static uart_instance_t *			uart_instances[UART_PERIPHERAL_NUMBER] 	= {NULL}; 	/**< Array of pointers to the current uart_instances (needed for rescheduling transmit or one byte receive operations) */
static uint32_t 					uart_instance_number = 1; 							/**< uart_instance_number starts at 1 not 0 because all entries in the uart_instances-arrays are 0. So the check for the uart_instance_id-element may not work correctly. */ 

static uart_handler_t				uart_handlers_transmit_bkgnd		[UART_PERIPHERAL_NUMBER];	/**< Array to save the application handlers for the transmit operation */
static uart_handler_t				uart_handlers_receive_bkgnd			[UART_PERIPHERAL_NUMBER];	/**< Array to save the application handlers for the receive operation */
static uart_handler_t				uart_handlers_receive_buffer_bkgnd	[UART_PERIPHERAL_NUMBER];	/**< Array to save the application handlers for the receive-buffer operation */


/**@brief UART transmit operation type. Needed to reschedule transmit operation that are too long. */
typedef struct {
	const uint8_t* p_remaining_data;			/**< Pointer to the next data that has to be sent */
	uint32_t remaining_data_len;				/**< Number of bytes that are pending to be sent */
} uart_transmit_operation_t;



static volatile uart_transmit_operation_t		uart_transmit_operations			[UART_PERIPHERAL_NUMBER];	/**< Array is needed to keep track of already sent data packets, to reschedule a transmit on the remaining bytes */
static volatile uint8_t							uart_receive_buffer_operations		[UART_PERIPHERAL_NUMBER];	/**< This is actually the buffer for one receive operation (1 Byte Buffer) */


/**@brief Function for handling the uart interrupts internally.
 *
 * @details This handler (only one because there is only one uart peripheral)
 * 			does different things depending on the current operation.
 *			If TRANSMIT_OPERATION, it checks for remaining bytes to transmit 
 *			and reschedules the transmit of the remaining bytes, otherwise it calls 
 *			the specified application handler (if it is not NULL) with event: UART_TRANSMIT_DONE or UART_ERROR. 
 *			If RECEIVE_OPERATION, the specified application handler (if it is not NULL) with event: UART_RECEIVE_DONE or UART_ERROR. 
 *			If UART_RECEIVE_BUFFER_OPERATION, the one byte receive function is rescheduled and calls
 *			the specified application handler (if it is not NULL) with event: UART_RECEIVE_DONE or UART_ERROR.
 */
#if UART0_ENABLED
static void uart_0_event_handler(nrf_drv_uart_event_t * p_event, void * p_context) {
	uart_evt_t evt;
	
	if(p_event->type == NRF_DRV_UART_EVT_TX_DONE) {
		
		// Check if we have some remaining bytes to send?
		uint32_t remaining_data_len = uart_transmit_operations[0].remaining_data_len;
		const uint8_t* p_remaining_data = uart_transmit_operations[0].p_remaining_data;
		if(remaining_data_len > 0 && p_remaining_data != NULL) {
			
			// Check if we are ready after this or if we have to reschedule the transmission again?
			if(remaining_data_len <= MAX_BYTES_PER_TRANSMIT_OPERATION) {
				uart_transmit_operations[0].p_remaining_data = NULL;
				uart_transmit_operations[0].remaining_data_len = 0;
			} else {
				uart_transmit_operations[0].p_remaining_data = p_remaining_data + MAX_BYTES_PER_TRANSMIT_OPERATION;
				uart_transmit_operations[0].remaining_data_len = remaining_data_len - MAX_BYTES_PER_TRANSMIT_OPERATION;
			}
			
			// Start sending the remaining bytes (here we assume that uart_instances[0] != NULL!
			nrf_drv_uart_tx(&((*uart_instances[0]).nrf_drv_uart_instance), p_remaining_data, remaining_data_len);
			
			
		} else { // We are done with transmiting
			evt.type = UART_TRANSMIT_DONE;
			
			// Clear the UART_TRANSMIT_OPERATION flag
			uart_operations[0] &= ~(UART_TRANSMIT_OPERATION);
			
			// Call the Callback handler if there exists one!
			if(uart_handlers_transmit_bkgnd[0] != NULL) {
				uart_handlers_transmit_bkgnd[0](&evt);
			}			
		}		
			
	} else if(p_event->type == NRF_DRV_UART_EVT_RX_DONE) {
		
		// Check the operation, if we are in operation receiving or receiving-buffer
		if(uart_operations[0] & UART_RECEIVE_OPERATION) {
			evt.type = UART_RECEIVE_DONE;
			
			// And disable the receive process  (here we assume that uart_instances[0] != NULL!
			nrf_drv_uart_rx_disable(&((*uart_instances[0]).nrf_drv_uart_instance));
			
			// Clear the UART_RECEIVE_OPERATION flag
			uart_operations[0] &= ~(UART_RECEIVE_OPERATION);
			
			// Call the Callback handler if there exists one!
			if(uart_handlers_receive_bkgnd[0] != NULL) {
				uart_handlers_receive_bkgnd[0](&evt);
			}	
			
		} else if(uart_operations[0] & UART_RECEIVE_BUFFER_OPERATION) {
			evt.type = UART_DATA_AVAILABLE;
			
			// (here we assume that uart_instances[0] != NULL!
			
			// Reschedule the one byte receive operation
			nrf_drv_uart_rx( &((*uart_instances[0]).nrf_drv_uart_instance), (uint8_t*) &uart_receive_buffer_operations[0], 1);
	
			
			// If you want you can check here if ((write_index + 1)%MAX == read_index) --> don't insert new elements into the buffer (until they have been read)
			// This is the case, if we have so much bytes, that our fifo will overflow --> of course in this case the new data get lost (but might be better than losing all the old data)
			// But you can just store size - 1 elements with this check!
				
			if(((*uart_instances[0]).uart_buffer.rx_buf_write_index + 1) % (*uart_instances[0]).uart_buffer.rx_buf_size != (*uart_instances[0]).uart_buffer.rx_buf_read_index) {
				
				
				// Now insert the currently received 1 Byte buffer to the uart-fifo-buffer!
				(*uart_instances[0]).uart_buffer.rx_buf  [(*uart_instances[0]).uart_buffer.rx_buf_write_index] = uart_receive_buffer_operations[0];
				// Increment the write index
				(*uart_instances[0]).uart_buffer.rx_buf_write_index = ((*uart_instances[0]).uart_buffer.rx_buf_write_index + 1) % (*uart_instances[0]).uart_buffer.rx_buf_size;
				
				
				// Call the Callback handler if there exists one!
				if(uart_handlers_receive_buffer_bkgnd[0] != NULL) {
					uart_handlers_receive_buffer_bkgnd[0](&evt);
				}			
			}
			
		}		
	} else if(p_event->type == NRF_DRV_UART_EVT_ERROR) {
		
		evt.type = UART_ERROR;
		
		// Check which operations are ongoing, call the corresponding handlers, and kill clear the operation bit!
		if(uart_operations[0] & UART_TRANSMIT_OPERATION) {
			if(uart_handlers_transmit_bkgnd[0] != NULL) {
				uart_handlers_transmit_bkgnd[0](&evt);
			}
			uart_operations[0] &= ~(UART_TRANSMIT_OPERATION);
		}
		if(uart_operations[0] & UART_RECEIVE_OPERATION) {
			if(uart_handlers_receive_bkgnd[0] != NULL) {
				uart_handlers_receive_bkgnd[0](&evt);
			}
			uart_operations[0] &= ~(UART_RECEIVE_OPERATION);
		}
		if(uart_operations[0] & UART_RECEIVE_BUFFER_OPERATION) {
			if(uart_handlers_receive_buffer_bkgnd[0] != NULL) {
				uart_handlers_receive_buffer_bkgnd[0](&evt);
			}
			uart_operations[0] &= ~(UART_RECEIVE_BUFFER_OPERATION);
		}	
		
	}
	
	
}
#endif


ret_code_t uart_init(uart_instance_t* uart_instance) {
	
	
	ret_code_t ret = NRF_SUCCESS;
	
	// Check if the selected uart peripheral exists
	if(uart_instance->uart_peripheral == 0) {
		#if UART0_ENABLED
		uart_instance->nrf_drv_uart_instance.drv_inst_idx = UART0_INSTANCE_INDEX; 
		uart_instance->nrf_drv_uart_instance.reg.p_uart = (NRF_UART_Type *) NRF_UART0;
		ret = nrf_drv_uart_init(&(uart_instance->nrf_drv_uart_instance), &(uart_instance->nrf_drv_uart_config), uart_0_event_handler);
		#else
		return NRF_ERROR_INVALID_PARAM;
		#endif
	} else {
		return NRF_ERROR_INVALID_PARAM;
	}
	
	
	// ret could be NRF_SUCCESS or NRF_ERROR_INVALID_STATE (we allow different instances on the same peripheral (because if the same uart peripheral is used by different modules)
	
	
	uart_instance->uart_instance_id = uart_instance_number;
	
	uart_instance_number++;
	
	return NRF_SUCCESS;
}



ret_code_t uart_printf_bkgnd(uart_instance_t* uart_instance, uart_handler_t uart_handler, const char* format, ...) {
	
	// Check if the tx buffer that we want to use is not NULL
	if(uart_instance->uart_buffer.tx_buf == NULL) {
		return NRF_ERROR_NULL;
	}
	
	uint32_t n = uart_instance->uart_buffer.tx_buf_size;
		
	va_list args;
	va_start(args, format);
	int ret_vsnprintf = vsnprintf((char*) uart_instance->uart_buffer.tx_buf, n, format, args);
	va_end(args);	
	
	// Check if the output of vsnprintf is correct
	if(ret_vsnprintf < 0 || ret_vsnprintf >= n) {
		return NRF_ERROR_NO_MEM;
	}
	
	
	return uart_transmit_bkgnd(uart_instance, uart_handler, uart_instance->uart_buffer.tx_buf, ret_vsnprintf);	
}


ret_code_t uart_printf_abort_bkgnd(const uart_instance_t* uart_instance) {
	return uart_transmit_abort_bkgnd(uart_instance);	
}


ret_code_t uart_printf(uart_instance_t* uart_instance, const char* format, ...) {

	// Check if the tx buffer that we want to use is not NULL
	if(uart_instance->uart_buffer.tx_buf == NULL) {
		return NRF_ERROR_NULL;
	}
	
	
	uint32_t n = uart_instance->uart_buffer.tx_buf_size;
		
	va_list args;
	va_start(args, format);
	
	int ret_vsnprintf = vsnprintf((char*) uart_instance->uart_buffer.tx_buf, n, format, args);
	va_end(args);	
	
	
	// Check if the output of vsnprintf is correct
	if(ret_vsnprintf < 0 || ret_vsnprintf >= n) {
		return NRF_ERROR_NO_MEM;
	}
	
	
	return uart_transmit(uart_instance, uart_instance->uart_buffer.tx_buf, ret_vsnprintf);	
}





ret_code_t uart_transmit_bkgnd(uart_instance_t* uart_instance, uart_handler_t uart_handler, const uint8_t* tx_data, uint32_t tx_data_len) {
	
	// Get the peripheral_index
	uint8_t peripheral_index = uart_instance->uart_peripheral;
	
	// Check if there is an ongoing transmit operation. Not check if there is no operation, because we can transmit while receiving!
	if((uart_operations[peripheral_index] & UART_TRANSMIT_OPERATION)) {
		return NRF_ERROR_BUSY;
	} 
	
	// Set the UART_TRANSMIT_OPERATION flag
	uart_operations[peripheral_index] |= UART_TRANSMIT_OPERATION;
	
	// Set the current uart instance (it is needed in transmit-case, because we must handle remaining bytes to transmit in IRQ-Handler)
	uart_instances[peripheral_index] = uart_instance;
	
	// Set the handler that has to be called
	uart_handlers_transmit_bkgnd[peripheral_index] = uart_handler;
	
	
	// Check the data length of the data to transmit. If bigger than 255, the transmission has to be splitted in smaller parts.
	if(tx_data_len <= MAX_BYTES_PER_TRANSMIT_OPERATION) {
		uart_transmit_operations[peripheral_index].p_remaining_data = NULL;
		uart_transmit_operations[peripheral_index].remaining_data_len = 0;
	} else {
		uart_transmit_operations[peripheral_index].p_remaining_data = tx_data + MAX_BYTES_PER_TRANSMIT_OPERATION;
		uart_transmit_operations[peripheral_index].remaining_data_len = tx_data_len - MAX_BYTES_PER_TRANSMIT_OPERATION;	
		tx_data_len = MAX_BYTES_PER_TRANSMIT_OPERATION; 	// Correct the data len sent with the first transmission
	}
	

	ret_code_t ret = nrf_drv_uart_tx(&(uart_instance->nrf_drv_uart_instance), tx_data, tx_data_len);
	
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_INVALID_ADDR or NRF_ERROR_FORBIDDEN
	
	// Set error code to internal to reduce error codes
	if(ret == NRF_ERROR_FORBIDDEN)
		ret = NRF_ERROR_INTERNAL;
	
	// If no success clear the UART_TRANSMIT_OPERATION flag
	
	if(ret != NRF_SUCCESS) {		
		// Stop the ongoing transmit operation
		nrf_drv_uart_tx_abort(&(uart_instance->nrf_drv_uart_instance));	
	
		// Clear the Transmit operation
		uart_operations[peripheral_index] &= ~(UART_TRANSMIT_OPERATION);
	}

	return ret;	
}


ret_code_t uart_transmit_abort_bkgnd(const uart_instance_t* uart_instance) {
	// Get the peripheral_index
	uint8_t peripheral_index = uart_instance->uart_peripheral;
	
	// Check if there is an ongoing transmit operation. If there is no transmit operation, directly return 
	if((uart_operations[peripheral_index] & UART_TRANSMIT_OPERATION) == 0) {
		return NRF_SUCCESS;
	}
	
	// You should only be able to abort an transmit operation, if your own instance is currently running. So check if the instance id is the same
	if((uart_instances[peripheral_index] != NULL) && ((*uart_instances[peripheral_index]).uart_instance_id != uart_instance->uart_instance_id)) {
			return NRF_ERROR_INVALID_STATE;
	} 
	// If NULL (it shouldn't be NULL here, because we have an ongoing operation) -> just proceed as normal..


	
	// Now stop the ongoing transmit operation
	nrf_drv_uart_tx_abort(&(uart_instance->nrf_drv_uart_instance));	
	
	// Clear the Transmit operation
	uart_operations[peripheral_index] &= ~(UART_TRANSMIT_OPERATION);
	
	return NRF_SUCCESS;
}


ret_code_t uart_transmit(uart_instance_t* uart_instance, const uint8_t* tx_data, uint32_t tx_data_len) {
	
	ret_code_t ret = uart_transmit_bkgnd(uart_instance, NULL, tx_data, tx_data_len);
	if(ret != NRF_SUCCESS) {
		return ret;
	}

	// Waiting until the UART operation has finished!
	while(uart_get_operation(uart_instance) & UART_TRANSMIT_OPERATION);	
	
	return NRF_SUCCESS;	
}




ret_code_t uart_receive_bkgnd(uart_instance_t* uart_instance, uart_handler_t uart_handler, uint8_t* rx_data, uint32_t rx_data_len) {
	// Get the peripheral_index
	uint8_t peripheral_index = uart_instance->uart_peripheral;
	
	
	// Check whether there is an ongoing receive or receive-buffer operation
	if((uart_operations[peripheral_index] & UART_RECEIVE_OPERATION) || (uart_operations[peripheral_index] & UART_RECEIVE_BUFFER_OPERATION)) {
		return NRF_ERROR_BUSY;
	}
	
	// Set the UART_RECEIVE_OPERATION flag
	uart_operations[peripheral_index] |= UART_RECEIVE_OPERATION;
	
	// Set the current uart instance (it is not needed in receive-case, because we need it only in receive-buffer case)
	uart_instances[peripheral_index] = uart_instance;
	
	// Set the handler that has to be called, reset the receive-buffer handler!
	uart_handlers_receive_bkgnd[peripheral_index] = uart_handler;
	uart_handlers_receive_buffer_bkgnd[peripheral_index] = NULL;		// We actually don't need to do this, but ok..
	
	
	nrf_drv_uart_rx_enable(&(uart_instance->nrf_drv_uart_instance));
	
	ret_code_t ret = nrf_drv_uart_rx(&(uart_instance->nrf_drv_uart_instance), rx_data, rx_data_len);
	
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_FORBIDDEN, NRF_ERROR_INTERNAL and NRF_ERROR_INVALID_ADDR
	
	// Set error code to internal to reduce error codes
	if(ret == NRF_ERROR_FORBIDDEN || ret == NRF_ERROR_INTERNAL)
		ret = NRF_ERROR_INTERNAL;
	
	// If no success clear the UART_RECEIVE_OPERATION flag
	if(ret != NRF_SUCCESS) {		
		
		// Now stop the ongoing receive operation
		nrf_drv_uart_rx_abort(&(uart_instance->nrf_drv_uart_instance));
		// And disable the receive process!
		nrf_drv_uart_rx_disable(&(uart_instance->nrf_drv_uart_instance));
	
	
		uart_operations[peripheral_index] &= ~(UART_RECEIVE_OPERATION);
	}
	
	return ret;
	
}


ret_code_t uart_receive_abort_bkgnd(const uart_instance_t* uart_instance) {
	
	// Get the peripheral_index
	uint8_t peripheral_index = uart_instance->uart_peripheral;
	
	// Check if there is an ongoing receive operation. If there is no receive operation, directly return 
	if((uart_operations[peripheral_index] & UART_RECEIVE_OPERATION)  == 0) {
		return NRF_SUCCESS;
	}
	
	// You should only be able to abort an receive operation, if your own instance is currently running. So check if the instance id is the same
	if((uart_instances[peripheral_index] != NULL) && ((*uart_instances[peripheral_index]).uart_instance_id != uart_instance->uart_instance_id)) {
			return NRF_ERROR_INVALID_STATE;
	}
	// If NULL (it shouldn't be NULL here, because we have an ongoing operation) -> just proceed as normal..

	
	// Now stop the ongoing receive operation
	nrf_drv_uart_rx_abort(&(uart_instance->nrf_drv_uart_instance));
	// And disable the receive process!
	nrf_drv_uart_rx_disable(&(uart_instance->nrf_drv_uart_instance));
	
	// Clear the Receive operation
	uart_operations[peripheral_index] &= ~(UART_RECEIVE_OPERATION);
	
	return NRF_SUCCESS;
	
}


ret_code_t uart_receive(uart_instance_t* uart_instance, uint8_t* rx_data, uint32_t rx_data_len) {
	
	ret_code_t ret = uart_receive_bkgnd(uart_instance, NULL, rx_data, rx_data_len);	
	
	if(ret != NRF_SUCCESS) {
		return ret;
	}

	// Waiting until the UART operation has finished!
	while(uart_get_operation(uart_instance) & UART_RECEIVE_OPERATION);	
	
	return NRF_SUCCESS;	
}




ret_code_t uart_receive_buffer_bkgnd(uart_instance_t* uart_instance,  uart_handler_t uart_handler){
	
	// Get the peripheral_index
	uint8_t peripheral_index = uart_instance->uart_peripheral;
	
	
	// Check if rx-buffer is not NUL, because it is needed for buffering the incoming data
	if(uart_instance->uart_buffer.rx_buf == NULL) 
		return NRF_ERROR_NULL;
	
	if(uart_instance->uart_buffer.rx_buf_size == 0)
		return NRF_ERROR_NO_MEM;
	
	// Check whether there is an ongoing receive or receive-buffer operation
	if((uart_operations[peripheral_index] & UART_RECEIVE_OPERATION) || (uart_operations[peripheral_index] & UART_RECEIVE_BUFFER_OPERATION)) {
		return NRF_ERROR_BUSY;
	}
	
	// Set the UART_RECEIVE_OPERATION flag
	uart_operations[peripheral_index] |= UART_RECEIVE_BUFFER_OPERATION;
	
	// Set the current uart instance (because we need it to reinitiate a receive operation)
	uart_instances[peripheral_index] = uart_instance;
	
	// Reset the receive-read and -write index of the uart-buffer
	(uart_instance->uart_buffer).rx_buf_read_index = 0;
	(uart_instance->uart_buffer).rx_buf_write_index = 0;
	
	
	// Set the handler that has to be called, reset the receive handler!
	uart_handlers_receive_bkgnd[peripheral_index] = NULL;					// We actually don't need to do this, but ok..
	uart_handlers_receive_buffer_bkgnd[peripheral_index] = uart_handler;
	
	// Enable the receive process
	nrf_drv_uart_rx_enable(&(uart_instance->nrf_drv_uart_instance));
	
	// Start the receive and buffer the data in the 1 Byte buffer
	ret_code_t ret = nrf_drv_uart_rx(&(uart_instance->nrf_drv_uart_instance), (uint8_t*) &uart_receive_buffer_operations[peripheral_index], 1);
	
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY, NRF_ERROR_FORBIDDEN, NRF_ERROR_INTERNAL and NRF_ERROR_INVALID_ADDR
	
	// Set error code to internal to reduce error codes
	if(ret == NRF_ERROR_FORBIDDEN || ret == NRF_ERROR_INTERNAL)
		ret = NRF_ERROR_INTERNAL;
	
	
	// If no success clear the UART_RECEIVE_BUFFER_OPERATION flag
	if(ret != NRF_SUCCESS) {		
		
		// Now stop the ongoing receive operation
		nrf_drv_uart_rx_abort(&(uart_instance->nrf_drv_uart_instance));
		// And disable the receive process!
		nrf_drv_uart_rx_disable(&(uart_instance->nrf_drv_uart_instance));
	
	
		uart_operations[peripheral_index] &= ~(UART_RECEIVE_BUFFER_OPERATION);
	}
	
	return ret;
	
}

ret_code_t uart_receive_buffer_abort_bkgnd(const uart_instance_t* uart_instance) {
	
	// Get the peripheral_index
	uint8_t peripheral_index = uart_instance->uart_peripheral;
	
	// Check if there is an ongoing receive-buffer operation. If there is no receive-buffer operation, directly return 
	if((uart_operations[peripheral_index] & UART_RECEIVE_BUFFER_OPERATION)  == 0) {
		return NRF_SUCCESS;
	}
	
	// You should only be able to abort an receive-buffer operation, if your own instance is currently running. So check if the instance id is the same
	if((uart_instances[peripheral_index] != NULL) && ((*uart_instances[peripheral_index]).uart_instance_id != uart_instance->uart_instance_id)) {
			return NRF_ERROR_INVALID_STATE;
	}
	// If NULL (it shouldn't be NULL here, because we have an ongoing operation) -> just proceed as normal..

	
	
	// Now stop the ongoing receive operation
	nrf_drv_uart_rx_abort(&(uart_instance->nrf_drv_uart_instance));
	// And disable the receive process!
	nrf_drv_uart_rx_disable(&(uart_instance->nrf_drv_uart_instance));
	
	// Clear the Receive operation
	uart_operations[peripheral_index] &= ~(UART_RECEIVE_BUFFER_OPERATION);
	
	return NRF_SUCCESS;
	
}

/**@brief Functions for retrieving the number of elements in the circular rx-buffer.
 *
 * @retval Number of elements in circular rx-buffer.
 */
static uint32_t uart_receive_buffer_get_elements(const uart_instance_t* uart_instance) {
	uint32_t size = (uart_instance->uart_buffer).rx_buf_size;
	uint32_t read_index = (uart_instance->uart_buffer).rx_buf_read_index;
	uint32_t write_index = (uart_instance->uart_buffer).rx_buf_write_index;
	uint32_t elements = 0;
	
	if(write_index >= read_index) {
		elements = write_index - read_index;
	} else {
		elements = (size - read_index) + write_index;
	}
	
	return elements;
}

ret_code_t uart_receive_buffer_get(uart_instance_t* uart_instance,  uint8_t* data_byte) {
	
	// It is important that we are now assuming that it's in the current instance, but we have not really a reference to it
	
	// Just reads out a byte from the circular buffer
	uint32_t elements = uart_receive_buffer_get_elements(uart_instance);
	
	if(elements == 0)
		return NRF_ERROR_NOT_FOUND;
	
	*data_byte = (uart_instance->uart_buffer).rx_buf[(uart_instance->uart_buffer).rx_buf_read_index];
	(uart_instance->uart_buffer).rx_buf_read_index = ((uart_instance->uart_buffer).rx_buf_read_index + 1) %  (uart_instance->uart_buffer).rx_buf_size;
	
	return NRF_SUCCESS;
}



uart_operation_t uart_get_operation(const uart_instance_t* uart_instance) {
	return uart_operations[uart_instance->uart_peripheral];
}

