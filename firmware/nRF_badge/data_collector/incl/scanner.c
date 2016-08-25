#include "scanner.h"






void printMac(unsigned char mac[6])
{
    debug_log("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",(int)mac[5],(int)mac[4],
                                              (int)mac[3],(int)mac[2],
                                              (int)mac[1],(int)mac[0]);
}



//================================ Scanning + scan result storage ===============================
//===============================================================================================

void scanner_init()
{
    /*
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
    
    */
    
    
    
    scan_params.active = 0;  //passive scanning, only looking for advertising packets
    scan_params.selective = 0;  //non-selective, don't use whitelist
    scan_params.p_whitelist = NULL;  //no whitelist
    scan_params.interval = (SCAN_INTERVAL * 1000) / 625;   //scan_params uses interval in units of 0.625ms
    scan_params.window = (SCAN_WINDOW * 1000) / 625;       //window also in units of 0.625ms
    scan_params.timeout = SCAN_TIMEOUT;                     //timeout is in s
    
    scanPeriod_ms = 1000UL * SCAN_PERIOD;
    
    //scan_state = SCAN_IDLE;
    
}


uint32_t startScan()
{
    sd_ble_gap_scan_stop();  // stop any in-progress scans
    
    scan.num = 0;
    scan.timestamp = now();
    
    return sd_ble_gap_scan_start(&scan_params);
}
    

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
                //debug_log("Badge seen: group %d, ID %hX, rssi %d.\r\n",(int)payload.group,payload.ID,(int)rssi);
                if(payload.group == badgeAssignment.group)
                {
                    bool prevSeen = false;
                    for(int i=0; i<scan.num; i++)        // check through list of already seen badges
                    {
                        if(payload.ID == scan.IDs[i])
                        {
                            scan.rssiSums[i] += rssi;
                            scan.counts[i]++;
                            prevSeen = true;
                            break;
                        }
                    }
                    if(!prevSeen)                               // is it a new badge
                    {
                        scan.IDs[scan.num] = payload.ID;
                        scan.rssiSums[scan.num] = rssi;
                        scan.counts[scan.num] = 1;
                        scan.num++;
                    }
                }
            }
            else
            {
                //debug_log("Badge seen, rssi %d, but wrong/missing adv data, len %d?\r\n",(int)rssi,payloadLen);
            }
        }
        else
        {
            //debug_log("Unknown device seen, name %.5s, rssi %d.\r\n",name,(int)rssi);
        }

        /*
        debug_log("found: ");
        printMac(addr);
        int ID = getIndexFromMac(addr,deviceList,NUM_DEVICES);
        debug_log(", ID %d\r\n",ID);
        */
    }
}

void BLEonScanTimeout()
{   
    scan_state = SCANNER_SAVE;

    /*#ifdef DEBUG_LOG_ENABLED
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
    
    
    enableStorage();   // enableStorage will check whether a BLE connection is active, to avoid conflicts*/
}


