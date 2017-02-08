//
// Created by Andrew Bartow on 1/11/17.
//

#include "gtest/gtest.h"
#include "calc_macros.h"
#include "flash.h"
#include "flash_simulator.h"
#include "flash_layout.h"
#include "nrf_error.h"

// These must match values declared in flash.c
#define FLASH_BUFFER_SIZE       2048
#define FLASH_WRITE_HEADER_SIZE sizeof(uint32_t)

#define TEST_READ_WRITE(DATA, FLASH_ADDR, LEN)      do { \
                                                EXPECT_EQ(NRF_SUCCESS, flash_write(FLASH_ADDR, (uint8_t *) DATA, LEN)); \
                                                char test_read_data[LEN] = {0}; \
                                                EXPECT_EQ(NRF_SUCCESS, flash_read((uint8_t *) &test_read_data, FLASH_ADDR, LEN)); \
                                                EXPECT_EQ(0, memcmp(DATA, test_read_data, LEN)); \
                                                    } while(0)

// There's a memset_pattern function in the Mac Standard Library, but I'll be nice an implement one that is portable.
// Note our arguments have to be word-aligned, but that shouldn't be an issue here.
static void memset_pattern(uint8_t * dst, uint32_t pattern, size_t len) {
    for (int i = 0; i < len; i = i + sizeof(pattern)) {
        memcpy(&dst[i], &pattern, sizeof(pattern));
    }
}

namespace {
    class FlashTest : public ::testing::Test {
        virtual void SetUp() {
            reset_simulated_flash();
            flash_init();
        }
    };

    TEST_F(FlashTest, TestEmptyWrite) {
        char test_write_data[] = "hi!";
        flash_write(FLASH_START, (uint8_t *) test_write_data, 0);
        flash_write(FLASH_START + 4, (uint8_t *) test_write_data, 0);
        flash_write(NRF_FLASH_START, (uint8_t *) test_write_data, 0);
        flash_write(NRF_FLASH_START + 4, (uint8_t *) test_write_data, 0);
        flash_write(FLASH_END, (uint8_t *) test_write_data, 0);
        flash_write(FLASH_END - 4, (uint8_t *) test_write_data, 0);

        char test_read_data[sizeof(test_write_data)] = {0};

        flash_read((uint8_t *) test_read_data, FLASH_START, sizeof(test_read_data));
        EXPECT_EQ(0, strlen(test_read_data));
        flash_read((uint8_t *) test_read_data, FLASH_START + 4, sizeof(test_read_data));
        EXPECT_EQ(0, strlen(test_read_data));
        flash_read((uint8_t *) test_read_data, NRF_FLASH_START, sizeof(test_read_data));
        EXPECT_EQ(0, strlen(test_read_data));
        flash_read((uint8_t *) test_read_data, NRF_FLASH_START + 4, sizeof(test_read_data));
        EXPECT_EQ(0, strlen(test_read_data));
        flash_read((uint8_t *) test_read_data, FLASH_END, sizeof(test_read_data));
        EXPECT_EQ(0, strlen(test_read_data));
        flash_read((uint8_t *) test_read_data, FLASH_END - 4, sizeof(test_read_data));
        EXPECT_EQ(0, strlen(test_read_data));
    }

    TEST_F(FlashTest, StartOfFlashTest) {
        // Write to start of flash.
        char test_write_data[] = "This is some test data!";  // Must be word-aligned (incl. null terminator).
        EXPECT_EQ(NRF_SUCCESS, flash_write(FLASH_START, (uint8_t *) test_write_data, sizeof(test_write_data)));

        // Read from start of flash.
        char test_read_data[sizeof(test_write_data)];
        EXPECT_EQ(NRF_SUCCESS, flash_read((uint8_t *) test_read_data, FLASH_START, sizeof(test_read_data)));

        // Make sure data written is still there.
        EXPECT_STREQ(test_write_data, test_read_data);
    }

    TEST_F(FlashTest, MultiPageEXTTest) {
        // Write to start of flash.
        char test_write_data[3 * FLASH_PAGE_SIZE];
        EXPECT_EQ(NRF_SUCCESS, flash_write(FLASH_START + 8, (uint8_t *) test_write_data, sizeof(test_write_data)));

        // Read from start of flash.
        char test_read_data[sizeof(test_write_data)];
        EXPECT_EQ(NRF_SUCCESS, flash_read((uint8_t *) test_read_data, FLASH_START + 8, sizeof(test_read_data)));

        EXPECT_TRUE(memcmp(test_write_data, test_read_data, sizeof(test_write_data)) == 0);
    }

    TEST_F(FlashTest, FlashRegionBoundaryTest) {
        // Write to just before the boundary between EXT and NRF flash.
        char test_write_data[] = "This is some test data!";
        EXPECT_EQ(NRF_SUCCESS, flash_write(EXT_FLASH_END - 7, (uint8_t *) test_write_data, sizeof(test_write_data)));

        // Read the data back and check that it's correct.
        char test_read_data[sizeof(test_write_data)];
        EXPECT_EQ(NRF_SUCCESS, flash_read((uint8_t *) test_read_data, EXT_FLASH_END - 7, sizeof(test_read_data)));
        EXPECT_STREQ(test_write_data, test_read_data);
    }

