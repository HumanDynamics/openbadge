/*
 * Classes for handling storage to flash, and retreival of data from flash for sending
 * FlashHandler simplifies writing to flash.  The program may always use storeWord() to
 *   store data; update(), if called repeatedly, handles the actual storage and organization
 *   in flash, including avoiding conflict with BLE activity.
 * RetrieveHandler simplifies sending data over BLE
 */



#include "collector.h"



void collector_init()
{
    // Set sampling timing parameters to defaults
    sampleWindow = SAMPLE_WINDOW;
    samplePeriod = SAMPLE_PERIOD;

    collect.to = 0;
    collect.loc = 0;

    // initialize sampling things
    takingReadings = false;
    readingsCount = 0;
    readingsSum = 0;
    sampleStart = 0;
    sampleStartms = 0;
    
    unsigned long startTime = millis();
    int calCount = 0;
    unsigned long calTotal = 0;
    while(millis() - startTime <= 1000UL)
    {
        calTotal += analogRead(MIC_PIN);
        calCount++;
    }
    int calVal = calTotal/calCount;
    UNUSED_VARIABLE(calVal);
    debug_log("cal: %d\r\n",calVal);
    
    isCollecting = false;
}


/**
 * Take a reading from the mic (and add to total for averaging later)
 */
void takeMicReading() 
{
    if(takingReadings == false)     // if we aren't currently taking readings for a sample, start new sample
    {
        readingsCount = 0;          // reset number of readings
        readingsSum = 0;            // reset readings total
        sampleStart = now();        // get timestamp for first reading in sample (also recalculates fractional part)
        sampleStartms = nowFractional();    // get fractional part of timestamp
        takingReadings = true;      // we're starting a reading
    }
    int sample = analogRead(MIC_PIN);
    readingsSum += abs(sample - MIC_ZERO);
    readingsCount++;
}

static void setupChunk(int chunk, unsigned long timestamp, unsigned long msTimestamp)
{
    if(chunk > LAST_RAM_CHUNK || chunk < 0)  // invalid chunk
    {
        debug_log("ERR: Invalid collector chunk\r\n");
        return;
    }
        
    memset(micBuffer[chunk].samples, INVALID_SAMPLE, sizeof(micBuffer[chunk].samples));  // reset sample array
    micBuffer[chunk].battery = getBatteryVoltage();
    micBuffer[chunk].timestamp = timestamp;     // record timestamp for chunk
    micBuffer[chunk].msTimestamp = msTimestamp;  // record fractional part of timestamp
    micBuffer[chunk].check = CHECK_INCOMPLETE;  // denote that chunk is incomplete
}

void collectSample()  {
    unsigned int micValue = readingsSum / (readingsCount/2);
    unsigned char reading = micValue <= MAX_MIC_SAMPLE ? micValue : MAX_MIC_SAMPLE;  //clip sample
    //debug_log("r%d n%d\r\n",(int)reading,(int)readingsCount);
    if(readingsCount < 10)
    {
        debug_log("hold up that's not enough samples man wtf\r\n");
    }
    //debug_log("readingsCount: %d\r\n",(int)readingsCount);
    readingsCount = 0;
    readingsSum = 0;
    
    if(collect.loc == 0)  // are we at start of a new chunk
    {
        setupChunk(collect.to,sampleStart,sampleStartms);
        debug_log("COLLECTOR: Started RAM chunk %d.\r\n",collect.to);
        //printCollectorChunk(collect.to);
    }
    
    
    micBuffer[collect.to].samples[collect.loc] = reading;    // add reading
    collect.loc++;                     // move to next location in sample array
    
    if(collect.loc >= SAMPLES_PER_CHUNK)    // did we reach the end of the chunk
    {
        micBuffer[collect.to].check = micBuffer[collect.to].timestamp;  // mark chunk as complete
        collect.to = (collect.to < LAST_RAM_CHUNK) ? collect.to+1 : 0;
        collect.loc = 0;
    }
    
    takingReadings = false;  // we finished taking readings for that sample
}

void startCollector()
{   
    if(!isCollecting)
    {
        isCollecting = true;
        debug_log("Collector started\r\n");
    }
}

void stopCollector()
{
    if(isCollecting)
    {
        // Reset internal collector variables
        takingReadings = false;
        readingsSum = 0;
        readingsCount = 0;

        // Current chunk may be incomplete, but if collecting restarts, it should resume from a new chunk in RAM buffer.

        micBuffer[collect.to].check = CHECK_TRUNC;          // mark chunk as truncated; it's not full, but we're done writing in it
        micBuffer[collect.to].samples[SAMPLES_PER_CHUNK-1] = collect.loc;   // store the number of samples in this chunk
    
        debug_log("Collector stopped.  Truncated RAM chunk %d.\r\n",collect.to);
    
        // Advance to next chunk in buffer (will resume collecting from a new chunk)
        collect.to = (collect.to < LAST_RAM_CHUNK) ? collect.to+1 : 0;
        collect.loc = 0;
    
        isCollecting = false;   // Disable collector
    }
}


/*float getBatteryVoltage()
{
    if(isCollecting)  // if collector is active, just return the battery level of current chunk (no reason to do another reading)
    {
        return micBuffer[collect.to].battery;
    }
    else
    {
        return readBattery();
    }
}*/


void printCollectorChunk(int chunk)
{
    if(chunk > LAST_RAM_CHUNK || chunk < 0)  // invalid chunk
    {
        debug_log("ERR: Invalid collector chunk to print\r\n");
        return;
    }
    debug_log("RAM chunk %d:\r\n",chunk);
    debug_log("ts: 0x%lX - ms: %hd - ba: %d -- ch: 0x%lX",micBuffer[chunk].timestamp,micBuffer[chunk].msTimestamp,
                                                          (int)(micBuffer[chunk].battery*1000),micBuffer[chunk].check);
    nrf_delay_ms(3);
    for(int i = 0; i < SAMPLES_PER_CHUNK; i++)
    {
        if(i%10 == 0)
        {
            debug_log("\r\n  ");
        }
        unsigned char sample = micBuffer[chunk].samples[i];
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
