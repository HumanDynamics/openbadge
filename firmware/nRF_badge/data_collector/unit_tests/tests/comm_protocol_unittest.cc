#include <comm_protocol.h>
#include <audio_chunks.h>
#include "gtest/gtest.h"
#include "comm_protocol.h"

#define OVERWRITE_CANARY 0xAB

TEST(RequestTest, TestStatusRequest) {
    uint8_t serialized_request[] = {'s', 0xEF, 0xCD, 0xAB, 0x00, 0xAB, 0x01};

    Request_t request = CommProtocol_DeserializeRequest(serialized_request, sizeof(serialized_request));
    EXPECT_TRUE(request.type == STATUS_REQUEST);
    EXPECT_EQ(0xABCDEF, request.status.request_time.seconds);
    EXPECT_EQ(0x1AB, request.status.request_time.milliseconds);
    EXPECT_FALSE(request.status.has_new_assignment);
}

TEST(RequestTest, TestStatusRequestWithNewAssignment) {
    uint8_t serialized_request[] = {'s', 0xE0, 0x39, 0x93, 0x58, 0x1F, 0x02, 0x39, 0x05, 0x07};

    Request_t request = CommProtocol_DeserializeRequest(serialized_request, sizeof(serialized_request));
    EXPECT_TRUE(request.type == STATUS_REQUEST);
    EXPECT_EQ(1486043616, request.status.request_time.seconds);
    EXPECT_EQ(543, request.status.request_time.milliseconds);
    EXPECT_TRUE(request.status.has_new_assignment);
    EXPECT_EQ(7, request.status.new_assignment.group_id);
    EXPECT_EQ(1337, request.status.new_assignment.member_id);
}

TEST(RequestTest, TestStartRecordRequest) {
    uint8_t serialized_request[] = {'1', 0xAB, 0xAB, 0xAB, 0xAB, 0x77, 0x00, 0xB4, 0x00};

    Request_t request = CommProtocol_DeserializeRequest(serialized_request, sizeof(serialized_request));
    EXPECT_TRUE(request.type == START_RECORD_REQUEST);
    EXPECT_EQ(0xABABABAB, request.start_record.request_time.seconds);
    EXPECT_EQ(119, request.start_record.request_time.milliseconds);
    EXPECT_EQ(180, request.start_record.timeout_mins);
}

TEST(RequestTest, StopRecordRequest) {
    uint8_t serialized_request[] = {'0'};

    Request_t request = CommProtocol_DeserializeRequest(serialized_request, sizeof(serialized_request));
    EXPECT_TRUE(request.type == STOP_RECORD_REQUEST);
}

TEST(RequestTest, StartScanRequest) {
    uint8_t serialized_request[] = {'p', 0x78, 0x56, 0x34, 0x12, 0x00, 0x00, /* Timestamp */
                                    0x00, 0x00,  /* Timeout = 0 */
                                    0x31, 0x03,  /* Window = 817 */
                                    0xDC, 0x05,  /* Interval = 1500 */
                                    0x0A, 0x00,  /* Duration = 10 */
                                    0x1E, 0x00}; /* Period = 30 */

    Request_t request = CommProtocol_DeserializeRequest(serialized_request, sizeof(serialized_request));
    EXPECT_TRUE(request.type == START_SCAN_REQUEST);
    EXPECT_EQ(0x12345678, request.start_scan.request_time.seconds);
    EXPECT_EQ(0, request.start_scan.request_time.milliseconds);
    EXPECT_EQ(0, request.start_scan.timeout_mins);
    EXPECT_EQ(817, request.start_scan.window_millis);
    EXPECT_EQ(1500, request.start_scan.interval_millis);
    EXPECT_EQ(10, request.start_scan.duration_seconds);
    EXPECT_EQ(30, request.start_scan.period_seconds);
}

TEST(RequestTest, StopScanRequest) {
    uint8_t serialized_request[] = {'q'};

    Request_t request = CommProtocol_DeserializeRequest(serialized_request, sizeof(serialized_request));
    EXPECT_TRUE(request.type == STOP_SCAN_REQUEST);
}

