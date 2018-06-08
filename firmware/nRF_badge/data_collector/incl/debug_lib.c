#include "debug_lib.h"


#ifdef DEBUG_ENABLE

#include "uart_lib.h"
#include <stdarg.h>		// Needed for the printf-function
#include <string.h>		// Needed for the printf-function
#include <stdio.h>		// Needed for the vsnprintf-function

#define UART_PRINTF_TX_BUFFER_SIZE	300



static uart_instance_t uart_instance;	/**< The private uart_instance used for the uart_printf operations */

void debug_init(void)
{
	
	uart_instance.uart_peripheral = 0;
	uart_instance.nrf_drv_uart_config.baudrate = (nrf_uart_baudrate_t) NRF_UART_BAUDRATE_115200;
    uart_instance.nrf_drv_uart_config.hwfc = NRF_UART_HWFC_DISABLED;
    uart_instance.nrf_drv_uart_config.interrupt_priority = 3;
    uart_instance.nrf_drv_uart_config.parity = NRF_UART_PARITY_EXCLUDED;
    uart_instance.nrf_drv_uart_config.pselcts = 0;
    uart_instance.nrf_drv_uart_config.pselrts = 0;
    uart_instance.nrf_drv_uart_config.pselrxd = 11;
    uart_instance.nrf_drv_uart_config.pseltxd = 10;
	
	ret_code_t ret;
	UART_BUFFER_INIT(&uart_instance, 0, UART_PRINTF_TX_BUFFER_SIZE, &ret);	
}


void debug_log( const char* format, ...) {
	
	
	// Check if the tx buffer that we want to use is not NULL
	if(uart_instance.uart_buffer.tx_buf == NULL) {
		return;
	}
	
	
	uint32_t n = uart_instance.uart_buffer.tx_buf_size;
		
	va_list args;
	va_start(args, format);
	
	int ret_vsnprintf = vsnprintf((char*) uart_instance.uart_buffer.tx_buf, n, format, args);
	va_end(args);	
	
	
	// Check if the output of vsnprintf is correct
	if(ret_vsnprintf < 0 || ret_vsnprintf >= n) {
		return;
	}
	
	
	uart_transmit(&uart_instance, uart_instance.uart_buffer.tx_buf, ret_vsnprintf);	
	
}


void debug_log_dump(uint8_t * p_buffer, uint32_t len)
{
    debug_log("\r\n");
    for (uint32_t index = 0; index <  len; index++)  {
        debug_log("0x%02X ", p_buffer[index]);
    }
    debug_log("\r\n");
}

#endif
