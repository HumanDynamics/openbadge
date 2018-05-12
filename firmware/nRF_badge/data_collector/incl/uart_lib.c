#include "uart_lib.h"




#define UART_PERIPHERAL_NUMBER 		UART_COUNT	// automatically imported through nrf_peripherals.h


static volatile uart_operation_t  	uart_operations[UART_PERIPHERAL_NUMBER] 	= {0};
static uart_instance_t				uart_instances[UART_PERIPHERAL_NUMBER] 	= {{0}}; // We need to save the instances, to use them in the IRQ-handler for UART_RECEIVE_BUFFER_OPERATION
static uint32_t 					uart_instance_number = 1; 	// Starts at 1 because the above init of the arrays are always to 0

// To have all the handlers to be independent of the initialization!!
static uart_handler_t				uart_handlers_transmit_IT		[UART_PERIPHERAL_NUMBER];
static uart_handler_t				uart_handlers_receive_IT		[UART_PERIPHERAL_NUMBER];
static uart_handler_t				uart_handlers_receive_buffer_IT	[UART_PERIPHERAL_NUMBER];



#if UART0_ENABLED
static void uart_0_event_handler(nrf_drv_uart_event_t * p_event, void * p_context) {
	uart_evt_t evt;
	
	if(p_event->type == NRF_DRV_UART_EVT_TX_DONE) {
		evt.type = UART_TRANSMIT_DONE;
		
		// Clear the UART_TRANSMIT_OPERATION flag
		uart_operations[0] &= ~(UART_TRANSMIT_OPERATION);
		
		// Call the Callback handler if there exists one!
		if(uart_handlers_transmit_IT[0] != NULL) {
			uart_handlers_transmit_IT[0](&evt);
		}		
			
	} else if(p_event->type == NRF_DRV_UART_EVT_RX_DONE) {
		
		// Check the operation, if we are in operation receiving or receiving-buffer
		if(uart_operations[0] & UART_RECEIVE_OPERATION) {
			evt.type = UART_RECEIVE_DONE;
			
			// Clear the UART_RECEIVE_OPERATION flag
			uart_operations[0] &= ~(UART_RECEIVE_OPERATION);
			
			// Call the Callback handler if there exists one!
			if(uart_handlers_receive_IT[0] != NULL) {
				uart_handlers_receive_IT[0](&evt);
			}	
			
		} else if(uart_operations[0] & UART_RECEIVE_BUFFER_OPERATION) {
			
			
			
			
			
			
			
		}		
	} else if(p_event->type == NRF_DRV_UART_EVT_ERROR) {
		
		evt.type = UART_ERROR;
		
		// Check which operations are ongoing, call the corresponding handlers, and kill clear the operation bit!
		if(uart_operations[0] & UART_TRANSMIT_OPERATION) {
			if(uart_handlers_transmit_IT[0] != NULL) {
				uart_handlers_transmit_IT[0](&evt);
			}
			uart_operations[0] &= ~(UART_TRANSMIT_OPERATION);
		}
		if(uart_operations[0] & UART_RECEIVE_OPERATION) {
			if(uart_handlers_receive_IT[0] != NULL) {
				uart_handlers_receive_IT[0](&evt);
			}
			uart_operations[0] &= ~(UART_RECEIVE_OPERATION);
		}
		if(uart_operations[0] & UART_RECEIVE_BUFFER_OPERATION) {
			if(uart_handlers_receive_buffer_IT[0] != NULL) {
				uart_handlers_receive_buffer_IT[0](&evt);
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
	
	
	
	
	
	// ret should be NRF_SUCCESS or NRF_ERROR_INVALID_STATE (we don't allow different instances on the UART, because it makes no sense!)
	if(ret != NRF_SUCCESS) {
		return ret;
	}
	
	
	uart_instance->uart_instance_id = uart_instance_number;
	
	uart_instance_number++;
	
	return NRF_SUCCESS;
}



ret_code_t uart_transmit_IT(const uart_instance_t* uart_instance, uart_handler_t uart_handler, const uint8_t* tx_data, uint32_t tx_data_len) {
	// Check if there is an ongoing transmit operation. Not check if there is no operation, because we can transmit while receiving!
	if((uart_operations[uart_instance->uart_peripheral] & UART_TRANSMIT_OPERATION)) {
		return NRF_ERROR_BUSY;
	} 
	
	// Set the UART_TRANSMIT_OPERATION flag
	uart_operations[uart_instance->uart_peripheral] |= UART_TRANSMIT_OPERATION;
	
	// Set the current uart instance (it is not needed in transmit-case, because we need it only in receive-buffer case)
	uart_instances[uart_instance->uart_peripheral] = *uart_instance;
	
	// Set the handler that has to be called
	uart_handlers_transmit_IT[uart_instance->uart_peripheral] = uart_handler;

	ret_code_t ret = nrf_drv_uart_tx(&(uart_instance->nrf_drv_uart_instance), tx_data, tx_data_len);

	// If no success clear the UART_TRANSMIT_OPERATION flag
	
	if(ret != NRF_SUCCESS) {		
		uart_operations[uart_instance->uart_peripheral] &= ~(UART_TRANSMIT_OPERATION);
	}
	
	
	return ret;
	
	
}


ret_code_t uart_transmit(const uart_instance_t* uart_instance, const uint8_t* tx_data, uint32_t tx_data_len) {
	ret_code_t ret = uart_transmit_IT(uart_instance, NULL, tx_data, tx_data_len);
	if(ret != NRF_SUCCESS) {
		return ret;
	}

	// Waiting until the UART operation has finished!
	while(uart_operations[uart_instance->uart_peripheral] & UART_TRANSMIT_OPERATION);	
	
	return NRF_SUCCESS;	
}



ret_code_t uart_receive_IT(const uart_instance_t* uart_instance, uart_handler_t uart_handler, uint8_t* rx_data, uint32_t rx_data_len) {
	
	// Check whether there is an ongoing receive or receive-buffer operation
	if((uart_operations[uart_instance->uart_peripheral] & UART_RECEIVE_OPERATION) || (uart_operations[uart_instance->uart_peripheral] & UART_RECEIVE_BUFFER_OPERATION)) {
		return NRF_ERROR_BUSY;
	}
	
	// Set the UART_RECEIVE_OPERATION flag
	uart_operations[uart_instance->uart_peripheral] |= UART_RECEIVE_OPERATION;
	
	// Set the current uart instance (it is not needed in receive-case, because we need it only in receive-buffer case)
	uart_instances[uart_instance->uart_peripheral] = *uart_instance;
	
	// Set the handler that has to be called, reset the receive-buffer handler!
	uart_handlers_receive_IT[uart_instance->uart_peripheral] = uart_handler;
	uart_handlers_receive_buffer_IT[uart_instance->uart_peripheral] = NULL;
	
	
	nrf_drv_uart_rx_enable(&(uart_instance->nrf_drv_uart_instance));
	
	ret_code_t ret = nrf_drv_uart_rx(&(uart_instance->nrf_drv_uart_instance), rx_data, rx_data_len);
	
	
	// If no success clear the UART_TRANSMIT_OPERATION flag
	if(ret != NRF_SUCCESS) {		
		uart_operations[uart_instance->uart_peripheral] &= ~(UART_RECEIVE_OPERATION);
	}
	
	return ret;
	
}


ret_code_t uart_receive(const uart_instance_t* uart_instance, uint8_t* rx_data, uint32_t rx_data_len) {
	ret_code_t ret = uart_receive_IT(uart_instance, NULL, rx_data, rx_data_len);	
	
	if(ret != NRF_SUCCESS) {
		return ret;
	}

	// Waiting until the UART operation has finished!
	while(uart_operations[uart_instance->uart_peripheral] & UART_RECEIVE_OPERATION);	
	
	return NRF_SUCCESS;	
}



/* TODO: Do the receive_stuff, with the buffer and the indexes. Init function for uart with buffer (with MACRO).
Initialize uart_buffer to NULL if not initalized through the right init function.

Check at the beginning of printf, and receive_buffer if there is a buffer present != NULL to do the stuff!

Again insert the instance to use it in the IRQ-handler to restart the reception.

And reread the code of uart, spi and adc again (e.g. the instance array!!)


*/



/*

// https://stackoverflow.com/questions/4867229/code-for-printf-function-in-c
// https://devzone.nordicsemi.com/f/nordic-q-a/28999/using-floats-with-sprintf-gcc-arm-none-eabi-nrf51822
// If you want to have float support, add "LDFLAGS += -u _printf_float" in Makefile!
void uart_printf() {
	va_list args;
	va_start(args, format);
	vsnprintf((char*)buf, UART_TX_BUFFER_SIZE, format, args);
	va_end(args);	
	nrf_drv_uart_tx(&_instance, buf, strlen((char*)buf));
}
*/


