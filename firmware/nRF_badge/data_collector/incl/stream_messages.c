#include "tinybuf.h"
#include "stream_messages.h"

const tb_field_t BatteryStream_fields[2] = {
	{513, tb_offsetof(BatteryStream, battery_data), 0, 0, tb_membersize(BatteryStream, battery_data), 0, 0, 0, &BatteryData_fields},
	TB_LAST_FIELD,
};

const tb_field_t MicrophoneStream_fields[2] = {
	{513, tb_offsetof(MicrophoneStream, microphone_data), 0, 0, tb_membersize(MicrophoneStream, microphone_data), 0, 0, 0, &MicrophoneData_fields},
	TB_LAST_FIELD,
};

const tb_field_t ScanStream_fields[2] = {
	{513, tb_offsetof(ScanStream, scan_device), 0, 0, tb_membersize(ScanStream, scan_device), 0, 0, 0, &ScanDevice_fields},
	TB_LAST_FIELD,
};

const tb_field_t AccelerometerStream_fields[2] = {
	{513, tb_offsetof(AccelerometerStream, accelerometer_raw_data), 0, 0, tb_membersize(AccelerometerStream, accelerometer_raw_data), 0, 0, 0, &AccelerometerRawData_fields},
	TB_LAST_FIELD,
};

const tb_field_t AccelerometerInterruptStream_fields[2] = {
	{513, tb_offsetof(AccelerometerInterruptStream, timestamp), 0, 0, tb_membersize(AccelerometerInterruptStream, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

