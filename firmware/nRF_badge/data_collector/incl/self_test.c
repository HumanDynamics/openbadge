#include "self_test.h"
#include "debug_log.h"          //UART debugging logger
bool testInternalFlash(void)
{
	// read entire flash
	for (int c = 0; c <= lastChunk; c++)  
    {
        uint32_t* addr = ADDRESS_OF_CHUNK(c);  //address of chunk
        debug_log("%d\r\n",sizeof(*addr));

    }

	// try one byte
    uint32_t* addr = ADDRESS_OF_PAGE((FIRST_PAGE))-1;
    ble_flash_word_write( addr, (uint32_t)1);
    addr = ADDRESS_OF_PAGE((FIRST_PAGE))-1;
    return (*addr == 1);
}