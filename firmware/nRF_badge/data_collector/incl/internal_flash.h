/*
 * Methods to simplifies writing to flash:  The program may always use storeWord() to
 *   store data; update(), if called repeatedly, handles the actual storage and organization
 *   in flash, including avoiding conflict with BLE activity.
 * Methods to automate sending data over BLE from flash
 */

#ifndef INTERNAL_FLASH_H
#define INTERNAL_FLASH_H

#include <stdint.h>
#include <string.h>

#include "debug_log.h"
#include "nrf_gpio.h"           //abstraction for dealing with gpio
#include "nrf_adc.h"            //abstraction for dealing with adc
#include "ble_flash.h"          //for accessing flash
#include "nrf_delay.h"

#include "nrf_drv_config.h"
#include "boards.h"

#define PANIC_LED LED_2

#include "ble_setup.h"
#include "analog.h"



/**=================== various constants/macros for defining "chunks" =============
 *
 * Data in flash needs to have periodic timestamps, to keep it ordered.
 * Timestamp is first 4 bytes of chunk; repeated at last 4 bytes after chunk has been sent
 * For robustness, flash data storage starts at the end, page 251, backwards, i.e. :
 *  . ...Page 250    ] [                               Page 251                              ]
 *  ...(CHNK9)(CHNK8)] [(CHNK7 )( CHNK6 )( CHNK5 )( CHNK4 )( CHNK3 )( CHNK2 )( CHNK1 )(CHNK0)]
 * 
 *  So there are 3 representations of locations in memory: chunk index, actual byte address, and page index
 *  The following defines and macros establish these representations and conversions between them
 *
 */
 

// Ideally this would be the first page free after program space.
//   May be acquirable from linker script.  But the following value is a rough estimated value.
#define FIRST_PAGE 151
#define LAST_PAGE 251  // last available FLASH page - rest is reserved
 
#define PAGE_SIZE 1024  //1kB pages in nrf51 flash
#define WORDS_PER_PAGE 256  //4-byte words, 1024/4
// divide the memory address by 1024 for page index
#define  PAGE_FROM_ADDRESS(address)  ((uint8_t)((uint32_t)address >> 10))
// multiply the page index by 1024 for memory address
#define  ADDRESS_OF_PAGE(page)  ((uint32_t*)(page << 10))
 

#define CHUNK_SIZE 128  //8 chunks per 1kB page.
#define WORDS_PER_CHUNK 32 //4-byte words, 128/4
#define LAST_CHUNK_ADDRESS 257920 // = last page address + offset of last chunk = 251*1024 + 7*128
// multiply chunk index by 128, count back from last memory address to get address of chunk
#define ADDRESS_OF_CHUNK(chnk) ( (uint32_t*)(LAST_CHUNK_ADDRESS - (chnk << 7)) )
// divide chunk index by 8, count back from last page
#define PAGE_OF_CHUNK(chnk) ((uint8_t)(LAST_PAGE - (chnk >> 3)))
// this is the last chunk before we enter program memory space
const int lastChunk;  // = numberOfPages*8 + 7, defined in flash_handling.c



// BLE does not have a native UART functionality; data can only be sent in 20 byte packets
#define PACKET_SIZE 20  // BLE supports 20 byte packets
#define WORDS_PER_PACKET 5 //4-byte words, 20/4


// Unix time in recent past (sometime June 2015), used to check for timestamp validity
#define MODERN_TIME 1434240000UL  // Unix time in the recent past (sometime June 2015), used to check for reasonable times
// Maximum number of chunks to be sent in one connection interval.  Very long connections can cause issues.
#define MAX_CHUNKS_SENT 50




/**========================= Internal flash storage handler ============================================
 *
 * Abstraction for storing data to chunks.
 * Microphone data words can be simply pushed to this module continuously, and the storage handler
 *   takes care of keeping the chunk organization in flash.  Stores words sequentially in a chunk,
 *   initializes the next chunk and starts storing there when a chunk fills.
 *
 * One caveat is that flash writes are totally blocking to the main CPU, which can cause issues
 *   with BLE activity.  Flash operations must be paused during a BLE connection, and the softdevice
 *   must be disabled during flash operations.
 * The microphone is still collecting data, so a simple FIFO buffer is implemented between the
 *   data collection and the actual flash storage
 *
 * Sending data from flash is also abstracted; the module, when sending is enabled, will automatically
 *   find the oldest unsent data and push, in 20byte packets, to BLE.
 *
 */

