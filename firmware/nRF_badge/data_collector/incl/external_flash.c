#include "external_flash.h"



void spi_evt_handler(spi_master_evt_t spi_master_evt)
{
    switch (spi_master_evt.evt_type)
    {
        case SPI_MASTER_EVT_TRANSFER_COMPLETED:
            switch (extFlashState)
            {
                case EXT_FLASH_COMMAND:                     //completed sending a one-off command
                    //debug_log("Sent flash command\r\n");
                    extFlashState = EXT_FLASH_SPI_IDLE;
                    break;
                case EXT_FLASH_READ:                    //completed reading values into rx buffer
                    //debug_log("Finished read\r\n");    
                    extFlashState = EXT_FLASH_SPI_IDLE;
                    break;
                case EXT_FLASH_WRITE:                    //completed writing values from tx buffer
                    //debug_log("Finished write\r\n");    
                    extFlashState = EXT_FLASH_SPI_IDLE;
                    break;
                default:
                    break;
            }
            break;
        default:
            // No implementation needed.
            break;
    }
}

void spi_init()
{
    uint32_t err_code = NRF_SUCCESS;
    
    //manually handle SS pin
    nrf_gpio_pin_set(SPIM0_SS_PIN);
    nrf_gpio_cfg_output(SPIM0_SS_PIN);

    // Configure SPI master.
    spi_master_config_t spi_config = {
        SPI_FREQUENCY_FREQUENCY_M8, // Serial clock frequency 1 Mbps.
        SPIM0_SCK_PIN,              // Defined in badge_03.h
        SPIM0_MISO_PIN,
        SPIM0_MOSI_PIN,
        SPIM0_SS_PIN,                         //    // SS, we'll handle that manually
        APP_IRQ_PRIORITY_LOW,       // Interrupt priority LOW.
        SPI_CONFIG_ORDER_MsbFirst,  // Bits order LSB.
        SPI_CONFIG_CPOL_ActiveLow, // Serial clock polarity ACTIVELOW.
        SPI_CONFIG_CPHA_Trailing,    // Serial clock phase TRAILING.
        0                           // Don't disable all IRQs.
    };

    err_code = spi_master_open(SPI_MASTER_0, &spi_config);
    if(err_code != NRF_SUCCESS)
    {
        debug_log("Err\r\n");
    }

    // Register event handler for SPI master.
    spi_master_evt_handler_reg(SPI_MASTER_0, spi_evt_handler);
}

void spi_end()  {
    spi_master_close(SPI_MASTER_0);
}

bool spi_busy()  {
    return (spi_master_get_state(SPI_MASTER_0) == SPI_MASTER_STATE_BUSY);    
}

ext_flash_status_t ext_flash_get_status()  {
    if(extFlashState != EXT_FLASH_SPI_IDLE)
    {
        return extFlashState;  // currently reading/writing on SPI
    }
    else
    {
        unsigned char status = ext_flash_read_status();
        if(status & EXT_FLASH_SREG_BUSY)
        {
            return EXT_FLASH_IC_BUSY;  // flash chip is busy writing/erasing
        }
        return EXT_FLASH_ALL_IDLE;       // flash chip is free, no SPI transfers pending
    }
}

bool ext_flash_busy()  {
    return (ext_flash_get_status() != EXT_FLASH_ALL_IDLE);
}

void ext_flash_wait()  {
    while(ext_flash_busy());
}


unsigned char dummyRxBuf[1];


//enable writes to flash chip, needed before any writing command (status reg write, page erase, array write, etc)
static void ext_flash_WEL()
{
    uint8_t txBuf[1] = {WEL_OPCODE};      //enable flash write
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),dummyRxBuf,0);
    while(spi_busy());
}

uint32_t ext_flash_global_unprotect()
{
    if(extFlashState != EXT_FLASH_SPI_IDLE)  {
        return EXT_FLASH_ERR_BUSY;
    }
    ext_flash_WEL();
                    //  opcode              global unprotect
    uint8_t txBuf[2] = {WRITESREG_OPCODE,   0x00};          //Write to status register
    extFlashState = EXT_FLASH_COMMAND;
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),dummyRxBuf,0);
    return EXT_FLASH_SUCCESS;
}

uint8_t ext_flash_read_status()
{
    if(extFlashState != EXT_FLASH_SPI_IDLE)  {
        return EXT_FLASH_ERR_BUSY;
    }
                    //  opcode
    uint8_t txBuf[1] = {READSREG_OPCODE};
    uint8_t rxBuf[2] = {0,0};
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),rxBuf,2);
    while(spi_busy());
    return rxBuf[1];
}

uint32_t ext_flash_read_MFID()
{
    if(extFlashState != EXT_FLASH_SPI_IDLE)  {
        return EXT_FLASH_ERR_BUSY;
    }
                    //  opcode
    uint8_t txBuf[1] = {READMFID_OPCODE};
    uint8_t rxBuf[5] = {0,0,0,0,0};
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),rxBuf,5);
    while(spi_busy());
    return (rxBuf[1]<<24) | (rxBuf[2]<<16) | (rxBuf[3]<<8) | rxBuf[4];
}


