/*
 * Classes for handling storage to flash, and retreival of data from flash for sending
 * FlashHandler simplifies writing to flash.  The program may always use storeWord() to
 *   store data; update(), if called repeatedly, handles the actual storage and organization
 *   in flash, including avoiding conflict with BLE activity.
 * RetrieveHandler simplifies sending data over BLE
 */



#include "internal_flash.h"

// this is the last chunk before we enter program memory space
const int lastChunk = ((int)(LAST_PAGE - FIRST_PAGE) << 3) + 7;  // = numberOfPages*8 + 7


//=======================  Storage-related function definitions ============================
//==========================================================================================


bool initStorageFromChunk(int chunk)  
{
    if (chunk < 0 || chunk > lastChunk)  //is the chunk index valid?
    {
        return false;  //return false if chunk number invalid
    }
    //set the initial chunk.  This one is full, storeWord will move to the next one
    store.chunk = chunk;
    uint32_t* chunkAddr = ADDRESS_OF_CHUNK(store.chunk);
    store.loc = 0;  //start from beginning of chunk
    unsigned char chunkPage = PAGE_OF_CHUNK(store.chunk);
    uint32_t* chunkPageAddr = ADDRESS_OF_PAGE(chunkPage);  //start address of _chunk's page
    
    // If it's the last chunk in page, no need to do anything here, storeWord will clear the next page
    // Otherwise, we need to clear out the unused space in the page, while retaining relevant old data
    if (chunkAddr != chunkPageAddr)  
    {
        int spaceToErase = (int)(chunkAddr) - (int)(chunkPageAddr); //how much of page we need to clear
        uint32_t temp[WORDS_PER_PAGE];  //temporary buffer for page
        memcpy(temp, chunkPageAddr, PAGE_SIZE); //copy old (possibly unsent) data to buffer
        memset(temp, 0xff, spaceToErase);  //erase unused portion of buffer (flash bits erase to all 1's)
        erasePageOfFlash(chunkPage);  //erase page
        writeBlockToFlash(chunkPageAddr, temp, WORDS_PER_PAGE);  //replace data from buffer
    }
    // We're now ready to start storing data to this page
    return true;
}


void disableStorage()  
{
    store.enabled = false;
}

bool enableStorage()  
{
    if (send.enabled)  { //Don't enable storage if sending is enabled (BLE - flash conflict)
        debug_log("ERR: can't store while sending\r\n");
        return false;
    }
    else  {
        store.enabled = true;
        return true;
    }
}



bool storeByte(uint8_t value, unsigned long timestamp)  
{
    // FIFO buffer - this function does not immediately store to flash
    int next = (store.buf.inPos + 1) % BUFSIZE;
    if(next == store.buf.outPos)  {  // Did we wrap around and fill buffer?
        debug_log("ERR: storeBud ovf\r\n");
        return false;
    }
    
    else  //put byte into buffer
    {
        //We don't need to keep a timestamp for every byte, just for the first datum in a word is okay
        if(store.buf.byteOffset == 0)  {
            store.buf.times[store.buf.inPos] = timestamp;
        }
        store.buf.bytes[store.buf.inPos*4 + store.buf.byteOffset] = value;  //Store byte
        store.buf.byteOffset++;  // Move to next byte
        if(store.buf.byteOffset > 3)  //If we wrote the last byte in a word, move to next word.
        {
            store.buf.byteOffset = 0;
            store.buf.inPos = next;
        }
        return true;
    }
}



bool updateStorage()  
{
    if (store.enabled && store.buf.inPos != store.buf.outPos)  // is storage enabled/is there data in buffer
    {
        BLEdisable();  //don't use BLE while manipulating flash
        if (store.loc == 0)  //if loc wrapped around to beginning (chunk filled), move to next chunk
        {
            unsigned char oldPage = PAGE_OF_CHUNK(store.chunk);  //the page of the complete chunk
            store.chunk = (store.chunk < lastChunk) ? store.chunk + 1 : 0; //advance to next chunk
            uint32_t* chunkAddr = ADDRESS_OF_CHUNK(store.chunk);
            unsigned char newPage = PAGE_OF_CHUNK(store.chunk);  //page of the new chunk, to be erased if it's a new page
            if (newPage != oldPage)  
            {  // do we have to start a new page
                //If storage wraps all the way around available flash, we need to bump toSend forward
                //  till it's out of the page we're erasing, so we still send from the oldest stored data
                while (PAGE_OF_CHUNK(send.chunk) == newPage)  
                {
                    send.chunk = (send.chunk < lastChunk) ? send.chunk + 1 : 0;
                }
                erasePageOfFlash(newPage);  //erase new page
                debug_log("Started page: %d\r\n",(int)(newPage));
            }
            //write the header info (sample period is constant, so we don't need to store that)
            store.timestamp = store.buf.times[store.buf.outPos];  //record the timestamp of first datum in chunk
            writeWordToFlash(chunkAddr + store.loc, store.timestamp);
            store.loc++;  //next long address
            long battery = float2Long(readBattery());
            writeWordToFlash(chunkAddr + store.loc, battery);
            store.loc++;  //next long address
            debug_log("Started chunk: %d\r\n",store.chunk);
        }
        uint32_t* chunkAddr = ADDRESS_OF_CHUNK(store.chunk);
        writeWordToFlash(chunkAddr + store.loc, store.buf.words[store.buf.outPos]);  //store the word from buffer
        store.buf.outPos = (store.buf.outPos + 1) % BUFSIZE; //increment outgoing index of ring buffer
        //debug_log("loc: %d\r\n",store.loc);
        store.loc++;
        if (store.loc >= WORDS_PER_CHUNK - 1)  
        { //if we finished a chunk, mark it as complete
            writeWordToFlash(chunkAddr + store.loc, store.timestamp); //mark as complete: write timestamp to end
            store.loc = 0;
        }
        BLEresume();
    }
    return (store.enabled && store.buf.inPos != store.buf.outPos);  //return true if there's still stuff in the buffer
}


