/*
 * INFORMATION ****************************************************
 */

#ifndef SENDER_H
#define SENDER_H

#include <stdint.h>
#include <string.h>

#include "debug_log.h"

#include "nrf_drv_config.h"
#include "boards.h"

#include "nrf_soc.h"

#include "nrf_gpio.h"

#include "ble_setup.h"
#include "collector.h"
#include "storer.h"
#include "scanner.h"


/*
 * === Protocol Summary ===
 * -Command/Notes-  . . . . . .  -Server Sends-  . . . . . .  -Badge Responds-  .
 *
 * CMD_STATUS                    
 *   Ask for badge status        's' (uchar)                  clock status (uchar) - 0, clock was unset; 1, clock was set
 *   and send date               timestamp (ulong)            scan status (uchar) - 1 if scanning
 *   Optionally, set             ms (ushort)                  collector status (uchar) - 1 if collecting samples
 *     badge ID and group      [ ID (ushort) ]                timestamp (ulong) - 0 if none set
 *                             [ group (uchar) ]              ms (ushort) - 0 if none set
 *                                                            battery voltage (float)
 *   Send ID=0xFFFF/group=0xFF to reset the persistent (non-volatile) assignments.
 *                    .  .  .  .  .  .  .  .
 * CMD_STARTREC
 *   Start collecting data       '1' (uchar)                  timestamp (ulong) - acknowledge time.  0 if none set.
 *                               timestamp (ulong)            ms (ushort)                            0 if none set.
 *                               ms (ushort)
 *                               timeout (ushort) - stop recording if didn't see server for [timeout] minutes
 *                                                  0 for no timeout
 *                    .  .  .  .  .  .  .  .
 * CMD_ENDREC
 *   Stop collecting data        '0' (uchar)                  none
 *                    .  .  .  .  .  .  .  .
 * CMD_STARTSCAN
 *   Start performing scans      'p' (uchar)                  timestamp (ulong) - acknowledge time.  0 if none set.
 *                               timestamp (ulong)            ms (ushort)                            0 if none set.
 *                               ms (ushort)
 *                               timeout (ushort) - stop scanning if didn't see server for [timeout] minutes
 *                                                  0 for no scan timeout
 *                               window (ushort)   \
 *                               interval (ushort)  } - short-scale scan parameters, ms.  0 for default values.
 *                               duration (ushort) - duration of one scan, s.  Called "timeout" in NRF APIs.  0 for default value
 *                               period (ushort)   - how often to perform scans, s.  0 for default.
 *                    .  .  .  .  .  .  .  .
 * CMD_ENDSCAN
 *   Stop collecting data        'q' (uchar)                  none
 *                    .  .  .  .  .  .  .  .
 * CMD_REQSINCE
 *   Request all data since      'r' (uchar)                  Chunks of data
 *   specified time,             timestamp (ulong)              Header (13bytes)
 *   from FLASH or RAM           ms (ushort)                      timestamp (ulong)
 *                                                                ms (ushort)
 *                                                                voltage (float)
 *                                                                sample period in ms (ushort)
 *                                                                number of samples in chunk (uchar)
 *                                                              Samples
 *                                                                in packets of 20bytes
 *                                                            End marker
 *                                                              Dummy header
 *                                                                all bytes 0
 *                    .  .  .  .  .  .  .  .
 * CMD_REQSCANS
 *   Request scan data           'b' (uchar)                  Scan results
 *   from ext. EEPROM.           timestamp (ulong)              Header (5bytes)
 *                                                                timestamp (ulong)
 *                                                                battery voltage (float)
 *                                                                number of devices in scan (uchar)
 *                                                              Devices, in packets of 20bytes (5 devices)
 *                                                                Device ID (ushort)
 *                                                                RSSI (signed char)
 *                                                                Count (signed char)
 *                                                            End marker
 *                                                              Dummy header
 *                                                                all bytes 0
 *                    .  .  .  .  .  .  .  .
 * CMD_IDENTIFY
 *   Light an LED for               'i'                          none (lights LED)
 *   specified duration             timeout (ushort) - 0 to turn off LED
 *                    .  .  .  .  .  .  .  .
 */

enum SERVER_COMMANDS
{
    CMD_NONE = 0,
    CMD_INVALID = 255,
    CMD_STATUS = 's',           // server requests send badge status
    CMD_STARTREC = '1',         // server requests start collecting
    CMD_ENDREC = '0',           // server requests stop collecting
    CMD_STARTSCAN = 'p',        // server requests start scanning
    CMD_ENDSCAN = 'q',          // server requests stop scanning
    CMD_REQSINCE = 'r',         // server requests send all data, flash or ram, since time X 
    CMD_REQSCANS = 'b',         // server requests send all scan results, since time X
    CMD_IDENTIFY = 'i',         // server requests light an LED for specified time
    
