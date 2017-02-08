//
// Created by Andrew Bartow on 1/26/17.
//

#include <string.h>

#include "audio_chunks.h"
#include "calc_macros.h"
#include "vaint_utils.h"

#define BITS_PER_BYTE     8

typedef struct {
    Timestamp_t timestamp;
    float battery_voltage;
    uint16_t num_samples;
} __attribute__((packed)) SerializedChunkHeader_t;

void AudioChunk_SerializeChunk(AudioChunk_t chunk, uint8_t * p_dst, uint16_t * p_serialized_len) {
    *p_serialized_len = 0;

    SerializedChunkHeader_t header = {{chunk.timestamp.seconds, chunk.timestamp.milliseconds},
                                      chunk.battery_voltage, chunk.num_samples};

    memcpy(p_dst, &header, sizeof(header));

    uint16_t bits_encoded = 0;
    for (int i = 0; i < chunk.num_samples; i++) {
        uint16_t dst_pos = sizeof(header) + (uint16_t) (bits_encoded / BITS_PER_BYTE);

        uint8_t vaint_len_bits;
        VaintUtil_EncodeVaint(chunk.samples[i], (bits_encoded % BITS_PER_BYTE) != 0, &p_dst[dst_pos], &vaint_len_bits);
        bits_encoded += vaint_len_bits;
    }

    *p_serialized_len =  sizeof(header) + CEIL(bits_encoded, BITS_PER_BYTE);
}

/**
 * Outputs the audio chunk representation of the byte stream by the 'len' bytes in 'p_src'.
 * @param p_src Pointer to serialized audio chunk created by AudioChunk_SerializeChunk.
 * @param len Length of byte stream.
 * @return The audio chunk encoded in the serial representation.
 */
AudioChunk_t AudioChunk_DeserializeChunk(const uint8_t * p_src) {
    AudioChunk_t chunk;

    SerializedChunkHeader_t * header = (SerializedChunkHeader_t *) p_src;
    chunk.timestamp.seconds = header->timestamp.seconds;
    chunk.timestamp.milliseconds = header->timestamp.milliseconds;
    chunk.battery_voltage = header->battery_voltage;
    chunk.num_samples = (uint8_t) header->num_samples;

    uint16_t bits_decoded = 0;
    for (int i = 0; i < chunk.num_samples; i++) {
        uint16_t src_pos = sizeof(SerializedChunkHeader_t) + (uint16_t) (bits_decoded / BITS_PER_BYTE);

        uint8_t vaint_len;
        chunk.samples[i] = (uint8_t) VaintUtil_DecodeVaint(&p_src[src_pos], (bits_decoded % BITS_PER_BYTE) != 0, &vaint_len);
        bits_decoded += vaint_len;
    }

    return chunk;
}