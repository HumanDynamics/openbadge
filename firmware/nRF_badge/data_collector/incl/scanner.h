/* Methods to simplify access to an external Adesto AT25XE041B flash memory IC
 *
 */


#ifndef SCANNER_H
#define SCANNER_H


#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "nrf.h"
#include "nrf51_bitfields.h"
#include "app_util_platform.h"

#include "nrf_drv_config.h"
#include "boards.h"

#include "app_error.h"          // error handling
#include "nrf_delay.h"          // includes blocking delay functions
#include "nrf_gpio.h"           // abstraction for dealing with gpio

#include "analog.h"
#include "rtc_timing.h"         // Timing-related code, incl. millis()
#include "ble_setup.h"          // BLE operations
#include "debug_log.h"          // UART debugging logger
                        // requires app_fifo, app_uart_fifo.c and retarget.c for printf to work properly
//#include "internal_flash.h"     // Relating to internal flash operation
                                //   relevant here because we cannot do BLE operations while manipulating
                                //   internal flash
#include "ext_eeprom.h"     // External flash manipulation


// Values from iBeacon spec
#define COMPANY_ID_APPLE  0x004C
#define IBEACON_TYPE_PROXIMITY 0x1502
#define IBEACON_MANUF_DATA_LEN 0x19

// Structure of iBeacon manufacturer-specific data, after the 0xFF type specifier byte.
typedef struct
{
    unsigned short companyID;
    unsigned short type;
    unsigned char UUID[16];
    unsigned char major[2];  // big-endian
    unsigned char minor[2];  // big-endian
    unsigned char measuredPower;
} iBeacon_data_t;


//================================ Scanning + scan result storage ===============================
//===============================================================================================

/**
 * External AT25XE041B flash (or EEPROM)
 *
 * Similarly to the internal flash, the external EEPROM is organized into "chunks" for storing scan data
 *   One chunk is one scan result, containing the ID, average RSSI, and number of sightings for each device
 * In a large scan, this might require more than one chunk's worth of space.  Such chunks are specially
 *   marked, and the data is divided into however many chunks are required.
 * 
 * Chunk structure:
 * [ timestamp ][num ][ padding ][  ID1  ][rssi1][count][  ID1  ][rssi1][count]...[  ID29  ][rssi29][count][ timestamp ]
 *    32bits     8bit    24bit     16bit   8bit   8bit    16bit   8bit   8bit       16bit    8bit    8bit      32bit
 *            128bytes total
 *   Timestamp is the timestamp of the beginning of the chunk.  The end timestamp is used to
 *     mark complete chunks, similar to the internal flash chunks
 *   Type says what's in the chunk.  E.g. a complete scan result report, or the second half
 *     of a scan results.  Or a table of device addresses.
 *   Num is the number of devices seen in the entire scan.  (may be more than what's listed in
 *     a single chunk)
 */

#define EXT_CHUNK_SIZE 128
#define EXT_ADDRESS_OF_CHUNK(chnk) ( (uint32_t)(chnk * 128) )
#define EXT_ADDRESS_OF_CHUNK_CHECK(chnk) ( EXT_ADDRESS_OF_CHUNK(chnk) + 124 )
#define EXT_FIRST_CHUNK 0
#define EXT_FIRST_DATA_CHUNK 8     // earlier chunks are for persistent device list, configuration, etc
#define EXT_LAST_CHUNK 2047  // 524288 / 128  =  2^18 / EXT_CHUNK_SIZE = 2048 chunks total


#define CHECK_INCOMPLETE 0xFFFFFFFFUL  // chunk check value for incomplete chunk
#define CHECK_TRUNC    0x7FFFFFFFUL  // chunk check value for truncated chunk - scan results truncated, continue into next chunk(s)
#define CHECK_CONTINUE 0x3FFFFFFFUL  // chunk check value for chunk that continues the data from a truncated scan chunk
                            // Note the num parameter in truncated/continuing chunks is the total devices seen in the whole scan,
                            //   not the number reported in the chunk itself.

#define SCAN_DEVICES_PER_CHUNK 29  // maximum devices that we can report in one chunk.  See chunk structure.

 
/**
 * Various data types for convenient EEPROM access
 * 
 * The following types define unions between structs with relevant scan chunk members,
 *   and unsigned char arrays, for use with EEPROM data transfer functions.
 * They also include the required 4 dummy bytes at the start.  (see ext_eeprom.h)
 * Example usage: 
 *   scan_chunk_t storeChunk;
 *   storeChunk.devices[0].ID = 12;
 *   ext_flash_write(address,storeChunk.buf,sizeof(storeChunk.buf));
 */

// A type used within scan_chunk_t to represent a scan-seen device and corresponding signal strength
typedef struct
{
    unsigned short ID;
    signed char rssi;
    signed char count;
} seenDevice_t;
    
