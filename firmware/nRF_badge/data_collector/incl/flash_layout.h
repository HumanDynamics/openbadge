//
// Created by Andrew Bartow on 1/11/17.
//

#ifndef OPENBADGE_FLASH_LAYOUT_H
#define OPENBADGE_FLASH_LAYOUT_H

// Size of a flash page, in bytes
#define FLASH_PAGE_SIZE  1024

// Size of NRF PStorage in pages, set in pstorage_platform.h
#define NRF_FLASH_SIZE_PAGES  100
#define NRF_FLASH_SIZE       (NRF_FLASH_SIZE_PAGES * FLASH_PAGE_SIZE)

// Size of the Atmel AT25M02 EEPROM in pages, from its specification sheet
#define EXT_FLASH_SIZE_PAGES 256
#define EXT_FLASH_SIZE       (EXT_FLASH_SIZE_PAGES * FLASH_PAGE_SIZE)

// Size of our single contiguous flash block abstraction
#define FLASH_SIZE           (EXT_FLASH_SIZE + NRF_FLASH_SIZE)
#define FLASH_START          0
#define FLASH_END            (FLASH_SIZE - 1)


// Our contiguous flash block is laid out such that the first portion is backed by the external flash and the second
//   portion is backed by the internal NRF flash.
#define EXT_FLASH_START      FLASH_START
#define EXT_FLASH_END        (EXT_FLASH_SIZE - 1)
#define NRF_FLASH_START      EXT_FLASH_SIZE
#define NRF_FLASH_END        FLASH_END

// We allocate a fourth of a page of flash to store various things directly, and the rest of flash for our filesystem.
#define MISC_FLASH_SIZE      (FLASH_PAGE_SIZE / 4)
#define BADGE_FS_SIZE        (FLASH_SIZE - MISC_FLASH_SIZE)

#define MISC_FLASH_START     FLASH_START
#define MISC_FLASH_END       (MISC_FLASH_START + MISC_FLASH_SIZE - 1)
#define BADGE_FS_START       (MISC_FLASH_END + 1)
#define BADGE_FS_END         (BADGE_FS_START + BADGE_FS_SIZE - 1)

#endif //OPENBADGE_FLASH_LAYOUT_H
