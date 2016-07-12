/*
 * INFORMATION ****************************************************
 */



#include "storer.h"


volatile bool flashWorking;
volatile storer_mode_t storerMode = STORER_IDLE;


// this is the last chunk before we enter program memory space
// const int LAST_CHUNK = ((int)(LAST_PAGE - FIRST_PAGE) << 3) + 7;  // = numberOfPages*8 + 7


void storer_init()
{
    
    storerMode = STORER_INIT;
    store.from = 0;
    store.to = 0;
    
    debug_log("\r\n[STORER INITIALIZATION]\r\n");
    
    unsigned long lastStoredTime = MODERN_TIME;
    
    for(int c = 0; c <= LAST_FLASH_CHUNK; c++)
    {
        mic_chunk_t* chunkPtr = (mic_chunk_t*)ADDRESS_OF_CHUNK(c);

        unsigned long timestamp = (*chunkPtr).timestamp;
        unsigned long chunkCheck = (*chunkPtr).check;
        
        /*if(timestamp != 0xffffffffUL)
        {
            debug_log("c: %d ts: 0x%lX ch: 0x%lX\r\n", c, timestamp, chunkCheck);
            nrf_delay_ms(20);
        }*/
        
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
        
    }
    
    if(lastStoredTime == MODERN_TIME)   // no valid chunk found
    {
        debug_log("No stored chunks found. Will start from chunk 0.\r\n");
        store.to = LAST_FLASH_CHUNK;    // will advance to chunk 0 below.
    }
    else
    {
        debug_log("Last stored chunk: %d at time: 0x%lX\r\n", store.to, lastStoredTime);
        printStorerChunk(store.to);
    }
    
    
    debug_log("Initializing FLASH for storage...\r\n");
    
    int newChunk = (store.to < LAST_FLASH_CHUNK) ? store.to+1 : 0;  // next chunk in FLASH (first unused chunk)
    
    unsigned char oldPage = PAGE_OF_CHUNK(store.to);
    unsigned char newPage = PAGE_OF_CHUNK(newChunk);
    
    if(oldPage != newPage)      // If new chunk is on a new page, we can just erase the whole page.
    {
        debug_log("Erasing new page: %d.\r\n",(int)newPage);
        nrf_delay_ms(20);
        erasePageOfFlash(newPage);
        while(flashWorking);
    }
    else                        // If new chunk is on same page as old data, we need to erase while preserving the old data
    {
        uint32_t* oldChunkAddr = ADDRESS_OF_CHUNK(store.to);   // address of latest stored data
        uint32_t* pageAddr = ADDRESS_OF_PAGE(newPage); // address of beginning of current page
        
        int bytesToErase = (int)(oldChunkAddr) - (int)(pageAddr); // how many bytes of current page we need to erase
        int chunksToSave = (BYTES_PER_PAGE - bytesToErase)/CHUNK_SIZE;  // how many old chunks are in this page
        UNUSED_VARIABLE(chunksToSave);
        
        debug_log("Copying old data to RAM. (%d chunks)\r\n", chunksToSave);
        uint32_t temp[WORDS_PER_PAGE];                            // temporary buffer for page
        memcpy(temp, pageAddr, BYTES_PER_PAGE);                   // copy old data to buffer
        debug_log("Clearing unused data in buffer.  (%d bytes)\r\n", bytesToErase);
        memset(temp, 0xff, bytesToErase);                         // clear unused portion of page in buffer
        
        debug_log("Erasing page %d...\r\n",(int)newPage);
        nrf_delay_ms(20);
        erasePageOfFlash(newPage);                              // erase page
        while(flashWorking);
        
        debug_log("Restoring old data to FLASH...\r\n");
        nrf_delay_ms(10);
        writeBlockToFlash(pageAddr, temp, WORDS_PER_PAGE);      // replace data from buffer
        while(flashWorking);
    }
    
    store.to = newChunk;
    debug_log("Ready to store to chunk %d.\r\n",newChunk); 
    debug_log("[/end storer initialization]\r\n");
    
    
    
    
    /*store.extFrom = 0;
    
    // We need to find the most recent stored chunk, to start storing after it
    
    scan_header_t header;
    scan_tail_t tail;
    
    scanStore.chunk = EXT_FIRST_DATA_CHUNK;
    unsigned long lastStoredTimestamp = 0;
    for(int i = EXT_FIRST_DATA_CHUNK; i <= EXT_LAST_CHUNK; i++)
    {
        unsigned int addr = EXT_ADDRESS_OF_CHUNK(i);
        ext_flash_read(addr,header.buf,sizeof(header.buf));               // Beginning of the chunk
        ext_flash_wait();
        ext_flash_read(addr+EXT_CHUNK_SIZE-4,tail.buf,sizeof(tail.buf));  // End of the chunk
        ext_flash_wait();
        
        // Print all written chunks
        / *if(header.timestamp != 0xFFFFFFFFUL)
        {
            debug_log("CH: %d,TS: %X\r\n",i,(int)header.timestamp);
            nrf_delay_ms(2);
        } * /
        
        // is it a completely stored scan result chunk?
        if(header.timestamp < FUTURE_TIME && header.timestamp > MODERN_TIME)
        {
            if(header.timestamp == tail.check || tail.check == CHECK_TRUNC || tail
            // is it the first of a multi-chunk scan result?
            if(header.type == EXT_COMPLETE_CHUNK || header.type == EXT_INCOMPLETE_CHUNK)
            {
                if(header.timestamp >= lastStoredTimestamp)  // is it the most recent one found yet?
                {
                    lastStoredTimestamp = header.timestamp;
                }
            }
            store.extTo = i;  // even if it's a continued chunk, we want to store after it.
        }
    }
    
    if(lastStoredTimestamp == 0)
    {
        debug_log("No stored scans found.\r\n");
    }
    else  {
        debug_log("Last stored scan in chunk %d, timestamp %X.\r\n",(int)scanStore.chunk,(int)lastStoredTimestamp);
    }
    
    / **
     * scanStore.chunk is now the most recent completely stored scan chunk in external flash
     *   startScan will begin storing to the next chunk
     * /
    */
    
    store.extFrom = 0;
    store.extTo = EXT_FIRST_DATA_CHUNK;
    ext_flash_global_unprotect();
    ext_flash_block_erase(EXT_ADDRESS_OF_CHUNK(EXT_FIRST_DATA_CHUNK));
    ext_flash_wait();
    
    
    
    storerMode = STORER_IDLE;
    
    // Chunk store.to is now ready to have data stored to it.
    
}

