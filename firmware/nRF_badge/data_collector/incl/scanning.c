#include "scanning.h"


//===========================  Device List Manipulation ====================================
//==========================================================================================

//Note that a MAC address written as 6f:5f:4f:3f:2f:1f is stored in memory as {0x1f,0x2f,0x3f,0x4f,0x5f,0x6f}
const device_t masterDeviceList[NUM_DEVICES] = 
{
    { .mac={0x66,0x55,0x44,0x33,0x22,0x11}, .ID=0},
    { .mac={0x67,0x55,0x44,0x33,0x22,0x11}, .ID=1},
    { .mac={0x66,0x56,0x44,0x33,0x22,0x11}, .ID=2},
    { .mac={0x66,0x55,0x45,0x33,0x22,0x11}, .ID=3},
    { .mac={0x66,0x55,0x44,0x34,0x22,0x11}, .ID=4},
    { .mac={0x66,0x55,0x44,0x33,0x23,0x11}, .ID=5},
    { .mac={0x66,0x55,0x44,0x33,0x22,0x12}, .ID=6},
    { .mac={0x68,0x55,0x44,0x33,0x22,0x11}, .ID=7},
    { .mac={0x66,0x57,0x44,0x33,0x22,0x11}, .ID=8},
    { .mac={0x66,0x55,0x46,0x33,0x22,0x11}, .ID=9},
    { .mac={0x1D,0xAB,0x9E,0xCB,0xA3,0xD2}, .ID=42},  //square blue badge with headers on back
    { .mac={0x10,0xD6,0x32,0xA5,0x4D,0x59}, .ID=43},  //random MAC picked up on scan
    { .mac={0xCA,0xD3,0x5E,0xCa,0x61,0xEE}, .ID=44}   //another random MAC
};



void printMac(unsigned char mac[6])
{
    debug_log("%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",(int)mac[5],(int)mac[4],
                                              (int)mac[3],(int)mac[2],
                                              (int)mac[1],(int)mac[0]);
}

void printDeviceList(device_t devices[],int numDevices)
{
    debug_log("\r\n%d known devices:\r\n",numDevices);
    for(int i = 0; i < numDevices; i++)
    {
        debug_log("  ");
        printMac(devices[i].mac);
        debug_log(", ID %d\r\n",(int)devices[i].ID);
        nrf_delay_ms(1);  //let UART buffer finish transmitting
    }
    debug_log("------\r\n\r\n");
    nrf_delay_ms(10);  //let UART buffer finish transmitting
}

uint64_t macLL(unsigned char mac[6])
{
    uint64_t wholeMac = 0;
    memcpy(&wholeMac,mac,6);
    return wholeMac;
}

void printMacLL(uint64_t mac64)  {
    debug_log("mac: %lX%lX\r\n",(long)(mac64>>32),(long)(mac64&0xffffffffUL));
    nrf_delay_ms(1);  //finish printing
}


void quicksortDevices(device_t devices[], int leftIndex, int rightIndex)
{
    if(leftIndex < rightIndex)
    {
        // -----Partition: divide list into 2 halves, whether items are greater or less than the pivot value
        
        int pivotIndex = (leftIndex+rightIndex)/2;  //middle of list
        int newPivotIndex = leftIndex;  //will eventually be the pivot location in the partitioned list
        // Swap pivot and right devices
        device_t pivotDevice = devices[pivotIndex];
        uint64_t pivotMac = macLL(pivotDevice.mac);
        devices[pivotIndex] = devices[rightIndex];
        // Check each other device against pivot device
        for(int i=leftIndex; i<rightIndex; i++)
        {
            device_t temp = devices[i];
            uint64_t testMac = macLL(temp.mac);
            if(testMac <= pivotMac)
            {
                // Device belongs to left of pivot.  Swap with newPivotIndex device, move newPivotIndex right
                devices[i] = devices[newPivotIndex];
                devices[newPivotIndex] = temp;
                newPivotIndex++;
            }
        }
        // Replace pivot value in its new location
        devices[rightIndex] = devices[newPivotIndex];
        devices[newPivotIndex] = pivotDevice;
        
        // -----Sort: sort each half of the partitioned list
        quicksortDevices(devices,leftIndex,newPivotIndex-1);
        quicksortDevices(devices,newPivotIndex+1,rightIndex);
    }
    /**
     * NOTE:
     * List is now sorted, but perhaps not in an intuitive way.  The list is sorted by ascending
     *   values of the macLL() result for each device.  Due to the endian-ness of the CPU,
     *   this is not necessarily the way we might intuitively arrange the MAC addresses themselves
     *   in "ascending" order.
     * However, this is beneficial for searching the list for a device; we can do a simple binary
     *   search, if we always compare devices by their macLL() results.
     */
}

