#include "ext_eeprom.h"



void spi_evt_handler(spi_master_evt_t spi_master_evt)
{
    if (spi_master_evt.evt_type == SPI_MASTER_EVT_TRANSFER_COMPLETED)  {
        switch (extEEPROMstate)  {
        case EXT_EEPROM_COMMAND:                     //completed sending a one-off command
            //debug_log("Sent EEPROM command\r\n");
            extEEPROMstate = EXT_EEPROM_SPI_IDLE;
            break;
        case EXT_EEPROM_READ:                    //completed reading values into rx buffer
            //debug_log("Finished read\r\n");
            extEEPROMstate = EXT_EEPROM_SPI_IDLE;
            break;
        case EXT_EEPROM_WRITE:                    //completed writing values from tx buffer
            //debug_log("Finished write\r\n");
            extEEPROMstate = EXT_EEPROM_SPI_IDLE;
            break;
        default:
            break;
        }
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
    if (err_code != NRF_SUCCESS)  {
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

ext_eeprom_status_t ext_eeprom_get_status()  {
    if (extEEPROMstate != EXT_EEPROM_SPI_IDLE)  {
        return extEEPROMstate;  // currently reading/writing on SPI
    }
    else  {
        unsigned char status = ext_eeprom_read_status();
        if (status & EXT_EEPROM_SREG_BUSY)  {
            return EXT_EEPROM_IC_BUSY;  // EEPROM chip is busy writing/erasing
        }
        return EXT_EEPROM_ALL_IDLE;       // EEPROM chip is free, no SPI transfers pending
    }
}

bool ext_eeprom_busy()  {
    return (ext_eeprom_get_status() != EXT_EEPROM_ALL_IDLE);
}

void ext_eeprom_wait()  {
    while (ext_eeprom_busy());
}


unsigned char dummyRxBuf[1];


//enable writes to EEPROM chip, needed before any writing command (status reg write, page write, etc)
static void ext_eeprom_WEL()
{
    uint8_t txBuf[1] = {WEL_OPCODE};      //enable EEPROM write
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),dummyRxBuf,0);
    while(spi_busy());
}

uint32_t ext_eeprom_global_unprotect()
{
    if (extEEPROMstate != EXT_EEPROM_SPI_IDLE)  {
        return EXT_EEPROM_ERR_BUSY;
    }
    ext_eeprom_WEL();
                    //  opcode              global unprotect
    uint8_t txBuf[2] = {WRITESREG_OPCODE,   0x00};          //Write to status register
    extEEPROMstate = EXT_EEPROM_COMMAND;
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),dummyRxBuf,0);
    while (spi_busy());
    return EXT_EEPROM_SUCCESS;
}

uint8_t ext_eeprom_read_status()
{
    if (extEEPROMstate != EXT_EEPROM_SPI_IDLE)  {
        return EXT_EEPROM_ERR_BUSY;
    }
                    //  opcode
    uint8_t txBuf[1] = {READSREG_OPCODE};
    uint8_t rxBuf[2] = {0,0};
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),rxBuf,2);
    while (spi_busy());
    return rxBuf[1];
}


uint32_t ext_eeprom_read(unsigned int address, uint8_t* rx, unsigned int numBytes)
{
    if(extEEPROMstate != EXT_EEPROM_SPI_IDLE)  {
        return EXT_EEPROM_ERR_BUSY;
    }
                    //  opcode        address
    uint8_t txBuf[4] = {READ_OPCODE,  (address>>16)&0xff,(address>>8)&0xff,address&0xff};
    extEEPROMstate = EXT_EEPROM_READ;
    spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),rx,numBytes);
    return EXT_EEPROM_SUCCESS;
}


uint32_t ext_eeprom_write(unsigned int address, uint8_t* txRaw, unsigned int numBytes)
{
    if(extEEPROMstate != EXT_EEPROM_SPI_IDLE)  {
        return EXT_EEPROM_ERR_BUSY;
    }
    ext_eeprom_WEL();  //enable writes
    txRaw[0] = WRITE_OPCODE;  //opcode
    //address bytes
    txRaw[1] = (address>>16)&0xff;
    txRaw[2] = (address>>8)&0xff;
    txRaw[3] = address&0xff;
    //data
    extEEPROMstate = EXT_EEPROM_WRITE;
    spi_master_send_recv(SPI_MASTER_0,txRaw,numBytes,dummyRxBuf,0);
    return EXT_EEPROM_SUCCESS;
}


bool testExternalEEPROM(void)
{

    // unlock EEPROM
    uint32_t stat = ext_eeprom_global_unprotect();
    if (stat != EXT_EEPROM_SUCCESS) {
        return false;
    }
    ext_eeprom_wait(); // wait for unlock to finish

    // write to the first block and test the result
    unsigned char value[8] = {0,0,0,0,1,2,3,4};  // includes padding bytes
    stat = ext_eeprom_write(0,value,sizeof(value));
    if (stat != EXT_EEPROM_SUCCESS) {
        return false;
    }

    ext_eeprom_wait(); // wait for write to finish

    /* not getting what I expected */
    unsigned char buf[8];  // includes padding bytes
    stat = ext_eeprom_read(0,buf,sizeof(buf));
    if (stat != EXT_EEPROM_SUCCESS) {
        return false;
    }

    if (buf[4] != 1) {
        debug_log("Error - got : %d\r\n", buf[0]);
        return false;
    }

    return true;
}
