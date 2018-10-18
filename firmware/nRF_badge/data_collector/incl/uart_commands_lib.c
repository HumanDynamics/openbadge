#include "uart_commands_lib.h"
#include "uart_lib.h"
#include "debug_lib.h"
#include "app_util_platform.h"
#include "custom_board.h"


#define MAX_COMMAND_LEN   32
#define COMMAND_BUF_SIZE  (MAX_COMMAND_LEN + 1)

#define UART_RX_BUFFER_SIZE	COMMAND_BUF_SIZE

typedef void (*uart_command_handler_t)(void);

typedef struct {
    char command[COMMAND_BUF_SIZE];
    uart_command_handler_t handler;
} uart_command_t;

static char command_buffer[COMMAND_BUF_SIZE];
static int command_buffer_pos = 0;

static uart_instance_t uart_instance;	/**< The private uart_instance. */



static void on_uart_event(uart_evt_t const * p_event);
static void on_command_received(const char * command);

/**@brief Command handler function that is called when the restart-command was received.
 */
static void on_restart_command(void) {
    NVIC_SystemReset();
}

/**< The command lookup table, maps textual string commands to methods executed when they're received. */
static uart_command_t uart_commands[] = {
        {
                .command = "restart",
                .handler = on_restart_command,
        },
};


ret_code_t uart_commands_init(void) {
	
	
	
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
	UART_BUFFER_INIT(&uart_instance, UART_RX_BUFFER_SIZE, 0, &ret);	
	
	if(ret != NRF_SUCCESS) return ret;
	
	memset(command_buffer, 0, sizeof(command_buffer));
	ret = uart_receive_buffer_bkgnd(&uart_instance, on_uart_event);
	
	
	return ret;
}

/**@brief Function that matches the received command to a command-handler and dispatches this handler.
 */
static void on_command_received(const char * command) {
    int num_commands = sizeof(uart_commands) / sizeof(uart_commands[0]);
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(command, uart_commands[i].command) == 0) {
            uart_commands[i].handler();
            return;
        }
    }

    debug_log("UART_COMMANDS: Unknown command: %s\n", command);
}


/**@brief The callback-function that is called when there was sth received via the UART-interface.
 */
static void on_uart_event(uart_evt_t const * p_event) {
	while(uart_receive_buffer_get(&uart_instance, (uint8_t*) &command_buffer[command_buffer_pos]) == NRF_SUCCESS) {		
		if(command_buffer[command_buffer_pos] == '\n' || command_buffer[command_buffer_pos] == '\r') {
			command_buffer[command_buffer_pos] = 0;
			on_command_received(command_buffer);
			memset(command_buffer, 0, sizeof(command_buffer));
			command_buffer_pos = 0;
			break;
		}
		if(command_buffer_pos <  COMMAND_BUF_SIZE - 1)
			command_buffer_pos++;		
		else {
			command_buffer_pos = 0;
			memset(command_buffer, 0, sizeof(command_buffer));
		}
		
	}

}

