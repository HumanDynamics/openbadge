//
// Created by Andrew Bartow on 10/16/16.
//
#include <app_error.h>

#include "app_fifo_util.h"

// Rewinds the given app_fifo len bytes.
void app_fifo_rewind(app_fifo_t * app_fifo, uint16_t len) {
    app_fifo->read_pos -= len;
}

// Returns the number of items currently in the app_fifo.
uint32_t app_fifo_len(app_fifo_t * app_fifo) {
    return app_fifo->write_pos - app_fifo->read_pos;
}


// Reads at most len bytes from app_fifo into dest.
// Returns the number of bytes that were read into dest.
uint16_t app_fifo_get_bytes(app_fifo_t * app_fifo, uint8_t * dest, uint16_t len) {
    APP_ERROR_CHECK_BOOL(app_fifo != NULL);
    APP_ERROR_CHECK_BOOL(dest != NULL);

    uint16_t num_bytes_read = 0;
    for (int i = 0; i < len; i ++) {
        if (app_fifo_get(app_fifo, &dest[i]) != NRF_SUCCESS) {
            break;
        }

        num_bytes_read++;
    }

    return num_bytes_read;
}

// Puts len bytes from data into app_fifo.
// Returns the number of bytes from data that were put into app_fifo.
uint16_t app_fifo_put_bytes(app_fifo_t * app_fifo, const uint8_t * data, const uint16_t len) {
    APP_ERROR_CHECK_BOOL(app_fifo != NULL);
    APP_ERROR_CHECK_BOOL(data != NULL);

    uint16_t num_bytes_put = 0;
    for (uint16_t i = 0; i < len; i++) {
        if (app_fifo_put(app_fifo, data[i]) != NRF_SUCCESS) {
            break;
        }

        num_bytes_put++;
    }

    return num_bytes_put;
}

// Puts the len bytes at data into the app_fifo, discarding existing data as needed.
// If len >= app_fifo_len(app_fifo), app_fifo will contain the last app_fifo_len bytes of data in order.'
// Returns NRF_SUCCESS if completed successfully.
uint32_t app_fifo_put_bytes_circular(app_fifo_t * app_fifo, const uint8_t * data, const uint16_t len) {
    APP_ERROR_CHECK_BOOL(app_fifo != NULL);
    APP_ERROR_CHECK_BOOL(data != NULL);

    for (uint16_t i = 0; i < len; i++) {
        uint32_t err_code = app_fifo_put(app_fifo, data[i]);

        if (err_code != NRF_SUCCESS) {
            if (err_code == NRF_ERROR_NO_MEM) {
                // app_fifo is full, remove last byte and try again.
                uint8_t byteToDiscard;
                APP_ERROR_CHECK(app_fifo_get(app_fifo, &byteToDiscard));
                APP_ERROR_CHECK(app_fifo_put(app_fifo, data[i]));
            } else {
                APP_ERROR_CHECK(err_code);
            }
        }
    }

    return NRF_SUCCESS;
}