#include "uart_lib.h"





typedef struct 
{
	nrf_drv_uart_t nrf_drv_uart_instance;	
	volatile uint8_t 
} uart_instance_t;





void uart_init(uint32_t* uart_instance_id, const uart_config_t* uart_config, const uart_buffer_t* uart_buffer) {
	// Check if uart is already initialized!!
	
}


void uart_available(uint32_t uart_instance_id) {
	
}

uint8_t uart_read() {
	
}

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



