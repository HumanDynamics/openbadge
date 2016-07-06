#include "scanner.h"






void printMac(unsigned char mac[6])
{
    debug_log("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",(int)mac[5],(int)mac[4],
                                              (int)mac[3],(int)mac[2],
                                              (int)mac[1],(int)mac[0]);
}



//================================ Scanning + scan result storage ===============================
//===============================================================================================

/*void scans_init()
{
    // We need to find the most recent stored chunk, to start storing after it
    
    scan_header_t header;
    scan_tail_t tail;
    
    scanStore.chunk = EXT_FIRST_DATA_CHUNK;
    unsigned long lastStoredTimestamp = 0;
    for(int i = EXT_FIRST_DATA_CHUNK; i <= EXT_LAST_CHUNK; i++)
    {
        unsigned int addr = EXT_ADDRESS_OF_CHUNK(i);
        ext_flash_read(addr,header.buf,sizeof(header.buf));               // Beginning of the chunk
        ext_flash_read(addr+EXT_CHUNK_SIZE-4,tail.buf,sizeof(tail.buf));  // End of the chunk
        
        // Print all written chunks
        / *if(header.timestamp != 0xFFFFFFFFUL)
        {
            debug_log("CH: %d,TS: %X\r\n",i,(int)header.timestamp);
            nrf_delay_ms(2);
        } * /
        
        // is it a completely stored scan result chunk?
        if(header.timestamp != 0xFFFFFFFFUL && header.timestamp == tail.check)
        {
            // is it the first of a multi-chunk scan result?
            if(header.type == EXT_COMPLETE_CHUNK || header.type == EXT_INCOMPLETE_CHUNK)
            {
                if(header.timestamp >= lastStoredTimestamp)  // is it the most recent one found yet?
                {
                    lastStoredTimestamp = header.timestamp;
                }
            }
            scanStore.chunk = i;  // even if it's a continued chunk, we want to store after it.
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
     
    while(ext_flash_global_unprotect() != EXT_FLASH_SUCCESS);  //enable writing to external flash
    
    lastScanTime = 0;
    MINIMUM_RSSI = -120;  
    scanTiming.interval = SCAN_INTERVAL;
    scanTiming.window = SCAN_WINDOW;
    scanTiming.timeout = SCAN_TIMEOUT;
    scanTiming.period = SCAN_PERIOD;
    
    scan_enable = false;  //default to no scanning
    scan_state = SCAN_IDLE;
    
}*/


/*void startScan(int interval_ms, int window_ms, int timeout_s)
{
    sd_ble_gap_scan_stop();  // stop any in-progress scans
    
    memset((char*)scanResults,0,sizeof(scanResults));  //clear previous scan results
    scanStore.resultLoc = 0;
    scanStore.numTotal = 0;
    scanStore.numStored = 0;
    
    // Move on to next chunk, and erase a new flash block if necessary
    int oldBlock = EXT_BLOCK_OF_CHUNK(scanStore.chunk);
    scanStore.chunk = (scanStore.chunk < EXT_LAST_CHUNK) ? scanStore.chunk+1 : EXT_FIRST_DATA_CHUNK;
    int newBlock = EXT_BLOCK_OF_CHUNK(scanStore.chunk);
    
    debug_log("Scanning - to chunk %d, block %d",(int)scanStore.chunk,newBlock);
    if(newBlock != oldBlock)
    {
        ext_flash_block_erase(EXT_ADDRESS_OF_CHUNK(scanStore.chunk));
        debug_log(" (erased)");
    }
    debug_log("\r\n");
    
    
    // Define scanning parameters
    ble_gap_scan_params_t scan_params;
    
    scan_params.active = 0;  //passive scanning, only looking for advertising packets
    scan_params.selective = 0;  //non-selective, don't use whitelist
    scan_params.p_whitelist = NULL;  //no whitelist
    scan_params.interval = (interval_ms * 1000) / 625;   //scan_params uses interval in units of 0.625ms
    scan_params.window = (window_ms * 1000) / 625;       //window also in units of 0.625ms
    scan_params.timeout = timeout_s;                     //timeout is in s
    
    
    // Initiate scan
    disableStorage();  // internal flash and BLE operations conflict
    
    scanStore.timestamp = now();
    lastScanTime = millis();
    sd_ble_gap_scan_start(&scan_params);
    
    scan_state = SCAN_SCANNING;
    //debug_log("Scan started\r\n");
}*/
    

