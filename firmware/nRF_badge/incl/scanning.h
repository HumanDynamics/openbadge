/* Methods to simplify access to an external Adesto AT25XE041B flash memory IC
 *
 */


#ifndef SCANNING_H
#define SCANNING_H


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


#include "rtc_timing.h"         // Timing-related code, incl. millis()
#include "ble_setup.h"          // BLE operations
#include "debug_log.h"          // UART debugging logger
                        // requires app_fifo, app_uart_fifo.c and retarget.c for printf to work properly
#include "internal_flash.h"     // Relating to internal flash operation
                                //   relevant here because we cannot do BLE operations while manipulating
                                //   internal flash
#include "external_flash.h"     // External flash manipulation






//===========================  Device List Manipulation ====================================
//==========================================================================================

/**
 * The badge acquires a full list of devices from the server, where each device is identified by a MAC address
 *   and an ID, 0-255.  The purpose of the ID is to allow for greatly reduced memory usage to store the
 *   results of a BLE scan: the 1byte ID is stored instead of the full 6byte MAC address.
 * This device list from the server does not need to be ordered in any particular way.
 *   I.e. the MAC addresses don't need to be sorted, and the indices do not need to be sequential.
 *   Internally, the badge sorts this device list based on the MAC address so that the device ID can 
 *   efficiently be retrieved for a given MAC address (by simple binary search).
 * The manner in which the list is sorted is perhaps not the most intuitive order for us; this is irrelevant,
 *   because the sorted list is used only internally, to facilitate faster mapping of MAC addresses to indices.
 *   The scan results that the badge returns to the server use the pre-established device indices.
 */

/** 
 * Parameters for during a scan.  To use less power, scanning is only active for scan_interval ms
 *   every scan_window ms, until scan_timeout seconds passes.
 * This constitutes one "scan".  All devices seen at least once during this scan are recorded,
 *   with signal strengths, in one scan chunk
 * When scanning is enabled, a scan happens every SCAN_PERIOD ms
 */
 
// Default scan timings
#define SCAN_INTERVAL 300
#define SCAN_WINDOW 100
#define SCAN_TIMEOUT 3
#define SCAN_PERIOD 30000UL

struct {
    int interval;
    int window;
    int timeout;
    int period;
} scanTiming;

unsigned long lastScanTime;  // millis() time of last scan


// Struct for representing a device, with a MAC address and associated device ID
typedef struct
{
    unsigned char mac[6];
    unsigned char ID;
} device_t;


// Total number of devices.
#define NUM_DEVICES 13

#define UNKNOWN_DEVICE -1

// List (in internal flash, not necessarily ordered in any way) of all known devices
//   Potentially replaced by a list stored in external flash, acquired from server
const device_t masterDeviceList[NUM_DEVICES];

// Working list of devices, in RAM for fast access/manipulation
device_t deviceList[NUM_DEVICES];



/**
 * Print a MAC address, via debug_log
 *   No CR/LF - requires a debug_log("\r\n") or similar afterwards.
 */
void printMac(unsigned char mac[6]);

/**
 * Print the MAC addresses and device indices of all devices in a list, via debug_log
 */
void printDeviceList(device_t devices[],int numDevices);

/**
 * Convert mac address char array to single 64-bit word (long long, LL) for comparing and such.
 *   e.g. passing {0x6f,0x5f,0x4f,0x3f,0x2f,0x1f} returns 0x00001f2f3f4f5f6f
 */
uint64_t macLL(unsigned char mac[6]);

/**
 * Print a 64-bit word, e.g. from macLL(), (not particularly useful) 
 */
void printMacLL(uint64_t mac64);

/**
 * Simple in-place quicksort, by increasing value of the macLL() results for each device MAC address
 */
void quicksortDevices(device_t devices[], int leftIndex, int rightIndex);

/**
 * Wrapper for quicksortDevices, for convenience and readability
 */
void sortDeviceList(device_t devices[], int numDevices);


/**
 * Given a MAC address, do a simple binary search to find the index of that device in
 *   a list (that has been sorted with sortDeviceList).  Returns the corresponding device's
 *   index in that list.
 */
int findDeviceInList(unsigned char mac[6], device_t devices[], int numDevices);

/**
 * Find the device ID of a device with a given MAC address.  Uses findDeviceInList to locate
 *   the device in the list, and returns that device's ID number
 */
int getIDfromMac(unsigned char mac[6], device_t devices[], int numDevices);





//================================ Scanning + scan result storage ===============================
//===============================================================================================

/**
 * External AT25XE041B flash
 *
 * Similarly to the internal flash, the external flash is organized into "chunks" for storing scan data
 *   One chunk is one scan result, containing the device indices and RSSI strengths of all devices seen
 * In a large scan, this might require more than one chunk's worth of space.  Such chunks are specially
 *   marked, and the data is divided into however many chunks are required.
 * 
 * Chunk structure:
 * [ timestamp ][type][num ][ ID1 ][rssi1][ ID2 ][rssi2]...[ ID59 ][rssi59][ timestamp ]
 *    32bits     8bit  8bit  8bit    8bit   8bit    8bit      8bit    8bit    32bits
 *            128bytes total
 *   Timestamp is the timestamp of the beginning of the chunk.  The end timestamp is used to
 *     mark complete chunks, similar to the internal flash chunks
 *   Type says what's in the chunk.  E.g. a complete scan result report, or the second half
 *     of a scan results.  Or a table of device addresses.
 *   Num is the number of devices seen in the entire scan.  (may be more than what's listed in
 *     a single chunk)
 */
 
 
