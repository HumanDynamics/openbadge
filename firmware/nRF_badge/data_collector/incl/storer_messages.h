#ifndef __STORER_MESSAGES_H
#define __STORER_MESSAGES_H

#include <stdint.h>
#include "tinybuf.h"
#include "common_messages.h"

#define STORER_MICROPHONE_DATA_SIZE 114
#define STORER_SCAN_DATA_SIZE 29


typedef struct {
	Timestamp timestamp;
	BatteryData battery_data;
} BatteryDataStoreMessage;

typedef struct {
	Timestamp timestamp;
	uint8_t scan_data_count;
	ScanData scan_data[29];
} ScanDataStoreMessage;

typedef struct {
	Timestamp timestamp;
	uint16_t sample_period_ms;
	MicrophoneData microphone_data[114];
} MicrophoneDataStoreMessage;

extern const tb_field_t BatteryDataStoreMessage_fields[3];
extern const tb_field_t ScanDataStoreMessage_fields[3];
extern const tb_field_t MicrophoneDataStoreMessage_fields[4];

#endif
