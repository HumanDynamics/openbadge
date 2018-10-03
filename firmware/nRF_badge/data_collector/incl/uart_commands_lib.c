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

extern uart_instance_t uart_instance;	/**< The uart_instance of debug_lib */



static void on_uart_event(uart_evt_t const * p_event);
static void on_command_received(const char * command);

// Command Handlers
static void on_restart_command(void) {
    NVIC_SystemReset();
}

// Command lookup table, maps textual string commands to methods executed when they're received.
static uart_command_t uart_commands[] = {
        {
                .command = "restart",
                .handler = on_restart_command,
        },
};


ret_code_t uart_commands_init(void) {
	
	memset(command_buffer, 0, sizeof(command_buffer));
	ret_code_t ret = uart_receive_buffer_bkgnd(&uart_instance, on_uart_event);
	
	
	return ret;
}

// Dispatches appropriate handler for matching command.
// 'command' should be null terminated string <= MAX_COMMAND_LEN
static void on_command_received(const char * command) {
    int num_commands = sizeof(uart_commands) / sizeof(uart_commands[0]);
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(command, uart_commands[i].command) == 0) {
            uart_commands[i].handler();
            return;
        }
    }

    debug_log_bkgnd("Unknown command: %s\n", command);
}


static void on_uart_event(uart_evt_t const * p_event) {
	

	while(uart_receive_buffer_get(&uart_instance, (uint8_t*) &command_buffer[command_buffer_pos]) == NRF_SUCCESS) {
		//uart_transmit_bkgnd(&uart_instance, NULL, (uint8_t*) &command_buffer[command_buffer_pos], 1);
		
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