int currentStoreChunk()  
{
    return store.chunk;
}

unsigned long currentStoreTime()  
{
    return store.timestamp;
}


// If we try an illegal flash write, something's very wrong, so we should not continue.
void writeWordToFlash(uint32_t* addr, uint32_t val)  
{
    uint8_t page = PAGE_FROM_ADDRESS(addr);
    if (page > LAST_PAGE || page < FIRST_PAGE)  
    {
        debug_log("Invalid word write address\r\n");
        panic(true); 
    }
    else  
    {
        ble_flash_word_write(addr,val);
    }
}

void writeBlockToFlash(uint32_t* to, uint32_t* from, int numWords)  
{
    uint8_t page = PAGE_FROM_ADDRESS(to);
    if (page > LAST_PAGE || page < FIRST_PAGE)  
    {
        debug_log("Invalid block write address\r\n");
        panic(true);
    }
    else  
    {
        ble_flash_block_write(to, from, numWords);
    }
}

void erasePageOfFlash(uint8_t page)  
{
    if (page > LAST_PAGE || page < FIRST_PAGE)  
    {
        debug_log("Invalid flash erase address\r\n");
        panic(true);
    }
    else  
    {
        ble_flash_page_erase(page);
    }
}


//=======================  Sending-related function definitions ============================
//==========================================================================================

//initialize sending from a chunk; if sending is enabled, updateSending will start sending chunk
bool initSendingFromChunk(int chunk)  
{
    if (chunk < 0 || chunk > lastChunk)  
    {
        return false;  //return false if chunk number invalid
    }
    send.chunk = chunk;
    send.sentHeader = false;
    send.loc = 0;
    return true;
}

//returns true if there's an unsent chunk ready
bool unsentChunkReady()  
{
    if (findNextUnsentChunk() != -1)  
    {
        return true;
    }
    return false;  //no unsent chunks found
}

bool updateSending()  
{
    if (send.enabled)  
    {
        if (send.sentHeader == false)  
        {  //try to send header
            //debug_log("Send header...");  //Spams UART if not immediately able to send
            uint32_t* toSendAddr = ADDRESS_OF_CHUNK(send.chunk);
            // send date
            char dateAsChars[4];
            char dateFractAsChars[2];
            char batAsChars[4];
            char delayAsChars[2];
            long2Chars(*toSendAddr, dateAsChars);  //get date from flash
            long2Chars(0, dateFractAsChars);  //get date fractional (ms) from flash -- TODO - replace dummy data with stored info
            long2Chars(*(toSendAddr + 1), batAsChars);  //get battery voltage from flash
            short2Chars(send.samplePeriod, delayAsChars);  //turn sample period into chars
            // pack and send
            unsigned char header[12];
            memcpy(header, dateAsChars, 4);
            memcpy(header + 4, dateFractAsChars, 2);
            memcpy(header + 6, batAsChars, 4);
            memcpy(header + 10, delayAsChars, 2);
            if (BLEwrite(header, sizeof(header)))  //try to send header
            {
                //debug_log("OK.\r\n");
                debug_log("Sent header.\r\n");
                send.loc = 2;  //should now start sending from 3rd word in chunk
                send.sentHeader = true;
            }
            else  //BLE was busy
            {
                //debug_log("busy.\r\n"); 
                send.sentHeader = false;
            }
        }
        else  
        {  //else "if send.sentHeader==true"
            // send data. The max buffer size to send it 20 byte
            // so we'll send up to 20 at a time
            if (send.loc < (WORDS_PER_CHUNK - 1)) 
            { // have we sent all actual data in the chunk?
                int wordsLeft = (WORDS_PER_CHUNK - 1) - send.loc;  //last word is timestamp, not data
                int wordsToSend = (wordsLeft > WORDS_PER_PACKET) ? WORDS_PER_PACKET : wordsLeft;
                unsigned long buf[wordsToSend];  // buffer to hold one packet of data to be sent
                memset(buf, 0, sizeof(buf));
                memcpy(buf, ADDRESS_OF_CHUNK(send.chunk) + send.loc, sizeof(buf));
                if (BLEwrite((uint8_t*)(buf), sizeof(buf)))  
                {  //can we send now?  (BLE not busy)
                    debug_log("pkt TS:%d SL:%d\r\n",send.chunk,send.loc);
                    send.loc += wordsToSend;  //if we sent, advance forward thru toSend chunk
                }
            }
            else  
            {
                writeWordToFlash(ADDRESS_OF_CHUNK(send.chunk) + WORDS_PER_CHUNK - 1, 0UL);  //mark chunk as sent
                send.numChunksSent++;
                int nextChunk = findNextUnsentChunk();
                if (nextChunk != -1 && send.numChunksSent < MAX_CHUNKS_SENT)  
                { //are there more unsent chunks ready?
                    initSendingFromChunk(nextChunk);
                    return true;
                }
                else  
                {
                    disableSending();  //if we've sent all ready chunks, stop sending.
                }
            }
        } //end of else "if send.sentHeader==true"
    }  //end of if(send.enabled)
    return false;  //return false if sending disabled, or if there's no more sending to be done
}