TEST(RequestTest, GetAudioDataRequest) {
    uint8_t serialized_request[] = {'r', 0x4C, 0xBD, 0x27, 0x56, 0x2A, 0x00};

    Request_t request = CommProtocol_DeserializeRequest(serialized_request, sizeof(serialized_request));
    EXPECT_TRUE(request.type == GET_AUDIO_DATA_REQUEST);
    EXPECT_EQ(1445444940, request.fetch_audio_data.send_since.seconds);
    EXPECT_EQ(42, request.fetch_audio_data.send_since.milliseconds);
}

TEST(RequestTest, GetScanDataRequest) {
    uint8_t serialized_request[] = {'b', 0x4C, 0xBD, 0x27, 0x56};

    Request_t request = CommProtocol_DeserializeRequest(serialized_request, sizeof(serialized_request));
    EXPECT_TRUE(request.type == GET_PROXIMITY_DATA_REQUEST);
    EXPECT_EQ(1445444940, request.fetch_proximity_data.send_since.seconds);
}

TEST(RequestTest, IdentifyRequest) {
    uint8_t serialized_request[] = {'i', 0x2C, 0x01};

    Request_t request = CommProtocol_DeserializeRequest(serialized_request, sizeof(serialized_request));
    EXPECT_TRUE(request.type == IDENTIFY_REQUEST);
    EXPECT_EQ(300, request.identify.timeout_seconds);
}

TEST(ResponseTest, StatusResponseTest) {
    Response_t response = {
            .type = STATUS_RESPONSE,
            .status = {
                    .response_time = {
                            .seconds = 1486118241,
                            .milliseconds = 42,
                    },
                    .battery_voltage = 2.2,
                    .is_recording = true,
                    .is_scanning = false,
                    .is_synced = true,
            }
    };

    uint8_t serialized_response[] = {
            0x01, // Sync Status
            0x00, // Scanner Status
            0x01, // Audio Status
            0x61, 0x5D, 0x94, 0x58, // Timestamp (seconds)
            0x2A, 0x00, // Timestamp (milliseconds)
            0xCD, 0xCC, 0x0C, 0x40, // Battery Voltage
    };

    uint8_t result[sizeof(serialized_response) + 1];
    memset(result, OVERWRITE_CANARY, sizeof(result));
    size_t result_len;

    CommProtocol_SerializeResponse(response, result, &result_len);

    EXPECT_EQ(sizeof(serialized_response), result_len);
    EXPECT_EQ(0, memcmp(serialized_response, result, sizeof(serialized_response)));
    EXPECT_EQ(OVERWRITE_CANARY, result[sizeof(serialized_response)]);
}

TEST(ResponseTest, StartRecordingResponse) {
    Response_t response = {
            .type = START_RECORD_RESPONSE,
            .start_record = {
                    .response_time = {
                            .seconds = 0,
                            .milliseconds = 101,
                    }
            }
    };

    uint8_t serialized_response[] = {
            0x00, 0x00, 0x00, 0x00, // Timestamp (seconds)
            0x65, 0x00, // Timestamp (milliseconds)
    };

    uint8_t result[sizeof(serialized_response) + 1];
    memset(result, OVERWRITE_CANARY, sizeof(result));
    size_t result_len;
    CommProtocol_SerializeResponse(response, result, &result_len);

    EXPECT_EQ(sizeof(serialized_response), result_len);
    EXPECT_EQ(0, memcmp(serialized_response, result, sizeof(serialized_response)));
    EXPECT_EQ(OVERWRITE_CANARY, result[sizeof(serialized_response)]);
}

TEST(ResponseTest, StartScanningResponse) {
    Response_t response = {
            .type = START_SCAN_RESPONSE,
            .start_scan = {
                    .response_time = {
                            .seconds = 4919,
                            .milliseconds = 0,
                    }
            }
    };

    uint8_t serialized_response[] = {
            0x37, 0x13, 0x00, 0x00, // Timestamp (seconds)
            0x00, 0x00, // Timestamp (milliseconds)
    };

    uint8_t result[sizeof(serialized_response) + 1];
    memset(result, OVERWRITE_CANARY, sizeof(result));
    size_t result_len;
    CommProtocol_SerializeResponse(response, result, &result_len);

    EXPECT_EQ(sizeof(serialized_response), result_len);
    EXPECT_EQ(0, memcmp(serialized_response, result, sizeof(serialized_response)));
    EXPECT_EQ(OVERWRITE_CANARY, result[sizeof(serialized_response)]);
}