bool storer_test()
{    
    storerMode = STORER_INIT;
    debug_log("Erasing first page...\r\n");
    erasePageOfFlash(FIRST_PAGE);
    while(flashWorking);
    
    debug_log("Writing a value to flash...\r\n");
    uint32_t* addr = ADDRESS_OF_PAGE(FIRST_PAGE);
    uint32_t value = 0x42;
    writeBlockToFlash(addr, &value, 1);  // write one word to FLASH
    while(flashWorking);
    
    uint32_t readVal1 = *addr;
    uint32_t readVal2 = *(addr+1);
    
    if(readVal1 != value)
    {
        debug_log("***Read wrong value?  Wrote 0x%X, read 0x%X.\r\n",(unsigned int)value,(unsigned int)readVal1);
        return false;
    }
    if(readVal2 != 0xffffffffUL)
    {
        debug_log("***Not erased?  Read 0x%X from erased memory.\r\n",(unsigned int)readVal2);
        return false;
    }
    return true;
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
                        // Storable chunk in collector RAM.
                        storerMode = STORER_STORE;
                    }
                    else
                    {
                        store.from = (store.from < LAST_RAM_CHUNK) ? store.from+1 : 0;   // somehow an invalid RAM chunk - move on
                    }
                }
                else if(store.extFrom != scan.to)
                {
                    if(scanBuffer[store.extFrom].check == scanBuffer[store.extFrom].timestamp
                        || scanBuffer[store.extFrom].check == CHECK_TRUNC
                        || scanBuffer[store.extFrom].check == CHECK_CONTINUE)     
                    {
                        // Storable chunk in scanner RAM
                        storerMode = STORER_STORE_EXT;
                    }
                    else // else invalid chunk, move on
                    {
                        store.extFrom = (store.extFrom < LAST_SCAN_CHUNK) ? store.extFrom+1 : 0;
                    }
                }
                else 
                {
                    if(BLEgetStatus() == BLE_INACTIVE)      // if storer is idle, we can advertise.
                    {
                        BLEresume(PAUSE_REQ_STORER);
                    }
                    storerActive = false;
                }
                break;
                
            case STORER_STORE:
                if(BLEgetStatus() != BLE_CONNECTED)          // no storage if we're in a connection
                {
                    if(BLEpause(PAUSE_REQ_STORER))      // ask for pause advertising
                    {
                        debug_log("STORER: writing RAM chunk %d to FLASH chunk %d\r\n",store.from,store.to);
                        //printCollectorChunk(store.from);
                        writeBlockToFlash(ADDRESS_OF_CHUNK(store.to),micBuffer[store.from].wordBuf,WORDS_PER_CHUNK);
                    }
                }
                break;
                
            case STORER_ADVANCE:
                ;  // can't have declaration directly after switch case label
                //printStorerChunk(store.to);
                unsigned char oldPage = PAGE_OF_CHUNK(store.to);          // get page of just finished chunk
                int newChunk = (store.to < LAST_FLASH_CHUNK) ? store.to+1 : 0;     // advance to next chunk
                unsigned char newPage = PAGE_OF_CHUNK(newChunk);          // get page of next chunk
                if(oldPage != newPage)  // Did we advance to a new page?
                {
                    if(BLEgetStatus() != BLE_CONNECTED)
                    {
                        if(BLEpause(PAUSE_REQ_STORER))
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
                
            case STORER_STORE_EXT:
                if(BLEgetStatus() != BLE_CONNECTED)          // no storage if we're in a connection
                {
                    if(BLEpause(PAUSE_REQ_STORER))      // ask for pause advertising
                    {
                        debug_log("STORER: writing SCAN chunk %d to EXT chunk %d\r\n",store.extFrom,store.extTo);
                        //printCollectorChunk(store.from);
                        ext_flash_write(EXT_ADDRESS_OF_CHUNK(store.extTo),scanBuffer[store.extFrom].buf,
                                                                          sizeof(scanBuffer[store.extFrom].buf));
                        storerMode = STORER_STORE_EXT_WAIT;
                    }
                }
                break;
            
            case STORER_STORE_EXT_WAIT:
                if(ext_flash_get_status() == EXT_FLASH_ALL_IDLE)
                {
                    storerMode = STORER_ADVANCE_EXT;
                }
                break;
                
            case STORER_ADVANCE_EXT:
                ;
                unsigned char oldBlock = EXT_BLOCK_OF_CHUNK(store.extTo);          // get page of just finished chunk
                int newExtChunk = (store.extTo < EXT_LAST_CHUNK) ? store.extTo+1 : EXT_FIRST_DATA_CHUNK;     // advance to next chunk
                unsigned char newBlock = EXT_BLOCK_OF_CHUNK(newExtChunk);          // get page of next chunk
                if(oldBlock != newBlock)  // Did we advance to a new page?
                {
                    if(BLEgetStatus() != BLE_CONNECTED)
                    {
                        if(BLEpause(PAUSE_REQ_STORER))
                        {
                            store.extTo = newExtChunk;
                            store.extFrom = (store.extFrom < LAST_SCAN_CHUNK) ? store.extFrom+1 : 0;  // advance to next RAM chunk
                            debug_log("STORER: erasing ext block %d.\r\n",newBlock);
                            ext_flash_block_erase(EXT_ADDRESS_OF_CHUNK(newExtChunk));
                            storerMode = STORER_ADVANCE_EXT_WAIT;
                        }
                    }
                }
                else
                {
                    store.extTo = newExtChunk;
                    store.extFrom = (store.extFrom < LAST_SCAN_CHUNK) ? store.extFrom+1 : 0;  // advance to next RAM chunk
                    storerMode = STORER_IDLE;         // If we don't need to erase, we can just go straight to idle mode
                }
            
                break;
            
            case STORER_ADVANCE_EXT_WAIT:
                if(ext_flash_get_status() == EXT_FLASH_ALL_IDLE)
                {
                    storerMode = STORER_IDLE;
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
                case STORER_INIT:
                    debug_log("  flash operation complete.\r\n");
                    break;
                case STORER_IDLE:
                default:
                    debug_log("ERR: unexpected flash operation success?\r\n");
                    break;
            }
            break;
        default:
            break;
    }
}


void printStorerChunk(int chunk)
{
    if(chunk > LAST_FLASH_CHUNK || chunk < 0)  // invalid chunk
    {
        debug_log("ERR: Invalid storer chunk to print\r\n");
        return;
    }
    
    mic_chunk_t* chunkPtr = (mic_chunk_t*)ADDRESS_OF_CHUNK(chunk);
    
    debug_log("-FLASH chunk %d:\r\n",chunk);
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