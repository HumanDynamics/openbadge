#include "gtest/gtest.h"
#include "flash_layout_utils.h"

TEST(FlashLayoutUtilsTest, ClipToPageSize) {
    EXPECT_EQ(100, clip_to_page_size(100));
    EXPECT_EQ(0, clip_to_page_size(0));
    EXPECT_EQ(FLASH_PAGE_SIZE, clip_to_page_size(FLASH_PAGE_SIZE + 12));
}

TEST(FlashLayoutUtilsTest, FlashAddressToEXTAddress) {
    EXPECT_EQ(12, flash_addr_to_ext_addr(EXT_FLASH_START + 12));

    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_ext_addr(FLASH_START - 1));
    EXPECT_EQ(0, flash_addr_to_ext_addr(FLASH_START));
    EXPECT_EQ(1, flash_addr_to_ext_addr(FLASH_START + 1));
    EXPECT_EQ(EXT_FLASH_SIZE - 2, flash_addr_to_ext_addr(EXT_FLASH_END - 1));
    EXPECT_EQ(EXT_FLASH_SIZE - 1, flash_addr_to_ext_addr(EXT_FLASH_END));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_ext_addr(EXT_FLASH_END + 1));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_ext_addr(NRF_FLASH_START));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_ext_addr(NRF_FLASH_END - 1));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_ext_addr(NRF_FLASH_END));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_ext_addr(NRF_FLASH_END + 1));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_ext_addr(FLASH_END - 1));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_ext_addr(FLASH_END));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_ext_addr(FLASH_END + 1));
}

TEST(FlashLayoutUtilsTest, FlashAddressToNRFAddress) {
    EXPECT_EQ(12, flash_addr_to_nrf_addr(NRF_FLASH_START + 12));

    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_nrf_addr(FLASH_START - 1));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_nrf_addr(FLASH_START));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_nrf_addr(FLASH_START + 1));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_nrf_addr(EXT_FLASH_END - 1));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_nrf_addr(EXT_FLASH_END));
    EXPECT_EQ(0, flash_addr_to_nrf_addr(EXT_FLASH_END + 1));
    EXPECT_EQ(0, flash_addr_to_nrf_addr(NRF_FLASH_START));
    EXPECT_EQ(NRF_FLASH_SIZE - 2, flash_addr_to_nrf_addr(NRF_FLASH_END - 1));
    EXPECT_EQ(NRF_FLASH_SIZE - 1, flash_addr_to_nrf_addr(NRF_FLASH_END));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_nrf_addr(NRF_FLASH_END + 1));
    EXPECT_EQ(NRF_FLASH_SIZE - 2, flash_addr_to_nrf_addr(FLASH_END - 1));
    EXPECT_EQ(NRF_FLASH_SIZE - 1, flash_addr_to_nrf_addr(FLASH_END));
    EXPECT_EQ(INVALID_FLASH_ADDR, flash_addr_to_nrf_addr(FLASH_END + 1));
}

TEST(FlashLayoutUtilsTest, FlashAddressInEXT) {
    EXPECT_TRUE(is_flash_addr_in_ext_section(FLASH_START));
    EXPECT_TRUE(is_flash_addr_in_ext_section(FLASH_START + 3));
    EXPECT_TRUE(is_flash_addr_in_ext_section(EXT_FLASH_END - 1));
    EXPECT_TRUE(is_flash_addr_in_ext_section(EXT_FLASH_END));
    EXPECT_FALSE(is_flash_addr_in_ext_section(EXT_FLASH_END + 1));
    EXPECT_FALSE(is_flash_addr_in_ext_section(NRF_FLASH_START));
    EXPECT_FALSE(is_flash_addr_in_ext_section(NRF_FLASH_END));
    EXPECT_FALSE(is_flash_addr_in_ext_section(NRF_FLASH_END - 3));
    EXPECT_FALSE(is_flash_addr_in_ext_section(FLASH_END + 1));
}

TEST(FlashLayoutUtilsTest, FlashAddressInNRF) {
    EXPECT_FALSE(is_flash_addr_in_nrf_section(FLASH_START));
    EXPECT_FALSE(is_flash_addr_in_nrf_section(FLASH_START + 3));
    EXPECT_FALSE(is_flash_addr_in_nrf_section(EXT_FLASH_END - 1));
    EXPECT_FALSE(is_flash_addr_in_nrf_section(EXT_FLASH_END));
    EXPECT_TRUE(is_flash_addr_in_nrf_section(EXT_FLASH_END + 1));
    EXPECT_TRUE(is_flash_addr_in_nrf_section(NRF_FLASH_START));
    EXPECT_TRUE(is_flash_addr_in_nrf_section(NRF_FLASH_END));
    EXPECT_TRUE(is_flash_addr_in_nrf_section(NRF_FLASH_END - 3));
    EXPECT_FALSE(is_flash_addr_in_nrf_section(FLASH_END + 1));
}

