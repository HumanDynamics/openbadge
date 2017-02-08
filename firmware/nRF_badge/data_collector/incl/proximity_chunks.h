//
// Created by Andrew Bartow on 2/3/17.
//

#ifndef OPENBADGE_PROXIMITY_CHUNKS_H
#define OPENBADGE_PROXIMITY_CHUNKS_H

#include "timestamp.h"

#define MAX_DEVICES_PER_CHUNK 28
#define MAX_SERIALIZED_PROXIMITY_CHUNK_LEN   ((sizeof(float) + sizeof(Timestamp_t) + sizeof(uint8_t)) +  \
                                                   (MAX_DEVICES_PER_CHUNK * sizeof(SeenDevice_t)))

typedef struct {
    uint16_t id;
    uint8_t average_rssi;
    uint8_t num_times_seen;
} SeenDevice_t;

typedef struct {
    Timestamp_t timestamp; // Time of capture of chunk.
    float battery_voltage; // Battery voltage during capture of chunk
    uint32_t num_devices_seen; // Number of samples currently in chunk.`
    SeenDevice_t seen_devices[MAX_DEVICES_PER_CHUNK]; // Array of samples values.
} ProximityChunk_t;

/**
 * Outputs a serialized representation of 'chunk' into 'p_dst' that can efficently be stored into flash.
 * @param chunk Chunk to serialize.
 * @param p_dst Pointer to buffer to place serialized representation of chunk.
 *    Must be >= MAX_SERIALIZED_PROXIMITY_CHUNK_LEN long.
 * @param p_serialized_len Pointer to integer the length of serialized chunk in bytes will be written to.
 */
void ProximityChunk_SerializeChunk(ProximityChunk_t chunk, uint8_t * p_dst, uint16_t * p_serialized_len);

/**
 * Outputs the proximity chunk representation of the byte stream by the 'len' bytes in 'p_src'.
 * @param p_src Pointer to serialized proximity chunk created by AudioChunk_SerializeChunk.
 * @param len The length of the serialized chunk.
 * @return The proximity chunk encoded in the serial representation.
 */
ProximityChunk_t ProximityChunk_DeserializeChunk(uint8_t * p_src, uint16_t len);
#endif //OPENBADGE_PROXIMITY_CHUNKS_H