void BLEonAdvReport(ble_gap_evt_adv_report_t* advReport)
{
    signed char rssi = advReport->rssi;
    
    if(rssi >= MINIMUM_RSSI)  // ignore signals that are too weak
    {
        unsigned char dataLength = advReport->dlen;
        unsigned char* data = (unsigned char*)advReport->data;
        unsigned char index = 0;
        unsigned char* name = NULL;
        bool gotPayload = false;
        custom_adv_data_t payload;
        int payloadLen = 0;
        
        // step through data until we find both the name and custom data, or have reached the end of the data
        while((gotPayload == false || name == NULL) && index < dataLength)  
        {
            unsigned char fieldLen = data[index];
            index++;
            unsigned char fieldType = data[index];
            if(fieldType == BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME
                || fieldType == BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME)
            {
                /*if(memcmp(&data[index],(const uint8_t *)DEVICE_NAME,strlen(DEVICE_NAME)) == 0)
                {
                    isBadge = true;
                    //name = &data[index];
                    break;  // don't need any more adv data for now.
                }*/
                name = &data[index+1];
            }
            else if(fieldType == BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA)
            {
                payloadLen = fieldLen - 3;  // length of field minus field type byte and manuf ID word
                if(payloadLen == CUSTOM_DATA_LEN)
                {
                    // Need to copy payload data so it is properly aligned.
                    memcpy(&payload,&data[index+3],CUSTOM_DATA_LEN);  // skip past field type byte, manuf ID word
                    gotPayload = true;
                }
            }
            index += fieldLen;
        }
        
        if(name != NULL && memcmp(name,(const uint8_t *)DEVICE_NAME,strlen(DEVICE_NAME)) == 0)  // is it a badge?
        {
            if(gotPayload)     // is there custom data, and the correct amount?
            {
                debug_log("Badge seen: group %d, ID %hX, rssi %d.\r\n",(int)payload.group,payload.ID,(int)rssi);
            }
            else
            {
                debug_log("Badge seen, rssi %d, but wrong/missing adv data, len %d?\r\n",(int)rssi,payloadLen);
            }
        }
        else
        {
            debug_log("Unknown device seen, name %.5s, rssi %d.\r\n",name,(int)rssi);
        }

        /*
        debug_log("found: ");
        printMac(addr);
        int ID = getIndexFromMac(addr,deviceList,NUM_DEVICES);
        debug_log(", ID %d\r\n",ID);
        */
    }
}

/*void BLEonScanTimeout()
{
    #ifdef DEBUG_LOG_ENABLED
    debug_log("Scan results:\r\n");
    for(int i = 0; i < NUM_DEVICES; i++)
    {
        if(scanResults[i] != 0)
        {
            debug_log("  Saw device #%d, ",(int)deviceList[i].ID);
            printMac(deviceList[i].mac);
            debug_log(", RSSI %d\r\n",(int)scanResults[i]);
            nrf_delay_ms(1);
        }
    }
    debug_log("------\r\n");
    #endif //DEBUG_LOG_ENABLED
    
    scan_state = SCAN_STORING;
    
    enableStorage();   // enableStorage will check whether a BLE connection is active, to avoid conflicts
}*/


