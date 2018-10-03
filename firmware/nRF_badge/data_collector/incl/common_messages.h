#ifndef __COMMON_MESSAGES_H
#define __COMMON_MESSAGES_H

#include <stdint.h>
#include "tinybuf.h"



typedef struct {
	uint32_t seconds;
	uint16_t ms;
} Timestamp;

typedef struct {
	uint16_t ID;
	uint8_t group;
} BadgeAssignement;

typedef struct {
	float voltage;
} BatteryData;

typedef struct {
	uint8_t value;
} MicrophoneData;

typedef struct {
	uint16_t ID;
	int8_t rssi;
} ScanDevice;

typedef struct {
	ScanDevice scan_device;
	uint8_t count;
} ScanResultData;

typedef struct {
	uint16_t acceleration;
} AccelerometerData;

typedef struct {
	int16_t raw_acceleration[3];
} AccelerometerRawData;

extern const tb_field_t Timestamp_fields[3];
extern const tb_field_t BadgeAssignement_fields[3];
extern const tb_field_t BatteryData_fields[2];
extern const tb_field_t MicrophoneData_fields[2];
extern const tb_field_t ScanDevice_fields[3];
extern const tb_field_t ScanResultData_fields[3];
extern const tb_field_t AccelerometerData_fields[2];
extern const tb_field_t AccelerometerRawData_fields[2];

#endif
