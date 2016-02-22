/* Methods to simplify access to an external Adesto AT25XE041B flash memory IC
 *
 */


#ifndef EXTERNAL_FLASH_H
#define EXTERNAL_FLASH_H


#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "nrf.h"
#include "nrf51_bitfields.h"
#include "app_util_platform.h"


#include "app_error.h"          //error handling
#include "nrf_delay.h"          //includes blocking delay functions
#include "nrf_gpio.h"           //abstraction for dealing with gpio
#include "spi_master.h"         //abstraction for dealing with SPI

#include "debug_log.h"          //UART debugging logger
//requires app_fifo, app_uart_fifo.c and retarget.c for printf to work properly

#include "nrf_drv_config.h"
#include "boards.h"

#define EXT_FLASH_SREG_BUSY 0x1         //bit in flash status register indicating an in-progress write or erase
#define EXT_FLASH_SREG_WEL 0x2          //write enable bit
#define EXT_FLASH_SREG_WPP 0x10         //state of write protect pin (active low)
#define EXT_FLASH_SREG_EPE 0x20         //erase/program error
#define EXT_FLASH_SREG_SPRL 0x80        //sector protect register lock

typedef enum
{
    EXT_FLASH_SPI_IDLE = 0,         // no pending SPI operations.  Flash IC may still be busy
    EXT_FLASH_COMMAND,              // sending a one-off command over SPI, e.g. write enable
    EXT_FLASH_READ,                 // sending a read command over SPI
    EXT_FLASH_WRITE,                // sending a write command over SPI
    // The following are only used in returns from ext_flash_get_state()
    //   because they require polling the state of the flash IC, which isn't done continuously
    EXT_FLASH_IC_BUSY,              // SPI is not active, but flash IC is still busy
    EXT_FLASH_ALL_IDLE              // SPI is not active, and flash IC not busy
} ext_flash_status_t;

typedef enum ext_flash_result_t
{
    EXT_FLASH_SUCCESS = 0,
    EXT_FLASH_ERR_BUSY = 1
} ext_flash_result_t;

#define READ_PADDING 4          //size of padding at beginning of rx buffer, for tx bytes




/**
 * Macro for creating a buffer appropriate for use with the ext_flash_write function.
 *   Buffer needs to have 4 extra bytes at start, for external flash opcode+address
 * The following macro sets up a union that accomplishes this, while allowing the actual data
 *   to be manipulated (via the .data member) as a normal array without the 4-byte padding.
 * Example usage: 
 *   CREATE_TX_BUFFER(buffer,1);
 *   buffer.data[0] = 42;
 *   ext_flash_write(address,buffer.whole,sizeof(buffer.whole));
 */
        
#define CREATE_TX_BUFFER(__BUF_NAME__,__BUF_LEN__)      \
    union                                               \
    {                                                   \
        struct                                          \
        {                                               \
            unsigned char dummy[4];                     \
            unsigned char data[__BUF_LEN__];            \
        };                                              \
        unsigned char whole[__BUF_LEN__+4];             \
    } __BUF_NAME__


volatile int extFlashState;

/*
 *  Called on SPI event interrupt
 */
void spi_evt_handler(spi_master_evt_t spi_master_evt);

/*
 *  Initialize SPI module (master 0)
 */
void spi_init();

/*
 *  Disable SPI module
 */
void spi_end();

/*
 *  Returns true if SPI module is busy with transfer
 */
bool spi_busy();

/*
 *  Returns flash status, including checking whether the external flash chip itself is busy
 */
ext_flash_status_t ext_flash_get_status();

/*
 *  Returns true if external flash access operations are active
 */
bool ext_flash_busy();

/*
 *  Waits until external flash access operations are done
 */
void ext_flash_wait();


/*
 *  Enable writing to external flash
 */
uint32_t ext_flash_write_enable();

/*
 *  Globally unprotect sectors on flash  (must call ext_flash_write_enable first)
 */
uint32_t ext_flash_global_unprotect();

/*
 *  Read status register of external flash chip
 */
uint8_t ext_flash_read_status();

/*
 *  Read manufacturer and device ID information
 */
uint32_t ext_flash_read_MFID();

/*
 *  Read from the OTP security register
 *  (useful to verify SPI communication is functional)
 *  Parameters: initial address, receiving buffer, number of bytes to read
 *  Returns EXT_FLASH_ERR_BUSY if there's currently another pending external flash operation
 */
uint32_t ext_flash_read_OTP(unsigned int address, uint8_t* rx, unsigned int numBytes);

/*
 *  Read from flash
 *  Parameters: initial address, receiving buffer, number of bytes to read
 ** NOTE: tx buffer should contain 4 dummy bytes at beginning, for transmitting opcode, address
 *  Returns EXT_FLASH_ERR_BUSY if there's currently another pending external flash operation
 */
uint32_t ext_flash_read(unsigned int address, uint8_t* rx, unsigned int numBytes);


/*
 *  Write to flash
 *  Parameters: initial address, transmitting buffer, number of bytes to write
 ** NOTE: tx buffer must contain 4 dummy bytes at beginning.  ext_flash_write will fill them
 *    with opcode and address bytes.
 *    For convenience, see CREATE_TX_BUFFER macro above
 *  Returns EXT_FLASH_ERR_BUSY if there's currently another pending external flash operation
 */
uint32_t ext_flash_write(unsigned int address, uint8_t* tx, unsigned int numBytes);


/*
 *  Write to flash - blocking till entire write is done (not interrupt-driven)
 *  Parameters: initial address, transmitting buffer, number of bytes to write
 *  Returns EXT_FLASH_ERR_BUSY if there's currently another pending external flash operation
 */
//uint32_t ext_flash_write_blocking(unsigned int address, uint8_t* tx, unsigned int numBytes);


uint32_t ext_flash_block_erase(uint32_t address);

/*
 *  NOT USEFUL - CAN ONLY ERASE WITHIN FIRST 64kB of MEMORY
 *  Erase a page of flash
 *  Parameters: byte page address
 *  Returns EXT_FLASH_ERR_BUSY if there's currently another pending external flash operation
 */
 //uint32_t ext_flash_page_erase(uint8_t page);





/*
 *  Manually clock out a bit on MOSI and SCK
 */
//void clock(unsigned char bit);


#endif //#ifndef EXTERNAL_FLASH_H