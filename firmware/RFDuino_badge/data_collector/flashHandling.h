/*
 * Classes for handling storage to flash, and retreival of data from flash for sending
 * FlashHandler simplifies writing to flash.  The program may always use storeWord() to
 *   store data; update(), if called repeatedly, handles the actual storage and organization
 *   in flash, including avoiding conflict with BLE activity.
 * RetrieveHandler simplifies sending data over BLE
 */

#ifndef flashHandling_h
#define flashHandling_h

#include <RFduinoBLE.h>
#include <Arduino.h>



#define PAGE_SIZE 1024  //1kB pages in nrf51 flash
#define LAST_PAGE 251  // last available FLASH page
// pages before this are used up by the program, and softDevice:  (refer to memory.h, memory.c)
const unsigned char firstPage = PAGE_FROM_ADDRESS(&_etextrelocate) + 1;

/*=================== various constants/macros for defining "chunks" =============
 Data in flash needs to have periodic timestamps, to keep it ordered.
 Timestamp is first 4 bytes of chunk; repeated at last 4 bytes after chunk has been sent
 For robustness, flash data storage starts at the end, page 251, backwards, i.e. :
  . ...Page 250    ] [                               Page 251                              ]
  ...(CHNK9)(CHNK8)] [(CHNK7 )( CHNK6 )( CHNK5 )( CHNK4 )( CHNK3 )( CHNK2 )( CHNK1 )(CHNK0)]
*/
#define CHUNK_SIZE 128  //8 chunks per 1kB page.
const int WORDS_PER_CHUNK = CHUNK_SIZE / 4; //4-byte words

#define LAST_CHUNK_ADDRESS 257920 // = last page address + offset of last chunk = 251*1024 + 7*128
// multiply chunk index by 128, count back from last address
#define ADDRESS_OF_CHUNK(chnk) ( (uint32_t*)(LAST_CHUNK_ADDRESS - (chnk << 7)) )
// divide chunk index by 8, count back from last page
#define PAGE_OF_CHUNK(chnk) ((uint8_t)(LAST_PAGE - (chnk >> 3)))
// this is the last chunk before we enter program memory space
const int lastChunk = (int(LAST_PAGE - firstPage) << 3) + 7;  // = numberOfPages*8 + 7

#define MODERN_TIME 1434240000UL  // Unix time in the recent past (sometime June 2015)

#define PACKET_SIZE 20  // BLE supports 20 byte packets
const int WORDS_PER_PACKET = PACKET_SIZE / 4; //4-byte words


/********************************************************
 * Storage handler
 ******************************************************/

#define BUFSIZE 32  //size of store buffer.  Keep a multiple of 2, for efficient modulo

class FlashHandler  {
  private:
    //============== Private storage-related members
    boolean _storageEnabled;  //whether we should write to flash (don't write while BLE busy)
    uint32_t _storeBuf[BUFSIZE];  //buffer to hold data temporarily, if we can't immediately store it
    unsigned long _storeBufTS[BUFSIZE];  //timestamps of data in _storeBuf
    int _bufIn, _bufOut;  //index for putting data into storage buffer
    int _chunk;  // index of current storage chunk  (see notes above)
    unsigned long _chunkTimestamp;  //timestamp of beginning of current chunk
    int _storeLoc;  // word location within current chunk to store to.  (i.e. index from 0 to 31)
    //Storage private functions
    void _writeWord(uint32_t* addr, uint32_t val);  //write one 4byte word to flash
    void _writeBlock(void* to, const void* from, int numBytes);  //write a block of bytes to flash
    void _erasePage(uint8_t page);  //erase a 1kB page
    void _startNewChunk(unsigned long timestamp);  //set up a new chunk, i.e. write header info
    void _panic(int doWePanic);  //Infinitely flash an led, in case of fault

    //============== Private sending-related members
    boolean _sendingEnabled;  //whether we should send data
    int _toSend;  // index of current sending chunk
    int _sendLoc;  // word location within current chunk to send from.  (i.e. index from 0 to 31)
    boolean _sentHeader;
    //Private sending functions
    void _sendHeader();  //try to send header — returns false if BLE was busy
    int _findNextUnsentChunk();
    void _markSent(int chunk);  //Denote that a chunk has been sent over BLE

    //============== misc
    float _readBattery();  //maybe move out of FlashHandler class sometime
    int _panicLEDpin;  //LED flashing signals critical fault

    
  public:
    FlashHandler();  //initialize variables

    //============== Public storage-related members
    boolean initStorageFromChunk(int chunk);  //ensure chunk is clear, without losing old data
    boolean updateStorage();  //Must be called repeatedly to actually store things to flash
    void disableStorage();  //Stop flash access (storeWord() will still add to buffer)
    boolean enableStorage();    //Resume flash access.  returns false if can't enable
    boolean storeWord(uint32_t value, unsigned long timestamp);  //queue a word to be stored
    int currentStoreChunk();  //get the index of the current storage chunk
    unsigned long currentStoreTime();  //get the start timestamp of the current storage chunk

    //============== Public storage-related members
    boolean initSendingFromChunk(int chunk);  //set us up for sending from a chunk
    boolean unsentChunkReady();  //returns whether there are unsent chunks ready
    boolean updateSending();  //Must be called repeatedly to actually send data
    void disableSending();  //Stop sending, and reset ourselves to re-try the same chunk next time
    boolean enableSending();  //Resume sending.  returns false if can't enable

    //============== misc
    //Some functions for converting types into other types, with the same bytes
    void short2Chars(short val, char* chars_array);
    void float2Chars(float val, char* chars_array);
    void long2Chars(long val, char* bytes_array);
    unsigned long float2Long(float val);

    int samplePeriod;
    
};

#endif //#ifndef flashHandling_h


