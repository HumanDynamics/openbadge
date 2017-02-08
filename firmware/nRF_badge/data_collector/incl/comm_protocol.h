//
// Created by Andrew Bartow on 2/2/17.
//

#ifndef OPENBADGE_BLE_COMMAND_H
#define OPENBADGE_BLE_COMMAND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "audio_chunks.h"
#include "proximity_chunks.h"
#include "group_assignment.h"
#include "timestamp.h"

typedef enum {
    STATUS_REQUEST,
    START_RECORD_REQUEST,
    STOP_RECORD_REQUEST,
    START_SCAN_REQUEST,
    STOP_SCAN_REQUEST,
    GET_AUDIO_DATA_REQUEST,
    GET_PROXIMITY_DATA_REQUEST,
    IDENTIFY_REQUEST,
} RequestType_t;

typedef struct {
    Timestamp_t request_time;
    bool has_new_assignment;
    GroupAssignment_t new_assignment;
} StatusRequest_t;

typedef struct {
    Timestamp_t request_time;
    uint16_t timeout_mins;
} StartRecordRequest_t;

// No payload data on StopRecordRequest.

typedef struct {
    Timestamp_t request_time;
    uint16_t timeout_mins;
    uint16_t window_millis;
    uint16_t interval_millis;
    uint16_t duration_seconds;
    uint16_t period_seconds;
} StartScanRequest_t;

// No payload data on StopScanningRequest.

typedef struct {
    Timestamp_t send_since;
} AudioDataRequest_t;

typedef struct {
    Timestamp_t send_since;
} ProximityDataRequest_t;

typedef struct {
    uint16_t timeout_seconds;
} IdentifyRequest_t;

typedef struct {
    RequestType_t type;

    // At most one of these members will be set, the one that has the payload data for this request type.
    StatusRequest_t status;
    StartRecordRequest_t start_record;
    StartScanRequest_t start_scan;
    AudioDataRequest_t fetch_audio_data;
    ProximityDataRequest_t fetch_proximity_data;
    IdentifyRequest_t identify;
} Request_t;

typedef enum {
    STATUS_RESPONSE,
    START_RECORD_RESPONSE,
    START_SCAN_RESPONSE,
    GET_AUDIO_DATA_RESPONSE,
    GET_PROXIMITY_DATA_RESPONSE,
} ResponseType_t;

typedef struct {
    Timestamp_t response_time;
    float battery_voltage;
    bool is_synced;
    bool is_scanning;
    bool is_recording;
} StatusResponse_t;

typedef struct {
    Timestamp_t response_time;
} StartRecordResponse_t;

typedef struct {
    Timestamp_t response_time;
} StartScanResponse_t;

typedef struct {
    // Marks whether this AudioDataResponse_t represents the special "end of response" packet.
    // If true, audio_data will be ignored.
    bool is_end_of_response;
    uint16_t sample_period_ms;
    AudioChunk_t audio_data;
} AudioDataResponse_t;

typedef struct {
    // Marks whether this ProximityDataResponse_t represents the special "end of response" packet.
    // If true, proximity_data will be ignored.
    bool is_end_of_response;
    ProximityChunk_t proximity_data;
} ProximityDataResponse_t;

typedef struct {
    ResponseType_t type;

    // At most one of these members will be set, the one that has the payload data for the response type.
    StatusResponse_t status;
    StartRecordResponse_t start_record;
    StartScanResponse_t start_scan;
    AudioDataResponse_t get_audio_data;
    ProximityDataResponse_t get_proximity_data;
} Response_t;

/**
 * Interprets the given input bytes according to the Communication Protocol.
 *
 * @param serialized_request The binary request to interpret.
 * @param len The length of the serialized request in bytes.
 * @return A Request_t representation of the input request.
 */
Request_t CommProtocol_DeserializeRequest(const uint8_t * serialized_request, const size_t len);

/**
 * Serializes the given response into the 'serialized_response' buffer. Writes length of response to len.
 * @param response The response to serialize according to the communication protocol.
 * @param serialized_response Destination buffer for serialized response.
 * @param len Length of serialized response.
 */
void CommProtocol_SerializeResponse(const Response_t response, uint8_t * serialized_response, size_t * len);

#endif //OPENBADGE_BLE_COMMAND_H
