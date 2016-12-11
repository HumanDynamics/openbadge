/*
 * INFORMATION ****************************************************
 */



#include "storer.h"

#include <app_scheduler.h>


volatile bool flashWorking;
volatile storer_mode_t storerMode = STORER_IDLE;


// this is the last chunk before we enter program memory space
// const int LAST_CHUNK = ((int)(LAST_PAGE - FIRST_PAGE) << 3) + 7;  // = numberOfPages*8 + 7


void storer_init()
{
    
    storerMode = STORER_INIT;
    store.from = 0;
    store.to = 0;
    
    debug_log("-storer initialization-\r\n");
    
    unsigned long lastStoredTime = MODERN_TIME;
    
    for (int c = 0; c <= LAST_FLASH_CHUNK; c++)  {
        mic_chunk_t* chunkPtr = (mic_chunk_t*)ADDRESS_OF_CHUNK(c);

        unsigned long timestamp = chunkPtr->timestamp;
        unsigned long check = chunkPtr->check;
        
        //is the timestamp possibly valid?
        if (timestamp < FUTURE_TIME && timestamp > MODERN_TIME)  { 
            //is it a completely stored chunk?
            if (timestamp == check || check == CHECK_TRUNC)  { 
                //is it later than the latest stored one found so far?
                if (timestamp > lastStoredTime)  { 
                    store.to = c; //keep track of latest stored chunk
                    lastStoredTime = timestamp;
                }
            }
        }
        
    }
    
    if (lastStoredTime == MODERN_TIME)  {  // no valid chunk found
        debug_log("  No stored chunks found. Will start from chunk 0.\r\n");
        store.to = LAST_FLASH_CHUNK;    // will advance to chunk 0 below.
    }
    else  {
        debug_log("  Last stored chunk: %d at time: 0x%lX\r\n", store.to, lastStoredTime);
        //printStorerChunk(store.to);
    }
    
    
    debug_log("  Initializing FLASH for storage...\r\n");
    
    int newChunk = (store.to < LAST_FLASH_CHUNK) ? store.to+1 : 0;  // next chunk in FLASH (first unused chunk)
    
    unsigned char oldPage = PAGE_OF_CHUNK(store.to);
    unsigned char newPage = PAGE_OF_CHUNK(newChunk);
    
    // If new chunk is on a new page, we can just erase the whole page.
    if (oldPage != newPage)  {
        debug_log("    erase pg%d.\r\n",(int)newPage);
        nrf_delay_ms(20);
        erasePageOfFlash(newPage);
        while(flashWorking);
    }
    // If new chunk is on same page as old data, we need to erase while preserving the old data
    else  {
        uint32_t* oldChunkAddr = ADDRESS_OF_CHUNK(store.to);   // address of latest stored data
        uint32_t* pageAddr = ADDRESS_OF_PAGE(newPage); // address of beginning of current page
        
        int bytesToErase = (int)(oldChunkAddr) - (int)(pageAddr); // how many bytes of current page we need to erase
        int chunksToSave = (BYTES_PER_PAGE - bytesToErase)/CHUNK_SIZE;  // how many old chunks are in this page
        UNUSED_VARIABLE(chunksToSave);
        
        debug_log("    pg%d (%d chunks)-->RAM...", newPage, chunksToSave);
        uint32_t temp[WORDS_PER_PAGE];                            // temporary buffer for page
        memcpy(temp, pageAddr, BYTES_PER_PAGE);                   // copy old data to buffer
        debug_log("clear %dbytes...", bytesToErase);
        memset(temp, 0xff, bytesToErase);                         // clear unused portion of page in buffer
        
        debug_log("erase pg%d...",(int)newPage);
        nrf_delay_ms(20);
        erasePageOfFlash(newPage);                              // erase page
        while(flashWorking);
        
        debug_log("restore data...\r\n");
        nrf_delay_ms(10);
        writeBlockToFlash(pageAddr, temp, WORDS_PER_PAGE);      // replace data from buffer
        while(flashWorking);
    }
    
    store.to = newChunk;
    debug_log("  Ready to store to chunk %d.\r\n",newChunk); 
    
    store.extFrom = 0;
    
    // We need to find the most recent stored chunk, to start storing after it
    
    scan_header_t header;
    scan_tail_t tail;
    
    ext_eeprom_wait();

    store.extTo = EXT_FIRST_DATA_CHUNK;
    unsigned long lastStoredTimestamp = 0;
    for (int i = EXT_FIRST_DATA_CHUNK; i <= EXT_LAST_CHUNK; i++)  {
        unsigned int addr = EXT_ADDRESS_OF_CHUNK(i);
        ext_eeprom_read(addr,header.buf,sizeof(header.buf));               // Beginning of the chunk
        ext_eeprom_wait();
        ext_eeprom_read(addr+EXT_CHUNK_SIZE-4,tail.buf,sizeof(tail.buf));  // End of the chunk
        ext_eeprom_wait();
        
        // Print all written chunks
        /*if(header.timestamp != 0xFFFFFFFFUL)
        {
            debug_log("CH: %d,TS: %X\r\n",i,(int)header.timestamp);
            nrf_delay_ms(2);
        } */
        
        // is it a completely stored scan result chunk?
        if (header.timestamp > MODERN_TIME && header.timestamp < FUTURE_TIME)  {
            if (tail.check == header.timestamp || tail.check == CHECK_TRUNC || tail.check == CHECK_CONTINUE)  {
                // is it the most recent one found yet?
                if (header.timestamp >= lastStoredTimestamp)  {
                    lastStoredTimestamp = header.timestamp;
                    store.extTo = i;
                }
            }
        }
    }
    
    if (lastStoredTimestamp == 0)  {
        debug_log("  No stored scans found.\r\n");
    }
    else  {
        debug_log("  Last stored scan in chunk %d, timestamp %X.\r\n",(int)store.extTo,(int)lastStoredTimestamp);
    }
    
    store.extTo = (store.extTo < EXT_LAST_CHUNK) ? store.extTo+1 : EXT_FIRST_DATA_CHUNK;
    
    
    //store.extFrom = 0;
    //store.extTo = EXT_FIRST_DATA_CHUNK;
    ext_eeprom_global_unprotect();
    ext_eeprom_wait();
    
    
    
    storerMode = STORER_IDLE;
    
}

