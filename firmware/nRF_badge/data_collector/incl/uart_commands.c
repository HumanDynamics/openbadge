//
// Created by Andrew Bartow on 11/3/16.
//

#include <app_uart.h>
#include <string.h>

#include "uart_commands.h"
#include "debug_log.h"

#define MAX_COMMAND_LEN   32
#define COMMAND_BUF_SIZE  (MAX_COMMAND_LEN + 1)
#define MAX_NUM_ARGUMENTS 6

static char mCommandBuffer[COMMAND_BUF_SIZE];
static int mCommandBufferPos = 0;

static uart_command_t mUARTCommands[] = {
        {
                "restart",
                on_restart_command,
        },
};

static void on_restart_command(void) {
    debug_log("Got restart command!\r\n");
}

// command should be null terminated
static void on_command_received(const char * command) {
    int numCommands = sizeof(mUARTCommands) / sizeof(mUARTCommands[0]);
    for (int i = 0; i < numCommands; i++) {
        debug_log("Checking command %d:%s\r\n", i, mUARTCommands[i].command);
        if (strcmp(command, mUARTCommands[i].command) == 0) {
            mUARTCommands[i].handler();
            return;
        }
    }

    debug_log("Could not recognize command %s\r\n", command);
}

void UARTCommands_ProcessChar(char commandChar) {
    app_uart_put(commandChar);

    if (commandChar == '\n') {
        on_command_received(mCommandBuffer);
        memset(mCommandBuffer, 0, sizeof(mCommandBuffer));
        mCommandBufferPos = 0;
    } else {
        if (mCommandBufferPos < MAX_COMMAND_LEN) {
            mCommandBuffer[mCommandBufferPos] = commandChar;
            mCommandBufferPos++;
        }
    }
}