#define BUFSIZE 64  //size of store buffer, in words.  Keep a multiple of 2, for efficient modulo


// Structs to keep things orderly.
struct
{
    struct        //FIFO word buffer
    {
        union
        {
            uint32_t words[BUFSIZE];
            uint8_t bytes[BUFSIZE*4];
        };          //Data is written as bytes (from mic), but must be stored to flash as words
        int byteOffset;  //Which byte is to be written in current word
        unsigned long times[BUFSIZE];  //Timestamps of words in buffer
        int inPos, outPos;      //inPos is the _word_ we're currently writing to in the buffer; 
                                //  outPos is the next word that will exit the buffer.
    } buf;          
    bool enabled;  //Whether storage is enabled (if disabled, words pushed to the storage handler will be buffered)
    int chunk;     //index of chunk currently being stored to
    int loc;       //word offset from beginning of chunk, to keep track of location within chunk
    unsigned long timestamp;  //Timestamp of first data in current chunk
} store;

struct
{
    bool enabled;       //whether sending is enabled (after request from master)
    bool sentHeader;    //whether the header (timestamp, battery level) for the current chunk has been sent yet
    int chunk;          //index of chunk currently being sent
    int loc;            //word offset from beginning of chunk, to keep track of location within chunk
    int samplePeriod;   //the sampling period in ms
    int numChunksSent;  //number of chunks sent so far in the current connection interval
} send;


//=========== Storage-related

/**
 * On restart, we need to make sure a page is prepped to have data stored to it
 * But we don't want to lose old data, e.g. if we're starting storage at chunk 4:
 * [                               Page 251                              ]
 * [(CHNK7 )( CHNK6 )( CHNK5 )( CHNK4 )( CHNK3 )( CHNK2 )( CHNK1 )(CHNK0)]
 *  \  need to be cleared   /  \start/  \   old, perhaps unsent, data  /
 *
 * This function's input should be the last complete chunk; storage will start immediately after it.
 */
bool initStorageFromChunk(int chunk);

// Enable and disable flash storage.  Will not enable if sending is enabled
void disableStorage();
bool enableStorage();

// Push a byte of data to storage buffer.  Returns false and does nothing if buffer is full
bool storeByte(uint8_t value, unsigned long timestamp);

// Must be called repeatedly to ensure storage buffer is transferred to flash
// Returns true if there is still buffered data to be stored, false if all up-to-date
bool updateStorage();

int currentStoreChunk();           // Returns index of chunk currently being stored to
unsigned long currentStoreTime();  // Returns timestamp of first data in chunk currently being stored to


// Functions for dealing with flash.  Protect from invalid writes/erases.
void writeWordToFlash(uint32_t* addr, uint32_t val);
void writeBlockToFlash(uint32_t* to, uint32_t* from, int numWords);
void erasePageOfFlash(uint8_t page);



//========== Sending-related

//Find the next ready unsent chunk in flash, or return -1 if there are none.
int findNextUnsentChunk();

// Initialize sending from a chunk; if sending is enabled, updateSending will then start sending that chunk
bool initSendingFromChunk(int chunk);

// Returns true if there's an unsent chunk ready to be sent
bool unsentChunkReady();

// Must be called repeatedly to relay packets of data to master
bool updateSending();

//Start or stop sending chunks over BLE.  Won't start if we're currently storing
void disableSending();
bool enableSending();




//=========== Utility functions

// Converts a short to an array of chars
void short2Chars(short val, char* chars_array);

// Converts a float to an array of chars
void float2Chars(float val, char* chars_array);

// Converts a long to an array of chars
void long2Chars(long val, char* bytes_array);

//Converts a float to a long with same bytes
unsigned long float2Long(float val);

unsigned long readLong(uint8_t *a);

//Halt the program if doWePanic == true
void panic(int doWePanic);





#endif //#ifndef INTERNAL_FLASH_H

