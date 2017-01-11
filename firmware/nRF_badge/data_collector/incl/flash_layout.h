//
// Created by Andrew Bartow on 1/11/17.
//

#ifndef OPENBADGE_FLASH_LAYOUT_H
#define OPENBADGE_FLASH_LAYOUT_H

#define FLASH_PAGE_SIZE  1024

#define NRF_FLASH_PAGE_START 155
#define NRF_FLASH_PAGE_END   251
#define NRF_FLASH_SIZE_PAGES (NRF_FLASH_PAGE_END - NRF_FLASH_PAGE_START + 1)
#define NRF_FLASH_SIZE       (NRF_FLASH_SIZE_PAGES * FLASH_PAGE_SIZE)

#define EXT_FLASH_SIZE_PAGES 256
#define EXT_FLASH_SIZE       (EXT_FLASH_SIZE_PAGES * FLASH_PAGE_SIZE)

#define FLASH_SIZE           (EXT_FLASH_SIZE + NRF_FLASH_SIZE)
#define FLASH_START          0
#define FLASH_END            (FLASH_SIZE - 1)


// Our contiguous flash block is laid out such that the first portion is backed by the external flash and the second
//   portion is backed by the internal NRF flash.
#define EXT_FLASH_START      FLASH_START
#define EXT_FLASH_END        (EXT_FLASH_SIZE - 1)
#define NRF_FLASH_START      EXT_FLASH_SIZE
#define NRF_FLASH_END        FLASH_END

#endif //OPENBADGE_FLASH_LAYOUT_H