TEST(ResponseTest, GetAudioDataResponse_WithPartiallyFullChunk) {
    Response_t response = {
            .type = GET_AUDIO_DATA_RESPONSE,
            .get_audio_data = {
                    .is_end_of_response = false,
                    .sample_period_ms = 50,
                    .audio_data = {
                            .timestamp = {
                                    .seconds = 1486120820,
                                    .milliseconds = 37,
                            },
                            .battery_voltage = 1.8,
                            .num_samples = 14,
                            .samples = {
                                    0x05,
                                    0x11,
                                    0x07,
                                    0x01,
                                    0x02,
                                    0x00,
                                    0x09,
                                    0x11,
                                    0x53,
                                    0x47,
                                    0x93,
                                    0x05,
                                    0x05,
                                    0x05,
                            }
                    }
            }
    };

    uint8_t serialized_response[] = {
            0x74, 0x67, 0x94, 0x58, // Timestamp (seconds)
            0x25, 0x00, // Timestamp (milliseconds)
            0x66, 0x66, 0xE6, 0x3F, // Battery voltage
            0x32, 0x00, // Sample period
            0x0E, // Number of samples in chunk
            0x05, 0x11, 0x07, 0x01, 0x02, 0x00, 0x09, 0x11, 0x53, 0x47, 0x93, 0x05, 0x05, 0x05, // Samples
    };

    uint8_t result[sizeof(serialized_response) + 1];
    memset(result, OVERWRITE_CANARY, sizeof(result));
    size_t result_len;
    CommProtocol_SerializeResponse(response, result, &result_len);

    EXPECT_EQ(sizeof(serialized_response), result_len);
    EXPECT_EQ(0, memcmp(serialized_response, result, sizeof(serialized_response)));
    EXPECT_EQ(OVERWRITE_CANARY, result[sizeof(serialized_response)]);
}

TEST(ResponseTest, GetAudioDataResponse_WithFullChunk) {
    uint8_t chunk_samples[MAX_AUDIO_SAMPLES_PER_CHUNK] = {
            0x05, 0x04, 0x05, 0x05, 0x05, 0x06, 0x05, 0x03, 0x05, 0x05, 0x05, 0x13, 0x19, 0x53, 0x93, 0x11,
            0x05, 0x03, 0x03, 0x04, 0x05, 0x78, 0x93, 0x13, 0x14, 0x18, 0x98, 0x98, 0xA3, 0x13, 0x11, 0x05,
            0x05, 0x03, 0x04, 0x05, 0x06, 0x11, 0x03, 0x05, 0x03, 0x05, 0x05, 0x04, 0x89, 0x89, 0x89, 0x91,
            0x05, 0x04, 0x05, 0x05, 0x05, 0x06, 0x05, 0x03, 0x05, 0x05, 0x05, 0x13, 0x19, 0x53, 0x93, 0x11,
            0x05, 0x03, 0x04, 0x05, 0x06, 0x11, 0x03, 0x05, 0x03, 0x05, 0x05, 0x04, 0x89, 0x89, 0x89, 0x91,
            0x05, 0x03, 0x03, 0x04, 0x05, 0x78, 0x93, 0x13, 0x14, 0x18, 0x98, 0x98, 0xA3, 0x13, 0x11, 0x05,
            0x11, 0xF9, 0xFF, 0xFA, 0xAF, 0x19, 0x03, 0x04, 0x05, 0x09, 0x91, 0x13, 0x01, 0x11, 0x13, 0x05,
            0x09, 0x05,
    };

    Response_t response = {
            .type = GET_AUDIO_DATA_RESPONSE,
            .get_audio_data = {
                    .is_end_of_response = false,
                    .sample_period_ms = 50,
                    .audio_data = {
                            .timestamp = {
                                    .seconds = 1486120820,
                                    .milliseconds = 37,
                            },
                            .battery_voltage = 1.8,
                            .num_samples = MAX_AUDIO_SAMPLES_PER_CHUNK,
                            // We'll memcpy in chunk_samples to .samples
                    },
            },
    };

    memcpy(response.get_audio_data.audio_data.samples, &chunk_samples, MAX_AUDIO_SAMPLES_PER_CHUNK);

    uint8_t serialized_response_header[] = {
            0x74, 0x67, 0x94, 0x58, // Timestamp (seconds)
            0x25, 0x00, // Timestamp (milliseconds)
            0x66, 0x66, 0xE6, 0x3F, // Battery voltage
            0x32, 0x00, // Sample period
            0x72, // Number of samples in chunk
    };

    uint8_t result[sizeof(serialized_response_header) + sizeof(chunk_samples) + 1];
    memset(result, OVERWRITE_CANARY, sizeof(result));
    size_t result_len;
    CommProtocol_SerializeResponse(response, result, &result_len);

    EXPECT_EQ(sizeof(serialized_response_header) + sizeof(chunk_samples), result_len);
    EXPECT_EQ(0, memcmp(serialized_response_header, result, sizeof(serialized_response_header)));
    EXPECT_EQ(0, memcmp(chunk_samples, &result[sizeof(serialized_response_header)], sizeof(chunk_samples)));
    EXPECT_EQ(OVERWRITE_CANARY, result[sizeof(serialized_response_header) + sizeof(chunk_samples)]);
}

