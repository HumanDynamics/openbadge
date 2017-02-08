#include <comm_protocol.h>
#include <timestamp.h>
#include <audio_chunks.h>
#include <badge_fs_config.h>
#include <proximity_chunks.h>
#include "gtest/gtest.h"
#include "nrf_error.h"
#include "request_handler.h"

// All of our test constants are basically randomly chosen numbers.

#define TIME_SECONDS             42
#define TIME_MILLISECONDS        190
#define BATTERY_VOLTAGE          2.13

#define GROUP_NUNBER             3
#define ID_NUMBER                41

#define CHUNK_STORAGE_START_TIME 904

static bool mIsSynced;
static bool mIsScanning;
static bool mIsRecording;

static int mAssignedGroup;
static int mAssignedID;

static uint32_t mTimeSeconds;
static uint32_t mTimeMilliseconds;

static int mScanTimeout;
static int mRecordTimeout;

static int mScanParamWindow;
static int mScanParamInterval;
static int mScanParamDuration;
static int mScanParamPeriod;

static int mIdentifyTimeout;

static float mBatteryVoltage;

namespace {
    class RequestHandlerTest : public ::testing::Test {
        virtual void SetUp() {
            mIsSynced = false;
            mIsScanning = false;
            mIsRecording = false;

            mTimeSeconds = TIME_SECONDS;
            mTimeMilliseconds = TIME_MILLISECONDS;
            mBatteryVoltage = BATTERY_VOLTAGE;

            mAssignedGroup = GROUP_NUNBER;
            mAssignedID = ID_NUMBER;

            // Store some audio and proximity data.

            BadgeFS_Init(mBadgeFSConfig_PartitionTable, 2, BADGE_FS_START);

            for (uint8_t i = 0; i < UINT8_MAX; i ++) {
                AudioChunk_t audio_chunk = {
                        .timestamp = {
                                .seconds = CHUNK_STORAGE_START_TIME + i,
                                .milliseconds = 0,
                        },
                        .battery_voltage = 1.3,
                        .num_samples = MAX_AUDIO_SAMPLES_PER_CHUNK,
                };
                memset(audio_chunk.samples, i, MAX_AUDIO_SAMPLES_PER_CHUNK);

                uint8_t serialized_audio_chunk[MAX_SERIALIZED_AUDIO_CHUNK_LEN];
                uint16_t serialized_audio_chunk_len;
                AudioChunk_SerializeChunk(audio_chunk, serialized_audio_chunk, &serialized_audio_chunk_len);

                BadgeFS_StoreItem(FS_PARTITION_AUDIO, serialized_audio_chunk, serialized_audio_chunk_len);

                ProximityChunk_t proximity_chunk = {
                        .timestamp = {
                                .seconds = CHUNK_STORAGE_START_TIME + i,
                                .milliseconds = 0,
                        },
                        .battery_voltage = 1.3,
                        .num_devices_seen = MAX_DEVICES_PER_CHUNK,
                };
                memset(proximity_chunk.seen_devices, i, sizeof(SeenDevice_t) * MAX_DEVICES_PER_CHUNK);

                uint8_t serialized_prox_chunk[MAX_SERIALIZED_PROXIMITY_CHUNK_LEN];
                uint16_t serialized_prox_chunk_len;
                ProximityChunk_SerializeChunk(proximity_chunk, serialized_prox_chunk, &serialized_prox_chunk_len);

                BadgeFS_StoreItem(FS_PARTITION_PROXIMITY, serialized_prox_chunk, serialized_audio_chunk_len);
            }
        }
    };

    TEST_F(RequestHandlerTest, TestStatusRequest) {
        mIsSynced = true;
        mIsScanning = true;

        Request_t request = {
                .type = STATUS_REQUEST,
                .status = {
                        .request_time = {
                                .seconds = 1337,
                                .milliseconds = 94,
                        },
                        .has_new_assignment = false,
                }
        };

        RequestHandler_HandleRequest(request);

        Response_t response;
        EXPECT_EQ(NRF_SUCCESS, RequestHandler_GetNextQueuedResponse(&response));

        EXPECT_TRUE(response.type == STATUS_RESPONSE);
        EXPECT_TRUE(response.status.is_synced);
        EXPECT_TRUE(response.status.is_scanning);
        EXPECT_FALSE(response.status.is_recording);
        EXPECT_FLOAT_EQ(BATTERY_VOLTAGE, response.status.battery_voltage);
        EXPECT_EQ(TIME_SECONDS, response.status.response_time.seconds);
        EXPECT_EQ(TIME_MILLISECONDS, response.status.response_time.milliseconds);

        // Should be no more responses queued.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, RequestHandler_GetNextQueuedResponse(&response));

        // Last request should have also synchronized the time.
        EXPECT_EQ(1337, mTimeSeconds);
        EXPECT_EQ(94, mTimeMilliseconds);

        // Check that we didn't accidently change our group assignment.
        EXPECT_EQ(GROUP_NUNBER, mAssignedGroup);
        EXPECT_EQ(ID_NUMBER, mAssignedID);
    }