void sortDeviceList(device_t devices[], int numDevices)
{
    quicksortDevices(deviceList,0,numDevices-1);  // sortDeviceList is a bit more readable than this quicksort call
}



int findDeviceInList(unsigned char mac[6], device_t devices[], int numDevices)
{
    uint64_t searchMac = macLL(mac);  // what we'll be comparing to
    int left = 0;   // left bound index of possible devices
    int right = numDevices - 1;  // right bound index
    int searchIndex;
    while(left <= right)
    {
        searchIndex = (left+right)/2;  //Check middle device
        uint64_t testMac = macLL(devices[searchIndex].mac);
        if(searchMac == testMac)  {
            return searchIndex;  // found it, return location of device in sorted deviceList
        }
        else if(searchMac < testMac)  {
            right = searchIndex-1;  // less, refine search to lower half of remaining candidates
        }
        else if(searchMac > testMac)  {
            left = searchIndex+1;   // more, refine search to upper half of remaining candidates
        }
    }
    return UNKNOWN_DEVICE;  //no device found
}

int getIndexFromMac(unsigned char mac[6], device_t devices[], int numDevices)
{
    int location = findDeviceInList(mac,devices,numDevices);   // get sorted list index of device
    return (location != UNKNOWN_DEVICE) ? devices[location].ID : UNKNOWN_DEVICE;  // return corresponding ID
}




//================================ Scanning + scan result storage ===============================
//===============================================================================================

void scans_init()
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
        /*if(header.timestamp != 0xFFFFFFFFUL)
        {
            debug_log("CH: %d,TS: %X\r\n",i,(int)header.timestamp);
            nrf_delay_ms(2);
        }*/
        
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
    
    /**
     * scanStore.chunk is now the most recent completely stored scan chunk in external flash
     *   startScan will begin storing to the next chunk
     */
     
    while(ext_flash_global_unprotect() != EXT_FLASH_SUCCESS);  //enable writing to external flash
    
    lastScanTime = 0;
    MINIMUM_RSSI = -120;  
    scanTiming.interval = SCAN_INTERVAL;
    scanTiming.window = SCAN_WINDOW;
    scanTiming.timeout = SCAN_TIMEOUT;
    scanTiming.period = SCAN_PERIOD;
}


void startScan(int interval_ms, int window_ms, int timeout_s)
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
}
    

void BLEonAdvReport(uint8_t addr[6], int8_t rssi)
{
    if(rssi >= MINIMUM_RSSI)  // ignore signals that are too weak
    {
        int index = findDeviceInList(addr,deviceList,NUM_DEVICES);  // Get device's index within sorted list
        if(index != UNKNOWN_DEVICE)  // is it a known device
        {
            if(scanResults[index] == 0)  // have we not seen this device yet
            {
                scanStore.numTotal++;   // if it's a new device, increment the total number of seen devices
            }
            scanResults[index] = rssi;
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
}


void updateScanning()
{
    switch(scan_state)
    {
        case SCAN_IDLE:   // nothing to do, but check whether it's time to scan again.
            //debug_log("scan idle\r\n");
            if(millis() - lastScanTime >= scanTiming.period)  // start scanning if it's time to scan
            {
                startScan(scanTiming.interval,scanTiming.window,scanTiming.timeout);
            }
            break;
        case SCAN_SCANNING:    // nothing to do, timeout callback will stop scans when it's time
            //debug_log("scan scanning\r\n");
            // Scan timeout is interrupt-driven; we don't need to manually stop it here.
            break;
        case SCAN_STORING:     // we need to store scan results
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
    
}


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
