#include "scanner.h"



static unsigned short hexStringToShort(unsigned char * string)
{
    unsigned short result = 0;
    for (int place = 0; place < 4; place++)  {
        result *= 16;
        if (string[place] >= '0' && string[place] <= '9')  {
            result += string[place] - '0';
        }
        else if (string[place] >= 'A' && string[place] <= 'F')  {
            result += string[place] - 'A' + 0xA;
        }
        else if (string[place] >= 'a' && string[place] <= 'f')  {
            result += string[place] - 'a' + 0xa;
        }
        else return 0xffff;  // invalid string
    }
    return result;
}


//================================ Scanning + scan result storage ===============================
//===============================================================================================

void scanner_init()
{   
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
    //debug_log("BLECH\r\n");
    signed char rssi = advReport->rssi;
    
    if (rssi < MINIMUM_RSSI)  {
        return;  // ignore signals that are too weak.
    }
    
    unsigned char dataLength = advReport->dlen;
    unsigned char* data = (unsigned char*)advReport->data;
    unsigned char index = 0;
    
    unsigned char* namePtr = NULL;
    //unsigned char  nameLen = 0;
    unsigned char* manufDataPtr = NULL;
    unsigned char  manufDataLen = 0;
    
    unsigned char group = BAD_GROUP;
    unsigned short ID = BAD_ID;
    
    while ((namePtr == NULL || manufDataPtr == NULL) && index < dataLength)  {
        unsigned char fieldLen = data[index];
        index++;
        unsigned char fieldType = data[index];
        if (fieldType == BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME || fieldType == BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME)  {
            namePtr = &data[index + 1];  // skip field type byte
            //nameLen = fieldLen - 1;
        }
        else if (fieldType == BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA)  {
            manufDataPtr = &data[index + 1];  // skip field type byte
            manufDataLen = fieldLen - 1;
        }
        index += fieldLen;
    }
    
    if (manufDataLen == BADGE_MANUF_DATA_LEN)  {
        if (namePtr != NULL && memcmp(namePtr,(const uint8_t *)DEVICE_NAME,strlen(DEVICE_NAME)) == 0)  {
            custom_adv_data_t badgeAdvData;
            memcpy(&badgeAdvData,&manufDataPtr[2],CUSTOM_ADV_DATA_LEN);  // skip past company ID; ensure data is properly aligned
            ID = badgeAdvData.ID;
            group = badgeAdvData.group;
            debug_log("---Badge seen: group %d, ID %.4X, rssi %d.\r\n",(int)group,(int)ID,(int)rssi);
        }
    }
    else if (manufDataLen == IBEACON_MANUF_DATA_LEN)  {
        iBeacon_data_t iBeaconData;
        memcpy(&iBeaconData,manufDataPtr,IBEACON_MANUF_DATA_LEN);  // ensure data is properly aligned
        if (iBeaconData.companyID == COMPANY_ID_APPLE && iBeaconData.type == IBEACON_TYPE_PROXIMITY)  {
            // major/minor values are big-endian
            unsigned short major = ((unsigned short)iBeaconData.major[0] * 256) + iBeaconData.major[1];
            UNUSED_VARIABLE(major);
            unsigned short minor = ((unsigned short)iBeaconData.minor[0] * 256) + iBeaconData.minor[1];
            debug_log("---iBeacon seen: major %d, minor %d, rssi %d.\r\n",(int)major,minor,(int)rssi);
            group = iBeaconData.major[1];  // take only lower byte of major value
            ID = minor;
        }
    }
    
    if (ID != BAD_ID && group == badgeAssignment.group && scan.num < MAX_SCAN_RESULTS)  {
        bool prevSeen = false;
        for(int i=0; i<scan.num; i++)  {      // check through list of already seen badges
            if(ID == scan.IDs[i])  {
                scan.rssiSums[i] += rssi;
                scan.counts[i]++;
                prevSeen = true;
                break;
            }
        }
        if(!prevSeen)  {                             // is it a new badge
            scan.IDs[scan.num] = ID;
            scan.rssiSums[scan.num] = rssi;
            scan.counts[scan.num] = 1;
            scan.num++;
        }
    }
    
    /*
    
    // Parse the broadcast packet.  (find+check name, find custom data if present)
    while ((name == NULL || gotPayload == false) && index < dataLength)  {
        unsigned char fieldLen = data[index];
        index++;
        unsigned char fieldType = data[index];
        if (fieldType == BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME || fieldType == BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME)  {
            name = &data[index + 1];
            if (memcmp(name,(const uint8_t *)DEVICE_NAME,strlen(DEVICE_NAME)) == 0)  {
                // name checks out
            }
            else if (name != NULL && memcmp(name,(const uint8_t *)BEACON_NAME,strlen(BEACON_NAME)) == 0)  {
                payload.group = BEACON_GROUP;
                // our beacons are named "[BEACON_NAME]XXXX", where XXXX is a hex ID number.
                payload.ID = hexStringToShort(&name[strlen(BEACON_NAME)]);
                if (payload.ID != 0xffff)  {  // was there a valid ID in name?
                    gotPayload = true;
                }
            }
            else  {
                gotPayload = false;
                break;  // if name doesn't match, stop parsing broadcast packet
            }
        }
        else if(fieldType == BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA)  {
            int payloadLen = fieldLen - 3;  // length of field minus field type byte and manuf ID word
            if(payloadLen == CUSTOM_DATA_LEN)  {
                // Need to copy payload data so it is properly aligned.
                memcpy(&payload,&data[index+3],CUSTOM_DATA_LEN);  // skip past field type byte, manuf ID word
                gotPayload = true;
            }
        }
        index += fieldLen;
    }
    
    if(gotPayload)  {   // did we see a valid device, and retrieve relevant data?
        debug_log("---Badge seen: group %d, ID %hX, rssi %d.\r\n",(int)payload.group,payload.ID,(int)rssi);
        if(payload.group == badgeAssignment.group || payload.group == BEACON_GROUP)  {
            bool prevSeen = false;
            for(int i=0; i<scan.num; i++)  {      // check through list of already seen badges
                if(payload.ID == scan.IDs[i])  {
                    scan.rssiSums[i] += rssi;
                    scan.counts[i]++;
                    prevSeen = true;
                    break;
                }
            }
            if(!prevSeen)  {                             // is it a new badge
                scan.IDs[scan.num] = payload.ID;
                scan.rssiSums[scan.num] = rssi;
                scan.counts[scan.num] = 1;
                scan.num++;
            }
        }
    }
    */
    
    /*
    // step through data until we find both the name and custom data, or have reached the end of the data
    while ((gotPayload == false || name == NULL) && index < dataLength)  {
        unsigned char fieldLen = data[index];
        index++;
        unsigned char fieldType = data[index];
        if(fieldType == BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME
            || fieldType == BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME)
        {
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
    
    if(name != NULL && memcmp(name,(const uint8_t *)DEVICE_NAME,strlen(DEVICE_NAME)) == 0)  {  // is it a badge?
        if(gotPayload)  {   // is there custom data, and the correct amount?
            //debug_log("Badge seen: group %d, ID %hX, rssi %d.\r\n",(int)payload.group,payload.ID,(int)rssi);
            if(payload.group == badgeAssignment.group)  {
                bool prevSeen = false;
                for(int i=0; i<scan.num; i++)  {      // check through list of already seen badges
                    if(payload.ID == scan.IDs[i])  {
                        scan.rssiSums[i] += rssi;
                        scan.counts[i]++;
                        prevSeen = true;
                        break;
                    }
                }
                if(!prevSeen)  {                             // is it a new badge
                    scan.IDs[scan.num] = payload.ID;
                    scan.rssiSums[scan.num] = rssi;
                    scan.counts[scan.num] = 1;
                    scan.num++;
                }
            }
        }
        else  {
            //debug_log("Badge seen, rssi %d, but wrong/missing adv data, len %d?\r\n",(int)rssi,payloadLen);
        }
    }
    else if (name != NULL && memcmp(name,(const uint8_t *)BEACON_NAME,strlen(BEACON_NAME)) == 0)  {
        // beacons are named [BEACON_NAME]XXXX, where XXXX is a hex ID number.
        unsigned short beaconID = hexStringToShort(&name[strlen(BEACON_NAME)]);  
    }
    else  {
        //debug_log("Unknown device seen, name %.5s, rssi %d.\r\n",name,(int)rssi);
    }
    */
        
}

