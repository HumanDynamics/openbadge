//
// Created by Andrew Bartow on 1/11/17.
//

#ifndef OPENBADGE_FLASH_LAYOUT_UTILS_H
#define OPENBADGE_FLASH_LAYOUT_UTILS_H

#include <stdint.h>
#include <app_error.h>

#include "flash_layout.h""

#define INVALID_FLASH_ADDR 0xFFFFFFFF

#define FLASH_ADDR_VALID(FLASH_ADDR) ((FLASH_START <= FLASH_ADDR) && (FLASH_ADDR <= FLASH_END))

#define FLASH_RANGE_VALID(START_ADDR, LEN) (FLASH_ADDR_VALID(START_ADDR) && (0 <= LEN) && (LEN <= (FLASH_SIZE - START_ADDR)))

#define APP_ERROR_CHECK_FLASH_ADDR_VALID(FLASH_ADDR)                                      \
            APP_ERROR_CHECK_BOOL(FLASH_ADDR_VALID(FLASH_ADDR))

#define APP_ERROR_CHECK_FLASH_RANGE_VALID(START_ADDR, LEN)                                \
            APP_ERROR_CHECK_BOOL(FLASH_RANGE_VALID(START_ADDR, LEN))

/**
 * @return len if len < FLASH_PAGE_SIZE, otherwise FLASH_PAGE_SIZE.
 */
static uint32_t clip_to_page_size(uint32_t len);

/**
 * Calculates the address in EXT flash for a given flash address.
 * @param flash_addr The address in our contingous flash block abstraction.
 * @return  The cooresponding address in EXT flash, or INVALID_FLASH_ADDR if no such address exists.
 */
uint32_t flash_addr_to_ext_addr(uint32_t flash_addr);

/**
 * Calculates the address in NRF flash for a given flash address.
 * @param flash_addr The address in our contingous flash block abstraction.
 * @return  The cooresponding address in NRF flash, or INVALID_FLASH_ADDR if no such address exists.
 */
uint32_t flash_addr_to_nrf_addr(uint32_t flash_addr);

/**
 * @param flash_addr An address in our contigous flash region
 * @return true iff that flash_addr is backed by EXT flash.
 */
bool flash_addr_in_ext_section(uint32_t flash_addr);

/**
 * @param flash_addr An address in our contigous flash region
 * @return true iff that flash_addr is backed by NRF flash.
 */
bool flash_addr_in_nrf_section(uint32_t flash_addr);

/**
 * Calculates the size of the given region in our contigious flash block that is in the EXT flash portion of the block.
 * @param start_addr The start address of the region in our contigous flash block.
 *    Must be FLASH_START <= start_addr <= FLASH_END
 * @param len The length of the region in our contigous flash block. Must be 0 <= len <= (FLASH_SIZE - start_addr)
 * @return The length of the region that is backed by EXT flash.
 */
uint32_t clip_region_len_to_ext_flash(uint32_t start_addr, uint32_t len);

/**
 * Calculates the size of the given region in our contigious flash block that is in the NRF flash portion of the block.
 * @param start_addr The start address of the region in our contigous flash block.
 *    Must be FLASH_START <= start_addr <= FLASH_END
 * @param len The length of the region in our contigous flash block. Must be 0 <= len <= (FLASH_SIZE - start_addr)
 * @return The length of the region that is backed by NRF flash.
 */
uint32_t clip_region_len_to_nrf_flash(uint32_t start_addr, uint32_t len);

/**
 * Checks if any of the given contigous flash region is backed by EXT flash.
 * @param start_addr The start address of the region in our contigous flash block.
 *    Must be FLASH_START <= start_addr <= FLASH_END
 * @param len The length of the region in our contigous flash block. Must be 0 <= len <= (FLASH_SIZE - start_addr)
 * @return True iff any of the given region is backed by external flash. False otherwise.
 */
bool region_contains_ext_flash(uint32_t start_addr, uint32_t len);

/**
 * Checks if any of the given contigous flash region is backed by NRF flash.
 * @param start_addr The start address of the region in our contigous flash block. Must be a valid flash address.
 * @param len The length of the region in our contigous flash block. Must be 0 <= len <= (FLASH_SIZE - start_addr)
 * @return True iff any of the given region is backed by internal flash. False otherwise.
 */
bool region_contains_nrf_flash(uint32_t start_addr, uint32_t len);

/**
 * Returns the address of the first byte of the region that is backed by NRF flash. At least some of the provided region
 *   must be backed by NRF flash.
 * @param start_addr The start address of the region in our contigous flash block. Must be a valid flash address.
 * @param len The length of the region in our cintigous flash block. Must be 0 <= len <= (FLASH_SIZE - start_addr)
 * @return The flash address of the first byte of the given range that is backed by the NRF flash.
 */
uint32_t clip_region_start_to_nrf_flash(uint32_t start_addr, uint32_t len);

#endif //OPENBADGE_FLASH_LAYOUT_UTILS_H
