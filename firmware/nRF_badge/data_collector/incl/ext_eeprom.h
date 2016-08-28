/* Methods to simplify access to an external Atmel AT25M02 (or equivalent) EEPROM IC
 *
 */


#ifndef EXT_EEPROM_H
#define EXT_EEPROM_H


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


#define WEL_OPCODE        0x06          // enable write
#define WRITESREG_OPCODE  0x01          // write to status register
#define READSREG_OPCODE   0x05          // read status register
#define WRITE_OPCODE      0x02          // write to main memory
#define READ_OPCODE       0x03          // read from main memory



#define EXT_EEPROM_SREG_BUSY 0x1         // indicates whether device is busy
#define EXT_EEPROM_SREG_WEL  0x2         // write enable bit
#define EXT_EEPROM_SREG_PR0  0x04        // block protect bit 0
#define EXT_EEPROM_SREG_PR1  0x08        // block protect bit 1
#define EXT_EEPROM_SREG_WPEN 0x80        // write protect enable


typedef enum
{
    EXT_EEPROM_SPI_IDLE = 0,         // no pending SPI operations.  EEPROM IC may still be busy
    EXT_EEPROM_COMMAND,              // sending a one-off command over SPI, e.g. write enable
    EXT_EEPROM_READ,                 // sending a read command over SPI
    EXT_EEPROM_WRITE,                // sending a write command over SPI
    EXT_EEPROM_PENDING,              // IC may be busy with a write operation
    // The following are only used in returns from ext_eeprom_get_state()
    //   because they require polling the state of the EEPROM IC, which isn't done continuously
    EXT_EEPROM_IC_BUSY,              // SPI is not active, but EEPROM IC is still busy
    EXT_EEPROM_ALL_IDLE              // SPI is not active, and EEPROM IC not busy
} ext_eeprom_status_t;

typedef enum ext_eeprom_result_t
{
    EXT_EEPROM_SUCCESS = 0,
    EXT_EEPROM_ERR_BUSY = 1
} ext_eeprom_result_t;

#define READ_PADDING 4          //size of padding at beginning of rx buffer, for tx bytes




/**
 * Macro for creating a buffer appropriate for use with the ext_eeprom_write function.
 *   Buffer needs to have 4 extra bytes at start, for external EEPROM opcode+address
 * The following macro sets up a union that accomplishes this, while allowing the actual data
 *   to be manipulated (via the .data member) as a normal array without the 4-byte padding.
 * Example usage: 
 *   CREATE_TX_BUFFER(buffer,1);
 *   buffer.data[0] = 42;
 *   ext_eeprom_write(address,buffer.whole,sizeof(buffer.whole));
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


volatile int extEEPROMstate;

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
 *  Returns EEPROM access status, including checking whether the external EEPROM chip itself is busy
 */
ext_eeprom_status_t ext_eeprom_get_status();

/*
 *  Returns true if external eeprom access operations are active
 */
bool ext_eeprom_busy();

/*
 *  Waits until external eeprom access operations are done
 */
void ext_eeprom_wait();


/*
 *  Enable writing to external EEPROM
 */
uint32_t ext_eeprom_write_enable();

/*
 *  Globally unprotect sectors on EEPROM  (must call ext_eeprom_write_enable first)
 */
uint32_t ext_eeprom_global_unprotect();

/*
 *  Read status register of external EEPROM chip
 */
uint8_t ext_eeprom_read_status();

/*
 *  Read manufacturer and device ID information
 */
uint32_t ext_eeprom_read_MFID();

/*
 *  Read from the OTP security register
 *  (useful to verify SPI communication is functional)
 *  Parameters: initial address, receiving buffer, number of bytes to read
 *  Returns EXT_EEPROM_ERR_BUSY if there's currently another pending external EEPROM operation
 */
uint32_t ext_eeprom_read_OTP(unsigned int address, uint8_t* rx, unsigned int numBytes);

/*
 *  Read from EEPROM
 *  Parameters: initial address, receiving buffer, number of bytes to read
 ** NOTE: rx buffer should contain 4 dummy bytes at beginning, for transmitting opcode, address
 *  Returns EXT_EEPROM_ERR_BUSY if there's currently another pending external EEPROM operation
 */
uint32_t ext_eeprom_read(unsigned int address, uint8_t* rx, unsigned int numBytes);


/*
 *  Write to EEPROM
 *  Parameters: initial address, transmitting buffer, number of bytes to write
 ** NOTE: tx buffer must contain 4 dummy bytes at beginning.  ext_eeprom_write will fill them
 *    with opcode and address bytes.
 *    For convenience, see CREATE_TX_BUFFER macro above
 *  Returns EXT_EEPROM_ERR_BUSY if there's currently another pending external EEPROM operation
 */
uint32_t ext_eeprom_write(unsigned int address, uint8_t* tx, unsigned int numBytes);



// Tests external EEPROM. Returns true on success
bool testExternalEEPROM(void);



/*
 *  Manually clock out a bit on MOSI and SCK
 */
//void clock(unsigned char bit);


#endif //#ifndef EXTERNAL_EEPROM_H