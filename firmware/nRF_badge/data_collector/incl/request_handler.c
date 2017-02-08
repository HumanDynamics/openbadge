//
// Created by Andrew Bartow on 2/5/17.
//

#include "request_handler.h"
#include "collector.h"
#include "badge_fs.h"
#include "comm_protocol.h"
#include "battery.h"
#include "badge_fs_config.h"
#include "audio_chunks.h"
#include "timestamp.h"
#include "proximity_chunks.h"

// Our queue is actually a slight illusion.
// Because the data we send back can actually be very large, much larger than we can fit into RAM, we need to pull
//  the data from the file system as we go.
// So we handle this situation as so:
//    STATUS_RESPONSE, START_RECORD_RESPONSE, and START_PROXIMITY_RESPONSE can all fit into RAM and only take a single
//      response, so we just set them and then unset them when we pull from the queue. No need to worry about there
//      being more than one of them.
//    GET_AUDIO_DATA and GET_PROXIMITY_DATA responses both require pulling from the file system, so we store the next
//          response that needs to be sent, and then we also store the corresponding filesystem key for that response.
//        When it comes time to pull the next one from the queue, we return next_response, and then use fs_key to
//          construct the response that comes after the one we just returned.
typedef struct {
    // If there's currently any responses in our queue, this is true.
    bool is_response_queued;

    // The next response in our queue.
    Response_t next_response;

    // If next_response is GET_AUDIO_DATA_RESPONSE or a GET_PROXIMITY_DATA_RESPONSE, this key points to the
    //   corresponding filesystem item for the data in the response.
    BadgeFS_Key_t * fs_key;
} ResponseQueue_t;

static ResponseQueue_t mResponseQueue;

static void queue_response(Response_t response, BadgeFS_Key_t * fs_key) {
    APP_ERROR_CHECK_BOOL(!mResponseQueue.is_response_queued);

    mResponseQueue.is_response_queued = true;
    mResponseQueue.next_response = response;
    mResponseQueue.fs_key = fs_key;
}


typedef void (* RequestHandler_t)(Request_t request);

typedef struct {
    RequestType_t type;
    RequestHandler_t handler;
} RequestHandlerForType_t;

static void status_request_handler(Request_t request) {
    if (request.status.has_new_assignment) {
        GroupAssignment_StoreAssignment(request.status.new_assignment);
    }

    Response_t response = {
            .type = STATUS_RESPONSE,
            .status = {
                    .response_time = Timestamp_GetCurrentTime(),
                    .battery_voltage = BatteryMonitor_getBatteryVoltage(),
                    .is_synced = Timestamp_IsTimeSynced(),
                    .is_recording = Collector_IsRecording(),
                    .is_scanning = Scanner_IsScannerRunning(),
            }
    };

    // Communication protocol dictates request time is not reflected in the status response.
    Timestamp_SetTime(request.status.request_time);

    queue_response(response, NULL);
}

static void start_record_request_handler(Request_t request) {
    Timestamp_SetTime(request.start_record.request_time);

    startCollector(request.start_record.timeout_mins);

    Response_t response = {
            .type =  START_RECORD_RESPONSE,
            .start_record = {
                    .response_time = Timestamp_GetCurrentTime()
            },
    };
    queue_response(response, NULL);
}

static void start_scan_request_handler(Request_t request) {
    Timestamp_SetTime(request.start_scan.request_time);

    startScanner(request.start_scan.window_millis,
                 request.start_scan.interval_millis,
                 request.start_scan.duration_seconds,
                 request.start_scan.period_seconds,
                 request.start_scan.timeout_mins);

    Response_t response = {
            .type = START_SCAN_RESPONSE,
            .start_scan = {
                    .response_time = Timestamp_GetCurrentTime(),
            },
    };
    queue_response(response, NULL);
}

static void stop_record_request_handler(Request_t request) {
    stopCollector();

    // No response to queue here.
}

static void stop_scan_request_handler(Request_t request) {
    stopScanner();

    // No response to queue here.
}

// Returns true if timestampA is at or after timestampB.
static bool timestamp_compare(Timestamp_t timestampA, Timestamp_t timestampB) {
    if (timestampA.seconds == timestampB.seconds) {
        return timestampA.milliseconds >= timestampB.milliseconds;
    } else if (timestampB.seconds > timestampB.seconds) {
        return true;
    } else {
        return false;
    }
}

// This is kind of hacky, but C isn't a functional programing language, so what're you gonna do?
// mAudioTimeSearchQuery is the target time for find_audio_key_condition_checker.
//   find_audio_key_condition_checker will return true for any chunk with a timestamp at or after mAudioTimeSearchQuery.
static Timestamp_t mAudioTimeSearchQuery;

static bool audio_chunk_is_after_search_time(uint8_t * item_data, uint16_t item_len) {
    AudioChunk_t chunk = AudioChunk_DeserializeChunk(item_data);
    return timestamp_compare(chunk.timestamp, mAudioTimeSearchQuery);
}