    TEST_F(RequestHandlerTest, TestStatusRequestNewAssignment) {
        Request_t request = {
                .type = STATUS_REQUEST,
                .status = {
                        .request_time = {
                                .seconds = 1337,
                                .milliseconds = 94,
                        },
                        .has_new_assignment = true,
                        .new_assignment = {
                                .group_id = 7,
                                .member_id = 42,
                        }
                }
        };

        RequestHandler_HandleRequest(request);

        Response_t response;
        EXPECT_EQ(NRF_SUCCESS, RequestHandler_GetNextQueuedResponse(&response));

        EXPECT_FALSE(response.type == STATUS_RESPONSE);
        EXPECT_FALSE(response.status.is_synced);
        EXPECT_FALSE(response.status.is_scanning);
        EXPECT_FALSE(response.status.is_recording);
        EXPECT_FLOAT_EQ(BATTERY_VOLTAGE, response.status.battery_voltage);
        EXPECT_EQ(TIME_SECONDS, response.status.response_time.seconds);
        EXPECT_EQ(TIME_MILLISECONDS, response.status.response_time.milliseconds);

        // Should be no more responses queued.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, RequestHandler_GetNextQueuedResponse(&response));

        // Last request should have also synchronized the time.
        EXPECT_EQ(1337, mTimeSeconds);
        EXPECT_EQ(94, mTimeMilliseconds);

        // Last request should also have assigned us new group and individual ids.
        EXPECT_EQ(7, mAssignedGroup);
        EXPECT_EQ(42, mAssignedID);
    }

    TEST_F(RequestHandlerTest, TestStartRecordRequest) {
        Request_t request = {
                .type = START_RECORD_REQUEST,
                .start_record = {
                        .request_time = {
                                .seconds = 1337,
                                .milliseconds = 94,
                        },
                        .timeout_mins = 240,
                },
        };

        RequestHandler_HandleRequest(request);

        Response_t response;
        EXPECT_EQ(NRF_SUCCESS, RequestHandler_GetNextQueuedResponse(&response));

        EXPECT_TRUE(response.type == START_RECORD_RESPONSE);
        EXPECT_EQ(TIME_SECONDS, response.start_record.response_time.seconds);
        EXPECT_EQ(TIME_MILLISECONDS, response.start_record.response_time.seconds);

        // Should be no more responses queued.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, RequestHandler_GetNextQueuedResponse(&response));

        EXPECT_TRUE(mIsRecording);
        EXPECT_EQ(240, mRecordTimeout);

        // Last request should have also synchronized the time.
        EXPECT_EQ(1337, mTimeSeconds);
        EXPECT_EQ(94, mTimeMilliseconds);
    }

    TEST_F(RequestHandlerTest, TestStartScanRequest) {
        Request_t request = {
                .type = START_SCAN_REQUEST,
                .start_scan = {
                        .request_time = {
                                .seconds = 1337,
                                .milliseconds = 94,
                        },

                        .timeout_mins = 240,

                        .window_millis = 13,
                        .interval_millis = 39,
                        .duration_seconds = 5,
                        .period_seconds = 30,
                },
        };

        RequestHandler_HandleRequest(request);

        Response_t response;
        EXPECT_EQ(NRF_SUCCESS, RequestHandler_GetNextQueuedResponse(&response));

        EXPECT_TRUE(response.type == START_SCAN_RESPONSE);
        EXPECT_EQ(TIME_SECONDS, response.start_scan.response_time.seconds);
        EXPECT_EQ(TIME_MILLISECONDS, response.start_scan.response_time.seconds);

        // Should be no more responses queued.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, RequestHandler_GetNextQueuedResponse(&response));

        EXPECT_TRUE(mIsScanning);
        EXPECT_EQ(240, mScanTimeout);
        EXPECT_EQ(13, mScanParamWindow);
        EXPECT_EQ(39, mScanParamInterval);
        EXPECT_EQ(5, mScanParamDuration);
        EXPECT_EQ(30, mScanParamPeriod);

        // Last request should have also synchronized the time.
        EXPECT_EQ(1337, mTimeSeconds);
        EXPECT_EQ(94, mTimeMilliseconds);
    }

