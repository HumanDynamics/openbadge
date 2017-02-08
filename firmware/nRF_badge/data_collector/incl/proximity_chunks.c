//
// Created by Andrew Bartow on 2/3/17.
//

#include <string.h>
#include <stdint.h>

#include "app_error.h"
#include "proximity_chunks.h"

#define CHUNK_HEADER_LEN (sizeof(ProximityChunk_t) - (sizeof(SeenDevice_t) * MAX_DEVICES_PER_CHUNK))

void ProximityChunk_SerializeChunk(ProximityChunk_t chunk, uint8_t * p_dst, uint16_t * p_serialized_len) {
    APP_ERROR_CHECK_BOOL(p_dst != NULL);
    APP_ERROR_CHECK_BOOL(p_serialized_len != NULL);

    // Implicit dependency warning: ProximityChunk_t stores the header at the beginning of the struct, so we
    //   just use that as the header.
    APP_ERROR_CHECK_BOOL(&chunk.seen_devices == (void * )(&chunk + CHUNK_HEADER_LEN));
    memcpy(p_dst, &chunk, CHUNK_HEADER_LEN);
    // Serialize just the captured devices into p_dst after the header.
    memcpy(&p_dst[CHUNK_HEADER_LEN], chunk.seen_devices, sizeof(SeenDevice_t) * chunk.num_devices_seen);
}

ProximityChunk_t ProximityChunk_DeserializeChunk(uint8_t * p_src, uint16_t len) {
    APP_ERROR_CHECK_BOOL(p_src != NULL);
    APP_ERROR_CHECK_BOOL(len >= CHUNK_HEADER_LEN);

    ProximityChunk_t proximity_chunk;
    memcpy(&proximity_chunk, p_src, CHUNK_HEADER_LEN);
    for (int i = 0; i < proximity_chunk.num_devices_seen; i++) {
        SeenDevice_t device;
        memcpy(&device, &p_src[CHUNK_HEADER_LEN + (i * sizeof(SeenDevice_t))], sizeof(SeenDevice_t));
        proximity_chunk.seen_devices[i] = device;
    }

    APP_ERROR_CHECK(len == CHUNK_HEADER_LEN + (sizeof(SeenDevice_t) * proximity_chunk.num_devices_seen));
}