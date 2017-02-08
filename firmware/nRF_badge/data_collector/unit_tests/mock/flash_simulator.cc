//
// Created by Andrew Bartow on 1/11/17.
//

#include <stdexcept>
#include "gtest/gtest.h"
#include "pstorage.h"
#include "flash_layout.h"
#include "nrf_error.h"
#include "app_util.h"

#define EXT_EEPROM_SUCCESS     0

#define PSTORAGE_MODULE_ID     0
#define PSTORAGE_BASE_BLOCK_ID 0

#define WRITE_TIMEOUTS_DISABLED -1

pstorage_ntf_cb_t mPStorageCallback;

static bool is_ext_flash_unlocked = false;
static int writes_until_pstorage_timeout = WRITE_TIMEOUTS_DISABLED;

static uint8_t mEEPROM[EXT_FLASH_SIZE];
static uint8_t mPStorage[NRF_FLASH_SIZE];

void reset_simulated_flash(void) {
    memset(mEEPROM, 0, sizeof(mEEPROM));
    memset(mPStorage, 0, sizeof(mPStorage));

    writes_until_pstorage_timeout = WRITE_TIMEOUTS_DISABLED;
}

void pstorage_update_trigger_timeout(int operations_to_wait) {
    writes_until_pstorage_timeout = operations_to_wait;
}

uint32_t pstorage_init(void) { return NRF_SUCCESS; }

uint32_t pstorage_register(pstorage_module_param_t * p_module_param, pstorage_handle_t * p_block_id) {
    if (p_module_param->block_count * p_module_param->block_size != NRF_FLASH_SIZE) {
        throw std::runtime_error("Requested incorrect flash size on pstorage_register()");
    }

    p_block_id->module_id = PSTORAGE_MODULE_ID;
    p_block_id->block_id = PSTORAGE_BASE_BLOCK_ID;
    mPStorageCallback = p_module_param->cb;

    return NRF_SUCCESS;
}

uint32_t pstorage_block_identifier_get(pstorage_handle_t * p_base_id, pstorage_size_t block_num,
                                       pstorage_handle_t * p_block_id) {
    if (p_base_id == NULL || p_block_id == NULL) return NRF_ERROR_INVALID_PARAM;
    if (p_base_id->module_id != PSTORAGE_MODULE_ID || p_base_id->block_id != PSTORAGE_BASE_BLOCK_ID) {
        throw std::runtime_error("Invalid PStorage Module or Base Block");
    }

    p_block_id->module_id = PSTORAGE_MODULE_ID;
    p_block_id->block_id = p_base_id->block_id + block_num;

    return NRF_SUCCESS;
}

static bool should_pstorage_update_timeout(void) {
    if (writes_until_pstorage_timeout == 0) {
        writes_until_pstorage_timeout = WRITE_TIMEOUTS_DISABLED;
        return true;
    } else if (writes_until_pstorage_timeout != WRITE_TIMEOUTS_DISABLED) {
        writes_until_pstorage_timeout--;
        return false;
    } else {
        return false;
    }
}

uint32_t pstorage_update(pstorage_handle_t * p_dest, uint8_t * p_src, pstorage_size_t size, pstorage_size_t offset) {
    if (p_dest == NULL || p_src == NULL) return NRF_ERROR_INVALID_PARAM;
    if (!is_word_aligned(p_src) || !is_word_aligned((void *) (uint32_t) offset)) return NRF_ERROR_INVALID_ADDR;
    if (p_dest->module_id != PSTORAGE_MODULE_ID) throw std::runtime_error("Invalid PStorage Module");
    if ((size + offset) > FLASH_PAGE_SIZE) throw std::runtime_error("Wrote past end of page!");

    if (should_pstorage_update_timeout()) {
        mPStorageCallback(p_dest, PSTORAGE_UPDATE_OP_CODE, NRF_ERROR_TIMEOUT, p_src, size);
    } else {
        uint32_t pstorage_dst_addr = (FLASH_PAGE_SIZE * p_dest->block_id) + offset;
        memcpy(&mPStorage[pstorage_dst_addr], p_src, size);
        mPStorageCallback(p_dest, PSTORAGE_UPDATE_OP_CODE, NRF_SUCCESS, p_src, size);
    }

    return NRF_SUCCESS;
}


uint32_t pstorage_load(uint8_t * p_dest, pstorage_handle_t * p_src, pstorage_size_t size, pstorage_size_t offset) {
    if (p_dest == NULL || p_src == NULL) return NRF_ERROR_INVALID_PARAM;
    if (!is_word_aligned(p_dest) || !is_word_aligned((void *) (uint32_t) offset)) return NRF_ERROR_INVALID_ADDR;
    if (p_src->module_id != PSTORAGE_MODULE_ID) throw std::runtime_error("Invalid PStorage Module");
    if ((size + offset) > FLASH_PAGE_SIZE) throw std::runtime_error("Read past end of page!");

    uint32_t pstorage_src_addr = (FLASH_PAGE_SIZE * p_src->block_id) + offset;
    memcpy(p_dest, &mPStorage[pstorage_src_addr], size);

    mPStorageCallback(p_src, PSTORAGE_LOAD_OP_CODE, NRF_SUCCESS, p_dest, size);

    return NRF_SUCCESS;
}

uint32_t ext_eeprom_global_unprotect() {
    is_ext_flash_unlocked = true;

    return EXT_EEPROM_SUCCESS;
}

void ext_eeprom_wait() {};

uint32_t ext_eeprom_write(unsigned int address, uint8_t* tx, unsigned int numBytes) {
    if (is_ext_flash_unlocked) {
        memcpy(&mEEPROM[address], tx, numBytes);
    } else {
        throw new std::runtime_error("Attempted to write to EEPROM without unlocking flash.\r\n");
    }

    return EXT_EEPROM_SUCCESS;
}

uint32_t ext_eeprom_read(unsigned int address, uint8_t* rx, unsigned int numBytes) {
    memcpy(rx, &mEEPROM[address], numBytes);
    return EXT_EEPROM_SUCCESS;
}