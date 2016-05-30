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


/*
 * === Protocol Summary ===
 * -Command/Notes-  . . . . . .  -Server Sends-  . . . . . .  -Badge Responds-  .
 *
 * CMD_STATUS                    
 *   Ask for badge status        "s" (uchar)                  clock status (uchar) - 0, clock was unset; 1, clock was set
 *   and send date               timestamp (ulong)            data status (uchar) - 1 if unsent data ready (backwards-compatibility)
 *                               ms (ushort)                  recording status (uchar) - 1 if collecting samples
 *                                                            timestamp (ulong) - 0 if none set
 *                                                            ms (ushort) - 0 if none set
 *                                                            battery voltage (float)
 *                    .  .  .  .  .  .  .  .
 * CMD_STARTREC
 *   Start collecting data       "1" (uchar)                  timestamp (ulong) - acknowledge time.  0 if none set.
 *                               timestamp (ulong)            ms (ushort)                            0 if none set.
 *                               ms (ushort)
 *                               timeout (ushort) - stop recording if didn't see server for [timeout] minutes
 *                                                  0 for no timeout
 *                    .  .  .  .  .  .  .  .
 * CMD_ENDREC
 *   Stop collecting data        "0" (uchar)                  none
 *                    .  .  .  .  .  .  .  .
 * CMD_REQUNSENT
 *   Request all unsent data     "d" (uchar)                  Chunks of data
 *   from FLASH.                                                Header (13bytes)
 *                                                                timestamp (ulong)
 * CMD_REQSINCE                  "r" (uchar)                      ms (ushort)
 *   Request all data since      timestamp (ulong)                voltage (float)
 *   specified time,             ms (ushort)                      sample period in ms (ushort)
 *   sent or unsent,                                              number of samples in chunk (uchar)
 *   from FLASH or RAM.                                         Samples
 *                                                                in packets of 20bytes
 *                                                            End marker
 *                                                              Dummy header
 *                                                                all bytes 0xff
 *                    .  .  .  .  .  .  .  .
 */

enum SERVER_COMMANDS
{
    CMD_NONE = 0,
    CMD_INVALID = 255,
    CMD_STATUS = 's',       // server requests send badge status
    CMD_STARTREC = '1',     // server requests start collecting
    CMD_ENDREC = '0',       // server requests stop collecting
    CMD_REQUNSENT = 'd',    // server requests send unsent data from flash
    CMD_REQSINCE = 'r'     // server requests send all data, flash or ram, since time X 
};

typedef struct
{
    unsigned long receiptTime;  // millis() time of command receipt
    
    unsigned long timestamp;
    unsigned short ms;
    unsigned short timeout;
    unsigned char cmd;      // ordered for nice packing
} server_command_params_t;

volatile server_command_params_t pendingCommand;

volatile bool dateReceived;     // whether the server has synced badge with date (e.g. thru status request)

unsigned long lastReceipt;      // time (i.e. millis) of the last command receipt from server
unsigned long collectorTimeout;   // time after last command receipt to stop collecting (e.g. if server can't send stop command)



enum SENDBUF_CONTENTS
{
    SENDBUF_EMPTY = 0,  // no valid data in send buffer
    SENDBUF_STATUS,     // status report packet
    SENDBUF_TIMESTAMP,  // timestamp report packet, i.e. for startrec response
    SENDBUF_HEADER,     // chunk header packet, for data sending commands
    SENDBUF_SAMPLES,    // chunk samples packet, for data sending commands
    SENDBUF_END         // null header, to mark end of data sending
};

// Special sendBuf sizes
#define SENDBUF_STATUS_SIZE 13  // see badge response structure above - 13 bytes total in status report packet
#define SENDBUF_HEADER_SIZE 13  // see badge response structure above - 13 bytes total in chunk header packet
#define SENDBUF_TIMESTAMP_SIZE 6    // 4byte timestamp + 2byte milliseconds
#define SAMPLES_PER_PACKET 20   // maximum by BLE spec

// Special values for send parameters, to help keep track of sending progress
#define SEND_LOC_HEADER -1      // send.loc if header is to be sent
#define SEND_FROM_END -1        // send.from if we're finished sending, and should send a null header

// To keep track of current origin of chunks being sent.  see send struct below.
enum SEND_SOURCE
{
    SRC_FLASH,          // sending from FLASH
    SRC_RAM,            // sending a complete (unstored) chunk from RAM
    SRC_REALTIME        // sending an incomplete (collector in-progress) chunk from RAM
};

// Struct to organize various variables/states related to sending
struct
{
    int from;                   // current chunk we're sending from
    int firstUnsent;            // the earliest unsent chunk (by REQUNSENT commands).  Updated at end of REQUNSENT execution.
    int source;                 // where send.from refers to (see SEND_SOURCE above)
    
    int loc;                    // index of next data to be sent from chunk - SEND_LOC_HEADER if header is to be sent next
    int numSamples;             // number of samples to be sent from send.from chunk (usually SAMPLES_PER_CHUNK)
    
    unsigned char buf[20];      // Buffer for BLE sending
    int bufContents;            // Records contents of send buffer
    int bufSize;                // Records length of actual data in send buffer
} send;



/*
 * Accepts a command packet from server, and parses it for relevant parameters into pendingCommand struct.
 * Called from BLEonReceive
 */
server_command_params_t unpackCommand(uint8_t* pkt);

/*
 * Initialize sender module.  Reset sending variables, and locate the earliest unsent chunk in FLASH (for unimplemented feature)
 */
void sender_init();

/*
 * Handle sending operations.  Must be called repeatedly, i.e. in main loop.
 */
bool updateSender();

        
    



#endif //#ifndef SENDER_H