//#define EXT_PAGE_SIZE 256  // External flash can be written only up to 1 page at a time
// But pages are not too useful in external flash, as page-level erase only works on first 64kB

// Most useful unit of memory is a 4kB block, the smallest erase that can be done on any memory
// External flash has 128 of these 4kB blocks
#define EXT_BLOCK_SIZE 4096  // External flash can only be erased in minimum 4kB units

#define EXT_CHUNK_SIZE 128
#define EXT_ADDRESS_OF_CHUNK(chnk) ( (uint32_t)(chnk * 128) )
#define EXT_FIRST_CHUNK 0
#define EXT_FIRST_DATA_CHUNK 8     // earlier chunks are for persistent device list, configuration, etc
#define EXT_LAST_CHUNK 4095  // 524288 / 128  =  2^19 / EXT_CHUNK_SIZE = 4096 chunks total
#define EXT_BLOCK_OF_CHUNK(chnk) ( (uint32_t)(chnk / 32) )

// Chunk types
#define EXT_COMPLETE_CHUNK 0x01   // a chunk that contains a complete scan result
#define EXT_INCOMPLETE_CHUNK 0x11 // a chunk that contains the first part of a longer scan result
#define EXT_CONT_CHUNK 0x02       // a chunk that continues a scan result started in a previous chunk

#define EXT_DEVICES_PER_CHUNK 59  // maximum devices that we can report in one chunk.  See chunk structure.

// Instead of a proper timestamp, chunks that complete a previous incomplete chunk are marked with
//   a special "timestamp"
#define EXT_CONT_TIMESTAMP 0xf00df00d

// Unix time in recent past (sometime June 2015), used to check for timestamp validity
#define MODERN_TIME 1434240000UL  // Unix time in the recent past (sometime June 2015), used to check for reasonable times

 

/**
 * Various unions/structs for making accessing the external flash more convenient.
 *
 * The external flash is most efficiently accessed with a single SPI transfer
 *   The following types define a union between an appropriate length buffer, and a 
 *   struct with members according to the above specified scan chunk structure.
 * They include 4 dummy bytes at the beginning, corresponding to the SPI transfer of
 *   opcodes and address bytes.
 *
 * Example usage: 
 *   scan_chunk_t storeChunk;
 *   storeChunk.devices[0].ID = 12;
 *   ext_flash_write(address,storeChunk.buf,sizeof(storeChunk.buf));
 */

// A type used within scan_chunk_t to represent a scan-seen device and corresponding signal strength
typedef struct
{
    unsigned char ID;
    signed char rssi;
} seenDevice_t;
    
// A type useful for dealing with entire scan chunks
typedef union
{
    struct
    {
        unsigned char dummy[4];  // Padding, for flash opcode, address.        (4byte)
        uint32_t timestamp;       // timestamp of scan start                    4byte
        unsigned char type;       // type of chunk                              1byte
        unsigned char num;        // number of devices seen in scan             1byte
        seenDevice_t devices[EXT_DEVICES_PER_CHUNK]; // all the devices seen    59*2byte
        uint32_t timestamp_check;      // copy of timestamp, to verify          4byte
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
        unsigned char type;     //  1byte
        unsigned char num;      //  1byte
    };                          //  4+6byte total
    unsigned char buf[10];
} scan_header_t;


// A type useful for reading only the tail (timestamp check) of a scan chunk
typedef union
{
    struct
    {
        unsigned char dummy[4]; // (4byte)
        uint32_t check;         //  4byte
    };                          //  4+4byte total
    unsigned char buf[8];
} scan_tail_t;

 




// For keeping track of current state of scanning
typedef enum scan_state_t
{
    SCAN_IDLE = 0,             //no scan-related activity occurring
    SCAN_SCANNING,              //scan in process
    SCAN_STORING,               //busy storing scan data
} scan_state_t;

volatile scan_state_t scan_state;



// Table for temporarily storing results of a scan, during the scan itself.
// When a device is seen with a strong enough RSSI, the RSSI is stored in this table,
//   in the same position in the table as in the deviceList.  (i.e. scanResults[n] is the 
//   detected signal strength of the device deviceList[n] )
volatile signed char scanResults[NUM_DEVICES];

signed char MINIMUM_RSSI;   // device seen on scan must reach this RSSI threshold to be acknowledged


// Keep track of storage of scan data.
struct
{
    unsigned char resultLoc;  //current location within scanResults table
    int chunk;     // index of external flash chunk currently being stored to
    unsigned long timestamp;  // Timestamp of beginning (end?) of scan period
    unsigned char numTotal;    // Total number of devices seen in scan
    unsigned char numStored;        // Number of seen devices stored so far
} scanStore;


/**
 * Initialize scanning module
 *   Finds where in external flash scan data should be stored (after most recent past data)
 */
void scans_init();


/**
 * Initiate BLE scan
 *   Scanning will occur for window_ms every interval_ms, and will stop after timeout_s
 */
void startScan(int interval_ms, int window_ms, int timeout_s);


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
void updateScanning();

// Returns scan_state, the current status of scanning.
scan_state_t getScanState();

// Read a scan chunk from flash, and print to debug log
void printScanResult(int chunk);




#endif //#ifndef SCANNING_H