TEST(FlashLayoutUtilsTest, ClipRegionLenToEXTFlash) {
    EXPECT_EQ(0, clip_region_len_to_ext_flash(FLASH_START, 0)); // Region of length 0
    EXPECT_EQ(1, clip_region_len_to_ext_flash(FLASH_START, 1)); // Starts at start of flash.
    EXPECT_EQ(EXT_FLASH_SIZE, clip_region_len_to_ext_flash(FLASH_START, FLASH_SIZE)); // Region contains all of flash.
    EXPECT_EQ(23, clip_region_len_to_ext_flash(FLASH_START + 13, 23)); // Region contained only in EXT flash
    EXPECT_EQ(0, clip_region_len_to_ext_flash(NRF_FLASH_START, 1)); // Start of NRF flash
    EXPECT_EQ(0, clip_region_len_to_ext_flash(NRF_FLASH_START, 0)); // NRF Flash Special Case
    EXPECT_EQ(0, clip_region_len_to_ext_flash(NRF_FLASH_START + 95, 108)); // Region contained only in NRF flash
    EXPECT_EQ(101, clip_region_len_to_ext_flash(EXT_FLASH_END - 100, 257)); // Region crossing both flash boundaries
    EXPECT_EQ(100, clip_region_len_to_ext_flash(EXT_FLASH_END - 100, 100)); // Region right before end of EXT flash
    EXPECT_EQ(0, clip_region_len_to_ext_flash(FLASH_END - 1, 2)); // Region contains very end of flash.
    EXPECT_ANY_THROW(clip_region_len_to_ext_flash(FLASH_END + 1, 0)); // Check exception throwing on precondition.
    EXPECT_ANY_THROW(clip_region_len_to_ext_flash(FLASH_END - 1, 3)); // Check exception throwing on precondition.
}

TEST(FlashLayoutUtilsTest, ClipRegionLenToNRFlash) {
    EXPECT_EQ(0, clip_region_len_to_nrf_flash(FLASH_START, 0)); // Region of length 0
    EXPECT_EQ(0, clip_region_len_to_nrf_flash(FLASH_START, 1)); // Starts at start of flash.
    EXPECT_EQ(NRF_FLASH_SIZE, clip_region_len_to_nrf_flash(FLASH_START, FLASH_SIZE)); // Region contains all of flash.
    EXPECT_EQ(0, clip_region_len_to_nrf_flash(FLASH_START + 13, 23)); // Region contained only in EXT flash
    EXPECT_EQ(1, clip_region_len_to_nrf_flash(NRF_FLASH_START, 1)); // Start of NRF flash
    EXPECT_EQ(0, clip_region_len_to_nrf_flash(NRF_FLASH_START, 0)); // NRF Flash Special Case
    EXPECT_EQ(108, clip_region_len_to_nrf_flash(NRF_FLASH_START + 95, 108)); // Region contained only in NRF flash
    EXPECT_EQ(156, clip_region_len_to_nrf_flash(EXT_FLASH_END - 100, 257)); // Region crossing both flash boundaries
    EXPECT_EQ(100, clip_region_len_to_nrf_flash(NRF_FLASH_END - 100, 100)); // Region right before end of NRF flash
    EXPECT_EQ(2, clip_region_len_to_nrf_flash(FLASH_END - 1, 2)); // Region contains very end of flash.
    EXPECT_ANY_THROW(clip_region_len_to_nrf_flash(FLASH_END + 1, 0)); // Check exception throwing on precondition.
    EXPECT_ANY_THROW(clip_region_len_to_nrf_flash(FLASH_END - 1, 3)); // Check exception throwing on precondition.
}

TEST(FlashLayoutUtilsTest, RegionContainsEXTFlash) {
    EXPECT_TRUE(region_contains_ext_flash(FLASH_PAGE_SIZE * 12, FLASH_PAGE_SIZE + 13));
    EXPECT_FALSE(region_contains_ext_flash(FLASH_PAGE_SIZE * (EXT_FLASH_SIZE_PAGES + 2), FLASH_PAGE_SIZE + 13));

    EXPECT_FALSE(region_contains_ext_flash(FLASH_START, 0)); // Region of length 0
    EXPECT_TRUE(region_contains_ext_flash(FLASH_START, 1)); // Starts at start of flash.
    EXPECT_TRUE(region_contains_ext_flash(FLASH_START, FLASH_SIZE)); // Region contains all of flash.
    EXPECT_TRUE(region_contains_ext_flash(FLASH_START + 13, 23)); // Region contained only in EXT flash
    EXPECT_FALSE(region_contains_ext_flash(NRF_FLASH_START, 1)); // Start of NRF flash
    EXPECT_FALSE(region_contains_ext_flash(NRF_FLASH_START, 0)); // NRF Flash Special Case
    EXPECT_FALSE(region_contains_ext_flash(NRF_FLASH_START + 95, 108)); // Region contained only in NRF flash
    EXPECT_TRUE(region_contains_ext_flash(EXT_FLASH_END - 100, 257)); // Region crossing both flash boundaries
    EXPECT_TRUE(region_contains_ext_flash(EXT_FLASH_END - 100, 100)); // Region right before end of EXT flash
    EXPECT_FALSE(region_contains_ext_flash(FLASH_END - 1, 2)); // Region contains very end of flash.
    EXPECT_ANY_THROW(region_contains_ext_flash(FLASH_END + 1, 0)); // Check exception throwing on precondition.
    EXPECT_ANY_THROW(region_contains_ext_flash(FLASH_END - 1, 3)); // Check exception throwing on precondition.
}

