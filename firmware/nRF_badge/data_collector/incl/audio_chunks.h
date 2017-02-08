//
// Created by Andrew Bartow on 1/26/17.
//

#include <stdint.h>

#include "timestamp.h"

#ifndef OPENBADGE_AUDIO_CHUNKS_H
#define OPENBADGE_AUDIO_CHUNKS_H

#define MAX_AUDIO_SAMPLES_PER_CHUNK    114
#define MAX_SERIALIZED_AUDIO_CHUNK_LEN (((MAX_AUDIO_SAMPLES_PER_CHUNK * sizeof(AudioChunk_t) * 3) / 2) \
                                          + (sizeof(float) + sizeof(uint16_t) + sizeof(Timestamp_t)))

typedef struct {
    Timestamp_t timestamp; // Time of capture of first sample in chunk.
    float battery_voltage; // Battery voltage during capture of first sample in chunk.
    uint8_t samples[MAX_AUDIO_SAMPLES_PER_CHUNK]; // Array of samples values.
    uint8_t num_samples; // Number of samples currently in chunk.
} AudioChunk_t;

/**
 * Outputs a serial representation of 'audio_chunk' into p_dst that can be stored efficently to flash.
 * @param audio_chunk Chunk to serialize.
 * @param p_dst Pointer to buffer to place serialized result of audio_chunk. Must be >= MAX_SERIALIZED_AUDIO_CHUNK_LEN long
 * @param p_serialized_len Pointer to integer the length of serialized chunk in bytes will be written to.
 */
void AudioChunk_SerializeChunk(AudioChunk_t chunk, uint8_t * p_dst, uint16_t * p_serialized_len);

/**
 * Outputs the audio chunk representation of the byte stream by the 'len' bytes in 'p_src'.
 * @param p_src Pointer to serialized audio chunk created by AudioChunk_SerializeChunk.
 * @return The audio chunk encoded in the serial representation.
 */
AudioChunk_t AudioChunk_DeserializeChunk(const uint8_t * p_src);

#endif //OPENBADGE_AUDIO_CHUNKS_H
