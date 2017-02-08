//
// Created by Andrew Bartow on 2/2/17.
//
#include <string.h>

#include "app_error.h"
#include "comm_protocol.h"
#include "group_assignment.h"
#include "timestamp.h"
#include "audio_chunks.h"
#include "proximity_chunks.h"

#define TIMESTAMP_LEN_BYTES   (sizeof(uint32_t) + sizeof(uint16_t))

#define PARSE_INT_FIELD(P_SERIALIZED_DATA, TYPE)                       \
     ((TYPE) parse_int_field(P_SERIALIZED_DATA, sizeof(TYPE)))

// We pass in the message not including the header into the payload deserializer.
typedef void (*PayloadDeserializer_t)(const uint8_t * p_payload, const size_t len, Request_t * p_request);
// Serializer outputs representation into response and writes length of representation to p_len.
typedef void (*ResponseSerializer_t)(const Response_t response, uint8_t * p_dst, size_t * p_len);

typedef struct {
    RequestType_t request_type; // The type of request this is.
    char serialized_request_type; // The header character indicating what sort of request this.

    // The deserializer that can decode the binary wire message's payload data into our struct.
    // Might be null if the given type has no metadata.
    PayloadDeserializer_t deserializer;
} RequestParser_t;

typedef struct {
    ResponseType_t response_type;
    ResponseSerializer_t serializer;
} TypeToResponseSerializer_t;

// Deserializes a little endian integer field of len bytes and returns the value.
static uint32_t parse_int_field(const uint8_t *serialized_field, size_t len) {
    APP_ERROR_CHECK_BOOL(len <= sizeof(uint32_t));

    uint32_t field_value = 0;
    // Our processor and our input are both little endian, so this should copy correctly.
    memcpy(&field_value, serialized_field, len);
    return field_value;
}

static Timestamp_t deserialize_timestamp(const uint8_t * serialized_timestamp) {
    Timestamp_t timestamp = {
            .seconds = PARSE_INT_FIELD(serialized_timestamp, int32_t),
            .milliseconds = PARSE_INT_FIELD(serialized_timestamp + sizeof(int32_t), int16_t),
    };

    return timestamp;
}

static void deserialize_status_payload(const uint8_t * p_payload, const size_t len, Request_t * p_request) {
    p_request->status.request_time = deserialize_timestamp(p_payload);


    if (len > TIMESTAMP_LEN_BYTES) {
        p_request->status.has_new_assignment = true;
        p_request->status.new_assignment.member_id = PARSE_INT_FIELD(p_payload + TIMESTAMP_LEN_BYTES, uint16_t);
        p_request->status.new_assignment.group_id = PARSE_INT_FIELD(p_payload + TIMESTAMP_LEN_BYTES + sizeof(uint16_t), uint8_t);
    } else {
        p_request->status.has_new_assignment = false;
    }
}

static void deserialize_start_record_payload(const uint8_t * p_payload, const size_t len, Request_t * p_request) {
    p_request->start_record.request_time = deserialize_timestamp(p_payload);
    p_request->start_record.timeout_mins = PARSE_INT_FIELD(p_payload + TIMESTAMP_LEN_BYTES, uint16_t);
}

static void deserialize_start_scan_payload(const uint8_t * p_payload, const size_t len, Request_t * p_request) {
    p_request->start_scan.request_time = deserialize_timestamp(p_payload);
    p_request->start_scan.timeout_mins = PARSE_INT_FIELD(p_payload + TIMESTAMP_LEN_BYTES, uint16_t);
    p_request->start_scan.window_millis = PARSE_INT_FIELD(p_payload + TIMESTAMP_LEN_BYTES + sizeof(uint16_t), uint16_t);
    p_request->start_scan.interval_millis = PARSE_INT_FIELD(p_payload + TIMESTAMP_LEN_BYTES + 2 * sizeof(uint16_t), uint16_t);
    p_request->start_scan.duration_seconds = PARSE_INT_FIELD(p_payload + TIMESTAMP_LEN_BYTES + 3 * sizeof(uint16_t), uint16_t);
    p_request->start_scan.period_seconds = PARSE_INT_FIELD(p_payload + TIMESTAMP_LEN_BYTES + 4 * sizeof(uint16_t), uint16_t);
}

