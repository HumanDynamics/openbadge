#include "tinybuf.h"
#include "chunk_messages.h"

const tb_field_t BatteryChunk_fields[3] = {
	{513, tb_offsetof(BatteryChunk, timestamp), 0, 0, tb_membersize(BatteryChunk, timestamp), 0, 0, 0, &Timestamp_fields},
	{513, tb_offsetof(BatteryChunk, battery_data), 0, 0, tb_membersize(BatteryChunk, battery_data), 0, 0, 0, &BatteryData_fields},
	TB_LAST_FIELD,
};

const tb_field_t MicrophoneChunk_fields[4] = {
	{513, tb_offsetof(MicrophoneChunk, timestamp), 0, 0, tb_membersize(MicrophoneChunk, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(MicrophoneChunk, sample_period_ms), 0, 0, tb_membersize(MicrophoneChunk, sample_period_ms), 0, 0, 0, NULL},
	{516, tb_offsetof(MicrophoneChunk, microphone_data), tb_delta(MicrophoneChunk, microphone_data_count, microphone_data), 1, tb_membersize(MicrophoneChunk, microphone_data[0]), tb_membersize(MicrophoneChunk, microphone_data)/tb_membersize(MicrophoneChunk, microphone_data[0]), 0, 0, &MicrophoneData_fields},
	TB_LAST_FIELD,
};

const tb_field_t ScanSamplingChunk_fields[3] = {
	{513, tb_offsetof(ScanSamplingChunk, timestamp), 0, 0, tb_membersize(ScanSamplingChunk, timestamp), 0, 0, 0, &Timestamp_fields},
	{516, tb_offsetof(ScanSamplingChunk, scan_result_data), tb_delta(ScanSamplingChunk, scan_result_data_count, scan_result_data), 1, tb_membersize(ScanSamplingChunk, scan_result_data[0]), tb_membersize(ScanSamplingChunk, scan_result_data)/tb_membersize(ScanSamplingChunk, scan_result_data[0]), 0, 0, &ScanResultData_fields},
	TB_LAST_FIELD,
};

const tb_field_t ScanChunk_fields[3] = {
	{513, tb_offsetof(ScanChunk, timestamp), 0, 0, tb_membersize(ScanChunk, timestamp), 0, 0, 0, &Timestamp_fields},
	{516, tb_offsetof(ScanChunk, scan_result_data), tb_delta(ScanChunk, scan_result_data_count, scan_result_data), 1, tb_membersize(ScanChunk, scan_result_data[0]), tb_membersize(ScanChunk, scan_result_data)/tb_membersize(ScanChunk, scan_result_data[0]), 0, 0, &ScanResultData_fields},
	TB_LAST_FIELD,
};

const tb_field_t AccelerometerChunk_fields[3] = {
	{513, tb_offsetof(AccelerometerChunk, timestamp), 0, 0, tb_membersize(AccelerometerChunk, timestamp), 0, 0, 0, &Timestamp_fields},
	{516, tb_offsetof(AccelerometerChunk, accelerometer_data), tb_delta(AccelerometerChunk, accelerometer_data_count, accelerometer_data), 1, tb_membersize(AccelerometerChunk, accelerometer_data[0]), tb_membersize(AccelerometerChunk, accelerometer_data)/tb_membersize(AccelerometerChunk, accelerometer_data[0]), 0, 0, &AccelerometerData_fields},
	TB_LAST_FIELD,
};

const tb_field_t AccelerometerInterruptChunk_fields[2] = {
	{513, tb_offsetof(AccelerometerInterruptChunk, timestamp), 0, 0, tb_membersize(AccelerometerInterruptChunk, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

