//
// Created by Andrew Bartow on 11/3/16.
//

#include <app_uart.h>
#include <string.h>

#include "uart_commands.h"
#include "debug_log.h"
#include "flash.h"
#include "flash_layout.h"
#include "nrf_delay.h"

#define MAX_COMMAND_LEN   32
#define COMMAND_BUF_SIZE  (MAX_COMMAND_LEN + 1)

static char mCommandBuffer[COMMAND_BUF_SIZE];
static int mCommandBufferPos = 0;

// Command Handlers
static void on_restart_command(void) {
    NVIC_SystemReset();
}

static void on_flash_test_command(void) {
    debug_log("Writing to flash...\r\n");

    char test_write_string[] = "This is a test string!!";
    flash_write(FLASH_START, (uint8_t *) test_write_string, sizeof(test_write_string));

    nrf_delay_ms(1000);

    char test_read_string[sizeof(test_write_string)];
    flash_read((uint8_t *) test_read_string, FLASH_START, sizeof(test_read_string));

    debug_log("Read from flash: %s\r\n", test_read_string);
}

// Command lookup table, maps textual string commands to methods executed when they're received.
static uart_command_t mUARTCommands[] = {
        {
                .command = "restart",
                .handler = on_restart_command,
        },
        {
                .command = "flash_test",
                .handler = on_flash_test_command,
        }
};

// Dispatches appropriate handler for matching command.
// 'command' should be null terminated string <= MAX_COMMAND_LEN
static void on_command_received(const char * command) {
    int numCommands = sizeof(mUARTCommands) / sizeof(mUARTCommands[0]);
    for (int i = 0; i < numCommands; i++) {
        if (strcmp(command, mUARTCommands[i].command) == 0) {
            mUARTCommands[i].handler();
            return;
        }
    }

    debug_log("Unknown command: %s\n", command);
}

void UARTCommands_ProcessChar(char commandChar) {
    app_uart_put(commandChar);

    if (commandChar == '\n' || commandChar == '\r') {
        if (mCommandBufferPos != 0) {
            on_command_received(mCommandBuffer);
            memset(mCommandBuffer, 0, sizeof(mCommandBuffer));
            mCommandBufferPos = 0;
        }
    } else {
        if (mCommandBufferPos < MAX_COMMAND_LEN) {
            mCommandBuffer[mCommandBufferPos] = commandChar;
            mCommandBufferPos++;
        }
    }
}

