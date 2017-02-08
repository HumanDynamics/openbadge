/*
 * Methods to simplifies writing to flash:  The program may always use storeWord() to
 *   store data; update(), if called repeatedly, handles the actual storage and organization
 *   in flash, including avoiding conflict with BLE activity.
 * Methods to automate sending data over BLE from flash
 */

#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <stdint.h>
#include <string.h>

#include "debug_log.h"
#include "nrf_gpio.h"           //abstraction for dealing with gpio
#include "nrf_adc.h"            //abstraction for dealing with adc
//#include "ble_flash.h"          //for accessing flash
#include "nrf_delay.h"

#include "nrf_drv_config.h"
#include "boards.h"

#define PANIC_LED LED_2

#include "ble_setup.h"         // uses updateAdvData() when collector started
#include "rtc_timing.h"        // uses now() to get chunk timestamp
#include "analog.h"            // uses analogRead() to get mic data; readBattery() to get battery voltage
//#include "internal_flash.h"    // uses some #defined constants related to memory usage
#include "storer.h"

// Setup expected "zero" for the mic value.
// analog ref is mic VCC, mic is 1/2 VCC biased, input is 1/3 scaled, so zero value is approx. (1023/2)/3 ~= 170
//#define MIC_ZERO 166
#define MIC_ZERO 125  // for 8bit conversion, full scale

#define MAX_MIC_SAMPLE 254         // mic samples will be clipped to this value
#define INVALID_SAMPLE 255     // dummy byte reserved for unused/invalid samples in chunk sample array

#define CHECK_INCOMPLETE 0xFFFFFFFFUL  // chunk check value for incomplete chunk
#define CHECK_TRUNC    0x7FFFFFFFUL  // chunk check value for truncated chunk - collector was stopped before complete chunk
                                     // Last byte of sample array stores the number of samples in the chunk.


// --------- Sampling timing parameters ----------
unsigned long sampleWindow;  // how long we should sample
unsigned long samplePeriod;   // time between samples - must exceed SAMPLE_WINDOW
// vvv Default values
//#define SAMPLE_PERIOD 250UL
//#define SAMPLE_WINDOW 100UL
#define SAMPLE_PERIOD                 50UL
#define SAMPLE_SLEEP_RATIO            0.075
#define READING_PERIODS_PER_SECOND    700.0
#define READING_PERIOD_MS             (1000.0 / READING_PERIODS_PER_SECOND)
#define READING_WINDOW_MS             (READING_PERIOD_MS * SAMPLE_SLEEP_RATIO)

bool takingReadings;  // whether we're currently taking readings for a sample
unsigned long sampleStart;    // timestamp of first reading for current sample
unsigned long sampleStartms;    // timestamp of first reading for current sample
unsigned int readingsCount;   // number of mic readings taken for current sample
unsigned long readingsSum;    // sum of all mic readings taken for current sample

#define SAMPLES_PER_CHUNK 114   // 128-(4+2+4+4)  ---  see chunk structure below
#define MIC_BUFFER_SIZE 20       // number of chunks in mic RAM buffer
#define LAST_RAM_CHUNK (MIC_BUFFER_SIZE - 1)        // index of last chunk in RAM buffer

typedef union
{
    struct
    {
        unsigned long timestamp;        // Timestamp of first sample in chunk           4byte
        float battery;                  // Battery voltage                              4byte
        unsigned short msTimestamp;     // Fractional part of chunk timestamp (0-999)   2byte
        unsigned char samples[SAMPLES_PER_CHUNK];    // Sound data samples               114byte
        unsigned long check;            // Copy of timestamp, to validate chunk         4byte
    };                                  //                                              128byte total
    unsigned long wordBuf[WORDS_PER_CHUNK];     // 128byte chunks = 32words (128/4)
} mic_chunk_t;



mic_chunk_t micBuffer[MIC_BUFFER_SIZE]; // RAM buffer for mic data - memory structure identical to data stored to flash

struct
{
    int to;      // which chunk in RAM buffer we're currently storing to
    int loc;        // next index in sample array to be written
} collect;          // Struct for keeping track of storing mic data to RAM


void collector_init();

/**
 * Take a reading from the mic (and add to total for averaging later)
 */
void takeMicReading();

/**
 * Take average of mic readings, put into mic RAM buffer
 */
void collectSample();

/**
 * Start (or restart) collecting.
 */
void startCollector(uint32_t timeout_minutes);

/**
 * Halt collecting; current chunk is likely incomplete, filled with some INVALID_READING samples.
 * Next call to addMicReading will start from next chunk in RAM buffer
 */
void stopCollector();

/**
 * @return True if collector is recording.
 */
bool Collector_IsRecording(void);

/**
 * Get battery voltage
 * Returns the current chunk's voltage if collecting is enabled; otherwise, returns an analogRead of VCC
 */
//float getBatteryVoltage();

/**
 * Print chunk contents from the RAM buffer to the debug log
 */
void printCollectorChunk(int chunk);


#endif //#ifndef MIC_H

