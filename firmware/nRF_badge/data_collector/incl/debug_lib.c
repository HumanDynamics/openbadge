#include "debug_lib.h"



#ifdef DEBUG_LOG_ENABLE
#include "app_util_platform.h"
#include "uart_lib.h"
#include <stdarg.h>		// Needed for the printf-function
#include <string.h>		// Needed for the printf-function
#include <stdio.h>		// Needed for the vsnprintf-function

#include "app_fifo.h"

#include "custom_board.h"

#define DEBUG_PRINTF_BUFFER_SIZE	200		/**< Buffer Size for one printf decoding */
#define DEBUG_FIFO_BUFFER_SIZE		2048		/**< Has to be a power of two */
#define UART_TRANSMIT_BUFFER_SIZE	100		/**< Size of the buffer of one UART transmit operation */

static uint8_t debug_fifo_buf[DEBUG_FIFO_BUFFER_SIZE];			/**< The buffer for the FIFO */
static uint8_t uart_transmit_buf[UART_TRANSMIT_BUFFER_SIZE];	/**< The buffer that is used to actually call the uart-transmit-function. Because the FIFO can't do that */
static volatile uint8_t uart_transmit_flag = 0;					/**< Flag that shows if there is already an ongoing transmit operation */

static app_fifo_t debug_fifo;			/**< The FIFO for the messages to transmit */

static uart_instance_t uart_instance;	/**< The private uart_instance used for the uart_printf operations */

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
	UART_INIT(&uart_instance, &ret);	
	
	app_fifo_init(&debug_fifo, debug_fifo_buf, sizeof(debug_fifo_buf));
	
	uart_transmit_flag = 0;

}

static void uart_handler(uart_evt_t const * p_event) {
	if(p_event->type == UART_TRANSMIT_DONE) {
		uint32_t len = sizeof(uart_transmit_buf);
		app_fifo_read(&debug_fifo, uart_transmit_buf, &len);
		// Check if we need to transmit again
		if(len > 0) {
			uart_transmit_bkgnd(&uart_instance, uart_handler, uart_transmit_buf, len);	
		} else {
			uart_transmit_flag = 0;
		}
		
	} else if(p_event->type == UART_ERROR) {
		uart_transmit_flag = 0;
	}
}


void debug_log(const char* format, ...) {
	
	
	// This is done locally in the function (because if more contextes tries to call this function at the same time..)
	uint8_t printf_buf[DEBUG_PRINTF_BUFFER_SIZE];
	
	va_list args;
	va_start(args, format);
	
	int ret_vsnprintf = vsnprintf((char*) printf_buf, sizeof(printf_buf), format, args);
	va_end(args);	
	
	// ret_vsnprintf == length or error code
	
	// Check if the output of vsnprintf is correct
	if(ret_vsnprintf < 0 || ret_vsnprintf >= sizeof(printf_buf)) {
		return;
	}
	
	// Flag if we need to start the UART transmit operation
	uint8_t start_transmit = 0;
	
	// Put on the FIFO:
	CRITICAL_REGION_ENTER();
	// Check if there is enough space in the fifo
	uint32_t available_size = 0;
	app_fifo_write(&debug_fifo, NULL, &available_size);
	
	if(ret_vsnprintf <= available_size) {
		uint32_t len = (uint32_t) ret_vsnprintf;
		
		app_fifo_write(&debug_fifo, printf_buf, &len);
		
		// Check if we need to start the uart operation
		
		if(!uart_transmit_flag) {
			uart_transmit_flag = 1;
			start_transmit = 1;
		}
	}
	CRITICAL_REGION_EXIT();
	
	// Check if we need to start transmitting, otherwise there should alredy be an transmit operation that queues the currently inserted one.
	if(start_transmit) {
		uint32_t len = sizeof(uart_transmit_buf);		
		app_fifo_read(&debug_fifo, uart_transmit_buf, &len);
		if(len > 0) {
			uart_transmit_bkgnd(&uart_instance, uart_handler, uart_transmit_buf, len);	
		} else {
			uart_transmit_flag = 0;
		}
		
	}	
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