bool storer_test()
{    
    storerMode = STORER_INIT;
    debug_log("Erasing first page...\r\n");
    erasePageOfFlash(FIRST_PAGE);
    while (flashWorking);
    
    debug_log("Writing a value to flash...\r\n");
    uint32_t* addr = ADDRESS_OF_PAGE(FIRST_PAGE);
    uint32_t value = 0x42;
    writeBlockToFlash(addr, &value, 1);  // write one word to FLASH
    while (flashWorking);
    
    uint32_t readVal1 = *addr;
    uint32_t readVal2 = *(addr+1);
    
    if (readVal1 != value)  {
        debug_log("***Read wrong value?  Wrote 0x%X, read 0x%X.\r\n",(unsigned int)value,(unsigned int)readVal1);
        return false;
    }
    if (readVal2 != 0xffffffffUL)  {
        debug_log("***Not erased?  Read 0x%X from erased memory.\r\n",(unsigned int)readVal2);
        return false;
    }
    return true;
}

static void data_storage_handler(void * p_event_data, uint16_t event_size) {
    bool storageOperationsRemaining = updateStorer();
    if (storageOperationsRemaining) {
        app_sched_event_put(NULL, 0, data_storage_handler);
    }
}

void triggerStorer(void) {
    app_sched_event_put(NULL, 0, data_storage_handler);
}


