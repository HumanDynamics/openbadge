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
 *   Start collecting data       "1" (uchar)                  timestamp (ulong) - acknowledge time
 *                               timestamp (ulong)            ms (ushort)
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

#define SAMPLES_PER_PACKET 20

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


volatile bool dateReceived;

typedef struct
{
    unsigned long timestamp;
    unsigned short ms;
    unsigned short timeout;
    unsigned char cmd;      // ordered for nice packing
} server_command_params_t;

volatile server_command_params_t pendingCommand;

server_command_params_t unpackCommand(uint8_t* pkt);


enum SENDBUF_CONTENTS
{
    SENDBUF_EMPTY = 0,
    SENDBUF_STATUS,
    SENDBUF_HEADER,
    SENDBUF_SAMPLES,
    SENDBUF_END
};

#define SENDBUF_STATUS_SIZE 13  // see badge response structure above - 13 bytes total in status report packet

#define SENDBUF_HEADER_SIZE 13  // see badge response structure above - 13 bytes total in chunk header packet
#define SEND_LOC_HEADER -1      // send.loc if header is to be sent

#define SEND_FROM_END -1        // send.from if we're finished sending, and should send a null header

enum SEND_SOURCE
{
    SRC_FLASH,          // sending from FLASH
    SRC_RAM,            // sending a complete (unstored) chunk from RAM
    SRC_REALTIME        // sending an incomplete (collector in-progress) chunk from RAM
};

struct
{
    int from;                    // current chunk we're sending from
    int firstUnsent;             // the earliest unsent chunk (by REQUNSENT commands).  Updated at end of REQUNSENT execution.
    int source;                  // where send.from refers to (see SEND_SOURCE above)
    
    int loc;                     // index of next data to be sent from chunk - SEND_LOC_HEADER if header is to be sent next
    int numSamples;              // number of samples to be sent from send.from chunk (usually SAMPLES_PER_CHUNK)
    
    int bufContents;
    int bufSize;
    unsigned char buf[20];
} send;


void sender_init();

bool updateSender();

        
    



#endif //#ifndef SENDER_H

