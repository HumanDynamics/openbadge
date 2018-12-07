#ifndef __STREAM_MESSAGES_H
#define __STREAM_MESSAGES_H

#include <stdint.h>
#include "tinybuf.h"
#include "common_messages.h"

#define STREAM_BATTERY_FIFO_SIZE 10
#define STREAM_MICROPHONE_FIFO_SIZE 100
#define STREAM_SCAN_FIFO_SIZE 100
#define STREAM_ACCELEROMETER_FIFO_SIZE 100
#define STREAM_ACCELEROMETER_INTERRUPT_FIFO_SIZE 10


typedef struct {
	BatteryData battery_data;
} BatteryStream;

typedef struct {
	MicrophoneData microphone_data;
} MicrophoneStream;

typedef struct {
	ScanDevice scan_device;
} ScanStream;

typedef struct {
	AccelerometerRawData accelerometer_raw_data;
} AccelerometerStream;

typedef struct {
	Timestamp timestamp;
} AccelerometerInterruptStream;

extern const tb_field_t BatteryStream_fields[2];
extern const tb_field_t MicrophoneStream_fields[2];
extern const tb_field_t ScanStream_fields[2];
extern const tb_field_t AccelerometerStream_fields[2];
extern const tb_field_t AccelerometerInterruptStream_fields[2];

#endif
