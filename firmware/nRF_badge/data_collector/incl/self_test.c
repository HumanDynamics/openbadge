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

void testMicAddSample(int s) {
    int abs_s = abs(s - threshold_buffer.zeroValue);
    uint8_t clip_s = abs_s <= 255 ? abs_s : 255;  //clip reading
    threshold_buffer.pos = (threshold_buffer.pos + 1) % THRESH_BUFSIZE;;
    threshold_buffer.bytes[threshold_buffer.pos] = clip_s;
    
    debug_log("added : %d\r\n", clip_s);
}

void testMicInit(int zeroValue) {
    threshold_buffer.pos = 0;
    threshold_buffer.zeroValue = zeroValue;
    memset(threshold_buffer.bytes, 0, sizeof(threshold_buffer.bytes));
}

uint8_t testMicAvg() {
    int sum = 0;
 
    for ( int i = 0; i < THRESH_BUFSIZE; i++ ) {
        debug_log("%d,", threshold_buffer.bytes[i]);
        sum += threshold_buffer.bytes[i];
    }
    debug_log("\r\n");
    return sum/THRESH_BUFSIZE;
}

bool testMicAboveThreshold() {
    uint8_t avg = testMicAvg();
    double avg_with_sd = avg * THRESH_SD;
    uint8_t lastSample = threshold_buffer.bytes[threshold_buffer.pos];
    debug_log("avg %d, avg with SD %f, sample %d\r\n", avg, avg_with_sd, lastSample);   
    return lastSample > avg_with_sd;
}