/*void updateScanning()
{
    switch(scan_state)
    {
        case SCAN_IDLE:   // nothing to do, but check whether it's time to scan again.
            //debug_log("scan idle\r\n");
            if(scan_enable)  // don't re-start scans if scanning is disabled
            {
                if(millis() - lastScanTime >= scanTiming.period)  // start scanning if it's time to scan
                {
                    startScan(scanTiming.interval,scanTiming.window,scanTiming.timeout);
                }
            }
            break;
        case SCAN_SCANNING:    // nothing to do, timeout callback will stop scans when it's time
            //debug_log("scan scanning\r\n");
            // Scan timeout is interrupt-driven; we don't need to manually stop it here.
            break;
        case SCAN_STORING:     // we need to store scan results
            debug_log("Scan storing not yet implemented.\r\n");
            
            break;
            
            
            //debug_log("scan storing\r\n");
            ;  //can't put a declaration directly after switch label
            ext_flash_status_t flashStatus = ext_flash_get_status();
            
            if(flashStatus != EXT_FLASH_ALL_IDLE)  {
                // Don't try anything if the external flash is busy with anything
                break;
            }
            else if(scanStore.numStored == scanStore.numTotal)
            {
                // If we've stored all the scan data to flash, we're done.
                scan_state = SCAN_IDLE;
                break;
            }
            
            
            // Otherwise we still need to store data
            
            scan_chunk_t chunk;  // Chunk buffer, will be written to external flash
            
            // Determine type of chunk to be stored
            if(scanStore.numStored == 0)  // we haven't stored any data yet
            {
                if(scanStore.numTotal <= EXT_DEVICES_PER_CHUNK)  // can we fit the results in one chunk
                {
                    chunk.type = EXT_COMPLETE_CHUNK;
                }
                else
                {
                    chunk.type = EXT_INCOMPLETE_CHUNK;  // we'll need multiple chunks to store these results
                }
                chunk.timestamp = scanStore.timestamp;
            }
            else  // we stored part of the scan already
            {
                chunk.type = EXT_CONT_CHUNK;
                chunk.timestamp = EXT_CONT_TIMESTAMP;
            }
            
            chunk.num = scanStore.numTotal;
            
            // Fill chunk buffer with scan data
            for(int i = 0; i < EXT_DEVICES_PER_CHUNK; i++)
            {
                // Find the next seen device in the scanResults table
                while(scanResults[scanStore.resultLoc] == 0)  {
                    scanStore.resultLoc++;
                    if(scanStore.resultLoc == NUM_DEVICES)  {
                        // We should never arrive here?
                        //   because we break as soon as we've stored numTotal devices
                        debug_log("ERR: not enough devices in scanResults?\r\n");
                        break;
                    }
                }
                
                // Store the respective ID and rssi of the seen device
                chunk.devices[i].ID = deviceList[scanStore.resultLoc].ID;
                chunk.devices[i].rssi = scanResults[scanStore.resultLoc];
                scanStore.numStored++;
                scanStore.resultLoc++;
                
                if(scanStore.numStored == scanStore.numTotal)  
                {
                    // Exit loop if we've stored all the devices
                    // Note: rest of chunk buffer might contain garbage.  But the stored num
                    //   tells us how far to look in the chunk(s) for valid scan data.
                    break;
                }
                
            }
            
            chunk.timestamp_check = chunk.timestamp;

            ext_flash_write(EXT_ADDRESS_OF_CHUNK(scanStore.chunk),chunk.buf,sizeof(chunk.buf));
            
            break;  // case: SCAN_STORING
        default:
            break;
    }  // switch(scan_state)
    
}*/


scan_state_t getScanState()  {
    return scan_state;
}


void printScanResult(int chunk)
{
    scan_chunk_t readChunk;
    ext_flash_read(EXT_ADDRESS_OF_CHUNK(chunk),readChunk.buf,sizeof(readChunk.buf));
    
    debug_log("Read: TS 0x%X, T 0x%X, N %d, TC 0x%X\r\n", (unsigned int)readChunk.timestamp,
                                                            (int)readChunk.type,
                                                            (int)readChunk.num,
                                                            (unsigned int)readChunk.timestamp_check 
                                                            );
                                                            
    for(int i = 0; i < EXT_DEVICES_PER_CHUNK; i++)
    {
        // Print all seen devices
        if(i < readChunk.num)
        {
            debug_log("  #%d, RSSI: %d\r\n",(int)readChunk.devices[i].ID,(signed int)readChunk.devices[i].rssi);
        }
        // Prints the remaining contents of the chunk (not real data)
        //else  
        //{
        //    debug_log("  // #%d, %d\r\n",(int)readChunk.devices[i].ID,(int)readChunk.devices[i].rssi);
        //}
        nrf_delay_ms(1);
    }
}
