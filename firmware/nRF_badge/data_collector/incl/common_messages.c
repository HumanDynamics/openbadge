#include "tinybuf.h"
#include "common_messages.h"

const tb_field_t Timestamp_fields[3] = {
	{65, tb_offsetof(Timestamp, seconds), 0, 0, tb_membersize(Timestamp, seconds), 0, 0, 0, NULL},
	{65, tb_offsetof(Timestamp, ms), 0, 0, tb_membersize(Timestamp, ms), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t BadgeAssignement_fields[3] = {
	{65, tb_offsetof(BadgeAssignement, ID), 0, 0, tb_membersize(BadgeAssignement, ID), 0, 0, 0, NULL},
	{65, tb_offsetof(BadgeAssignement, group), 0, 0, tb_membersize(BadgeAssignement, group), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t BatteryData_fields[2] = {
	{129, tb_offsetof(BatteryData, voltage), 0, 0, tb_membersize(BatteryData, voltage), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t MicrophoneData_fields[2] = {
	{65, tb_offsetof(MicrophoneData, value), 0, 0, tb_membersize(MicrophoneData, value), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t ScanDevice_fields[3] = {
	{65, tb_offsetof(ScanDevice, ID), 0, 0, tb_membersize(ScanDevice, ID), 0, 0, 0, NULL},
	{33, tb_offsetof(ScanDevice, rssi), 0, 0, tb_membersize(ScanDevice, rssi), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t ScanResultData_fields[3] = {
	{513, tb_offsetof(ScanResultData, scan_device), 0, 0, tb_membersize(ScanResultData, scan_device), 0, 0, 0, &ScanDevice_fields},
	{65, tb_offsetof(ScanResultData, count), 0, 0, tb_membersize(ScanResultData, count), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t AccelerometerData_fields[2] = {
	{65, tb_offsetof(AccelerometerData, acceleration), 0, 0, tb_membersize(AccelerometerData, acceleration), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t AccelerometerRawData_fields[2] = {
	{40, tb_offsetof(AccelerometerRawData, raw_acceleration), 0, 0, tb_membersize(AccelerometerRawData, raw_acceleration[0]), tb_membersize(AccelerometerRawData, raw_acceleration)/tb_membersize(AccelerometerRawData, raw_acceleration[0]), 0, 0, NULL},
	TB_LAST_FIELD,
};