static void deserialize_get_audio_payload(const uint8_t * p_payload, const size_t len, Request_t * p_request) {
    p_request->fetch_audio_data.send_since = deserialize_timestamp(p_payload);
}

static void deserialize_get_proximity_payload(const uint8_t * p_payload, const size_t len, Request_t * p_request) {
    p_request->fetch_proximity_data.send_since = deserialize_timestamp(p_payload);
}

static void deserialize_identify_payload(const uint8_t * p_payload, const size_t len, Request_t * p_request) {
    p_request->identify.timeout_seconds = PARSE_INT_FIELD(p_payload, uint16_t);
}

static RequestParser_t mRequestParsers[] = {
        {
                .request_type = STATUS_REQUEST,
                .serialized_request_type = 's',
                .deserializer = deserialize_status_payload,
        },
        {
                .request_type = START_RECORD_REQUEST,
                .serialized_request_type = '1',
                .deserializer = deserialize_start_record_payload,
        },
        {
                .request_type = STOP_RECORD_REQUEST,
                .serialized_request_type = '0',
                .deserializer = NULL,
        },
        {
                .request_type = START_SCAN_REQUEST,
                .serialized_request_type = 'p',
                .deserializer = deserialize_start_scan_payload,
        },
        {
                .request_type = STOP_SCAN_REQUEST,
                .serialized_request_type = 'q',
                .deserializer = NULL,
        },
        {
                .request_type = GET_AUDIO_DATA_REQUEST,
                .serialized_request_type = 'r',
                .deserializer = deserialize_get_audio_payload,
        },
        {
                .request_type = GET_PROXIMITY_DATA_REQUEST,
                .serialized_request_type = 'b',
                .deserializer = deserialize_get_proximity_payload,
        },
        {
                .request_type = IDENTIFY_REQUEST,
                .serialized_request_type = 'i',
                .deserializer = deserialize_identify_payload,
        },
};

Request_t CommProtocol_DeserializeRequest(const uint8_t * serialized_request, const size_t len) {
    Request_t request;

    for (int i = 0; i < (sizeof(mRequestParsers) / sizeof(RequestParser_t)); i++) {
        if (serialized_request[0] == mRequestParsers[i].serialized_request_type) {
            request.type = mRequestParsers[i].request_type;

            if (mRequestParsers[i].deserializer != NULL) {
                mRequestParsers[i].deserializer(serialized_request + 1, len - 1, &request);
            }

            return request;
        }
    }

    APP_ERROR_CHECK(NRF_ERROR_NOT_FOUND);
}

// These serializers look terrible, but I haven't thought up a more readable way to write them?

static void serialize_timestamp(const Timestamp_t timestamp, uint8_t * p_dst) {
    memcpy(&p_dst[0], &timestamp.seconds, sizeof(uint32_t));
    memcpy(&p_dst[sizeof(uint32_t)], &timestamp.milliseconds, sizeof(uint16_t));
}

static void serialize_status_response(const Response_t response, uint8_t * p_dst, size_t * p_len) {
    p_dst[0] = (uint8_t) response.status.is_synced;
    p_dst[1] = (uint8_t) response.status.is_scanning;
    p_dst[2] = (uint8_t) response.status.is_recording;
    serialize_timestamp(response.status.response_time, &p_dst[3]);
    memcpy(&p_dst[9], &response.status.battery_voltage, sizeof(float));
    *p_len = 13;
}

static void serialize_start_record_response(const Response_t response, uint8_t * p_dst, size_t * p_len) {
    serialize_timestamp(response.start_record.response_time, p_dst);
    *p_len = TIMESTAMP_LEN_BYTES;
}

static void serialize_start_scan_response(const Response_t response, uint8_t * p_dst, size_t * p_len) {
    serialize_timestamp(response.start_scan.response_time, p_dst);
    *p_len = TIMESTAMP_LEN_BYTES;
}

