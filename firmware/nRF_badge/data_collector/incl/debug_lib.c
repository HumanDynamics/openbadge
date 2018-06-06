#include "debug_lib.h"


#ifdef DEBUG_ENABLE

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
	UART_BUFFER_INIT(&uart_instance, 0, 300, &ret);
	
	// Set the global pointer to the private uart instance
	p_debug_uart_instance = &uart_instance;
	
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