//Start or stop sending chunks over BLE.  Don't start if we're currently storing
void disableSending()  
{
    send.enabled = false;
}
bool enableSending()  
{
    if (store.enabled)  
    {
        debug_log("ERR: can't send while storing\r\n");
        return false;  //sending not enabled because storage is active
    }
    else  
    {
        if (send.enabled == false)  
        { //don't reset stuff if we're already sending
            int nextChunk = findNextUnsentChunk();
            if (nextChunk == -1)  
            { //no unsent chunks ready
                debug_log("ERR: nothing to send\r\n");
                return false;  //sending not enabled because there's nothing to send
            }
            else  
            {
                initSendingFromChunk(nextChunk);  //Initialize sending from that chunk
                send.enabled = true;
                send.numChunksSent = 0;
            }
        }
        return true;  //sending enabled
    }
}



//Find the next ready unsent chunk in flash, or return -1 if there are none.
int findNextUnsentChunk()  
{
    int lookChunk = send.chunk;
    while (lookChunk != store.chunk)  
    { //look until we're caught up with storage
        uint32_t* lookAddr = ADDRESS_OF_CHUNK(lookChunk);
        unsigned long lookTimestamp = *lookAddr;
        unsigned long lookCheck = *(lookAddr + WORDS_PER_CHUNK - 1);
        //is the chunk valid, i.e. not apparently from past or future
        if (lookTimestamp > MODERN_TIME && lookTimestamp < store.timestamp)  
        {
            if (lookTimestamp == lookCheck)  
            { //is it marked as completely stored
                return lookChunk;  //found a valid unsent chunk
            }
        }
        lookChunk = (lookChunk < lastChunk) ? lookChunk + 1 : 0; //look at next chunk
    }
    return -1;  //no unsent chunks found
}





//============================= misc function definitions ==================================
//==========================================================================================

// Converts a short to an array of chars
void short2Chars(short val, char* chars_array) 
{
    // Create union of shared memory space
    union 
    {
        short short_variable;
        char temp_array[2];
    } u;
    u.short_variable = val;  // Overwrite bytes of union with float variable
    memcpy(chars_array, u.temp_array, 2);  // Assign bytes to input array
}

// Converts a float to an array of chars
void float2Chars(float val, char* chars_array) 
{
  // Create union of shared memory space
    union 
    {
        float float_variable;
        char temp_array[4];
    } u;
    u.float_variable = val;  // Overwrite bytes of union with float variable
    memcpy(chars_array, u.temp_array, 4);  // Assign bytes to input array
}

// Converts a long to an array of chars
void long2Chars(long val, char* bytes_array) 
{
    // Create union of shared memory space
    union 
    {
        long long_variable;
        char temp_array[4];
    } u;
    u.long_variable = val;  // Overwrite bytes of union with float variable
    memcpy(bytes_array, u.temp_array, 4);  // Assign bytes to input array
}

//Converts a float to a long with same bytes
unsigned long float2Long(float val) 
{
    // Create union of shared memory space
    union 
    {
        float float_variable;
        unsigned long long_variable;
    } u;
    u.float_variable = val;  // Overwrite bytes of union with float variable
    return u.long_variable;  // Return long version of float
}

// Convert chars to long (expects little endian)
unsigned long readLong(uint8_t *a) {
  unsigned long retval;
  retval  = (unsigned long) a[3] << 24 | (unsigned long) a[2] << 16;
  retval |= (unsigned long) a[1] << 8 | a[0];
  return retval;
}


//Halt the main loop if doWePanic == true
void panic(int doWePanic)  
{
    if (doWePanic)  
    {
        debug_log("Halting...\r\n");
        while (1)  
        {
            nrf_gpio_pin_toggle(PANIC_LED);
            nrf_delay_ms(50);
        }
    }
}