bool updateStorer()
{
    // This will be the function return value.  if there is no storage actions to be completed, this will be set to false.
    //   if not, it will return true.
    bool storerActive = true;
    
    if (!flashWorking)  {  // Can't do storage work if flash operation pending
        storer_mode_t modeLocal = storerMode;  // local copy, in case interrupt somehow changes it in the middle of this switch
        switch(modeLocal){
        
        case STORER_IDLE:
            //debug_log("Storer is idle.\r\n");
            if (lastStoredAssignment.ID != badgeAssignment.ID || lastStoredAssignment.group != badgeAssignment.group)  {
                storerMode = STORER_STORE_ASSIGNMENT;
            }
            else if (store.from != collect.to)  {
                if(micBuffer[store.from].check == micBuffer[store.from].timestamp || micBuffer[store.from].check == CHECK_TRUNC)  {
                    // Storable chunk in collector RAM.
                    storerMode = STORER_STORE;
                }
                else  {
                    store.from = (store.from < LAST_RAM_CHUNK) ? store.from+1 : 0;   // somehow an invalid RAM chunk - move on
                }
            }
            else if (store.extFrom != scan.to)  {
                if (scanBuffer[store.extFrom].check == scanBuffer[store.extFrom].timestamp
                  || scanBuffer[store.extFrom].check == CHECK_TRUNC
                  || scanBuffer[store.extFrom].check == CHECK_CONTINUE)  
                {
                    // Storable chunk in scanner RAM
                    storerMode = STORER_STORE_EXT;
                }
                else  {
                    // else invalid chunk, move on
                    store.extFrom = (store.extFrom < LAST_SCAN_CHUNK) ? store.extFrom+1 : 0;
                }
            }
            else  {
                BLEresume(PAUSE_REQ_STORER);
                storerActive = false;
            }
            break;
            
        case STORER_STORE:
            debug_log("STORER: writing RAM chunk %d to FLASH chunk %d\r\n",store.from,store.to);
            //printCollectorChunk(store.from);
            writeBlockToFlash(ADDRESS_OF_CHUNK(store.to),micBuffer[store.from].wordBuf,WORDS_PER_CHUNK);
            break;
            
        case STORER_ADVANCE:
            ;  // can't have declaration directly after switch case label
            //printStorerChunk(store.to);
            unsigned char oldPage = PAGE_OF_CHUNK(store.to);          // get page of just finished chunk
            int newChunk = (store.to < LAST_FLASH_CHUNK) ? store.to+1 : 0;     // advance to next chunk
            unsigned char newPage = PAGE_OF_CHUNK(newChunk);          // get page of next chunk
            if (oldPage != newPage)  {
                if (BLEpause(PAUSE_REQ_STORER))  {
                    store.to = newChunk;
                    store.from = (store.from < LAST_RAM_CHUNK) ? store.from+1 : 0;  // advance to next RAM chunk
                    erasePageOfFlash(newPage);      // Erase the page.  FLASH success system event will switch to idle mode
                }
            }
            else  {
                store.to = newChunk;
                store.from = (store.from < LAST_RAM_CHUNK) ? store.from+1 : 0;  // advance to next RAM chunk
                storerMode = STORER_IDLE;         // If we don't need to erase, we can just go straight to idle mode
            }
            break;
            
        case STORER_STORE_EXT:
            debug_log("STORER: writing SCAN chunk %d to EXT chunk %d\r\n",store.extFrom,store.extTo);
            //printScanResult(&scanBuffer[store.extFrom]);
            ext_eeprom_write(EXT_ADDRESS_OF_CHUNK(store.extTo),scanBuffer[store.extFrom].buf,
                                                              sizeof(scanBuffer[store.extFrom].buf));
            storerMode = STORER_STORE_EXT_WAIT;
            break;
        
        case STORER_STORE_EXT_WAIT:
            if (ext_eeprom_get_status() == EXT_EEPROM_ALL_IDLE)  {
                scanBuffer[store.extFrom].check = CHECK_STORED;
                storerMode = STORER_ADVANCE_EXT;
            }
            break;
            
        case STORER_ADVANCE_EXT:
            ;
            store.extTo = (store.extTo < EXT_LAST_CHUNK) ? store.extTo+1 : EXT_FIRST_DATA_CHUNK;
            store.extFrom = (store.extFrom < LAST_SCAN_CHUNK) ? store.extFrom+1 : 0;  // advance to next RAM chunk
            storerMode = STORER_IDLE;
            break;
        
        case STORER_STORE_ASSIGNMENT:
            lastStoredAssignment.ID = badgeAssignment.ID;
            lastStoredAssignment.group = badgeAssignment.group;
            lastStoredAssignment.magicNumber = STORED_ASSIGNMENT_MAGIC_NUMBER;
            ext_eeprom_write(STORED_ASSIGNMENT_ADDRESS,lastStoredAssignment.buf,sizeof(lastStoredAssignment.buf));
            ext_eeprom_wait();
            debug_log("STORER: stored assignment ID:0x%hX group:%d.\r\n",   lastStoredAssignment.ID, 
                                                                        (int)lastStoredAssignment.group);
            storerMode = STORER_IDLE;
        default:
            break;
        }   // switch(modeLocal)
    }    // if(!flashWorking)
    
    return storerActive;  // If flash is working, storer is still active.
}


