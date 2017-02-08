#include <audio_chunks.h>
#include <timestamp.h>
#include "gtest/gtest.h"
#include "audio_chunks.h"

// There's a memset_pattern function in the Mac Standard Library, but I'll be nice an implement one that is portable.
// Note our arguments have to be word-aligned, but that shouldn't be an issue here.
static void memset_pattern(uint8_t * dst, uint32_t pattern, size_t len) {
    for (int i = 0; i < len; i = i + sizeof(pattern)) {
        size_t bytes_to_write = (i + sizeof(pattern)) > len ? (len - i) : sizeof(pattern);
        memcpy(&dst[i], &pattern, bytes_to_write);
    }
}

static void test_serialize_deserialize(AudioChunk_t chunk){
    uint8_t serialized_chunk[MAX_SERIALIZED_AUDIO_CHUNK_LEN];
    uint16_t serialized_chunk_len;
    AudioChunk_SerializeChunk(chunk, serialized_chunk, &serialized_chunk_len);
    EXPECT_LE(serialized_chunk_len, MAX_SERIALIZED_AUDIO_CHUNK_LEN);

    memset(&serialized_chunk[serialized_chunk_len], 0, MAX_SERIALIZED_AUDIO_CHUNK_LEN - serialized_chunk_len);

    AudioChunk_t deserialized_chunk = AudioChunk_DeserializeChunk(serialized_chunk);

    EXPECT_EQ(chunk.timestamp.seconds, deserialized_chunk.timestamp.seconds);
    EXPECT_EQ(chunk.timestamp.milliseconds, deserialized_chunk.timestamp.milliseconds);
    EXPECT_EQ(chunk.battery_voltage, deserialized_chunk.battery_voltage);
    EXPECT_EQ(chunk.num_samples, deserialized_chunk.num_samples);
    EXPECT_EQ(0, memcmp(chunk.samples, deserialized_chunk.samples, chunk.num_samples));

    if (memcmp(chunk.samples, deserialized_chunk.samples, chunk.num_samples) != 0) {
        printf("Differing values in arr of len %lu: ", chunk.num_samples);
        for (int i = 0; i < chunk.num_samples; i++) {
            printf("(%02X, %02X), ", chunk.samples[i], deserialized_chunk.samples[i]);
        }
        printf("\r\n");
    }
}

TEST(AudioChunksTest, TestEmptyChunk) {
    AudioChunk_t empty_chunk = {0};

    test_serialize_deserialize(empty_chunk);
}

TEST(AudioChunksTest, TestFullChunk) {
    AudioChunk_t full_chunk;
    full_chunk.timestamp.seconds = 1337;
    full_chunk.timestamp.milliseconds = 42;
    full_chunk.battery_voltage = 2.111;
    memset_pattern(full_chunk.samples, 0x030911FF, MAX_AUDIO_SAMPLES_PER_CHUNK);
    full_chunk.num_samples = MAX_AUDIO_SAMPLES_PER_CHUNK;

    test_serialize_deserialize(full_chunk);
}


TEST(AudioChunksTest, TestPartialChunk) {
    AudioChunk_t partial_chunk;
    partial_chunk.timestamp.seconds = 1337;
    partial_chunk.timestamp.milliseconds = 42;
    partial_chunk.battery_voltage = 2.111;
    memset_pattern(partial_chunk.samples, 0x000132FF, MAX_AUDIO_SAMPLES_PER_CHUNK / 3);
    partial_chunk.num_samples = MAX_AUDIO_SAMPLES_PER_CHUNK / 3;

    test_serialize_deserialize(partial_chunk);
}


TEST(AudioChunksTest, TestAllLoudChunk) {
    AudioChunk_t loud_chunk;
    loud_chunk.timestamp.seconds = 1337;
    loud_chunk.timestamp.milliseconds = 42;
    loud_chunk.battery_voltage = 2.111;
    memset(loud_chunk.samples, 0xFF, MAX_AUDIO_SAMPLES_PER_CHUNK);
    loud_chunk.num_samples = MAX_AUDIO_SAMPLES_PER_CHUNK;

    test_serialize_deserialize(loud_chunk);
}

TEST(AudioChunksTest, TestMaxTimestamp) {
    AudioChunk_t max_timestamp_chunk;
    max_timestamp_chunk.timestamp.seconds = UINT32_MAX;
    max_timestamp_chunk.timestamp.milliseconds = 999;
    max_timestamp_chunk.battery_voltage = 5.0;
    max_timestamp_chunk.num_samples = 0;

    test_serialize_deserialize(max_timestamp_chunk);
}

TEST(AudioChunksTest, TestSingleSampleChunk) {
    AudioChunk_t single_sample_chunk;
    single_sample_chunk.timestamp.seconds = 1337;
    single_sample_chunk.timestamp.milliseconds = 42;
    single_sample_chunk.battery_voltage = 2.11;
    single_sample_chunk.num_samples = 1;
    single_sample_chunk.samples[0] = 42;

    test_serialize_deserialize(single_sample_chunk);
}

TEST(AudioChunksTest, TestSerializedLength) {
    AudioChunk_t chunk;
    chunk.timestamp.seconds = 1337;
    chunk.timestamp.milliseconds = 42;
    chunk.battery_voltage = 2.11;
    chunk.num_samples = MAX_AUDIO_SAMPLES_PER_CHUNK / 2;
    memset_pattern(chunk.samples, 0x01020304, MAX_AUDIO_SAMPLES_PER_CHUNK / 2);

    uint8_t serialized_chunk[MAX_SERIALIZED_AUDIO_CHUNK_LEN];
    uint16_t serialized_chunk_len;
    AudioChunk_SerializeChunk(chunk, serialized_chunk, &serialized_chunk_len);

    size_t max_header_size = 16 * sizeof(uint8_t); // Loose upper bound on header length.
    size_t max_samples_size = (MAX_AUDIO_SAMPLES_PER_CHUNK / 2) / 2; // Each of these samples should only take 4 bits.
    EXPECT_LE(serialized_chunk_len, max_header_size + max_samples_size);

    test_serialize_deserialize(chunk);
}

TEST(AudioChunksTest, TestAllZeroSamples) {
    AudioChunk_t all_zero_chunk;
    all_zero_chunk.timestamp.seconds = 1337;
    all_zero_chunk.timestamp.milliseconds = 42;
    all_zero_chunk.battery_voltage = 2.1;
    memset(all_zero_chunk.samples, 0, MAX_AUDIO_SAMPLES_PER_CHUNK);
    all_zero_chunk.num_samples = MAX_AUDIO_SAMPLES_PER_CHUNK;

    test_serialize_deserialize(all_zero_chunk);
}