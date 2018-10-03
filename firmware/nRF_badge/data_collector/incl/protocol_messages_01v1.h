#ifndef __PROTOCOL_MESSAGES_01V1_H
#define __PROTOCOL_MESSAGES_01V1_H

#include <stdint.h>
#include "tinybuf.h"
#include "common_messages.h"

#define PROTOCOL_MICROPHONE_DATA_SIZE 114
#define PROTOCOL_SCAN_DATA_SIZE 29

#define Request_status_request_tag 115
#define Request_status_assign_request_tag 83
#define Request_start_microphone_request_tag 49
#define Request_stop_microphone_request_tag 48
#define Request_start_scan_request_tag 112
#define Request_stop_scan_request_tag 113
#define Request_microphone_data_request_tag 114
#define Request_scan_data_request_tag 98
#define Request_identify_request_tag 105
#define Response_status_response_tag 1
#define Response_start_microphone_response_tag 2
#define Response_stop_microphone_response_tag 3
#define Response_start_scan_response_tag 4
#define Response_stop_scan_response_tag 5
#define Response_microphone_data_response_tag 6
#define Response_scan_data_response_tag 7
#define Response_identify_response_tag 8

typedef struct {
	Timestamp timestamp;
} StatusRequest;

typedef struct {
	Timestamp timestamp;
	BadgeAssignement badge_assignement;
} StatusAssignRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
} StartMicrophoneRequest;

typedef struct {
} StopMicrophoneRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
	uint16_t window;
	uint16_t interval;
	uint16_t duration;
	uint16_t period;
} StartScanRequest;

typedef struct {
} StopScanRequest;

typedef struct {
	Timestamp timestamp;
} MicrophoneDataRequest;

typedef struct {
	uint32_t seconds;
} ScanDataRequest;

typedef struct {
	uint16_t timeout;
} IdentifyRequest;

typedef struct {
	uint8_t which_type;
	union {
		StatusRequest status_request;
		StatusAssignRequest status_assign_request;
		StartMicrophoneRequest start_microphone_request;
		StopMicrophoneRequest stop_microphone_request;
		StartScanRequest start_scan_request;
		StopScanRequest stop_scan_request;
		MicrophoneDataRequest microphone_data_request;
		ScanDataRequest scan_data_request;
		IdentifyRequest identify_request;
	} type;
} Request;

typedef struct {
	uint8_t clock_status;
	uint8_t scan_status;
	uint8_t collector_status;
	Timestamp timestamp;
	BatteryData battery_data;
} StatusResponse;

typedef struct {
	Timestamp timestamp;
} StartMicrophoneResponse;

typedef struct {
} StopMicrophoneResponse;

typedef struct {
	Timestamp timestamp;
} StartScanResponse;

typedef struct {
} StopScanResponse;

typedef struct {
	Timestamp timestamp;
	BatteryData battery_data;
	uint16_t sample_period_ms;
} MicrophoneDataResponseHeader;

typedef struct {
	MicrophoneDataResponseHeader microphone_data_response_header;
	uint8_t microphone_data_count;
	MicrophoneData microphone_data[114];
} MicrophoneDataResponse;

typedef struct {
	uint32_t timestamp_seconds;
	BatteryData battery_data;
} ScanDataResponseHeader;

typedef struct {
	ScanDataResponseHeader scan_data_response_header;
	uint8_t scan_result_data_count;
	ScanResultData scan_result_data[29];
} ScanDataResponse;

typedef struct {
} IdentifyResponse;

typedef struct {
	uint8_t which_type;
	union {
		StatusResponse status_response;
		StartMicrophoneResponse start_microphone_response;
		StopMicrophoneResponse stop_microphone_response;
		StartScanResponse start_scan_response;
		StopScanResponse stop_scan_response;
		MicrophoneDataResponse microphone_data_response;
		ScanDataResponse scan_data_response;
		IdentifyResponse identify_response;
	} type;
} Response;

extern const tb_field_t StatusRequest_fields[2];
extern const tb_field_t StatusAssignRequest_fields[3];
extern const tb_field_t StartMicrophoneRequest_fields[3];
extern const tb_field_t StopMicrophoneRequest_fields[1];
extern const tb_field_t StartScanRequest_fields[7];
extern const tb_field_t StopScanRequest_fields[1];
extern const tb_field_t MicrophoneDataRequest_fields[2];
extern const tb_field_t ScanDataRequest_fields[2];
extern const tb_field_t IdentifyRequest_fields[2];
extern const tb_field_t Request_fields[10];
extern const tb_field_t StatusResponse_fields[6];
extern const tb_field_t StartMicrophoneResponse_fields[2];
extern const tb_field_t StopMicrophoneResponse_fields[1];
extern const tb_field_t StartScanResponse_fields[2];
extern const tb_field_t StopScanResponse_fields[1];
extern const tb_field_t MicrophoneDataResponseHeader_fields[4];
extern const tb_field_t MicrophoneDataResponse_fields[3];
extern const tb_field_t ScanDataResponseHeader_fields[3];
extern const tb_field_t ScanDataResponse_fields[3];
extern const tb_field_t IdentifyResponse_fields[1];
extern const tb_field_t Response_fields[9];

#endif
