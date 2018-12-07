#ifndef __CHUNK_MESSAGES_H
#define __CHUNK_MESSAGES_H

#include <stdint.h>
#include "tinybuf.h"
#include "common_messages.h"

#define MICROPHONE_CHUNK_DATA_SIZE 114
#define ACCELEROMETER_CHUNK_DATA_SIZE 100
#define SCAN_CHUNK_DATA_SIZE 29
#define SCAN_SAMPLING_CHUNK_DATA_SIZE 255
#define SCAN_CHUNK_AGGREGATE_TYPE_MAX 0
#define SCAN_CHUNK_AGGREGATE_TYPE_MEAN 1


typedef struct {
	Timestamp timestamp;
	BatteryData battery_data;
} BatteryChunk;

typedef struct {
	Timestamp timestamp;
	uint16_t sample_period_ms;
	uint8_t microphone_data_count;
	MicrophoneData microphone_data[114];
} MicrophoneChunk;

typedef struct {
	Timestamp timestamp;
	uint8_t scan_result_data_count;
	ScanResultData scan_result_data[255];
} ScanSamplingChunk;

typedef struct {
	Timestamp timestamp;
	uint8_t scan_result_data_count;
	ScanResultData scan_result_data[29];
} ScanChunk;

typedef struct {
	Timestamp timestamp;
	uint8_t accelerometer_data_count;
	AccelerometerData accelerometer_data[100];
} AccelerometerChunk;

typedef struct {
	Timestamp timestamp;
} AccelerometerInterruptChunk;

extern const tb_field_t BatteryChunk_fields[3];
extern const tb_field_t MicrophoneChunk_fields[4];
extern const tb_field_t ScanSamplingChunk_fields[3];
extern const tb_field_t ScanChunk_fields[3];
extern const tb_field_t AccelerometerChunk_fields[3];
extern const tb_field_t AccelerometerInterruptChunk_fields[2];

#endif