TEST(FlashLayoutUtilsTest, RegionContainsNRFFlash) {
    EXPECT_FALSE(region_contains_nrf_flash(FLASH_PAGE_SIZE * 12, FLASH_PAGE_SIZE + 13));
    EXPECT_TRUE(region_contains_nrf_flash(FLASH_PAGE_SIZE * (EXT_FLASH_SIZE_PAGES + 2), FLASH_PAGE_SIZE + 13));

    EXPECT_FALSE(region_contains_nrf_flash(FLASH_START, 0)); // Region of length 0
    EXPECT_FALSE(region_contains_nrf_flash(FLASH_START, 1)); // Starts at start of flash.
    EXPECT_TRUE(region_contains_nrf_flash(FLASH_START, FLASH_SIZE)); // Region contains all of flash.
    EXPECT_FALSE(region_contains_nrf_flash(FLASH_START + 13, 23)); // Region contained only in EXT flash
    EXPECT_TRUE(region_contains_nrf_flash(NRF_FLASH_START, 1)); // Start of NRF flash
    EXPECT_FALSE(region_contains_nrf_flash(NRF_FLASH_START, 0)); // NRF Flash Special Case
    EXPECT_TRUE(region_contains_nrf_flash(NRF_FLASH_START + 95, 108)); // Region contained only in NRF flash
    EXPECT_TRUE(region_contains_nrf_flash(EXT_FLASH_END - 100, 257)); // Region crossing both flash boundaries
    EXPECT_TRUE(region_contains_nrf_flash(NRF_FLASH_END - 100, 100)); // Region right before end of NRF flash
    EXPECT_TRUE(region_contains_nrf_flash(FLASH_END - 1, 2)); // Region contains very end of flash.
    EXPECT_ANY_THROW(region_contains_nrf_flash(FLASH_END + 1, 0)); // Check exception throwing on precondition.
    EXPECT_ANY_THROW(region_contains_nrf_flash(FLASH_END - 1, 3)); // Check exception throwing on precondition.
}

TEST(FlashLayoutUtilsTest, ClipRegionStartToNRFFlash) {
    EXPECT_ANY_THROW(clip_region_start_to_nrf_flash(FLASH_PAGE_SIZE * 12, FLASH_PAGE_SIZE + 13));
    EXPECT_TRUE(clip_region_start_to_nrf_flash(FLASH_PAGE_SIZE * (EXT_FLASH_SIZE_PAGES + 2), FLASH_PAGE_SIZE + 13));

    EXPECT_ANY_THROW(clip_region_start_to_nrf_flash(FLASH_START, 0)); // Region of length 0
    EXPECT_ANY_THROW(clip_region_start_to_nrf_flash(FLASH_START, 1)); // Starts at start of flash.
    EXPECT_EQ(NRF_FLASH_START, clip_region_start_to_nrf_flash(FLASH_START, FLASH_SIZE)); // Region contains all of flash.
    EXPECT_ANY_THROW(clip_region_start_to_nrf_flash(FLASH_START + 13, 23)); // Region contained only in EXT flash
    EXPECT_EQ(NRF_FLASH_START, clip_region_start_to_nrf_flash(NRF_FLASH_START, 1)); // Start of NRF flash
    EXPECT_ANY_THROW(clip_region_start_to_nrf_flash(NRF_FLASH_START, 0)); // NRF Flash Special Case
    EXPECT_EQ(NRF_FLASH_START + 95, clip_region_start_to_nrf_flash(NRF_FLASH_START + 95, 108)); // Region contained only in NRF flash
    EXPECT_EQ(NRF_FLASH_START, clip_region_start_to_nrf_flash(EXT_FLASH_END - 100, 257)); // Region crossing both flash boundaries
    EXPECT_EQ(NRF_FLASH_END - 100, clip_region_start_to_nrf_flash(NRF_FLASH_END - 100, 100)); // Region right before end of NRF flash
    EXPECT_EQ(FLASH_END - 1, clip_region_start_to_nrf_flash(FLASH_END - 1, 2)); // Region contains very end of flash.
    EXPECT_ANY_THROW(clip_region_start_to_nrf_flash(FLASH_END + 1, 0)); // Check exception throwing on precondition.
    EXPECT_ANY_THROW(clip_region_start_to_nrf_flash(FLASH_END - 1, 3)); // Check exception throwing on precondition.
}