void BLEonScanTimeout()
{   
    scan_state = SCANNER_SAVE;
}

// Compare function for stdlib's QuickSort function that will sort a list of seenDevice_t by RSSI descendingly.
// More information can be found here: http://www.cplusplus.com/reference/cstdlib/qsort/
static int compareSeenDeviceByRSSI(const void * a, const void * b) {
    seenDevice_t * seenDeviceA = (seenDevice_t *) a;
    seenDevice_t * seenDeviceB = (seenDevice_t *) b;

    if (seenDeviceA->rssi > seenDeviceB->rssi) {
        return -1;
    } else if (seenDeviceA->rssi == seenDeviceB->rssi) {
        return 0;
    } else if (seenDeviceA->rssi < seenDeviceB->rssi) {
        return 1;
    }

    // We should never get here?
    APP_ERROR_CHECK_BOOL(false);

    return -1;
}

// This function is kind of hacky.
// It sorts our global scan variable in place descendingly according to RSSI.
// It's written this way so we can easily put it into the existing codebase with minimal changes.
static void sortScanByRSSIDescending(void) {
    seenDevice_t seenDevices[MAX_SCAN_RESULTS];

    // Convert scan into an array of structs for sorting.
    for (int i = 0; i < scan.num; i++) {
        seenDevices[i].ID = scan.IDs[i];
        seenDevices[i].rssi = (scan.rssiSums[i] / scan.counts[i]);
        seenDevices[i].count = scan.counts[i];
    }

    // Sort seen devices in place
    qsort(seenDevices, (size_t) scan.num, sizeof(seenDevice_t), compareSeenDeviceByRSSI);

    for (int i = 0; i < scan.num; i++) {
        scan.IDs[i] = seenDevices[i].ID;
        scan.rssiSums[i] = seenDevices[i].rssi * seenDevices[i].count;
        scan.counts[i] = seenDevices[i].count;
    }
}

