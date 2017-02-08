//
// Created by Andrew Bartow on 1/11/17.
//

#include "flash_layout_utils.h"
#include "debug_log.h"

/**
 * @return len if len < FLASH_PAGE_SIZE, otherwise FLASH_PAGE_SIZE.
 */
uint32_t clip_to_page_size(uint32_t len) {
    return (len > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : len;
}

/**
 * Calculates the address in EXT flash for a given flash address.
 * @param flash_addr The address in our contingous flash block abstraction.
 * @return  The cooresponding address in EXT flash, or INVALID_FLASH_ADDR if no such address exists.
 */
uint32_t flash_addr_to_ext_addr(uint32_t flash_addr) {
    if (flash_addr >= EXT_FLASH_START && flash_addr <= EXT_FLASH_END) {
        return flash_addr - EXT_FLASH_START;
    } else {
        return INVALID_FLASH_ADDR;
    }
}

/**
 * Calculates the address in NRF flash for a given flash address.
 * @param flash_addr The address in our contingous flash block abstraction.
 * @return  The cooresponding address in NRF flash, or INVALID_FLASH_ADDR if no such address exists.
 */
uint32_t flash_addr_to_nrf_addr(uint32_t flash_addr) {
    if (flash_addr >= NRF_FLASH_START && flash_addr <= NRF_FLASH_END) {
        return flash_addr - NRF_FLASH_START;
    } else {
        return INVALID_FLASH_ADDR;
    }
}

bool is_flash_addr_in_ext_section(uint32_t flash_addr) {
    return flash_addr_to_ext_addr(flash_addr) != INVALID_FLASH_ADDR;
}

bool is_flash_addr_in_nrf_section(uint32_t flash_addr) {
    return flash_addr_to_nrf_addr(flash_addr) != INVALID_FLASH_ADDR;
}

/**
 * Calculates the size of the given region in our contigious flash block that is in the EXT flash portion of the block.
 * @param start_addr The start address of the region in our contigous flash block.
 *    Must be FLASH_START <= start_addr <= FLASH_END
 * @param len The length of the region in our contigous flash block. Must be 0 <= len <= (FLASH_SIZE - start_addr)
 * @return The length of the region that is backed by EXT flash.
 */
uint32_t clip_region_len_to_ext_flash(uint32_t start_addr, uint32_t len) {
    APP_ERROR_CHECK_FLASH_RANGE_VALID(start_addr, len);
    if (len == 0) return 0;

    uint32_t end_addr = start_addr + len - 1;

    if (is_flash_addr_in_ext_section(start_addr) && is_flash_addr_in_ext_section(end_addr)) {
        return end_addr - start_addr + 1;
    } else if (!is_flash_addr_in_ext_section(start_addr) && is_flash_addr_in_ext_section(end_addr)) {
        APP_ERROR_CHECK_BOOL(false); // This is impossible, would mean start_addr is before start of flash.
    } else if (is_flash_addr_in_ext_section(start_addr) && !is_flash_addr_in_ext_section(end_addr)) {
        return EXT_FLASH_END - start_addr + 1;
    } else if (!is_flash_addr_in_ext_section(start_addr) && !is_flash_addr_in_ext_section(end_addr)) {
        return 0;
    }

    APP_ERROR_CHECK_BOOL(false);
    return NRF_ERROR_INTERNAL;
}


/**
 * Calculates the size of the given region in our contigious flash block that is in the NRF flash portion of the block.
 * @param start_addr The start address of the region in our contigous flash block.
 *    Must be FLASH_START <= start_addr <= FLASH_END
 * @param len The length of the region in our contigous flash block. Must be 0 <= len <= (FLASH_SIZE - start_addr)
 * @return The length of the region that is backed by NRF flash.
 */
uint32_t clip_region_len_to_nrf_flash(uint32_t start_addr, uint32_t len) {
    APP_ERROR_CHECK_FLASH_RANGE_VALID(start_addr, len);
    if (len == 0) return 0;

    uint32_t end_addr = start_addr + len - 1;

    if (is_flash_addr_in_nrf_section(start_addr) && is_flash_addr_in_nrf_section(end_addr)) {
        return end_addr - start_addr + 1;
    } else if (!is_flash_addr_in_nrf_section(start_addr) && is_flash_addr_in_nrf_section(end_addr)) {
        return end_addr - NRF_FLASH_START + 1;
    } else if (is_flash_addr_in_nrf_section(start_addr) && !is_flash_addr_in_nrf_section(end_addr)) {
        APP_ERROR_CHECK_BOOL(false); // This is impossible, would be mean end_addr is after end of flash.
    } else if (!is_flash_addr_in_nrf_section(start_addr) && !is_flash_addr_in_nrf_section(end_addr)) {
        return 0;
    }

    APP_ERROR_CHECK_BOOL(false);
    return NRF_ERROR_INTERNAL;
}

/**
 * Checks if any of the given contigous flash region is backed by EXT flash.
 * @param start_addr The start address of the region in our contigous flash block.
 *    Must be FLASH_START <= start_addr <= FLASH_END
 * @param len The length of the region in our contigous flash block. Must be 0 <= len <= (FLASH_SIZE - start_addr)
 * @return True iff any of the given region is backed by external flash. False otherwise.
 */
bool region_contains_ext_flash(uint32_t start_addr, uint32_t len) {
    APP_ERROR_CHECK_FLASH_RANGE_VALID(start_addr, len);

    return clip_region_len_to_ext_flash(start_addr, len) > 0;
}

/**
 * Checks if any of the given contigous flash region is backed by NRF flash.
 * @param start_addr The start address of the region in our contigous flash block. Must be a valid flash address.
 * @param len The length of the region in our contigous flash block. Must be 0 <= len <= (FLASH_SIZE - start_addr)
 * @return True iff any of the given region is backed by internal flash. False otherwise.
 */
bool region_contains_nrf_flash(uint32_t start_addr, uint32_t len) {
    APP_ERROR_CHECK_FLASH_RANGE_VALID(start_addr, len);

    return clip_region_len_to_nrf_flash(start_addr, len) > 0;
}

/**
 * Returns the address of the first byte of the region that is backed by NRF flash. At least some of the provided region
 *   must be backed by NRF flash.
 * @param start_addr The start address of the region in our contigous flash block. Must be a valid flash address.
 * @param len The length of the region in our cintigous flash block. Must be 0 <= len <= (FLASH_SIZE - start_addr)
 * @return The flash address of the first byte of the given range that is backed by the NRF flash.
 */
uint32_t clip_region_start_to_nrf_flash(uint32_t start_addr, uint32_t len) {
    APP_ERROR_CHECK_FLASH_RANGE_VALID(start_addr, len);
    APP_ERROR_CHECK_BOOL(region_contains_nrf_flash(start_addr, len));

    return is_flash_addr_in_nrf_section(start_addr) ? start_addr : NRF_FLASH_START;
}