bool updateScanner()
{
    bool scannerActive = true;

    switch(scan_state)
    {
        case SCANNER_IDLE:   // nothing to do, but check whether it's time to scan again.
            if(scanner_enable)  // don't re-start scans if scanning is disabled
            {
                
                if(millis() - lastScanTime >= scanPeriod_ms)  // start scanning if it's time to scan
                {
                    // don't start scans if BLE is inactive (paused)
                    if(BLEgetStatus() != BLE_INACTIVE)
                    {
                        uint32_t result = startScan();
                        if(result == NRF_SUCCESS)
                        {
                            scan_state = SCANNER_SCANNING;
                            debug_log("SCANNER: Started scan.\r\n");
                        }
                        else
                        {
                            debug_log("ERR: error starting scan, #%u\r\n",(unsigned int)result);
                        }
                        lastScanTime = millis();
                    }
                }
            }
            else
            {
                scannerActive = false;
            }
            break;
        case SCANNER_SCANNING:    // nothing to do, timeout callback will stop scans when it's time
            //debug_log("scan scanning\r\n");
            // Scan timeout is interrupt-driven; we don't need to manually stop it here.
            break;
        case SCANNER_SAVE:     // we need to store scan results
            //debug_log("SCANNER: Scan storing not yet implemented.\r\n");
            //scan_state = SCANNER_IDLE;
            
            // ************* MUST VERIFY NUM = 0 CASE
            
            debug_log("SCANNER: Saving scan results. %d devices seen\r\n",scan.num);
            
            int numSaved = 0;
            int chunksUsed = 0;
            
            do
            {
                // Fill chunk header
                scanBuffer[scan.to].timestamp = scan.timestamp;
                scanBuffer[scan.to].num = scan.num;
                
                debug_log("  C:%d\r\n",scan.to);
            
                // Fill chunk with results
                int numLeft = scan.num - numSaved;
                int numThisChunk = (numLeft <= SCAN_DEVICES_PER_CHUNK) ? numLeft : SCAN_DEVICES_PER_CHUNK;
                for(int i = 0; i < numThisChunk; i++)
                {
                    scanBuffer[scan.to].devices[i].ID = scan.IDs[numSaved + i];
                    scanBuffer[scan.to].devices[i].rssi = scan.rssiSums[numSaved + i] / scan.counts[numSaved + i];
                    scanBuffer[scan.to].devices[i].count = scan.counts[numSaved + i];
                    debug_log("    bdg ID#%.4hX, rssi %d, count %d\r\n", scanBuffer[scan.to].devices[i].ID,
                                                                    (int)scanBuffer[scan.to].devices[i].rssi,
                                                                    (int)scanBuffer[scan.to].devices[i].count );
                }
                numSaved += numThisChunk;  
                
                // Terminate chunk
                if(chunksUsed == 0)     // first chunk of saved results
                {
                    if(numSaved >= scan.num)    // did all the results fit in the first chunk
                    {
                        scanBuffer[scan.to].check = scanBuffer[scan.to].timestamp;  // mark as complete
                    }
                    else
                    {
                        scanBuffer[scan.to].check = CHECK_TRUNC;
                    }
                }
                else    // else we're continuing results from a previous chunk
                {
                    scanBuffer[scan.to].check = CHECK_CONTINUE;
                }
                //debug_log("--num: %d\r\n",scanBuffer[scan.to].num);
                
                chunksUsed++;
                scan.to = (scan.to < LAST_SCAN_CHUNK) ? scan.to+1 : 0;
            } while(numSaved < scan.num);
            
            debug_log("SCANNER: Done saving results.  used %d chunks.\r\n",chunksUsed);
            
            scan_state = SCANNER_IDLE;
            
            break;
            
            /*
            
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
                scan_state = SCANNER_IDLE;
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
            */
        default:
            break;
    }  // switch(scan_state)
    
    return scannerActive;
    
}


void startScanner(unsigned short window_ms,unsigned short interval_ms,unsigned short duration_s,unsigned short period_s)
{
    scan_params.interval = (interval_ms * 1000UL) / 625;   //scan_params uses interval in units of 0.625ms
    scan_params.window = (window_ms * 1000UL) / 625;       //window also in units of 0.625ms
    scan_params.timeout = duration_s;                     //timeout is in s
    
    scanPeriod_ms = 1000UL * period_s;
    
    scanner_enable = true;
}

void stopScanner()
{
    scanner_enable = false;
}


scan_state_t getScanState()  {
    return scan_state;
}


unsigned long getScanTimestamp(int chunk)
{
    ext_flash_wait();
    scan_header_t header;
    ext_flash_read(EXT_ADDRESS_OF_CHUNK(chunk),header.buf,sizeof(header.buf));
    while(spi_busy());
    return header.timestamp;
}

unsigned long getScanCheck(int chunk)
{
    ext_flash_wait();
    scan_tail_t tail;
    ext_flash_read(EXT_ADDRESS_OF_CHUNK_CHECK(chunk),tail.buf,sizeof(tail.buf));
    while(spi_busy());
    return tail.check;
}

void getScanChunk(scan_chunk_t* destPtr,int chunk)
{
    ext_flash_wait();
    ext_flash_read(EXT_ADDRESS_OF_CHUNK(chunk),destPtr->buf,sizeof(destPtr->buf));
    while(spi_busy());
}


void printScanResult(scan_chunk_t* srcPtr)
{
    //scan_chunk_t readChunk;
    //ext_flash_read(EXT_ADDRESS_OF_CHUNK(chunk),readChunk.buf,sizeof(readChunk.buf));
    
    debug_log("Read: TS 0x%X, N %d, C 0x%X\r\n", (unsigned int)srcPtr->timestamp,
                                                            (int)srcPtr->num,
                                                            (unsigned int)srcPtr->check 
                                                            );
                                                            
    for(int i = 0; i < SCAN_DEVICES_PER_CHUNK; i++)
    {
        // Print all seen devices
        if(i < srcPtr->num)
        {
            debug_log("  #%X, R: %d, c: %d\r\n",(int)srcPtr->devices[i].ID,(signed int)srcPtr->devices[i].rssi
                                                    ,(int)srcPtr->devices[i].count);
        }
        // Prints the remaining contents of the chunk (not real data)
        //else  
        //{
        //    debug_log("  // #%d, %d\r\n",(int)readChunk.devices[i].ID,(int)readChunk.devices[i].rssi);
        //}
        nrf_delay_ms(1);
    }
}