static void get_audio_data_request_handler(Request_t request) {
    mAudioTimeSearchQuery = request.fetch_audio_data.send_since;

    BadgeFS_Key_t * key;
    uint32_t found_key = BadgeFS_FindKey(FS_PARTITION_AUDIO, audio_chunk_is_after_search_time, &key);
    if (found_key == NRF_SUCCESS) {
        uint8_t item[MAX_SERIALIZED_AUDIO_CHUNK_LEN];
        uint16_t item_len;
        uint32_t err_code = BadgeFS_GetItem(item, &item_len, key);
        APP_ERROR_CHECK(err_code);

        Response_t response = {
                .type = GET_AUDIO_DATA_RESPONSE,
                .get_audio_data = {
                        .is_end_of_response = false,
                        .sample_period_ms = 50, // Todo: don't hardcode this!
                        .audio_data = AudioChunk_DeserializeChunk(item),
                },
        };

        queue_response(response, key);
    } else if (found_key == NRF_ERROR_NOT_SUPPORTED) {
        Response_t response = {
                .type = GET_AUDIO_DATA_RESPONSE,
                .get_audio_data = {
                        .is_end_of_response = true,
                },
        };

        queue_response(response, NULL);
    } else {
        APP_ERROR_CHECK(found_key);
    }
}

static Timestamp_t mProximityTimeSearchQuery;

static bool proximity_chunk_is_after_search_time(uint8_t * item_data, uint16_t item_len) {
    ProximityChunk_t chunk = ProximityChunk_DeserializeChunk(item_data, item_len);
    return timestamp_compare(chunk.timestamp, mProximityTimeSearchQuery);
}

static void get_proximity_data_request_handler(Request_t request) {
    mProximityTimeSearchQuery = request.fetch_proximity_data.send_since;

    BadgeFS_Key_t * key;
    uint32_t found_key = BadgeFS_FindKey(FS_PARTITION_PROXIMITY, proximity_chunk_is_after_search_time, &key);
    if (found_key == NRF_SUCCESS) {
        uint8_t item[MAX_SERIALIZED_AUDIO_CHUNK_LEN];
        uint16_t item_len;
        uint32_t err_code = BadgeFS_GetItem(item, &item_len, key);
        APP_ERROR_CHECK(err_code);

        Response_t response = {
                .type = GET_PROXIMITY_DATA_RESPONSE,
                .get_proximity_data = {
                        .is_end_of_response = false,
                        .proximity_data = ProximityChunk_DeserializeChunk(item, item_len);
                }
        };

        queue_response(response, key);
    } else if (found_key == NRF_ERROR_NOT_FOUND) {
        Response_t response = {
                .type = GET_PROXIMITY_DATA_RESPONSE,
                .get_proximity_data = {
                        .is_end_of_response = true,
                }
        };

        queue_response(response, NULL);
    } else {
        APP_ERROR_CHECK(found_key);
    }
}

static void identify_request_handler(Request_t request) {
    IdentifyLED_On(request.identify.timeout_seconds);
}

static RequestHandlerForType_t mRequestHandlers[] = {
        {
                .type = STATUS_REQUEST,
                .handler = status_request_handler,
        },
        {
                .type = START_RECORD_REQUEST,
                .handler = start_record_request_handler,
        },
        {
                .type = START_SCAN_REQUEST,
                .handler = start_scan_request_handler,
        },
        {
                .type = STOP_RECORD_REQUEST,
                .handler = stop_record_request_handler,
        },
        {
                .type = STOP_SCAN_REQUEST,
                .handler = stop_scan_request_handler,
        },
        {
                .type = GET_AUDIO_DATA_REQUEST,
                .handler = get_audio_data_request_handler,
        },
        {
                .type = GET_PROXIMITY_DATA_REQUEST,
                .handler = get_proximity_data_request_handler,
        },
        {
                .type = IDENTIFY_REQUEST,
                .handler = identify_request_handler,
        }
};

void RequestHandler_HandleRequest(Request_t request) {
    APP_ERROR_CHECK_BOOL(!mResponseQueue.is_response_queued);

    for (int i = 0; i < sizeof(mRequestHandlers) / sizeof(RequestHandlerForType_t); i++) {
        if (mRequestHandlers[i].type == request.type) {
            mRequestHandlers[i].handler(request);
            return;
        }
    }

    // Should never get here, should have a handler for each request type.
    APP_ERROR_CHECK_BOOL(false);
}

uint32_t RequestHandler_GetNextQueuedResponse(Response_t * p_response) {
    if (!mResponseQueue.is_response_queued) return NRF_ERROR_NOT_FOUND;

    // Handle audio and proximity data requests separately.
    if (mResponseQueue.next_response.type == GET_AUDIO_DATA_RESPONSE) {

    } else if (mResponseQueue.next_response.type == GET_PROXIMITY_DATA_RESPONSE) {

    } else {
        memcpy(p_response, mResponseQueue.next_response, sizeof(Response_t));
        mResponseQueue.is_response_queued = false;
    }
}