    TEST_F(FlashTest, WriteEndOfFlash) {
        // Write to just before the boundary between EXT and NRF flash.
        char test_write_data[] = "This is some test data!";
        EXPECT_EQ(NRF_SUCCESS, flash_write(FLASH_END - sizeof(test_write_data) + 1, (uint8_t *) test_write_data, sizeof(test_write_data)));

        // Read the data back and check that it's correct.
        char test_read_data[sizeof(test_write_data)];
        EXPECT_EQ(NRF_SUCCESS, flash_read((uint8_t *) test_read_data, FLASH_END - sizeof(test_write_data) + 1, sizeof(test_read_data)));
        EXPECT_STREQ(test_write_data, test_read_data);
    }

    TEST_F(FlashTest, WriteAllOfFlash) {
        char test_write_data[FLASH_SIZE] = {0} ;
        memset(test_write_data, 0xAB, FLASH_SIZE);
        EXPECT_EQ(NRF_SUCCESS, flash_write(FLASH_START, (uint8_t *) test_write_data, FLASH_SIZE));

        char test_read_data[sizeof(test_write_data)];
        EXPECT_EQ(NRF_SUCCESS, flash_read((uint8_t *) test_read_data, FLASH_START, FLASH_SIZE));

        EXPECT_TRUE(memcmp(test_write_data, test_read_data, FLASH_SIZE) == 0);
    }

    TEST_F(FlashTest, FlashOverwrite) {
        char test_write_data[] = "abcdefghijklmno";
        EXPECT_EQ(NRF_SUCCESS, flash_write(EXT_FLASH_END - 7, (uint8_t *) test_write_data, sizeof(test_write_data)));

        char test_read_data[sizeof(test_write_data)];
        EXPECT_EQ(NRF_SUCCESS, flash_read((uint8_t *) test_read_data, EXT_FLASH_END - 7, sizeof(test_read_data)));
        EXPECT_STREQ(test_write_data, test_read_data);

        strcpy(test_write_data, "12345678");
        EXPECT_EQ(NRF_SUCCESS, flash_write(EXT_FLASH_END - 3, (uint8_t *) test_write_data, strlen(test_write_data)));

        EXPECT_EQ(NRF_SUCCESS, flash_read((uint8_t *) test_read_data, EXT_FLASH_END - 7, sizeof(test_read_data)));
        EXPECT_STREQ("abcd12345678mno", test_read_data);
    }

    TEST_F(FlashTest, TestNonWordAlign) {
        char test_write_data[] = "1234567"; // sizeof(test_write_data) == 8, (8 % 4) = 0
        EXPECT_EQ(NRF_SUCCESS, flash_write(FLASH_START, (uint8_t *) test_write_data, sizeof(test_write_data) - 1));

        TEST_READ_WRITE(test_write_data, FLASH_START + 1, sizeof(test_write_data));
        TEST_READ_WRITE(test_write_data, FLASH_START + 1, sizeof(test_write_data) - 1);
        TEST_READ_WRITE(test_write_data, FLASH_START + 3, sizeof(test_write_data) - 1);
        TEST_READ_WRITE(test_write_data + 1, FLASH_START, sizeof(test_write_data));
    }


    TEST_F(FlashTest, TestMultiPageNRFWrite) {
        uint8_t test_write_data[FLASH_PAGE_SIZE * 3];
        memset_pattern(test_write_data, 0xABCDEF00, sizeof(test_write_data));

        EXPECT_EQ(NRF_SUCCESS, flash_write(NRF_FLASH_START + 12, test_write_data, sizeof(test_write_data)));

        uint8_t test_read_data[sizeof(test_write_data)];
        EXPECT_EQ(NRF_SUCCESS, flash_read(test_read_data, NRF_FLASH_START + 12, sizeof(test_read_data)));

        EXPECT_TRUE(memcmp(test_write_data, test_read_data, sizeof(test_write_data)) == 0);
    }

    TEST_F(FlashTest, TestNRFWriteTimeout) {
        // Cause the next pstorage_update to produce a timeout.
        pstorage_update_trigger_timeout(0);

        char test_write_data[] = "This is some test data!";
        EXPECT_EQ(NRF_SUCCESS, flash_write(NRF_FLASH_START + 8, (uint8_t *) test_write_data, sizeof(test_write_data)));

        char test_read_data[sizeof(test_write_data)];
        EXPECT_EQ(NRF_SUCCESS, flash_read((uint8_t *) test_read_data, NRF_FLASH_START + 8, sizeof(test_read_data)));

        EXPECT_STREQ(test_write_data, test_read_data);
    }

    TEST_F(FlashTest, TestMultiPageNRFWriteTimeout) {
        // Cause a pstorage_update timeout in a page in the middle of a flash write.
        pstorage_update_trigger_timeout(1);

        uint8_t test_write_data[FLASH_PAGE_SIZE * 3];
        memset_pattern(test_write_data, 0xABCDEF00, sizeof(test_write_data));

        EXPECT_EQ(NRF_SUCCESS, flash_write(NRF_FLASH_START + 12, test_write_data, sizeof(test_write_data)));

        uint8_t test_read_data[sizeof(test_write_data)];
        EXPECT_EQ(NRF_SUCCESS, flash_read(test_read_data, NRF_FLASH_START + 12, sizeof(test_read_data)));

        EXPECT_TRUE(memcmp(test_write_data, test_read_data, sizeof(test_write_data)) == 0);
    }
};



