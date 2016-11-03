//
// Created by Andrew Bartow on 11/3/16.
//

#ifndef OPENBADGE_UART_COMMANDS_H
#define OPENBADGE_UART_COMMANDS_H

#define MAX_COMMAND_LEN   32
#define COMMAND_BUF_SIZE  (MAX_COMMAND_LEN + 1)

typedef void (*uart_command_handler_t)(void);

typedef struct {
    char command[COMMAND_BUF_SIZE];
    uart_command_handler_t handler;
} uart_command_t;

/**
 * This method should be called when a char is recieved that is to be processed by the UART Command Handler.
 * Internally buffers the characters and and calls the necessary command handlers.
 *
 * @param commandChar char recieved over UART
 */
void UARTCommands_ProcessChar(char commandChar);

#endif //OPENBADGE_UART_COMMANDS_H
