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

    collect.chunk = 0;
    collect.loc = 0;

    // initialize sampling things
    takingReadings = false;
    readingsCount = 0;
    readingsSum = 0;
    sampleStart = 0;
    sampleStartms = 0;
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
    if(chunk >= MIC_BUFFER_SIZE || chunk < 0)  // invalid chunk
    {
        debug_log("ERR: Invalid collector chunk\r\n");
        return;
    }
        
    memset(micBuffer[chunk].samples, INVALID_SAMPLE, sizeof(micBuffer[chunk].samples));  // reset sample array
    micBuffer[chunk].battery = readBattery();
    micBuffer[chunk].timestamp = timestamp;     // record timestamp for chunk
    micBuffer[chunk].msTimestamp = msTimestamp;  // record fractional part of timestamp
    micBuffer[chunk].check = CHECK_INCOMPLETE;  // denote that chunk is incomplete
}

void collectSample()  {
    unsigned int micValue = readingsSum / readingsCount;
    unsigned char reading = micValue <= MAX_MIC_SAMPLE ? micValue : MAX_MIC_SAMPLE;  //clip sample
    readingsCount = 0;
    readingsSum = 0;
    
    if(collect.loc == 0)  // are we at start of a new chunk
    {
        setupChunk(collect.chunk,sampleStart,sampleStartms);
    }
    
    micBuffer[collect.chunk].samples[collect.loc] = reading;    // add reading
    collect.loc++;                     // move to next location in sample array
    
    if(collect.loc >= SAMPLES_PER_CHUNK)    // did we reach the end of the chunk
    {
        micBuffer[collect.chunk].check = micBuffer[collect.chunk].timestamp;  // mark chunk as complete
        //printCollectorChunk(collect.chunk);
        collect.chunk++;
        if(collect.chunk >= MIC_BUFFER_SIZE)  //did we reach end of buffer
        {
            collect.chunk = 0;
        }
        collect.loc = 0;
    }
    
    takingReadings = false;  // we finished taking readings for that sample
}


void stopCollector()
{
    takingReadings = false;
    readingsSum = 0;
    readingsCount = 0;
    // Current chunk may be incomplete, but if collecting restarts, it should resume from a new chunk in RAM buffer.
    micBuffer[collect.chunk].check = CHECK_TRUNC;  // mark chunk as truncated; it's not full, but we're done writing in it
    collect.loc = 0;    // reset to start of sample array
    collect.chunk = (collect.chunk < MIC_BUFFER_SIZE) ? collect.chunk+1 : 0;  // advance to next chunk in buffer
}


void printCollectorChunk(int chunk)
{
    if(chunk >= MIC_BUFFER_SIZE || chunk < 0)  // invalid chunk
    {
        debug_log("ERR: Invalid collector chunk to print\r\n");
        return;
    }
    debug_log("RAM chunk %d:\r\n",chunk);
    debug_log("ts: 0x%lX - ms: %hd - ba: %d",micBuffer[chunk].timestamp,micBuffer[chunk].msTimestamp,
                                               (int)(micBuffer[chunk].battery*1000));
    for(int i = 0; i < SAMPLES_PER_CHUNK; i++)
    {
        if(i%10 == 0)
        {
            debug_log("\r\n  ");
            nrf_delay_ms(2);
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
    }
    debug_log("\r\n---\r\n");
}