//
// Created by Andrew Bartow on 10/16/16.
//

#include "app_fifo.h"

#ifndef OPENBADGE_APP_FIFO_UTIL_H
#define OPENBADGE_APP_FIFO_UTIL_H

// Rewinds the given app_fifo len bytes.
void app_fifo_rewind(app_fifo_t * app_fifo, uint16_t len);

// Returns the number of items currently in the app_fifo.
uint32_t app_fifo_len(app_fifo_t * app_fifo);

// Reads at most len bytes from app_fifo into dest.
// Returns the number of bytes that were read into dest.
uint16_t app_fifo_get_bytes(app_fifo_t * app_fifo, uint8_t * dest, uint16_t len);

// Puts len bytes from data into app_fifo.
// Returns the number of bytes from data that were put into app_fifo.
uint16_t app_fifo_put_bytes(app_fifo_t * app_fifo, const uint8_t * data, const uint16_t len);

// Puts the len bytes at data into the app_fifo, discarding existing data as needed.
// If len >= app_fifo_len(app_fifo), app_fifo will contain the last app_fifo_len bytes of data in order.'
// Returns NRF_SUCCESS if completed successfully.
uint32_t app_fifo_put_bytes_circular(app_fifo_t * app_fifo, const uint8_t * data, const uint16_t len);

#endif //OPENBADGE_APP_FIFO_UTIL_H
