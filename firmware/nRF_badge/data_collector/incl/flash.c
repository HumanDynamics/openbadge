//
// Created by Andrew Bartow on 1/11/17.
//


#include <app_util.h>
#include <pstorage.h>

#include "flash.h"
#include "ext_eeprom.h"
#include "flash_layout_utils.h"

static pstorage_handle_t mPStorageHandle;

static void pstorage_cb_handler(pstorage_handle_t * handle, uint8_t op_code, uint32_t result, uint8_t * p_data,
                                uint32_t data_len) {
    debug_log("Got PStorage Callback!\r\n");
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

uint32_t flash_write(uint8_t * src, uint32_t dst, uint32_t len) {
    if (!FLASH_RANGE_VALID(dst, len)) return NRF_ERROR_INVALID_PARAM;
    if (!is_word_aligned(src) || (dst % 4) != 0 || (len % 4) != 0) return NRF_ERROR_INVALID_ADDR;

    // We require out input parameters to be word aligned, and both of the EXT and NRF flash portions are word aligned,
    //   so each of our sub-writes should also be word aligned.

    if (region_contains_ext_flash(dst, len)) {
        uint32_t ext_len = clip_region_len_to_ext_flash(dst, len);
        ext_eeprom_write(dst, src, ext_len);
    }

    if (region_contains_nrf_flash(dst, len)) {
        uint32_t nrf_dst_offset = clip_region_start_to_nrf_flash(dst, len) - dst;

        uint8_t * nrf_src = &src[nrf_dst_offset];
        uint32_t nrf_dst = flash_addr_to_nrf_addr(clip_region_start_to_nrf_flash(dst, len));
        uint32_t nrf_len = clip_region_len_to_nrf_flash(dst, len);

        uint32_t bytes_written = 0;
        while (bytes_written < nrf_len) {
            pstorage_size_t page = (pstorage_size_t) ((nrf_dst + bytes_written) / FLASH_PAGE_SIZE);
            pstorage_size_t page_write_len = (pstorage_size_t) clip_to_page_size(nrf_len - bytes_written);
            pstorage_size_t page_offset = (pstorage_size_t) ((nrf_dst + bytes_written) % FLASH_PAGE_SIZE);

            pstorage_handle_t block_handle;
            uint32_t err_code = pstorage_block_identifier_get(&mPStorageHandle, page, &block_handle);
            APP_ERROR_CHECK(err_code);

            err_code = pstorage_update(&block_handle, &nrf_src[bytes_written], page_write_len, page_offset);
            APP_ERROR_CHECK(err_code);

            bytes_written += page_write_len;
        }
    }
}