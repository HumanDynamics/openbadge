#include "debug_lib.h"
#include "app_util_platform.h"


#ifdef DEBUG_LOG_ENABLE

#include "uart_lib.h"
#include <stdarg.h>		// Needed for the printf-function
#include <string.h>		// Needed for the printf-function
#include <stdio.h>		// Needed for the vsnprintf-function

#include "custom_board.h"

#define UART_PRINTF_TX_BUFFER_SIZE	300
#define UART_RX_BUFFER_SIZE			50



uart_instance_t uart_instance;	/**< The public uart_instance used for the uart_printf operations */

void debug_init(void)
{
	
	uart_instance.uart_peripheral = 0;
	uart_instance.nrf_drv_uart_config.baudrate = (nrf_uart_baudrate_t) NRF_UART_BAUDRATE_115200;
    uart_instance.nrf_drv_uart_config.hwfc = HWFC_ENABLED ? NRF_UART_HWFC_ENABLED : NRF_UART_HWFC_DISABLED;
    uart_instance.nrf_drv_uart_config.interrupt_priority = APP_IRQ_PRIORITY_MID;
    uart_instance.nrf_drv_uart_config.parity = NRF_UART_PARITY_EXCLUDED;
    uart_instance.nrf_drv_uart_config.pselcts = UART_CTS_PIN;
    uart_instance.nrf_drv_uart_config.pselrts = UART_RTS_PIN;
    uart_instance.nrf_drv_uart_config.pselrxd = UART_RX_PIN;
    uart_instance.nrf_drv_uart_config.pseltxd = UART_TX_PIN;
	
	ret_code_t ret;
	UART_BUFFER_INIT(&uart_instance, UART_RX_BUFFER_SIZE, UART_PRINTF_TX_BUFFER_SIZE, &ret);	
}

void debug_log_bkgnd( const char* format, ...) {
	// Check if the tx buffer that we want to use is not NULL
	if(uart_instance.uart_buffer.tx_buf == NULL) {
		return;
	}
	
	// Check if uart is already transmitting sth
	if(uart_get_operation(&uart_instance) & UART_TRANSMIT_OPERATION) {
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
	
	
	uart_transmit_bkgnd(&uart_instance, NULL, uart_instance.uart_buffer.tx_buf, ret_vsnprintf);	
	
}

void debug_log( const char* format, ...) {
	
	
	// Check if the tx buffer that we want to use is not NULL
	if(uart_instance.uart_buffer.tx_buf == NULL) {
		return;
	}
	
	// Check if uart is already transmitting sth
	if(uart_get_operation(&uart_instance) & UART_TRANSMIT_OPERATION) {
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
