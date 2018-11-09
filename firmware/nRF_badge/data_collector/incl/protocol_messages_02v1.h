#ifndef __PROTOCOL_MESSAGES_02V1_H
#define __PROTOCOL_MESSAGES_02V1_H

#include <stdint.h>
#include "tinybuf.h"
#include "common_messages.h"
#include "stream_messages.h"

#define PROTOCOL_MICROPHONE_DATA_SIZE 114
#define PROTOCOL_SCAN_DATA_SIZE 29
#define PROTOCOL_ACCELEROMETER_DATA_SIZE 100
#define PROTOCOL_MICROPHONE_STREAM_SIZE 10
#define PROTOCOL_SCAN_STREAM_SIZE 10
#define PROTOCOL_ACCELEROMETER_STREAM_SIZE 10
#define PROTOCOL_ACCELEROMETER_INTERRUPT_STREAM_SIZE 10
#define PROTOCOL_BATTERY_STREAM_SIZE 10

#define Request_status_request_tag 1
#define Request_start_microphone_request_tag 2
#define Request_stop_microphone_request_tag 3
#define Request_start_scan_request_tag 4
#define Request_stop_scan_request_tag 5
#define Request_start_accelerometer_request_tag 6
#define Request_stop_accelerometer_request_tag 7
#define Request_start_accelerometer_interrupt_request_tag 8
#define Request_stop_accelerometer_interrupt_request_tag 9
#define Request_start_battery_request_tag 10
#define Request_stop_battery_request_tag 11
#define Request_microphone_data_request_tag 12
#define Request_scan_data_request_tag 13
#define Request_accelerometer_data_request_tag 14
#define Request_accelerometer_interrupt_data_request_tag 15
#define Request_battery_data_request_tag 16
#define Request_start_microphone_stream_request_tag 17
#define Request_stop_microphone_stream_request_tag 18
#define Request_start_scan_stream_request_tag 19
#define Request_stop_scan_stream_request_tag 20
#define Request_start_accelerometer_stream_request_tag 21
#define Request_stop_accelerometer_stream_request_tag 22
#define Request_start_accelerometer_interrupt_stream_request_tag 23
#define Request_stop_accelerometer_interrupt_stream_request_tag 24
#define Request_start_battery_stream_request_tag 25
#define Request_stop_battery_stream_request_tag 26
#define Request_identify_request_tag 27
#define Request_test_request_tag 28
#define Request_restart_request_tag 29
#define Response_status_response_tag 1
#define Response_start_microphone_response_tag 2
#define Response_start_scan_response_tag 3
#define Response_start_accelerometer_response_tag 4
#define Response_start_accelerometer_interrupt_response_tag 5
#define Response_start_battery_response_tag 6
#define Response_microphone_data_response_tag 7
#define Response_scan_data_response_tag 8
#define Response_accelerometer_data_response_tag 9
#define Response_accelerometer_interrupt_data_response_tag 10
#define Response_battery_data_response_tag 11
#define Response_stream_response_tag 12
#define Response_test_response_tag 13

typedef struct {
	Timestamp timestamp;
	uint8_t has_badge_assignement;
	BadgeAssignement badge_assignement;
} StatusRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
	uint16_t period_ms;
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
	uint8_t aggregation_type;
} StartScanRequest;

typedef struct {
} StopScanRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
	uint8_t operating_mode;
	uint8_t full_scale;
	uint16_t datarate;
	uint16_t fifo_sampling_period_ms;
} StartAccelerometerRequest;

typedef struct {
} StopAccelerometerRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
	uint16_t threshold_mg;
	uint16_t minimal_duration_ms;
	uint32_t ignore_duration_ms;
} StartAccelerometerInterruptRequest;

typedef struct {
} StopAccelerometerInterruptRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
	uint32_t period_ms;
} StartBatteryRequest;

typedef struct {
} StopBatteryRequest;

typedef struct {
	Timestamp timestamp;
} MicrophoneDataRequest;

typedef struct {
	Timestamp timestamp;
} ScanDataRequest;

typedef struct {
	Timestamp timestamp;
} AccelerometerDataRequest;

typedef struct {
	Timestamp timestamp;
} AccelerometerInterruptDataRequest;

typedef struct {
	Timestamp timestamp;
} BatteryDataRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
	uint16_t period_ms;
} StartMicrophoneStreamRequest;

