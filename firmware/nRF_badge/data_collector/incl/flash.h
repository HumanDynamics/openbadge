//
// Created by Andrew Bartow on 1/11/17.
//

#ifndef OPENBADGE_FLASH_H
#define OPENBADGE_FLASH_H

#import <stdint.h>

/**
 * Initializes the flash storage module.
 *
 * The flash storage module synthesizes both the built in flash storage in the NRF51 and the EXT EEPROM chip into a
 *   single block of contiguous flash of size FLASH_SIZE. This block of flash is addressed in range
 *   [FLASH_START, FLASH_END]
 *
 * Writes/reads to earlier addresses in flash space (<= EXT_FLASH_END) should be perfered to writes later on in flash as
 *   the first portion of this block is backed by the EEPROM, which has perferable physical characteristics to the
 *   internal Nordic flash.
 */
void flash_init(void);

/**
 * Writes 'len' bytes from 'src' to 'dst' in flash. Data currently in flash will be over written if neccesary.
 * Method does not block until data is written, but data is gauranteed to be written in FIFO manner.
 *
 * @param src Byte stream to write to flash.
 *     Buffer must be at least 'len' bytes long.
 *     Buffer can be freed after method returns
 *     Buffer must be word aligned.
 * @param dst Flash address of destination. Must be FLASH_START <= dst <= FLASH_END. Must be word aligned.
 * @param len Number of bytes to write. Must be 0 <= len <= (FLASH_SIZE - dst). Must be word aligned.
 *
 * @return NRF_SUCCESS iff successful (gaurantees data will eventually be written to flash). NRF_NO_MEN if no memory
 *    existed to buffer write in. Other error codes may also be returned.
 */
uint32_t flash_write(uint8_t * src, uint32_t dst, uint32_t len);

/**
 * Reads 'len' bytes from 'src' to 'dst' from flash.
 * Method blocks until read is complete.
 *
 * @param src Flash address of source. Must be FLASH_START <= src <= FLASH_END. Must be word aligned.
 * @param dst Buffer to place data read from flash. Must be at least 'len' bytes long. Must be word aligned.
 * @param len Number of bytes to read. Must be 0 <= len <= (FLASH_SIZE - src). Must be word aligned.
 * @return NRF_SUCCESS iff successful. Error code will be returned in case of failure.
 */
uint32_t flash_read(uint32_t src, uint8_t * dst, uint32_t len);

#endif //OPENBADGE_FLASH_H