    TEST_F(RequestHandlerTest, TestStopRecordRequest) {
        mIsRecording = true;

        Request_t request = {
                .type = STOP_RECORD_REQUEST,
        };

        RequestHandler_HandleRequest(request);

        Response_t response;
        // Should be no response for a stop record request.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, RequestHandler_GetNextQueuedResponse(&response));

        EXPECT_FALSE(mIsRecording);
    }

    TEST_F(RequestHandlerTest, TestStopScanRequest) {
        mIsScanning = true;

        Request_t request = {
                .type = STOP_SCAN_REQUEST,
        };

        RequestHandler_HandleRequest(request);

        Response_t response;
        // Should be no response for a stop scan request.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, RequestHandler_GetNextQueuedResponse(&response));

        EXPECT_FALSE(mIsScanning);
    }

    TEST_F(RequestHandlerTest, TestIdentifyRequest) {
        Request_t request = {
                .type = IDENTIFY_REQUEST,
                .identify = {
                        .timeout_seconds = 30,
                }
        };

        RequestHandler_HandleRequest(request);

        Response_t response;
        // Should be no response for an identify request.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, RequestHandler_GetNextQueuedResponse(&response));

        EXPECT_EQ(30, mIdentifyTimeout);
    }

    TEST_F(RequestHandlerTest, TestGetAudioDataRequest) {
        Request_t request = {
                .type = GET_AUDIO_DATA_REQUEST,
                .fetch_audio_data = {
                        .send_since = {
                                .seconds = CHUNK_STORAGE_START_TIME,
                                .milliseconds = 0,
                        }
                }
        };

        RequestHandler_HandleRequest(request);

        Response_t response;

        for (uint8_t i = 0; i < UINT8_MAX; i++) {
            EXPECT_EQ(NRF_SUCCESS, RequestHandler_GetNextQueuedResponse(&response));

            EXPECT_TRUE(response.type == GET_AUDIO_DATA_RESPONSE);
            EXPECT_EQ(i, response.get_audio_data.audio_data.timestamp.seconds - CHUNK_STORAGE_START_TIME);
            EXPECT_EQ(i, response.get_audio_data.audio_data.samples[1]);
        }

        // Check that we also queue our of end of transmission message.
        EXPECT_EQ(NRF_SUCCESS, RequestHandler_GetNextQueuedResponse(&response));
        EXPECT_TRUE(response.type == GET_AUDIO_DATA_RESPONSE);
        EXPECT_TRUE(response.get_audio_data.is_end_of_response);

        // That should be our last message.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, RequestHandler_GetNextQueuedResponse(&response));
    }

    TEST_F(RequestHandlerTest, TestGetProximityDataRequest) {
        Request_t request = {
                .type = GET_PROXIMITY_DATA_REQUEST,
                .fetch_proximity_data = {
                        .send_since = {
                                .seconds = CHUNK_STORAGE_START_TIME,
                                .milliseconds = 0,
                        }
                }
        };

        RequestHandler_HandleRequest(request);

        Response_t response;

        for (uint8_t i = 0; i < UINT8_MAX; i++) {
            EXPECT_EQ(NRF_SUCCESS, RequestHandler_GetNextQueuedResponse(&response));

            EXPECT_TRUE(response.type == GET_PROXIMITY_DATA_RESPONSE);
            EXPECT_EQ(i, response.get_proximity_data.proximity_data.timestamp.seconds - CHUNK_STORAGE_START_TIME);
            EXPECT_EQ(i, response.get_proximity_data.proximity_data.seen_devices[1].average_rssi);
        }

        // CHeck that we also queue our end of transmission message.
        EXPECT_EQ(NRF_SUCCESS, RequestHandler_GetNextQueuedResponse(&response));
        EXPECT_TRUE(response.type == GET_PROXIMITY_DATA_RESPONSE);
        EXPECT_TRUE(response.get_proximity_data.is_end_of_response);

        // That should be our last message.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, RequestHandler_GetNextQueuedResponse(&response));
    }
}