uint32_t ext_flash_read_OTP(unsigned int address, uint8_t* rx, unsigned int numBytes)
{
    if(extFlashState != EXT_FLASH_SPI_IDLE)  {
        return EXT_FLASH_ERR_BUSY;
    }
                    //  opcode           address                                             dummy
    uint8_t txBuf[6] = {READOTP_OPCODE,  (address>>16)&0xff,(address>>8)&0xff,address&0xff,  0x00,0x00};
    extFlashState = EXT_FLASH_READ;
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),rx,numBytes);
    return EXT_FLASH_SUCCESS;
}


uint32_t ext_flash_read(unsigned int address, uint8_t* rx, unsigned int numBytes)
{
    if(extFlashState != EXT_FLASH_SPI_IDLE)  {
        return EXT_FLASH_ERR_BUSY;
    }
                    //  opcode        address
    uint8_t txBuf[4] = {READ_OPCODE,  (address>>16)&0xff,(address>>8)&0xff,address&0xff};
    extFlashState = EXT_FLASH_READ;
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),rx,numBytes);
    return EXT_FLASH_SUCCESS;
}


/**
 * Old implementation.  Wastes memory and time in copying the tx input buffer to another buffer
 *   that includes the opcode+address bytes
 *
uint32_t ext_flash_write(unsigned int address, uint8_t* tx, unsigned int numBytes)
{
    if(extFlashState != EXT_FLASH_SPI_IDLE)  {
        return EXT_FLASH_ERR_BUSY;
    }
    ext_flash_WEL();  //enable writes
    uint8_t txBuf[numBytes+4];  //4 extra bytes for opcode+address
    txBuf[0] = 0x02;  //opcode
    //address bytes
    txBuf[1] = (address>>16)&0xff;
    txBuf[2] = (address>>8)&0xff;
    txBuf[3] = address&0xff;
    //data
    memcpy(txBuf+4,tx,numBytes);
    extFlashState = EXT_FLASH_WRITE;
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),dummyRxBuf,0);
    return EXT_FLASH_SUCCESS;
}
*/

/**
 * Now, input tx buffer is assumed to have 4 unused bytes at start
 *  and is used as the SPI buffer directly
 */
uint32_t ext_flash_write(unsigned int address, uint8_t* txRaw, unsigned int numBytes)
{
    if(extFlashState != EXT_FLASH_SPI_IDLE)  {
        return EXT_FLASH_ERR_BUSY;
    }
    ext_flash_WEL();  //enable writes
    txRaw[0] = WRITE_OPCODE;  //opcode
    //address bytes
    txRaw[1] = (address>>16)&0xff;
    txRaw[2] = (address>>8)&0xff;
    txRaw[3] = address&0xff;
    //data
    extFlashState = EXT_FLASH_WRITE;
    spi_master_send_recv(SPI_MASTER_0,txRaw,numBytes,dummyRxBuf,0);
    return EXT_FLASH_SUCCESS;
}


uint32_t ext_flash_block_erase(uint32_t address)
{
    if(extFlashState != EXT_FLASH_SPI_IDLE)  {
        return EXT_FLASH_ERR_BUSY;
    }
    ext_flash_WEL();  //enable writes
    
                    //  opcode              address bytes
    uint8_t txBuf[4] = {BLOCKERASE_OPCODE,  (address>>16)&0xff,(address>>8)&0xff,address&0xff};
    extFlashState = EXT_FLASH_COMMAND;
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),dummyRxBuf,0);
    return EXT_FLASH_SUCCESS;
}


bool testExternalFlash(void)
{
    
    // unlock flash
    uint32_t stat = ext_flash_global_unprotect();
    if (stat != EXT_FLASH_SUCCESS) {
        return false;
    }
    ext_flash_wait(); // wait for unlock to finish

    // erase the first data block
    stat = ext_flash_block_erase(0);
    if (stat != EXT_FLASH_SUCCESS) {
        return false;
    }

    ext_flash_wait(); // wait for erase to finish

    // write to the first block and test the result  
    unsigned char value[8] = {0,0,0,0,1,2,3,4};  // includes padding bytes
    stat = ext_flash_write(0,value,sizeof(value));
    if (stat != EXT_FLASH_SUCCESS) {
        return false;
    }

    ext_flash_wait(); // wait for write to finish

    /* not getting what I expected */
    unsigned char buf[8];  // includes padding bytes
    stat = ext_flash_read(0,buf,sizeof(buf));               
    if (stat != EXT_FLASH_SUCCESS) {
        return false;
    }

    if (buf[4] != 1) {
        debug_log("Error - got : %d\r\n", buf[0]);
        return false;
    }
    
    return true;
}

// NOT USEFUL - CAN ONLY ERASE WITHIN FIRST 64kB of MEMORY
/*uint32_t ext_flash_page_erase(uint8_t page)
{
    if(extFlashState != EXT_FLASH_SPI_IDLE)  {
        return EXT_FLASH_ERR_BUSY;
    }
    ext_flash_WEL();  //enable writes
    
                    //  opcode  dummy   address dummy
    uint8_t txBuf[4] = {0x81,   0x0,    page,   0x0};
    extFlashState = EXT_FLASH_COMMAND;
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),dummyRxBuf,0);
    return EXT_FLASH_SUCCESS;
}*/


//For manually bit-banging SPI
/*void clock(unsigned char bit)
{
    nrf_gpio_pin_write(SPIM0_MOSI_PIN,bit);
    nrf_gpio_pin_set(SPIM0_SCK_PIN);
    nrf_gpio_pin_clear(SPIM0_SCK_PIN);
}*/