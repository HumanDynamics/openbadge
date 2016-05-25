/*
 * INFORMATION ****************************************************
 */



#include "storer.h"


volatile bool flashWorking;
volatile storer_mode_t storerMode = STORER_IDLE;


// this is the last chunk before we enter program memory space
const int LAST_CHUNK = ((int)(LAST_PAGE - FIRST_PAGE) << 3) + 7;  // = numberOfPages*8 + 7


void storer_init()
{
    store.from = 0;
    store.loc = 0;
    store.to = 0;
    
    storerMode = STORER_IDLE;
    
    unsigned long lastStoredTime = MODERN_TIME;
    
    for(int c = 0; c <= LAST_CHUNK; c++)
    {
        mic_chunk_t* chunkPtr = (mic_chunk_t*)ADDRESS_OF_CHUNK(c);

        unsigned long timestamp = (*chunkPtr).timestamp;
        unsigned long chunkCheck = (*chunkPtr).check;
        
        //debug_log("time: 0x%lX\r\n", timestamp);
        
        if (timestamp != 0xffffffffUL && timestamp > MODERN_TIME)  //is the timestamp possibly valid?
        { 
            if (timestamp == chunkCheck || chunkCheck == 0)  //is it a completely stored chunk?
            { 
                if (timestamp > lastStoredTime)  //is it later than the latest stored one found so far?
                { 
                    store.to = c; //keep track of latest stored chunk
                    lastStoredTime = timestamp;
                }
            }
        }
        //nrf_delay_ms(50);
    }
    
    if(lastStoredTime == MODERN_TIME)   // no valid chunk found
    {
        debug_log("No stored chunks found. Starting from chunk 0\r\n");
        store.to = 0;
    }
    else
    {
        debug_log("Last stored chunk: %d at time: 0x%lX\r\n\r\n", store.to, lastStoredTime);
        printStorerChunk(store.to);
        store.to = (store.to < LAST_CHUNK) ? store.to+1 : 0;  // advance to next chunk
    }
    
    
    uint32_t* chunkAddr = ADDRESS_OF_CHUNK(store.to);
    unsigned char chunkPage = PAGE_OF_CHUNK(store.to);
    uint32_t* chunkPageAddr = ADDRESS_OF_PAGE(chunkPage);  //start address of _chunk's page
    
    // If the chunk is in the middle of the page, we want to preserve any potentially unsent data.

    if (chunkAddr != chunkPageAddr)  
    {
        int spaceToErase = (int)(chunkAddr) - (int)(chunkPageAddr); //how much of page we need to clear
        uint32_t temp[WORDS_PER_PAGE];  //temporary buffer for page
        memcpy(temp, chunkPageAddr, BYTES_PER_PAGE); //copy old (possibly unsent) data to buffer
        memset(temp, 0xff, spaceToErase);  //erase unused portion of buffer (flash bits erase to all 1's)
        erasePageOfFlash(chunkPage);  //erase page
        writeBlockToFlash(chunkPageAddr, temp, WORDS_PER_PAGE);  //replace data from buffer
    }
    else        // If we're trying to write to the beginning of a page, we can just erase the whole page.
    {
        erasePageOfFlash(chunkPage);  //erase page
    }
    
    // Chunk store.to is now ready to have data stored to it.
    
}