    CMD_STATUS_ASSIGN = 'S'     // Only used internally to the badge, to distinguish status requests with optional ID/group assignment
};

enum SERVER_COMMAND_LENGTHS
{
    CMD_NONE_LEN = 0,
    CMD_STATUS_LEN = 7,
    CMD_STATUS_ASSIGN_LEN = 10,
    CMD_STARTREC_LEN = 9,
    CMD_ENDREC_LEN = 1,
    CMD_STARTSCAN_LEN = 17,
    CMD_ENDSCAN_LEN = 1,
    CMD_REQSINCE_LEN = 7,
    CMD_REQSCANS_LEN = 5,
    CMD_IDENTIFY_LEN = 3
};

typedef struct
{
    unsigned long receiptTime;  // millis() time of command receipt
    
    // Timestamp receipt
    unsigned long timestamp;
    unsigned short ms;
    unsigned short timeout;
    unsigned short duration;
    unsigned short period;
    unsigned short window;
    unsigned short interval;
    unsigned short ID;
    unsigned char group;
    unsigned char cmd;      // ordered for nice packing
} server_command_params_t;

volatile server_command_params_t pendingCommand;

volatile bool dateReceived;     // whether the server has synced badge with date (e.g. thru status request)

unsigned long lastReceipt;      // time (i.e. millis) of the last command receipt from server
unsigned long collectorTimeout;   // time after last command receipt to stop collecting (e.g. if server doesn't send stop command)
unsigned long scannerTimeout;   // similar to collectorTimeout, but for scanner module.


enum SENDBUF_CONTENTS
{
    SENDBUF_EMPTY = 0,  // no valid data in send buffer
    SENDBUF_STATUS,     // status report packet
    SENDBUF_TIMESTAMP,  // timestamp report packet, i.e. for startrec response
    SENDBUF_HEADER,     // chunk header packet, for data sending commands
    SENDBUF_SAMPLES,    // chunk samples packet, for data sending commands
    SENDBUF_SCANHEADER,
    SENDBUF_SCANDEVICES,
    SENDBUF_END         // null header, to mark end of data sending
};

// Special sendBuf sizes
#define SENDBUF_STATUS_SIZE 13  // see badge response structure above - 13 bytes total in status report packet
#define SENDBUF_HEADER_SIZE 13  // see badge response structure above - 13 bytes total in chunk header packet
#define SENDBUF_SCANHEADER_SIZE 9
#define SENDBUF_TIMESTAMP_SIZE 6    // 4byte timestamp + 2byte milliseconds
#define SAMPLES_PER_PACKET 20   // maximum by BLE spec
#define DEVICES_PER_PACKET 5    // scan devices per packet - one scan device report is 4bytes

// Special values for send parameters, to help keep track of sending progress
#define SEND_LOC_HEADER -1      // send.loc if header is to be sent
#define SEND_FROM_END -1        // send.from if we're finished sending, and should send a null header

// To keep track of current origin of chunks being sent.  see send struct below.
enum SEND_SOURCE
{
    SRC_FLASH,          // sending from FLASH
    SRC_RAM,            // sending a complete (unstored) chunk from RAM
    SRC_REALTIME,       // sending an incomplete (collector in-progress) chunk from RAM
    SRC_SCAN_RAM,       // sending scan chunks from RAM
    SRC_EXT             // sending chunks stored to external EEPROM
};


// Struct to organize various variables/states related to sending
struct
{
    int from;                   // current chunk we're sending from
    int source;                 // where send.from refers to (see SEND_SOURCE above)
    
    int loc;                    // index of next data to be sent from chunk - SEND_LOC_HEADER if header is to be sent next
    int num;                    // number of items (samples, devices) to be sent from send.from chunk
    
    unsigned char buf[20];      // Buffer for BLE sending
    int bufContents;            // Records contents of send buffer
    int bufSize;                // Records length of actual data in send buffer

    int nextLoc;                // The value of send.loc if the curent send.buf is sent successfuly.
                                // (Used during sending scan data across multiple chunks)
    int numDevicesSent;         // The number of devices that has been sent.
} send;



/*
 * Accepts a command packet from server, and parses it for relevant parameters into pendingCommand struct.
 * Called from BLEonReceive
 */
server_command_params_t unpackCommand(uint8_t* pkt, unsigned char len);

/*
 * Initialize sender module.  Reset sending variables, and locate the earliest unsent chunk in FLASH (for unimplemented feature)
 */
void sender_init();

/*
 * Handle sending operations.  Must be called repeatedly, i.e. in main loop.
 */
bool updateSender();

        
    



#endif //#ifndef SENDER_H

