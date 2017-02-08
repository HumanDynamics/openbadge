//
// Created by Andrew Bartow on 1/11/17.
//


#include <app_util.h>
#include <pstorage.h>
#include <app_fifo.h>
#include <pstorage_platform.h>

#include "flash.h"
#include "calc_macros.h"
#include "ext_eeprom.h"
#include "flash_layout_utils.h"

#define WRITE_BUFFER_HEADER_SIZE sizeof(uint32_t)
#define MACHINE_WORD_SIZE_BYTES  (sizeof(uint32_t) / sizeof(uint8_t))

static pstorage_handle_t mPStorageHandle;

static void handle_flash_success(pstorage_handle_t * handle, uint8_t op_code, uint8_t * p_data, uint32_t data_len) {
    switch (op_code) {
        case PSTORAGE_UPDATE_OP_CODE:
            free(p_data - WRITE_BUFFER_HEADER_SIZE);
            break;
        case PSTORAGE_LOAD_OP_CODE:
            // Nothing to do here.
            break;
        default:
            // Unexpected PStorage Callback Type.
            APP_ERROR_CHECK_BOOL(false);
    }
}

static void handle_flash_timeout(pstorage_handle_t * handle, uint8_t op_code, uint8_t * p_data, uint32_t data_len) {
    // The SD130 Specification indicates we should try again in the case of a PStorage Timeout.

    switch (op_code) {
        case PSTORAGE_UPDATE_OP_CODE:
            debug_log("PStorage Write Timeout, Retrying...\r\n");

            // Get the offset from our buffer. (It's annoyingly not included in the callback.)
            uint32_t write_offset = (uint32_t) *(p_data - WRITE_BUFFER_HEADER_SIZE);

            // Retry writing the data.
            pstorage_update(handle, p_data, (pstorage_size_t) data_len, (uint16_t) write_offset);
            break;
    }
}

static void pstorage_cb_handler(pstorage_handle_t * handle, uint8_t op_code, uint32_t result, uint8_t * p_data,
                                uint32_t data_len) {
    switch (result) {
        case NRF_SUCCESS:
            handle_flash_success(handle, op_code, p_data, data_len);
            break;
        case NRF_ERROR_TIMEOUT:
            handle_flash_timeout(handle, op_code, p_data, data_len);
            break;
        default:
            APP_ERROR_CHECK(result);
            break;

    }
}

// Returns the smallest machine-word aligned range that contains the range defined by start_addr and len.
static void align_range(const uint32_t start_addr, const uint32_t len, uint32_t * aligned_start_addr, uint32_t * aligned_len) {
    // Round our start address down to nearest machine word boundary using integer division truncation.
    *aligned_start_addr = (start_addr / MACHINE_WORD_SIZE_BYTES) * MACHINE_WORD_SIZE_BYTES;

    // Calculate the end of our input range and also round it up to nearest word boundary.
    uint32_t end_addr = start_addr + len - 1;
    uint32_t end_addr_word_index =  (end_addr / MACHINE_WORD_SIZE_BYTES);
    uint32_t aligned_end_addr = ((end_addr_word_index + 1) * MACHINE_WORD_SIZE_BYTES) - 1;

    // Calculate the length of the word aligned range by using the aligned start and end addresses.
    *aligned_len = (aligned_end_addr - *aligned_start_addr + 1);
}

static uint8_t * buffer_pstorage_write(uint8_t * src, uint16_t len, uint32_t offset) {
    uint8_t * buffer = (uint8_t *) malloc(len + WRITE_BUFFER_HEADER_SIZE);
    APP_ERROR_CHECK_BOOL(buffer != NULL);

    memcpy(buffer, &offset, WRITE_BUFFER_HEADER_SIZE);
    memcpy(&buffer[WRITE_BUFFER_HEADER_SIZE], src, len);
    return &buffer[WRITE_BUFFER_HEADER_SIZE];
}

void flash_init(void) {
    ext_eeprom_global_unprotect();
    ext_eeprom_wait();

    pstorage_init();

    pstorage_module_param_t params;
    params.block_count = NRF_FLASH_SIZE_PAGES;
    params.block_size = FLASH_PAGE_SIZE;
    params.cb = pstorage_cb_handler;

    APP_ERROR_CHECK(pstorage_register(&params, &mPStorageHandle));
}

