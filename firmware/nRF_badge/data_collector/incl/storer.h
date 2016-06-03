/*
 * INFORMATION ****************************************************
 */

#ifndef STORER_H
#define STORER_H

#include <stdint.h>
#include <string.h>

#include "debug_log.h"

#include "nrf_drv_config.h"
#include "boards.h"

#include "nrf_soc.h"




/**=================== various constants/macros for defining "chunks" =============
 *
 * Data in flash needs to have periodic timestamps, to keep it ordered.
 * Timestamp is first 4 bytes of chunk; repeated at last 4 bytes after chunk has been sent
 * For robustness, flash data storage starts at the end, page 251, backwards, i.e. :
 *  . ...Page 250    ] [                               Page 251                              ]
 *  ...(CHNK9)(CHNK8)] [(CHNK7 )( CHNK6 )( CHNK5 )( CHNK4 )( CHNK3 )( CHNK2 )( CHNK1 )(CHNK0)]
 * 
 *  So there are 3 representations of locations in memory: chunk index, actual byte/word address, and page index
 *  The following defines and macros establish these representations and conversions between them
 *
 */
 

// Ideally this would be the first page free after program space.
//   May be acquirable from linker script.  But the following value is a (conservative) estimated value.
#define FIRST_PAGE 150
#define LAST_PAGE 251  // last available FLASH page - rest is reserved
 
#define BYTES_PER_PAGE 1024  //1kB pages in nrf51 flash
#define WORDS_PER_PAGE 256  //4-byte words, 1024/4
// divide the memory address by 1024 for page index
#define  PAGE_FROM_ADDRESS(address)  ((uint8_t)((uint32_t)address >> 10))
// multiply the page index by 1024 for memory address
#define  ADDRESS_OF_PAGE(page)  ((uint32_t*)(page << 10))
 

#define CHUNK_SIZE 128  //8 chunks per 1kB page.
#define WORDS_PER_CHUNK 32 //4-byte words, 128/4
#define LAST_CHUNK_ADDRESS ((LAST_PAGE*1024)+(7*CHUNK_SIZE)) // = last page address + offset of last chunk = 251*1024 + 7*128
// multiply chunk index by 128, count back from last memory address to get address of chunk
#define ADDRESS_OF_CHUNK(chnk) ( (uint32_t*)(LAST_CHUNK_ADDRESS - (chnk << 7)) )
// divide chunk index by 8, count back from last page
#define PAGE_OF_CHUNK(chnk) ((uint8_t)(LAST_PAGE - (chnk >> 3)))
// this is the last chunk before we enter program memory space
//const int LAST_CHUNK;  // = numberOfPages*8 + 7, defined in flash_handling.c
#define LAST_FLASH_CHUNK (((LAST_PAGE - FIRST_PAGE) << 3) + 7)  // = numberOfPages*8 + 7

#define NO_CHUNK 0xffff


#define MODERN_TIME 1434240000UL  // Unix time in the recent past (sometime June 2015), used to check for reasonable timestamps
#define FUTURE_TIME 2524608000UL  // Unix time somewhere around 2050, used to check for reasonable timestamps

#define CHECK_STORED 0x2UL        // Used to mark a RAM chunk once it's been stored to FLASH (to avoid having 2 identical copies)

#include "collector.h"
#include "sender.h"
#include "analog.h"



typedef enum storer_mode_t
{
    STORER_IDLE,        // storer inactive; nothing to store
    STORER_STORE,       // waiting to store data (BLE must be disabled first, then data must be written to flash)
    STORER_ADVANCE,     // advancing to next from/to chunks, possibly erasing a new page
    STORER_INIT         // performing various initialization tasks
} storer_mode_t;


struct
{
    int from;      // which chunk in RAM buffer we're currently storing from
    int to;        // which chunk in flash we're currently storing to
    int loc;        // next word index in chunk word buffer to be transferred to flash
} store;          // Struct for keeping track of storing mic data to RAM


void storer_init();

/*
 * Called within main loop cycle to manage FLASH storage.
 *   - Avoids conflicts with BLE
 * Returns whether the storer has any pending operations.
 */
bool updateStorer();

void storer_on_sys_evt(uint32_t sys_evt);

void printStorerChunk(int chunk);


// Functions for dealing with flash.  Protect from invalid writes/erases.
void writeWordToFlash(uint32_t* addr, uint32_t val);
void writeBlockToFlash(uint32_t* to, uint32_t* from, int numWords);
void erasePageOfFlash(uint8_t page);


#endif //#ifndef STORER_H