typedef struct {
} StopMicrophoneStreamRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
	uint16_t window;
	uint16_t interval;
	uint16_t duration;
	uint16_t period;
	uint8_t aggregation_type;
} StartScanStreamRequest;

typedef struct {
} StopScanStreamRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
	uint8_t operating_mode;
	uint8_t full_scale;
	uint16_t datarate;
	uint16_t fifo_sampling_period_ms;
} StartAccelerometerStreamRequest;

typedef struct {
} StopAccelerometerStreamRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
	uint16_t threshold_mg;
	uint16_t minimal_duration_ms;
	uint32_t ignore_duration_ms;
} StartAccelerometerInterruptStreamRequest;

typedef struct {
} StopAccelerometerInterruptStreamRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
	uint32_t period_ms;
} StartBatteryStreamRequest;

typedef struct {
} StopBatteryStreamRequest;

typedef struct {
	uint16_t timeout;
} IdentifyRequest;

typedef struct {
} TestRequest;

typedef struct {
} RestartRequest;

typedef struct {
	uint8_t which_type;
	union {
		StatusRequest status_request;
		StartMicrophoneRequest start_microphone_request;
		StopMicrophoneRequest stop_microphone_request;
		StartScanRequest start_scan_request;
		StopScanRequest stop_scan_request;
		StartAccelerometerRequest start_accelerometer_request;
		StopAccelerometerRequest stop_accelerometer_request;
		StartAccelerometerInterruptRequest start_accelerometer_interrupt_request;
		StopAccelerometerInterruptRequest stop_accelerometer_interrupt_request;
		StartBatteryRequest start_battery_request;
		StopBatteryRequest stop_battery_request;
		MicrophoneDataRequest microphone_data_request;
		ScanDataRequest scan_data_request;
		AccelerometerDataRequest accelerometer_data_request;
		AccelerometerInterruptDataRequest accelerometer_interrupt_data_request;
		BatteryDataRequest battery_data_request;
		StartMicrophoneStreamRequest start_microphone_stream_request;
		StopMicrophoneStreamRequest stop_microphone_stream_request;
		StartScanStreamRequest start_scan_stream_request;
		StopScanStreamRequest stop_scan_stream_request;
		StartAccelerometerStreamRequest start_accelerometer_stream_request;
		StopAccelerometerStreamRequest stop_accelerometer_stream_request;
		StartAccelerometerInterruptStreamRequest start_accelerometer_interrupt_stream_request;
		StopAccelerometerInterruptStreamRequest stop_accelerometer_interrupt_stream_request;
		StartBatteryStreamRequest start_battery_stream_request;
		StopBatteryStreamRequest stop_battery_stream_request;
		IdentifyRequest identify_request;
		TestRequest test_request;
		RestartRequest restart_request;
	} type;
} Request;

typedef struct {
	uint8_t clock_status;
	uint8_t microphone_status;
	uint8_t scan_status;
	uint8_t accelerometer_status;
	uint8_t accelerometer_interrupt_status;
	uint8_t battery_status;
	Timestamp timestamp;
	BatteryData battery_data;
} StatusResponse;

typedef struct {
	Timestamp timestamp;
} StartMicrophoneResponse;

typedef struct {
	Timestamp timestamp;
} StartScanResponse;

typedef struct {
	Timestamp timestamp;
} StartAccelerometerResponse;

typedef struct {
	Timestamp timestamp;
} StartAccelerometerInterruptResponse;

typedef struct {
	Timestamp timestamp;
} StartBatteryResponse;

typedef struct {
	uint8_t last_response;
	Timestamp timestamp;
	uint16_t sample_period_ms;
	uint8_t microphone_data_count;
	MicrophoneData microphone_data[114];
} MicrophoneDataResponse;

typedef struct {
	uint8_t last_response;
	Timestamp timestamp;
	uint8_t scan_result_data_count;
	ScanResultData scan_result_data[29];
} ScanDataResponse;

typedef struct {
	uint8_t last_response;
	Timestamp timestamp;
	uint8_t accelerometer_data_count;
	AccelerometerData accelerometer_data[100];
} AccelerometerDataResponse;

typedef struct {
	uint8_t last_response;
	Timestamp timestamp;
} AccelerometerInterruptDataResponse;