uint32_t flash_write(uint32_t dst, uint8_t * src, uint32_t len) {
    if (!FLASH_RANGE_VALID(dst, len)) return NRF_ERROR_INVALID_PARAM;
    if(len == 0) return NRF_SUCCESS; // Writes with no length should be NOPs.

    uint32_t aligned_dst = dst;
    uint8_t * aligned_src = src;
    uint32_t aligned_len = len;

    // Check if input arguments are machine word aligned, if they're not, pad them with existing flash data so they are.
    if (!is_word_aligned(src) || (dst % 4) != 0 || (len % 4) != 0) {
        // Calculate the smallest machine aligned range that contains the write destination.
        align_range(dst, len, &aligned_dst, &aligned_len);

        // Allocate a buffer to hold the padded write data and read the current contents of flash at the destination into
        //  that buffer.
        aligned_src = (uint8_t *) malloc(aligned_len);
        flash_read(aligned_src, aligned_dst, aligned_len);

        // Calculate where in our aligned buffer our write begins.
        uint8_t * aligned_src_input_write_start = &aligned_src[dst - aligned_dst];

        // Copy our write contents into our aligned buffer at the proper location.
        memcpy(aligned_src_input_write_start, src, len);
    }

    // Our input parameters are now word aligned, and both of the EXT and NRF flash portions are word aligned,
    //   so each of our sub-writes should also be word aligned.

    if (region_contains_ext_flash(aligned_dst, aligned_len)) {
        uint32_t ext_len = clip_region_len_to_ext_flash(aligned_dst, aligned_len);

        ext_eeprom_write(aligned_dst, aligned_src, ext_len);
        // It should not take long for the SPI TX to complete, so let's just wait for it.
        // This simplifies our code and does not require us to deal with queueing multiple EEPROM operations.
        ext_eeprom_wait();
    }

    if (region_contains_nrf_flash(aligned_dst, aligned_len)) {
        uint32_t nrf_dst_offset = clip_region_start_to_nrf_flash(aligned_dst, aligned_len) - aligned_dst;

        uint8_t * nrf_src = &aligned_src[nrf_dst_offset];
        uint32_t nrf_dst = flash_addr_to_nrf_addr(clip_region_start_to_nrf_flash(aligned_dst, aligned_len));
        uint32_t nrf_len = clip_region_len_to_nrf_flash(aligned_dst, aligned_len);

        uint32_t bytes_written = 0;
        while (bytes_written < nrf_len) {
            pstorage_size_t page = (pstorage_size_t) ((nrf_dst + bytes_written) / FLASH_PAGE_SIZE);
            pstorage_size_t page_offset = (pstorage_size_t) ((nrf_dst + bytes_written) % FLASH_PAGE_SIZE);
            pstorage_size_t page_write_len = (pstorage_size_t) MIN(nrf_len - bytes_written, (uint32_t) FLASH_PAGE_SIZE - page_offset);

            pstorage_handle_t block_handle;
            uint32_t err_code = pstorage_block_identifier_get(&mPStorageHandle, page, &block_handle);
            APP_ERROR_CHECK(err_code);

            // Writing to PStorage may take some time, and we don't want our clients to have to deal with callbacks or to
            //   block for that long, so we're going to transfer the data for the PStorage write to a buffer we manage,
            //   so that our client can free its memory immediately after we return.
            uint8_t * nrf_src_buffered = buffer_pstorage_write(&nrf_src[bytes_written], page_write_len, page_offset);

            err_code = pstorage_update(&block_handle, nrf_src_buffered, page_write_len, page_offset);
            APP_ERROR_CHECK(err_code);

            bytes_written += page_write_len;
        }
    }

    if (aligned_src != src) free(aligned_src); // We allocated an aligned_src buffer, we should free it.

    return NRF_SUCCESS;
}

uint32_t flash_read(uint8_t * dst, uint32_t src, uint32_t len) {
    if (!FLASH_RANGE_VALID(src, len)) return NRF_ERROR_INVALID_PARAM;
    if (len == 0) return NRF_SUCCESS;

    uint8_t * aligned_dst = dst;
    uint32_t aligned_src = src;
    uint32_t aligned_len = len;

    if (!is_word_aligned(dst) || (src % 4) != 0 || (len % 4) != 0) {
        align_range(src, len, &aligned_src, &aligned_len);
        aligned_dst = (uint8_t *) malloc(aligned_len);
    }

    if (region_contains_ext_flash(aligned_src, aligned_len)) {
        uint32_t ext_len = clip_region_len_to_ext_flash(aligned_src, aligned_len);
        ext_eeprom_read(aligned_src, aligned_dst, ext_len);
        ext_eeprom_wait();
    }

    if (region_contains_nrf_flash(aligned_src, aligned_len)) {
        uint32_t nrf_src_offset = clip_region_start_to_nrf_flash(aligned_src, aligned_len) - aligned_src;

        uint8_t * nrf_dst = &dst[nrf_src_offset];
        uint32_t nrf_src = flash_addr_to_nrf_addr(clip_region_start_to_nrf_flash(aligned_src, aligned_len));
        uint32_t nrf_len = clip_region_len_to_nrf_flash(aligned_src, aligned_len);

        uint32_t bytes_read = 0;
        while (bytes_read < nrf_len) {
            pstorage_size_t page = (pstorage_size_t) ((nrf_src + bytes_read) / FLASH_PAGE_SIZE);
            pstorage_size_t page_offset = (pstorage_size_t) ((nrf_src + bytes_read) % FLASH_PAGE_SIZE);
            pstorage_size_t page_read_len = (pstorage_size_t) MIN(nrf_len - bytes_read, (uint32_t) FLASH_PAGE_SIZE - page_offset);

            pstorage_handle_t block_handle;
            uint32_t err_code = pstorage_block_identifier_get(&mPStorageHandle, page, &block_handle);
            APP_ERROR_CHECK(err_code);

            err_code = pstorage_load(&nrf_dst[bytes_read], &block_handle, page_read_len, page_offset);
            APP_ERROR_CHECK(err_code);

            bytes_read += page_read_len;
        }
    }

    if (dst != aligned_dst) {
        // We allocated an aligned_dst buffer, we should copy our result from it into our input argument and free
        //   the buffer we allocated.
        uint8_t * aligned_dst_input_read_start = &aligned_dst[src - aligned_src];
        memcpy(dst, aligned_dst_input_read_start, len);
        free(aligned_dst);
    }

    return NRF_SUCCESS;
}