static void serialize_get_audio_response(const Response_t response, uint8_t * p_dst, size_t * p_len) {
    typedef struct {
        uint32_t timestamp_seconds;
        uint16_t timestamp_millis;
        float battery_voltage;
        uint16_t sample_period_ms;
        uint8_t samples_in_chunk;
    } __attribute__((packed)) AudioResponseHeader_t; // This struct controls ordering in serialized output.

    if (response.get_audio_data.is_end_of_response) {
        memset(p_dst, 0, sizeof(AudioResponseHeader_t));
        *p_len = sizeof(AudioResponseHeader_t);
        return;
    }

    AudioResponseHeader_t header = {
            .timestamp_seconds = response.get_audio_data.audio_data.timestamp.seconds,
            .timestamp_millis = response.get_audio_data.audio_data.timestamp.milliseconds,
            .battery_voltage = response.get_audio_data.audio_data.battery_voltage,
            .sample_period_ms = 50, // TODO: Should not hardcode this.
            .samples_in_chunk = response.get_audio_data.audio_data.num_samples,
    };

    memcpy(p_dst, &header, sizeof(AudioResponseHeader_t));
    memcpy(&p_dst[sizeof(AudioResponseHeader_t)],
           &response.get_audio_data.audio_data.samples,
           response.get_audio_data.audio_data.num_samples);

    *p_len = sizeof(AudioResponseHeader_t) + response.get_audio_data.audio_data.num_samples;
}

static void serialize_get_proximity_response(const Response_t response, uint8_t * p_dst, size_t * p_len) {
    typedef struct {
        uint32_t timestamp_seconds;
        float battery_voltage;
        uint8_t num_devices_seen;
    } __attribute__((packed)) ProximityResponseHeader_t; // This struct controls ordering in serialized output.

    if (response.get_proximity_data.is_end_of_response) {
        memset(p_dst, 0, sizeof(ProximityResponseHeader_t));
        *p_len = sizeof(ProximityResponseHeader_t);
        return;
    }

    ProximityResponseHeader_t header = {
            .timestamp_seconds = response.get_proximity_data.proximity_data.timestamp.seconds,
            .battery_voltage = response.get_proximity_data.proximity_data.battery_voltage,
            .num_devices_seen = (uint8_t) response.get_proximity_data.proximity_data.num_devices_seen,
    };

    memcpy(p_dst, &header, sizeof(ProximityResponseHeader_t));
    memcpy(&p_dst[sizeof(ProximityResponseHeader_t)],
           &response.get_proximity_data.proximity_data.seen_devices,
           response.get_proximity_data.proximity_data.num_devices_seen * sizeof(SeenDevice_t));

    *p_len = sizeof(ProximityResponseHeader_t) + response.get_proximity_data.proximity_data.num_devices_seen * sizeof(SeenDevice_t);
}

static TypeToResponseSerializer_t mResponseSerializers[] = {
        {
                .response_type = STATUS_RESPONSE,
                .serializer = serialize_status_response,
        },
        {
                .response_type = START_RECORD_RESPONSE,
                .serializer = serialize_start_record_response,
        },
        {
                .response_type = START_SCAN_RESPONSE,
                .serializer = serialize_start_scan_response,
        },
        {
                .response_type = GET_AUDIO_DATA_RESPONSE,
                .serializer = serialize_get_audio_response,
        },
        {
                .response_type = GET_PROXIMITY_DATA_RESPONSE,
                .serializer = serialize_get_proximity_response,
        }
};

void CommProtocol_SerializeResponse(const Response_t response, uint8_t * serialized_response, size_t * len) {
    for (int i = 0; i < (sizeof(mResponseSerializers) / sizeof(TypeToResponseSerializer_t)); i++) {
        if (mResponseSerializers[i].response_type == response.type) {
            mResponseSerializers[i].serializer(response, serialized_response, len);
            return;
        }
    }

    // Something went horribly wrong. (Response malformed? Loop broken?)
    APP_ERROR_CHECK_BOOL(false);
}