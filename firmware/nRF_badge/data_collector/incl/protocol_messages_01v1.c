#include "tinybuf.h"
#include "protocol_messages_01v1.h"

//#define PROTOCOL_01v1
#ifdef PROTOCOL_01v1

const tb_field_t StatusRequest_fields[2] = {
	{513, tb_offsetof(StatusRequest, timestamp), 0, 0, tb_membersize(StatusRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t StatusAssignRequest_fields[3] = {
	{513, tb_offsetof(StatusAssignRequest, timestamp), 0, 0, tb_membersize(StatusAssignRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{513, tb_offsetof(StatusAssignRequest, badge_assignement), 0, 0, tb_membersize(StatusAssignRequest, badge_assignement), 0, 0, 0, &BadgeAssignement_fields},
	TB_LAST_FIELD,
};

const tb_field_t StartMicrophoneRequest_fields[3] = {
	{513, tb_offsetof(StartMicrophoneRequest, timestamp), 0, 0, tb_membersize(StartMicrophoneRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartMicrophoneRequest, timeout), 0, 0, tb_membersize(StartMicrophoneRequest, timeout), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopMicrophoneRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t StartScanRequest_fields[7] = {
	{513, tb_offsetof(StartScanRequest, timestamp), 0, 0, tb_membersize(StartScanRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	{65, tb_offsetof(StartScanRequest, timeout), 0, 0, tb_membersize(StartScanRequest, timeout), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanRequest, window), 0, 0, tb_membersize(StartScanRequest, window), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanRequest, interval), 0, 0, tb_membersize(StartScanRequest, interval), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanRequest, duration), 0, 0, tb_membersize(StartScanRequest, duration), 0, 0, 0, NULL},
	{65, tb_offsetof(StartScanRequest, period), 0, 0, tb_membersize(StartScanRequest, period), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t StopScanRequest_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t MicrophoneDataRequest_fields[2] = {
	{513, tb_offsetof(MicrophoneDataRequest, timestamp), 0, 0, tb_membersize(MicrophoneDataRequest, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t ScanDataRequest_fields[2] = {
	{65, tb_offsetof(ScanDataRequest, seconds), 0, 0, tb_membersize(ScanDataRequest, seconds), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t IdentifyRequest_fields[2] = {
	{65, tb_offsetof(IdentifyRequest, timeout), 0, 0, tb_membersize(IdentifyRequest, timeout), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t Request_fields[10] = {
	{528, tb_offsetof(Request, type.status_request), tb_delta(Request, which_type, type.status_request), 1, tb_membersize(Request, type.status_request), 0, 115, 1, &StatusRequest_fields},
	{528, tb_offsetof(Request, type.status_assign_request), tb_delta(Request, which_type, type.status_assign_request), 1, tb_membersize(Request, type.status_assign_request), 0, 83, 0, &StatusAssignRequest_fields},
	{528, tb_offsetof(Request, type.start_microphone_request), tb_delta(Request, which_type, type.start_microphone_request), 1, tb_membersize(Request, type.start_microphone_request), 0, 49, 0, &StartMicrophoneRequest_fields},
	{528, tb_offsetof(Request, type.stop_microphone_request), tb_delta(Request, which_type, type.stop_microphone_request), 1, tb_membersize(Request, type.stop_microphone_request), 0, 48, 0, &StopMicrophoneRequest_fields},
	{528, tb_offsetof(Request, type.start_scan_request), tb_delta(Request, which_type, type.start_scan_request), 1, tb_membersize(Request, type.start_scan_request), 0, 112, 0, &StartScanRequest_fields},
	{528, tb_offsetof(Request, type.stop_scan_request), tb_delta(Request, which_type, type.stop_scan_request), 1, tb_membersize(Request, type.stop_scan_request), 0, 113, 0, &StopScanRequest_fields},
	{528, tb_offsetof(Request, type.microphone_data_request), tb_delta(Request, which_type, type.microphone_data_request), 1, tb_membersize(Request, type.microphone_data_request), 0, 114, 0, &MicrophoneDataRequest_fields},
	{528, tb_offsetof(Request, type.scan_data_request), tb_delta(Request, which_type, type.scan_data_request), 1, tb_membersize(Request, type.scan_data_request), 0, 98, 0, &ScanDataRequest_fields},
	{528, tb_offsetof(Request, type.identify_request), tb_delta(Request, which_type, type.identify_request), 1, tb_membersize(Request, type.identify_request), 0, 105, 0, &IdentifyRequest_fields},
	TB_LAST_FIELD,
};

const tb_field_t StatusResponse_fields[6] = {
	{65, tb_offsetof(StatusResponse, clock_status), 0, 0, tb_membersize(StatusResponse, clock_status), 0, 0, 0, NULL},
	{65, tb_offsetof(StatusResponse, scan_status), 0, 0, tb_membersize(StatusResponse, scan_status), 0, 0, 0, NULL},
	{65, tb_offsetof(StatusResponse, collector_status), 0, 0, tb_membersize(StatusResponse, collector_status), 0, 0, 0, NULL},
	{513, tb_offsetof(StatusResponse, timestamp), 0, 0, tb_membersize(StatusResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	{513, tb_offsetof(StatusResponse, battery_data), 0, 0, tb_membersize(StatusResponse, battery_data), 0, 0, 0, &BatteryData_fields},
	TB_LAST_FIELD,
};

const tb_field_t StartMicrophoneResponse_fields[2] = {
	{513, tb_offsetof(StartMicrophoneResponse, timestamp), 0, 0, tb_membersize(StartMicrophoneResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t StopMicrophoneResponse_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t StartScanResponse_fields[2] = {
	{513, tb_offsetof(StartScanResponse, timestamp), 0, 0, tb_membersize(StartScanResponse, timestamp), 0, 0, 0, &Timestamp_fields},
	TB_LAST_FIELD,
};

const tb_field_t StopScanResponse_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t MicrophoneDataResponseHeader_fields[4] = {
	{513, tb_offsetof(MicrophoneDataResponseHeader, timestamp), 0, 0, tb_membersize(MicrophoneDataResponseHeader, timestamp), 0, 0, 0, &Timestamp_fields},
	{513, tb_offsetof(MicrophoneDataResponseHeader, battery_data), 0, 0, tb_membersize(MicrophoneDataResponseHeader, battery_data), 0, 0, 0, &BatteryData_fields},
	{65, tb_offsetof(MicrophoneDataResponseHeader, sample_period_ms), 0, 0, tb_membersize(MicrophoneDataResponseHeader, sample_period_ms), 0, 0, 0, NULL},
	TB_LAST_FIELD,
};

const tb_field_t MicrophoneDataResponse_fields[3] = {
	{513, tb_offsetof(MicrophoneDataResponse, microphone_data_response_header), 0, 0, tb_membersize(MicrophoneDataResponse, microphone_data_response_header), 0, 0, 0, &MicrophoneDataResponseHeader_fields},
	{516, tb_offsetof(MicrophoneDataResponse, microphone_data), tb_delta(MicrophoneDataResponse, microphone_data_count, microphone_data), 1, tb_membersize(MicrophoneDataResponse, microphone_data[0]), tb_membersize(MicrophoneDataResponse, microphone_data)/tb_membersize(MicrophoneDataResponse, microphone_data[0]), 0, 0, &MicrophoneData_fields},
	TB_LAST_FIELD,
};

const tb_field_t ScanDataResponseHeader_fields[3] = {
	{65, tb_offsetof(ScanDataResponseHeader, timestamp_seconds), 0, 0, tb_membersize(ScanDataResponseHeader, timestamp_seconds), 0, 0, 0, NULL},
	{513, tb_offsetof(ScanDataResponseHeader, battery_data), 0, 0, tb_membersize(ScanDataResponseHeader, battery_data), 0, 0, 0, &BatteryData_fields},
	TB_LAST_FIELD,
};

const tb_field_t ScanDataResponse_fields[3] = {
	{513, tb_offsetof(ScanDataResponse, scan_data_response_header), 0, 0, tb_membersize(ScanDataResponse, scan_data_response_header), 0, 0, 0, &ScanDataResponseHeader_fields},
	{516, tb_offsetof(ScanDataResponse, scan_result_data), tb_delta(ScanDataResponse, scan_result_data_count, scan_result_data), 1, tb_membersize(ScanDataResponse, scan_result_data[0]), tb_membersize(ScanDataResponse, scan_result_data)/tb_membersize(ScanDataResponse, scan_result_data[0]), 0, 0, &ScanResultData_fields},
	TB_LAST_FIELD,
};

const tb_field_t IdentifyResponse_fields[1] = {
	TB_LAST_FIELD,
};

const tb_field_t Response_fields[9] = {
	{528, tb_offsetof(Response, type.status_response), tb_delta(Response, which_type, type.status_response), 1, tb_membersize(Response, type.status_response), 0, 1, 1, &StatusResponse_fields},
	{528, tb_offsetof(Response, type.start_microphone_response), tb_delta(Response, which_type, type.start_microphone_response), 1, tb_membersize(Response, type.start_microphone_response), 0, 2, 0, &StartMicrophoneResponse_fields},
	{528, tb_offsetof(Response, type.stop_microphone_response), tb_delta(Response, which_type, type.stop_microphone_response), 1, tb_membersize(Response, type.stop_microphone_response), 0, 3, 0, &StopMicrophoneResponse_fields},
	{528, tb_offsetof(Response, type.start_scan_response), tb_delta(Response, which_type, type.start_scan_response), 1, tb_membersize(Response, type.start_scan_response), 0, 4, 0, &StartScanResponse_fields},
	{528, tb_offsetof(Response, type.stop_scan_response), tb_delta(Response, which_type, type.stop_scan_response), 1, tb_membersize(Response, type.stop_scan_response), 0, 5, 0, &StopScanResponse_fields},
	{528, tb_offsetof(Response, type.microphone_data_response), tb_delta(Response, which_type, type.microphone_data_response), 1, tb_membersize(Response, type.microphone_data_response), 0, 6, 0, &MicrophoneDataResponse_fields},
	{528, tb_offsetof(Response, type.scan_data_response), tb_delta(Response, which_type, type.scan_data_response), 1, tb_membersize(Response, type.scan_data_response), 0, 7, 0, &ScanDataResponse_fields},
	{528, tb_offsetof(Response, type.identify_response), tb_delta(Response, which_type, type.identify_response), 1, tb_membersize(Response, type.identify_response), 0, 8, 0, &IdentifyResponse_fields},
	TB_LAST_FIELD,
};

#endif