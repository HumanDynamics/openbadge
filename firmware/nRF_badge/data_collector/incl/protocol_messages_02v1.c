#include "tinybuf.h"
#include "protocol_messages_02v1.h"


//#define PROTOCOL_02v1
#ifdef PROTOCOL_02v1


const tb_field_t StatusRequest_fields[3] = {
	{513, tb_offsetof(StatusRequest, timestamp), 0, 0, tb_membersize(StatusRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{514, tb_offsetof(StatusRequest, badge_assignement), tb_delta(StatusRequest, has_badge_assignement, badge_assignement), 1, tb_membersize(StatusRequest, badge_assignement), 0, 0, 0, &BadgeAssignement_fields},
	TB_LAST_FIELD,
};

const tb_field_t StartMicrophoneRequest_fields[4] = {
	{513, tb_offsetof(StartMicrophoneRequest, timestamp), 0, 0, tb_membersize(StartMicrophoneRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartMicrophoneRequest, timeout), 0, 0, tb_membersize(StartMicrophoneRequest, timeout), 0, 0, 0, NULL},
	{65, tb_offsetof(StartMicrophoneRequest, period_ms), 0, 0, tb_membersize(StartMicrophoneRequest, period_ms), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopMicrophoneRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t StartScanRequest_fields[8] = {
	{513, tb_offsetof(StartScanRequest, timestamp), 0, 0, tb_membersize(StartScanRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartScanRequest, timeout), 0, 0, tb_membersize(StartScanRequest, timeout), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanRequest, window), 0, 0, tb_membersize(StartScanRequest, window), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanRequest, interval), 0, 0, tb_membersize(StartScanRequest, interval), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanRequest, duration), 0, 0, tb_membersize(StartScanRequest, duration), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanRequest, period), 0, 0, tb_membersize(StartScanRequest, period), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanRequest, aggregation_type), 0, 0, tb_membersize(StartScanRequest, aggregation_type), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopScanRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t StartAccelerometerRequest_fields[7] = {
	{513, tb_offsetof(StartAccelerometerRequest, timestamp), 0, 0, tb_membersize(StartAccelerometerRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartAccelerometerRequest, timeout), 0, 0, tb_membersize(StartAccelerometerRequest, timeout), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerRequest, operating_mode), 0, 0, tb_membersize(StartAccelerometerRequest, operating_mode), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerRequest, full_scale), 0, 0, tb_membersize(StartAccelerometerRequest, full_scale), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerRequest, datarate), 0, 0, tb_membersize(StartAccelerometerRequest, datarate), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerRequest, fifo_sampling_period_ms), 0, 0, tb_membersize(StartAccelerometerRequest, fifo_sampling_period_ms), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopAccelerometerRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t StartAccelerometerInterruptRequest_fields[6] = {
	{513, tb_offsetof(StartAccelerometerInterruptRequest, timestamp), 0, 0, tb_membersize(StartAccelerometerInterruptRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartAccelerometerInterruptRequest, timeout), 0, 0, tb_membersize(StartAccelerometerInterruptRequest, timeout), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerInterruptRequest, threshold_mg), 0, 0, tb_membersize(StartAccelerometerInterruptRequest, threshold_mg), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerInterruptRequest, minimal_duration_ms), 0, 0, tb_membersize(StartAccelerometerInterruptRequest, minimal_duration_ms), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerInterruptRequest, ignore_duration_ms), 0, 0, tb_membersize(StartAccelerometerInterruptRequest, ignore_duration_ms), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopAccelerometerInterruptRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t StartBatteryRequest_fields[4] = {
	{513, tb_offsetof(StartBatteryRequest, timestamp), 0, 0, tb_membersize(StartBatteryRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartBatteryRequest, timeout), 0, 0, tb_membersize(StartBatteryRequest, timeout), 0, 0, 0, NULL},
	{65, tb_offsetof(StartBatteryRequest, period_ms), 0, 0, tb_membersize(StartBatteryRequest, period_ms), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopBatteryRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t MicrophoneDataRequest_fields[2] = {
	{513, tb_offsetof(MicrophoneDataRequest, timestamp), 0, 0, tb_membersize(MicrophoneDataRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t ScanDataRequest_fields[2] = {
	{513, tb_offsetof(ScanDataRequest, timestamp), 0, 0, tb_membersize(ScanDataRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t AccelerometerDataRequest_fields[2] = {
	{513, tb_offsetof(AccelerometerDataRequest, timestamp), 0, 0, tb_membersize(AccelerometerDataRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t AccelerometerInterruptDataRequest_fields[2] = {
	{513, tb_offsetof(AccelerometerInterruptDataRequest, timestamp), 0, 0, tb_membersize(AccelerometerInterruptDataRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t BatteryDataRequest_fields[2] = {
	{513, tb_offsetof(BatteryDataRequest, timestamp), 0, 0, tb_membersize(BatteryDataRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t StartMicrophoneStreamRequest_fields[4] = {
	{513, tb_offsetof(StartMicrophoneStreamRequest, timestamp), 0, 0, tb_membersize(StartMicrophoneStreamRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartMicrophoneStreamRequest, timeout), 0, 0, tb_membersize(StartMicrophoneStreamRequest, timeout), 0, 0, 0, NULL},
	{65, tb_offsetof(StartMicrophoneStreamRequest, period_ms), 0, 0, tb_membersize(StartMicrophoneStreamRequest, period_ms), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopMicrophoneStreamRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t StartScanStreamRequest_fields[8] = {
	{513, tb_offsetof(StartScanStreamRequest, timestamp), 0, 0, tb_membersize(StartScanStreamRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartScanStreamRequest, timeout), 0, 0, tb_membersize(StartScanStreamRequest, timeout), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanStreamRequest, window), 0, 0, tb_membersize(StartScanStreamRequest, window), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanStreamRequest, interval), 0, 0, tb_membersize(StartScanStreamRequest, interval), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanStreamRequest, duration), 0, 0, tb_membersize(StartScanStreamRequest, duration), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanStreamRequest, period), 0, 0, tb_membersize(StartScanStreamRequest, period), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanStreamRequest, aggregation_type), 0, 0, tb_membersize(StartScanStreamRequest, aggregation_type), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopScanStreamRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t StartAccelerometerStreamRequest_fields[7] = {
	{513, tb_offsetof(StartAccelerometerStreamRequest, timestamp), 0, 0, tb_membersize(StartAccelerometerStreamRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartAccelerometerStreamRequest, timeout), 0, 0, tb_membersize(StartAccelerometerStreamRequest, timeout), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerStreamRequest, operating_mode), 0, 0, tb_membersize(StartAccelerometerStreamRequest, operating_mode), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerStreamRequest, full_scale), 0, 0, tb_membersize(StartAccelerometerStreamRequest, full_scale), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerStreamRequest, datarate), 0, 0, tb_membersize(StartAccelerometerStreamRequest, datarate), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerStreamRequest, fifo_sampling_period_ms), 0, 0, tb_membersize(StartAccelerometerStreamRequest, fifo_sampling_period_ms), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopAccelerometerStreamRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t StartAccelerometerInterruptStreamRequest_fields[6] = {
	{513, tb_offsetof(StartAccelerometerInterruptStreamRequest, timestamp), 0, 0, tb_membersize(StartAccelerometerInterruptStreamRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartAccelerometerInterruptStreamRequest, timeout), 0, 0, tb_membersize(StartAccelerometerInterruptStreamRequest, timeout), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerInterruptStreamRequest, threshold_mg), 0, 0, tb_membersize(StartAccelerometerInterruptStreamRequest, threshold_mg), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerInterruptStreamRequest, minimal_duration_ms), 0, 0, tb_membersize(StartAccelerometerInterruptStreamRequest, minimal_duration_ms), 0, 0, 0, NULL},
	{65, tb_offsetof(StartAccelerometerInterruptStreamRequest, ignore_duration_ms), 0, 0, tb_membersize(StartAccelerometerInterruptStreamRequest, ignore_duration_ms), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopAccelerometerInterruptStreamRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t StartBatteryStreamRequest_fields[4] = {
	{513, tb_offsetof(StartBatteryStreamRequest, timestamp), 0, 0, tb_membersize(StartBatteryStreamRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartBatteryStreamRequest, timeout), 0, 0, tb_membersize(StartBatteryStreamRequest, timeout), 0, 0, 0, NULL},
	{65, tb_offsetof(StartBatteryStreamRequest, period_ms), 0, 0, tb_membersize(StartBatteryStreamRequest, period_ms), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopBatteryStreamRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t IdentifyRequest_fields[2] = {
	{65, tb_offsetof(IdentifyRequest, timeout), 0, 0, tb_membersize(IdentifyRequest, timeout), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t TestRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t Request_fields[29] = {
	{528, tb_offsetof(Request, type.status_request), tb_delta(Request, which_type, type.status_request), 1, tb_membersize(Request, type.status_request), 0, 1, 1, &StatusRequest_fields},
	{528, tb_offsetof(Request, type.start_microphone_request), tb_delta(Request, which_type, type.start_microphone_request), 1, tb_membersize(Request, type.start_microphone_request), 0, 2, 0, &StartMicrophoneRequest_fields},
	{528, tb_offsetof(Request, type.stop_microphone_request), tb_delta(Request, which_type, type.stop_microphone_request), 1, tb_membersize(Request, type.stop_microphone_request), 0, 3, 0, &StopMicrophoneRequest_fields},
	{528, tb_offsetof(Request, type.start_scan_request), tb_delta(Request, which_type, type.start_scan_request), 1, tb_membersize(Request, type.start_scan_request), 0, 4, 0, &StartScanRequest_fields},
	{528, tb_offsetof(Request, type.stop_scan_request), tb_delta(Request, which_type, type.stop_scan_request), 1, tb_membersize(Request, type.stop_scan_request), 0, 5, 0, &StopScanRequest_fields},
	{528, tb_offsetof(Request, type.start_accelerometer_request), tb_delta(Request, which_type, type.start_accelerometer_request), 1, tb_membersize(Request, type.start_accelerometer_request), 0, 6, 0, &StartAccelerometerRequest_fields},
	{528, tb_offsetof(Request, type.stop_accelerometer_request), tb_delta(Request, which_type, type.stop_accelerometer_request), 1, tb_membersize(Request, type.stop_accelerometer_request), 0, 7, 0, &StopAccelerometerRequest_fields},
	{528, tb_offsetof(Request, type.start_accelerometer_interrupt_request), tb_delta(Request, which_type, type.start_accelerometer_interrupt_request), 1, tb_membersize(Request, type.start_accelerometer_interrupt_request), 0, 8, 0, &StartAccelerometerInterruptRequest_fields},
	{528, tb_offsetof(Request, type.stop_accelerometer_interrupt_request), tb_delta(Request, which_type, type.stop_accelerometer_interrupt_request), 1, tb_membersize(Request, type.stop_accelerometer_interrupt_request), 0, 9, 0, &StopAccelerometerInterruptRequest_fields},
	{528, tb_offsetof(Request, type.start_battery_request), tb_delta(Request, which_type, type.start_battery_request), 1, tb_membersize(Request, type.start_battery_request), 0, 10, 0, &StartBatteryRequest_fields},
	{528, tb_offsetof(Request, type.stop_battery_request), tb_delta(Request, which_type, type.stop_battery_request), 1, tb_membersize(Request, type.stop_battery_request), 0, 11, 0, &StopBatteryRequest_fields},
	{528, tb_offsetof(Request, type.microphone_data_request), tb_delta(Request, which_type, type.microphone_data_request), 1, tb_membersize(Request, type.microphone_data_request), 0, 12, 0, &MicrophoneDataRequest_fields},
	{528, tb_offsetof(Request, type.scan_data_request), tb_delta(Request, which_type, type.scan_data_request), 1, tb_membersize(Request, type.scan_data_request), 0, 13, 0, &ScanDataRequest_fields},
	{528, tb_offsetof(Request, type.accelerometer_data_request), tb_delta(Request, which_type, type.accelerometer_data_request), 1, tb_membersize(Request, type.accelerometer_data_request), 0, 14, 0, &AccelerometerDataRequest_fields},
	{528, tb_offsetof(Request, type.accelerometer_interrupt_data_request), tb_delta(Request, which_type, type.accelerometer_interrupt_data_request), 1, tb_membersize(Request, type.accelerometer_interrupt_data_request), 0, 15, 0, &AccelerometerInterruptDataRequest_fields},
	{528, tb_offsetof(Request, type.battery_data_request), tb_delta(Request, which_type, type.battery_data_request), 1, tb_membersize(Request, type.battery_data_request), 0, 16, 0, &BatteryDataRequest_fields},
	{528, tb_offsetof(Request, type.start_microphone_stream_request), tb_delta(Request, which_type, type.start_microphone_stream_request), 1, tb_membersize(Request, type.start_microphone_stream_request), 0, 17, 0, &StartMicrophoneStreamRequest_fields},
	{528, tb_offsetof(Request, type.stop_microphone_stream_request), tb_delta(Request, which_type, type.stop_microphone_stream_request), 1, tb_membersize(Request, type.stop_microphone_stream_request), 0, 18, 0, &StopMicrophoneStreamRequest_fields},
	{528, tb_offsetof(Request, type.start_scan_stream_request), tb_delta(Request, which_type, type.start_scan_stream_request), 1, tb_membersize(Request, type.start_scan_stream_request), 0, 19, 0, &StartScanStreamRequest_fields},
	{528, tb_offsetof(Request, type.stop_scan_stream_request), tb_delta(Request, which_type, type.stop_scan_stream_request), 1, tb_membersize(Request, type.stop_scan_stream_request), 0, 20, 0, &StopScanStreamRequest_fields},
	{528, tb_offsetof(Request, type.start_accelerometer_stream_request), tb_delta(Request, which_type, type.start_accelerometer_stream_request), 1, tb_membersize(Request, type.start_accelerometer_stream_request), 0, 21, 0, &StartAccelerometerStreamRequest_fields},
	{528, tb_offsetof(Request, type.stop_accelerometer_stream_request), tb_delta(Request, which_type, type.stop_accelerometer_stream_request), 1, tb_membersize(Request, type.stop_accelerometer_stream_request), 0, 22, 0, &StopAccelerometerStreamRequest_fields},
	{528, tb_offsetof(Request, type.start_accelerometer_interrupt_stream_request), tb_delta(Request, which_type, type.start_accelerometer_interrupt_stream_request), 1, tb_membersize(Request, type.start_accelerometer_interrupt_stream_request), 0, 23, 0, &StartAccelerometerInterruptStreamRequest_fields},
	{528, tb_offsetof(Request, type.stop_accelerometer_interrupt_stream_request), tb_delta(Request, which_type, type.stop_accelerometer_interrupt_stream_request), 1, tb_membersize(Request, type.stop_accelerometer_interrupt_stream_request), 0, 24, 0, &StopAccelerometerInterruptStreamRequest_fields},
	{528, tb_offsetof(Request, type.start_battery_stream_request), tb_delta(Request, which_type, type.start_battery_stream_request), 1, tb_membersize(Request, type.start_battery_stream_request), 0, 25, 0, &StartBatteryStreamRequest_fields},
	{528, tb_offsetof(Request, type.stop_battery_stream_request), tb_delta(Request, which_type, type.stop_battery_stream_request), 1, tb_membersize(Request, type.stop_battery_stream_request), 0, 26, 0, &StopBatteryStreamRequest_fields},
	{528, tb_offsetof(Request, type.identify_request), tb_delta(Request, which_type, type.identify_request), 1, tb_membersize(Request, type.identify_request), 0, 27, 0, &IdentifyRequest_fields},
	{528, tb_offsetof(Request, type.test_request), tb_delta(Request, which_type, type.test_request), 1, tb_membersize(Request, type.test_request), 0, 28, 0, &TestRequest_fields},
	TB_LAST_FIELD,
};

const tb_field_t StatusResponse_fields[9] = {
	{65, tb_offsetof(StatusResponse, clock_status), 0, 0, tb_membersize(StatusResponse, clock_status), 0, 0, 0, NULL},
	{65, tb_offsetof(StatusResponse, microphone_status), 0, 0, tb_membersize(StatusResponse, microphone_status), 0, 0, 0, NULL},
	{65, tb_offsetof(StatusResponse, scan_status), 0, 0, tb_membersize(StatusResponse, scan_status), 0, 0, 0, NULL},
	{65, tb_offsetof(StatusResponse, accelerometer_status), 0, 0, tb_membersize(StatusResponse, accelerometer_status), 0, 0, 0, NULL},
	{65, tb_offsetof(StatusResponse, accelerometer_interrupt_status), 0, 0, tb_membersize(StatusResponse, accelerometer_interrupt_status), 0, 0, 0, NULL},
	{65, tb_offsetof(StatusResponse, battery_status), 0, 0, tb_membersize(StatusResponse, battery_status), 0, 0, 0, NULL},
	{513, tb_offsetof(StatusResponse, timestamp), 0, 0, tb_membersize(StatusResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	{513, tb_offsetof(StatusResponse, battery_data), 0, 0, tb_membersize(StatusResponse, battery_data), 0, 0, 0, &BatteryData_fields},
	TB_LAST_FIELD,
};

const tb_field_t StartMicrophoneResponse_fields[2] = {
	{513, tb_offsetof(StartMicrophoneResponse, timestamp), 0, 0, tb_membersize(StartMicrophoneResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t StartScanResponse_fields[2] = {
	{513, tb_offsetof(StartScanResponse, timestamp), 0, 0, tb_membersize(StartScanResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t StartAccelerometerResponse_fields[2] = {
	{513, tb_offsetof(StartAccelerometerResponse, timestamp), 0, 0, tb_membersize(StartAccelerometerResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t StartAccelerometerInterruptResponse_fields[2] = {
	{513, tb_offsetof(StartAccelerometerInterruptResponse, timestamp), 0, 0, tb_membersize(StartAccelerometerInterruptResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t StartBatteryResponse_fields[2] = {
	{513, tb_offsetof(StartBatteryResponse, timestamp), 0, 0, tb_membersize(StartBatteryResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t MicrophoneDataResponse_fields[5] = {
	{65, tb_offsetof(MicrophoneDataResponse, last_response), 0, 0, tb_membersize(MicrophoneDataResponse, last_response), 0, 0, 0, NULL},
	{513, tb_offsetof(MicrophoneDataResponse, timestamp), 0, 0, tb_membersize(MicrophoneDataResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(MicrophoneDataResponse, sample_period_ms), 0, 0, tb_membersize(MicrophoneDataResponse, sample_period_ms), 0, 0, 0, NULL},
	{516, tb_offsetof(MicrophoneDataResponse, microphone_data), tb_delta(MicrophoneDataResponse, microphone_data_count, microphone_data), 1, tb_membersize(MicrophoneDataResponse, microphone_data[0]), tb_membersize(MicrophoneDataResponse, microphone_data)/tb_membersize(MicrophoneDataResponse, microphone_data[0]), 0, 0, &MicrophoneData_fields},
	TB_LAST_FIELD,
};

const tb_field_t ScanDataResponse_fields[4] = {
	{65, tb_offsetof(ScanDataResponse, last_response), 0, 0, tb_membersize(ScanDataResponse, last_response), 0, 0, 0, NULL},
	{513, tb_offsetof(ScanDataResponse, timestamp), 0, 0, tb_membersize(ScanDataResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	{516, tb_offsetof(ScanDataResponse, scan_result_data), tb_delta(ScanDataResponse, scan_result_data_count, scan_result_data), 1, tb_membersize(ScanDataResponse, scan_result_data[0]), tb_membersize(ScanDataResponse, scan_result_data)/tb_membersize(ScanDataResponse, scan_result_data[0]), 0, 0, &ScanResultData_fields},
	TB_LAST_FIELD,
};

const tb_field_t AccelerometerDataResponse_fields[4] = {
	{65, tb_offsetof(AccelerometerDataResponse, last_response), 0, 0, tb_membersize(AccelerometerDataResponse, last_response), 0, 0, 0, NULL},
	{513, tb_offsetof(AccelerometerDataResponse, timestamp), 0, 0, tb_membersize(AccelerometerDataResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	{516, tb_offsetof(AccelerometerDataResponse, accelerometer_data), tb_delta(AccelerometerDataResponse, accelerometer_data_count, accelerometer_data), 1, tb_membersize(AccelerometerDataResponse, accelerometer_data[0]), tb_membersize(AccelerometerDataResponse, accelerometer_data)/tb_membersize(AccelerometerDataResponse, accelerometer_data[0]), 0, 0, &AccelerometerData_fields},
	TB_LAST_FIELD,
};

const tb_field_t AccelerometerInterruptDataResponse_fields[3] = {
	{65, tb_offsetof(AccelerometerInterruptDataResponse, last_response), 0, 0, tb_membersize(AccelerometerInterruptDataResponse, last_response), 0, 0, 0, NULL},
	{513, tb_offsetof(AccelerometerInterruptDataResponse, timestamp), 0, 0, tb_membersize(AccelerometerInterruptDataResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t BatteryDataResponse_fields[4] = {
	{65, tb_offsetof(BatteryDataResponse, last_response), 0, 0, tb_membersize(BatteryDataResponse, last_response), 0, 0, 0, NULL},
	{513, tb_offsetof(BatteryDataResponse, timestamp), 0, 0, tb_membersize(BatteryDataResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	{513, tb_offsetof(BatteryDataResponse, battery_data), 0, 0, tb_membersize(BatteryDataResponse, battery_data), 0, 0, 0, &BatteryData_fields},
	TB_LAST_FIELD,
};

const tb_field_t StreamResponse_fields[7] = {
	{513, tb_offsetof(StreamResponse, timestamp), 0, 0, tb_membersize(StreamResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	{516, tb_offsetof(StreamResponse, battery_stream), tb_delta(StreamResponse, battery_stream_count, battery_stream), 1, tb_membersize(StreamResponse, battery_stream[0]), tb_membersize(StreamResponse, battery_stream)/tb_membersize(StreamResponse, battery_stream[0]), 0, 0, &BatteryStream_fields},
	{516, tb_offsetof(StreamResponse, microphone_stream), tb_delta(StreamResponse, microphone_stream_count, microphone_stream), 1, tb_membersize(StreamResponse, microphone_stream[0]), tb_membersize(StreamResponse, microphone_stream)/tb_membersize(StreamResponse, microphone_stream[0]), 0, 0, &MicrophoneStream_fields},
	{516, tb_offsetof(StreamResponse, scan_stream), tb_delta(StreamResponse, scan_stream_count, scan_stream), 1, tb_membersize(StreamResponse, scan_stream[0]), tb_membersize(StreamResponse, scan_stream)/tb_membersize(StreamResponse, scan_stream[0]), 0, 0, &ScanStream_fields},
	{516, tb_offsetof(StreamResponse, accelerometer_stream), tb_delta(StreamResponse, accelerometer_stream_count, accelerometer_stream), 1, tb_membersize(StreamResponse, accelerometer_stream[0]), tb_membersize(StreamResponse, accelerometer_stream)/tb_membersize(StreamResponse, accelerometer_stream[0]), 0, 0, &AccelerometerStream_fields},
	{516, tb_offsetof(StreamResponse, accelerometer_interrupt_stream), tb_delta(StreamResponse, accelerometer_interrupt_stream_count, accelerometer_interrupt_stream), 1, tb_membersize(StreamResponse, accelerometer_interrupt_stream[0]), tb_membersize(StreamResponse, accelerometer_interrupt_stream)/tb_membersize(StreamResponse, accelerometer_interrupt_stream[0]), 0, 0, &AccelerometerInterruptStream_fields},
	TB_LAST_FIELD,
};

const tb_field_t TestResponse_fields[2] = {
	{65, tb_offsetof(TestResponse, test_passed), 0, 0, tb_membersize(TestResponse, test_passed), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t Response_fields[14] = {
	{528, tb_offsetof(Response, type.status_response), tb_delta(Response, which_type, type.status_response), 1, tb_membersize(Response, type.status_response), 0, 1, 1, &StatusResponse_fields},
	{528, tb_offsetof(Response, type.start_microphone_response), tb_delta(Response, which_type, type.start_microphone_response), 1, tb_membersize(Response, type.start_microphone_response), 0, 2, 0, &StartMicrophoneResponse_fields},
	{528, tb_offsetof(Response, type.start_scan_response), tb_delta(Response, which_type, type.start_scan_response), 1, tb_membersize(Response, type.start_scan_response), 0, 3, 0, &StartScanResponse_fields},
	{528, tb_offsetof(Response, type.start_accelerometer_response), tb_delta(Response, which_type, type.start_accelerometer_response), 1, tb_membersize(Response, type.start_accelerometer_response), 0, 4, 0, &StartAccelerometerResponse_fields},
	{528, tb_offsetof(Response, type.start_accelerometer_interrupt_response), tb_delta(Response, which_type, type.start_accelerometer_interrupt_response), 1, tb_membersize(Response, type.start_accelerometer_interrupt_response), 0, 5, 0, &StartAccelerometerInterruptResponse_fields},
	{528, tb_offsetof(Response, type.start_battery_response), tb_delta(Response, which_type, type.start_battery_response), 1, tb_membersize(Response, type.start_battery_response), 0, 6, 0, &StartBatteryResponse_fields},
	{528, tb_offsetof(Response, type.microphone_data_response), tb_delta(Response, which_type, type.microphone_data_response), 1, tb_membersize(Response, type.microphone_data_response), 0, 7, 0, &MicrophoneDataResponse_fields},
	{528, tb_offsetof(Response, type.scan_data_response), tb_delta(Response, which_type, type.scan_data_response), 1, tb_membersize(Response, type.scan_data_response), 0, 8, 0, &ScanDataResponse_fields},
	{528, tb_offsetof(Response, type.accelerometer_data_response), tb_delta(Response, which_type, type.accelerometer_data_response), 1, tb_membersize(Response, type.accelerometer_data_response), 0, 9, 0, &AccelerometerDataResponse_fields},
	{528, tb_offsetof(Response, type.accelerometer_interrupt_data_response), tb_delta(Response, which_type, type.accelerometer_interrupt_data_response), 1, tb_membersize(Response, type.accelerometer_interrupt_data_response), 0, 10, 0, &AccelerometerInterruptDataResponse_fields},
	{528, tb_offsetof(Response, type.battery_data_response), tb_delta(Response, which_type, type.battery_data_response), 1, tb_membersize(Response, type.battery_data_response), 0, 11, 0, &BatteryDataResponse_fields},
	{528, tb_offsetof(Response, type.stream_response), tb_delta(Response, which_type, type.stream_response), 1, tb_membersize(Response, type.stream_response), 0, 12, 0, &StreamResponse_fields},
	{528, tb_offsetof(Response, type.test_response), tb_delta(Response, which_type, type.test_response), 1, tb_membersize(Response, type.test_response), 0, 13, 0, &TestResponse_fields},
	TB_LAST_FIELD,
};

#endif