TEST(ResponseTest, GetAudioDataResponse_EndMarker) {
    Response_t response = {
            .type = GET_AUDIO_DATA_RESPONSE,
            .get_audio_data = {
                    .is_end_of_response = true,
            }
    };

    // The end of response header is the length of the header but set to all zeroes.
    uint8_t serialized_response[] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    uint8_t result[sizeof(serialized_response) + 1];
    memset(result, OVERWRITE_CANARY, sizeof(result));
    size_t result_len;
    CommProtocol_SerializeResponse(response, result, &result_len);

    EXPECT_EQ(sizeof(serialized_response), result_len);
    EXPECT_EQ(0, memcmp(serialized_response, result, sizeof(serialized_response)));
    EXPECT_EQ(OVERWRITE_CANARY, result[sizeof(serialized_response)]);
}

TEST(ResponseTest, GetProximityDataResponse_WithDevices) {
    Response_t response = {
            .type = GET_PROXIMITY_DATA_RESPONSE,
            .get_proximity_data = {
                    .is_end_of_response = false,
                    .proximity_data = {
                            .timestamp = {
                                    .seconds = 1486120820,
                                    .milliseconds = 37,
                            },
                            .battery_voltage = 1.8,
                            .num_devices_seen = 4,
                            .seen_devices = {
                                    {
                                            .average_rssi = 102,
                                            .id = 13,
                                            .num_times_seen = 3,
                                    },
                                    {
                                            .average_rssi = 105,
                                            .id = 1,
                                            .num_times_seen = 7,
                                    },
                                    {
                                            .average_rssi = 0,
                                            .id = 1337,
                                            .num_times_seen = 3,
                                    },
                                    {
                                            .average_rssi = 7,
                                            .id = 42,
                                            .num_times_seen = 1,
                                    },
                            },
                    },
            },
    };

    uint8_t serialized_response[] = {
            0x74, 0x67, 0x94, 0x58, // Timestamp (seconds)
            0x66, 0x66, 0xE6, 0x3F, // Battery voltage
            0x04, // Number of devices
            0x0D, 0x00, 102, 3, // First device
            0x01, 0x00, 105, 7, // Second device
            0x39, 0x05, 0, 3, // Third device
            0x2A, 0x00, 7, 1, // Fourth device
    };

    uint8_t result[sizeof(serialized_response) + 1];
    memset(result, OVERWRITE_CANARY, sizeof(result));
    size_t result_len;
    CommProtocol_SerializeResponse(response, result, &result_len);

    EXPECT_EQ(sizeof(serialized_response), result_len);
    EXPECT_EQ(0, memcmp(serialized_response, result, sizeof(serialized_response)));
    EXPECT_EQ(OVERWRITE_CANARY, result[sizeof(serialized_response)]);
}

TEST(ResponseTest, GetProximityDataResponse_EndMarker) {
    Response_t response = {
            .type = GET_PROXIMITY_DATA_RESPONSE,
            .get_audio_data = {
                    .is_end_of_response = true,
            }
    };

    // The end of response header is the length of the header but set to all zeroes.
    uint8_t serialized_response[] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    uint8_t result[sizeof(serialized_response) + 1];
    memset(result, OVERWRITE_CANARY, sizeof(result));
    size_t result_len;
    CommProtocol_SerializeResponse(response, result, &result_len);

    EXPECT_EQ(sizeof(serialized_response), result_len);
    EXPECT_EQ(0, memcmp(serialized_response, result, sizeof(serialized_response)));
    EXPECT_EQ(OVERWRITE_CANARY, result[sizeof(serialized_response)]);
}