bool updateScanner()
{
    bool scannerActive = true;

    switch(scan_state)
    {
    case SCANNER_IDLE:   // nothing to do, but check whether it's time to scan again.
        if(scanner_enable)  {  // don't re-start scans if scanning is disabled
            if(millis() - lastScanTime >= scanPeriod_ms)  {  // start scanning if it's time to scan
                // don't start scans if BLE is inactive (paused)
                if(BLEgetStatus() != BLE_INACTIVE)  {
                    uint32_t result = startScan();
                    if(result == NRF_SUCCESS)  {
                        scan_state = SCANNER_SCANNING;
                        debug_log("SCANNER: Started scan.\r\n");
                    }
                    else  {
                        debug_log("ERR: error starting scan, #%u\r\n",(unsigned int)result);
                    }
                    lastScanTime = millis();
                }
            }
        }
        else  {
            scannerActive = false;
        }
        break;
    case SCANNER_SCANNING:
        // Scan timeout callback will exit scanning state.
        break;
    case SCANNER_SAVE:
        debug_log("SCANNER: Saving scan results. %d devices seen\r\n",scan.num);
        
        int numSaved = 0;
        int chunksUsed = 0;

        if (scan.num > SCAN_DEVICES_PER_CHUNK) {
            // We have scanned more devices than we can store in a chunk, prune off the top SCAN_DEVICES_PER_CHUNK
            //   devices by RSSI values
            // This requirement is in place because even though we can currently handle storing > SCAN_DEVICES_PER_CHUNK,
            //   these chunks break the sender.
            sortScanByRSSIDescending();
            debug_log("SCANNER: Pruned %d devices with low RSSI\r\n", scan.num - SCAN_DEVICES_PER_CHUNK);
            scan.num = SCAN_DEVICES_PER_CHUNK;
        }
        
        do  {
            // Fill chunk header
            scanBuffer[scan.to].timestamp = scan.timestamp;
            scanBuffer[scan.to].num = scan.num;
            int scaledBatteryLevel = (int)(100.0*getBatteryVoltage()) - 100;
            scanBuffer[scan.to].batteryLevel =  (scaledBatteryLevel <= 255) ? scaledBatteryLevel : 255;  // clip scaled level
            
            debug_log("  C:%d\r\n",scan.to);
        
            // Fill chunk with results
            int numLeft = scan.num - numSaved;
            int numThisChunk = (numLeft <= SCAN_DEVICES_PER_CHUNK) ? numLeft : SCAN_DEVICES_PER_CHUNK;
            for(int i = 0; i < numThisChunk; i++)  {
                scanBuffer[scan.to].devices[i].ID = scan.IDs[numSaved + i];
                scanBuffer[scan.to].devices[i].rssi = scan.rssiSums[numSaved + i] / scan.counts[numSaved + i];
                scanBuffer[scan.to].devices[i].count = scan.counts[numSaved + i];
                debug_log("    bdg ID#%.4hX, rssi %d, count %d\r\n", scanBuffer[scan.to].devices[i].ID,
                                                                (int)scanBuffer[scan.to].devices[i].rssi,
                                                                (int)scanBuffer[scan.to].devices[i].count );
            }
            numSaved += numThisChunk;  
            
            // Terminate chunk
            if(chunksUsed == 0)  {   // first chunk of saved results
                if(numSaved >= scan.num)  {  // did all the results fit in the first chunk
                    scanBuffer[scan.to].check = scanBuffer[scan.to].timestamp;  // mark as complete
                }
                else  {
                    scanBuffer[scan.to].check = CHECK_TRUNC;
                }
            }
            else  {  // else we're continuing results from a previous chunk
                scanBuffer[scan.to].check = CHECK_CONTINUE;
            }
            //debug_log("--num: %d\r\n",scanBuffer[scan.to].num);
            
            chunksUsed++;
            scan.to = (scan.to < LAST_SCAN_CHUNK) ? scan.to+1 : 0;
        } while(numSaved < scan.num);
        
        debug_log("SCANNER: Done saving results.  used %d chunks.\r\n",chunksUsed);
        
        scan_state = SCANNER_IDLE;
        
        break;
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


scan_state_t getScanState()  
{
    return scan_state;
}


unsigned long getScanTimestamp(int chunk)
{
    ext_eeprom_wait();
    scan_header_t header;
    ext_eeprom_read(EXT_ADDRESS_OF_CHUNK(chunk),header.buf,sizeof(header.buf));
    while(spi_busy());
    return header.timestamp;
}

unsigned long getScanCheck(int chunk)
{
    ext_eeprom_wait();
    scan_tail_t tail;
    ext_eeprom_read(EXT_ADDRESS_OF_CHUNK_CHECK(chunk),tail.buf,sizeof(tail.buf));
    while(spi_busy());
    return tail.check;
}

void getScanChunk(scan_chunk_t* destPtr,int chunk)
{
    ext_eeprom_wait();
    ext_eeprom_read(EXT_ADDRESS_OF_CHUNK(chunk),destPtr->buf,sizeof(destPtr->buf));
    while(spi_busy());
}


void printScanResult(scan_chunk_t* srcPtr)
{
    //scan_chunk_t readChunk;
    //ext_eeprom_read(EXT_ADDRESS_OF_CHUNK(chunk),readChunk.buf,sizeof(readChunk.buf));
    
    debug_log("Read: TS 0x%X, N %d, C 0x%X\r\n", (unsigned int)srcPtr->timestamp,
                                                            (int)srcPtr->num,
                                                            (unsigned int)srcPtr->check 
                                                            );
                                                            
    for(int i = 0; i < SCAN_DEVICES_PER_CHUNK; i++)  {
        // Print all seen devices
        if(i < srcPtr->num)  {
            debug_log("  #%X, R: %d, c: %d\r\n",(int)srcPtr->devices[i].ID,(signed int)srcPtr->devices[i].rssi
                                                    ,(int)srcPtr->devices[i].count);
        }
        // Prints the remaining contents of the chunk (not real data)
        //else  {
        //    debug_log("  // #%d, %d\r\n",(int)readChunk.devices[i].ID,(int)readChunk.devices[i].rssi);
        //}
        nrf_delay_ms(1);
    }
}
