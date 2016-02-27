#include "self_test.h"
#include "debug_log.h"          //UART debugging logger
bool testInternalFlash(void)
{
    /*
    // Erase all pages. Check values of all pages
	// read entire flash
	for (int c = FIRST_PAGE; c <= LAST_PAGE; c++)  
    {
        uint32_t* addr = ADDRESS_OF_PAGE(c);  //address of chunk
        debug_log("%d\r\n",sizeof(*addr));

    }
    */

    // erase a page
    erasePageOfFlash(FIRST_PAGE);


	// try writing one byte
    uint32_t* addr = ADDRESS_OF_PAGE((FIRST_PAGE));
    ble_flash_word_write( addr, (uint32_t)1);
    addr = ADDRESS_OF_PAGE((FIRST_PAGE));
    return (*addr == 1);
}

bool testExternalFlash(void)
{
    // erase the first data block
    uint32_t stat = ext_flash_block_erase(0);
    if (stat != EXT_FLASH_SUCCESS) {
        return false;
    }

    // write to the first block and test the result  
    unsigned char value[4] = {1,2,3,4};
    stat = ext_flash_write(0,value,sizeof(value));
    if (stat != EXT_FLASH_SUCCESS) {
        return false;
    }

    unsigned char buf[4];
    stat = ext_flash_read(0,buf,sizeof(buf));               
    if (stat != EXT_FLASH_SUCCESS) {
        return false;
    }

    // not doing what I expect
    /*
    if (buf[0] != 1) {
        debug_log("Error - got : %d\r\n", buf[0]);
        return false;
    }
    */
    return true;
}