void storer_on_sys_evt(uint32_t sys_evt)
{
    switch(sys_evt)  {
    case NRF_EVT_FLASH_OPERATION_ERROR:
        debug_log("STORER: flash error?\r\n");
        flashWorking = false;
        break;
    case NRF_EVT_FLASH_OPERATION_SUCCESS:
        flashWorking = false;
        storer_mode_t modeLocal = storerMode;
        switch(modeLocal)  {
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
            //debug_log("  flash operation complete.\r\n");
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
    if (chunk > LAST_FLASH_CHUNK || chunk < 0)  {  // invalid chunk
        debug_log("ERR: Invalid storer chunk to print\r\n");
        return;
    }
    
    mic_chunk_t* chunkPtr = (mic_chunk_t*)ADDRESS_OF_CHUNK(chunk);
    
    debug_log("-FLASH chunk %d:\r\n",chunk);
    debug_log("ts: 0x%lX - ms: %hd - ba: %d -- ch: 0x%lX",(*chunkPtr).timestamp, (*chunkPtr).msTimestamp,
                                                          (int)((*chunkPtr).battery*1000), (*chunkPtr).check );
    nrf_delay_ms(3);
    for (int i = 0; i < SAMPLES_PER_CHUNK; i++)  {
        if (i%10 == 0)  debug_log("\r\n  ");
        unsigned char sample = (*chunkPtr).samples[i];
        if (sample != INVALID_SAMPLE)  {
            debug_log("%3u, ",(int)sample);
        }
        else  {
            debug_log("- , ");
        }
        nrf_delay_ms(2);
    }
    debug_log("\r\n---\r\n");
    nrf_delay_ms(10);
}


badge_assignment_t getStoredBadgeAssignment()
{
    badge_assignment_t assignment;
    ext_eeprom_wait();
    ext_eeprom_read(STORED_ASSIGNMENT_ADDRESS,lastStoredAssignment.buf,sizeof(lastStoredAssignment.buf));
    // If magic number is not present, then the data found is not actually a valid assignment.
    if (lastStoredAssignment.magicNumber != STORED_ASSIGNMENT_MAGIC_NUMBER)  {
        lastStoredAssignment.group = 0xff;  // invalid
        lastStoredAssignment.ID = 0xffff;   // invalid
    }
    assignment.ID = lastStoredAssignment.ID;
    assignment.group = lastStoredAssignment.group;
    return assignment;
}


// If we try an illegal flash write, something's very wrong, so we should not continue.

void writeBlockToFlash(uint32_t* to, uint32_t* from, int numWords)  
{
    uint8_t page = PAGE_FROM_ADDRESS(to);
    if (page > LAST_PAGE || page < FIRST_PAGE)  {
        debug_log("Invalid block write address\r\n");
        APP_ERROR_CHECK_BOOL(false);
    }
    else  {
        flashWorking = true;
        sd_flash_write(to, from, numWords);
    }
}

void erasePageOfFlash(uint8_t page)  
{
    if (page > LAST_PAGE || page < FIRST_PAGE)  {
        debug_log("Invalid flash erase address\r\n");
        APP_ERROR_CHECK_BOOL(false);
    }
    else  {
        flashWorking = true;
        sd_flash_page_erase(page);
    }
}