#include "tinybuf.h"
#include "storer_messages.h"

const tb_field_t BatteryDataStoreMessage_fields[3] = {
	{513, tb_offsetof(BatteryDataStoreMessage, timestamp), 0, 0, tb_membersize(BatteryDataStoreMessage, timestamp), 0, 0, 0, &Timestamp_fields},
	{513, tb_offsetof(BatteryDataStoreMessage, battery_data), 0, 0, tb_membersize(BatteryDataStoreMessage, battery_data), 0, 0, 0, &BatteryData_fields},
	TB_LAST_FIELD,
};

const tb_field_t ScanDataStoreMessage_fields[3] = {
	{513, tb_offsetof(ScanDataStoreMessage, timestamp), 0, 0, tb_membersize(ScanDataStoreMessage, timestamp), 0, 0, 0, &Timestamp_fields},
	{516, tb_offsetof(ScanDataStoreMessage, scan_data), tb_delta(ScanDataStoreMessage, scan_data_count, scan_data), 1, tb_membersize(ScanDataStoreMessage, scan_data[0]), tb_membersize(ScanDataStoreMessage, scan_data)/tb_membersize(ScanDataStoreMessage, scan_data[0]), 0, 0, &ScanData_fields},
	TB_LAST_FIELD,
};

const tb_field_t MicrophoneDataStoreMessage_fields[4] = {
	{513, tb_offsetof(MicrophoneDataStoreMessage, timestamp), 0, 0, tb_membersize(MicrophoneDataStoreMessage, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(MicrophoneDataStoreMessage, sample_period_ms), 0, 0, tb_membersize(MicrophoneDataStoreMessage, sample_period_ms), 0, 0, 0, NULL},
	{520, tb_offsetof(MicrophoneDataStoreMessage, microphone_data), 0, 0, tb_membersize(MicrophoneDataStoreMessage, microphone_data[0]), tb_membersize(MicrophoneDataStoreMessage, microphone_data)/tb_membersize(MicrophoneDataStoreMessage, microphone_data[0]), 0, 0, &MicrophoneData_fields},
	TB_LAST_FIELD,
};