// A type useful for dealing with entire scan chunks
typedef union
{
    struct
    {
        unsigned char dummy[4];  // Padding, for flash opcode, address.        (4byte)
        uint32_t timestamp;       // timestamp of scan start                    4byte
        unsigned char num;        // number of devices seen in scan             1byte
        unsigned char batteryLevel;  // battery voltage = 1V + 0.01V*level      1byte
        unsigned char padding[2];     // padding to manually fill chunk         2byte
        seenDevice_t devices[SCAN_DEVICES_PER_CHUNK]; // all the devices seen    29*4byte
        uint32_t check;      // copy of timestamp, to verify                    4byte
    };                                                             //           4+128byte total
    unsigned char buf[EXT_CHUNK_SIZE+4];  // everything above all in one array, for read/writing flash
} scan_chunk_t;


// A type useful for reading only the header (timestamp, type, number) of a scan chunk
typedef union
{
    struct
    {
        unsigned char dummy[4]; // (4byte)
        uint32_t timestamp;     //  4byte
        unsigned char num;      //  1byte
    };                          //  4+5byte total
    unsigned char buf[9];
} scan_header_t;


// A type useful for reading only the tail (check) of a scan chunk
typedef union
{
    struct
    {
        unsigned char dummy[4]; // (4byte)
        uint32_t check;         //  4byte
    };                          //  4+4byte total
    unsigned char buf[8];
} scan_tail_t;

 
 
 
 
volatile bool scanner_enable;

// Default scan timings
#define SCAN_WINDOW 100     // Milliseconds of active scanning
#define SCAN_INTERVAL 300   // Millisecond interval at which a scan window is performed  
#define SCAN_TIMEOUT 5  // Scan duration, seconds.
#define SCAN_PERIOD 10 // Scan period, s.  A scan is started at this interval.


ble_gap_scan_params_t scan_params;

unsigned long lastScanTime;  // millis() time of last scan
unsigned long scanPeriod_ms;  // scan period in ms

// For keeping track of current state of scanning
typedef enum scan_state_t
{
    SCANNER_IDLE = 0,             // no scan-related activity occurring
    SCANNER_SCANNING,              // scan in process
    SCANNER_SAVE,               // need to save scan results in buffer
} scan_state_t;

volatile scan_state_t scan_state;


#define MAX_SCAN_RESULTS    300    // max number of individual devices that will be reported in a scan
#define MAX_SCAN_COUNT      127      // maximum number of RSSI readings for one device reported in a scan

#define BEACON_PRIORITY     4       // always store at least this many beacons
#define BEACON_ID_THRESHOLD 16000   // IDs at or above this threshold signify a beacon. IDs below this are badges
#define MINIMUM_RSSI (-120)
signed char minimumRSSI;   // device seen on scan must reach this RSSI threshold to be acknowledged


#define MAX_AGGR true       // indicates the function (true=max/false=mean) used to aggregate samples


void sortScanByRSSIDescending(void);

struct
{
    int to;
    volatile int timestamp;
    volatile int num, numbeacons;
    volatile unsigned short IDs[MAX_SCAN_RESULTS];
    volatile signed short rssiSums[MAX_SCAN_RESULTS];
    volatile signed char counts[MAX_SCAN_RESULTS];
} scan;


#define SCAN_BUFFER_SIZE 10
#define LAST_SCAN_CHUNK (SCAN_BUFFER_SIZE - 1)

scan_chunk_t scanBuffer[SCAN_BUFFER_SIZE];


/**
 * Initialize scanning module
 *   Finds where in external flash scan data should be stored (after most recent past data)
 */
void scanner_init();


/**
 * Initiate BLE scan
 *   Scanning will occur for window_ms every interval_ms, and will stop after timeout_s
 */
//void startScan(int interval_ms, int window_ms, int timeout_s);
uint32_t startScan();


void startScanner(unsigned short window_ms,unsigned short interval_ms,unsigned short duration_s,unsigned short period_s);
void stopScanner();


unsigned long getScanTimestamp(int chunk);
unsigned long getScanCheck(int chunk);
void getScanChunk(scan_chunk_t* destPtr,int chunk);

/**
 * Scan callback - called when another device's advertising packet is detected.
 *   Updates the scanResults array with the new RSSI reading
 */
//Declared in ble_setup.h:  BLEonAdvReport(uint8_t addr[6], int8_t rssi);

/**
 * Scanning timeout callback - called at end of scanning.
 *   Prints list of devices seen if debug log enabled
 *   Stores scan result to flash
 */
//Declared in ble_setup.h:  BLEonScanTimeout();

// Must be called repeatedly to ensure scans are performed, and that scan results are stored.
bool updateScanner();

// Returns scan_state, the current status of scanning.
scan_state_t getScanState();

// Print a scan chunk to debug log
void printScanResult(scan_chunk_t* sourceChunk);




#endif //#ifndef SCANNER_H