typedef struct {
	uint8_t last_response;
	Timestamp timestamp;
	BatteryData battery_data;
} BatteryDataResponse;

typedef struct {
	Timestamp timestamp;
	uint8_t battery_stream_count;
	BatteryStream battery_stream[10];
	uint8_t microphone_stream_count;
	MicrophoneStream microphone_stream[10];
	uint8_t scan_stream_count;
	ScanStream scan_stream[10];
	uint8_t accelerometer_stream_count;
	AccelerometerStream accelerometer_stream[10];
	uint8_t accelerometer_interrupt_stream_count;
	AccelerometerInterruptStream accelerometer_interrupt_stream[10];
} StreamResponse;

typedef struct {
	uint8_t test_failed;
} TestResponse;

typedef struct {
	uint8_t which_type;
	union {
		StatusResponse status_response;
		StartMicrophoneResponse start_microphone_response;
		StartScanResponse start_scan_response;
		StartAccelerometerResponse start_accelerometer_response;
		StartAccelerometerInterruptResponse start_accelerometer_interrupt_response;
		StartBatteryResponse start_battery_response;
		MicrophoneDataResponse microphone_data_response;
		ScanDataResponse scan_data_response;
		AccelerometerDataResponse accelerometer_data_response;
		AccelerometerInterruptDataResponse accelerometer_interrupt_data_response;
		BatteryDataResponse battery_data_response;
		StreamResponse stream_response;
		TestResponse test_response;
	} type;
} Response;

extern const tb_field_t StatusRequest_fields[3];
extern const tb_field_t StartMicrophoneRequest_fields[4];
extern const tb_field_t StopMicrophoneRequest_fields[1];
extern const tb_field_t StartScanRequest_fields[8];
extern const tb_field_t StopScanRequest_fields[1];
extern const tb_field_t StartAccelerometerRequest_fields[7];
extern const tb_field_t StopAccelerometerRequest_fields[1];
extern const tb_field_t StartAccelerometerInterruptRequest_fields[6];
extern const tb_field_t StopAccelerometerInterruptRequest_fields[1];
extern const tb_field_t StartBatteryRequest_fields[4];
extern const tb_field_t StopBatteryRequest_fields[1];
extern const tb_field_t MicrophoneDataRequest_fields[2];
extern const tb_field_t ScanDataRequest_fields[2];
extern const tb_field_t AccelerometerDataRequest_fields[2];
extern const tb_field_t AccelerometerInterruptDataRequest_fields[2];
extern const tb_field_t BatteryDataRequest_fields[2];
extern const tb_field_t StartMicrophoneStreamRequest_fields[4];
extern const tb_field_t StopMicrophoneStreamRequest_fields[1];
extern const tb_field_t StartScanStreamRequest_fields[8];
extern const tb_field_t StopScanStreamRequest_fields[1];
extern const tb_field_t StartAccelerometerStreamRequest_fields[7];
extern const tb_field_t StopAccelerometerStreamRequest_fields[1];
extern const tb_field_t StartAccelerometerInterruptStreamRequest_fields[6];
extern const tb_field_t StopAccelerometerInterruptStreamRequest_fields[1];
extern const tb_field_t StartBatteryStreamRequest_fields[4];
extern const tb_field_t StopBatteryStreamRequest_fields[1];
extern const tb_field_t IdentifyRequest_fields[2];
extern const tb_field_t TestRequest_fields[1];
extern const tb_field_t RestartRequest_fields[1];
extern const tb_field_t Request_fields[30];
extern const tb_field_t StatusResponse_fields[9];
extern const tb_field_t StartMicrophoneResponse_fields[2];
extern const tb_field_t StartScanResponse_fields[2];
extern const tb_field_t StartAccelerometerResponse_fields[2];
extern const tb_field_t StartAccelerometerInterruptResponse_fields[2];
extern const tb_field_t StartBatteryResponse_fields[2];
extern const tb_field_t MicrophoneDataResponse_fields[5];
extern const tb_field_t ScanDataResponse_fields[4];
extern const tb_field_t AccelerometerDataResponse_fields[4];
extern const tb_field_t AccelerometerInterruptDataResponse_fields[3];
extern const tb_field_t BatteryDataResponse_fields[4];
extern const tb_field_t StreamResponse_fields[7];
extern const tb_field_t TestResponse_fields[2];
extern const tb_field_t Response_fields[14];

#endif