bool updateStorer()
{
    // This will be the function return value.  if there is no storage actions to be completed, this will be set to false.
    //   if not, it will return true.
    bool storerActive = true;
    
    if(!flashWorking)       // Can't do any flash stuff if there's a pending flash operation already
    {
        storer_mode_t modeLocal = storerMode;  // local copy, in case interrupt somehow changes it in the middle of this switch
        switch(modeLocal)
        {
        
            case STORER_IDLE:
                if(store.from != collect.to)
                {
                    if(micBuffer[store.from].check == micBuffer[store.from].timestamp || micBuffer[store.from].check == CHECK_TRUNC)
                    {
                        // Storable chunk in RAM.
                        storerMode = STORER_STORE;
                    }
                    else
                    {
                        store.from = (store.from < LAST_RAM_CHUNK) ? store.from+1 : 0;   // somehow an invalid RAM chunk - move on
                    }
                }
                else 
                {
                    if(BLEgetStatus() == BLE_INACTIVE)      // if storer is idle, we can advertise.
                    {
                        BLEresume();
                    }
                    storerActive = false;
                }
                break;
                
            case STORER_STORE:
                if(BLEgetStatus() != BLE_CONNECTED)          // no storage if we're in a connection
                {
                    if(BLEpause())      // ask for pause advertising
                    {
                        debug_log("STORER: writing RAM chunk %d to FLASH chunk %d\r\n",store.from,store.to);
                        //printCollectorChunk(store.from);
                        writeBlockToFlash(ADDRESS_OF_CHUNK(store.from),micBuffer[store.from].wordBuf,WORDS_PER_CHUNK);
                    }
                }
                break;
                
            case STORER_ADVANCE:
                ;  // can't have declaration directly after switch case label
                //printStorerChunk(store.to);
                unsigned char oldPage = PAGE_OF_CHUNK(store.to);          // get page of just finished chunk
                int newChunk = (store.to < LAST_CHUNK) ? store.to+1 : 0;     // advance to next chunk
                unsigned char newPage = PAGE_OF_CHUNK(newChunk);          // get page of next chunk
                if(oldPage != newPage)  // Did we advance to a new page?
                {
                    if(BLEgetStatus() != BLE_CONNECTED)
                    {
                        if(BLEpause())
                        {
                            store.to = newChunk;
                            store.from = (store.from < LAST_RAM_CHUNK) ? store.from+1 : 0;  // advance to next RAM chunk
                            erasePageOfFlash(newPage);      // Erase the page.  FLASH success system event will switch to idle mode
                        }
                    }
                }
                else
                {
                    store.to = newChunk;
                    store.from = (store.from < LAST_RAM_CHUNK) ? store.from+1 : 0;  // advance to next RAM chunk
                    storerMode = STORER_IDLE;         // If we don't need to erase, we can just go straight to idle mode
                }
                break;
                
            default:
                break;
        }   // switch(modeLocal)
    }    // if(!flashWorking)
    
    return storerActive;  // If flash is working, storer is still active.
}


void storer_on_sys_evt(uint32_t sys_evt)
{
    switch(sys_evt)
    {
        case NRF_EVT_FLASH_OPERATION_ERROR:
            debug_log("STORER: flash error?\r\n");
            flashWorking = false;
            break;
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
            flashWorking = false;
            storer_mode_t modeLocal = storerMode;
            switch(modeLocal)
            {
                case STORER_STORE:     // just completed a store
                    debug_log("STORER: stored RAM chunk %d to FLASH chunk %d\r\n",store.from,store.to);
                    micBuffer[store.from].check = CHECK_STORED;  // mark RAM chunk as stored (don't need to keep track of it in RAM)
                    storerMode = STORER_ADVANCE;    // Advance to the next chunk (may require erasing new page)
                    break;
                case STORER_ADVANCE:   // just completed erasing for advancing to new chunk
                    debug_log("STORER: erased page %d\r\n",(int)PAGE_OF_CHUNK(store.to));
                    storerMode = STORER_IDLE;       // Finished erasing new page.  Idle till new chunk ready.
                    break;
                case STORER_IDLE:
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}


void printStorerChunk(int chunk)
{
    if(chunk > LAST_CHUNK || chunk < 0)  // invalid chunk
    {
        debug_log("ERR: Invalid storer chunk to print\r\n");
        return;
    }
    
    mic_chunk_t* chunkPtr = (mic_chunk_t*)ADDRESS_OF_CHUNK(chunk);
    
    debug_log("FLASH chunk %d:\r\n",chunk);
    debug_log("ts: 0x%lX - ms: %hd - ba: %d -- ch: 0x%lX",(*chunkPtr).timestamp, (*chunkPtr).msTimestamp,
                                                          (int)((*chunkPtr).battery*1000), (*chunkPtr).check );
    nrf_delay_ms(3);
    for(int i = 0; i < SAMPLES_PER_CHUNK; i++)
    {
        if(i%10 == 0)
        {
            debug_log("\r\n  ");
        }
        unsigned char sample = (*chunkPtr).samples[i];
        if(sample != INVALID_SAMPLE)
        {
            debug_log("%3u, ",(int)sample);
        }
        else
        {
            debug_log("- , ");
        }
        nrf_delay_ms(2);
    }
    debug_log("\r\n---\r\n");
    nrf_delay_ms(10);
}




// If we try an illegal flash write, something's very wrong, so we should not continue.

void writeBlockToFlash(uint32_t* to, uint32_t* from, int numWords)  
{
    uint8_t page = PAGE_FROM_ADDRESS(to);
    if (page > LAST_PAGE || page < FIRST_PAGE)  
    {
        debug_log("Invalid block write address\r\n");
        while(1);
    }
    else  
    {
        flashWorking = true;
        sd_flash_write(to, from, numWords);
    }
}

void erasePageOfFlash(uint8_t page)  
{
    if (page > LAST_PAGE || page < FIRST_PAGE)  
    {
        debug_log("Invalid flash erase address\r\n");
        while(1);
    }
    else  
    {
        flashWorking = true;
        sd_flash_page_erase(page);
    }
}