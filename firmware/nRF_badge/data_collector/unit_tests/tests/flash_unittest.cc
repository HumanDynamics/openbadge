//
// Created by Andrew Bartow on 1/11/17.
//

#include "gtest/gtest.h"
#include "flash.h"
#include "flash_simulator.h"
#include "flash_layout.h"
#include "nrf_error.h"

namespace {
    class FlashTest : public ::testing::Test {
        virtual void SetUp() {
            reset_flash();
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

        // Read the data back and check that it's correct.
        char test_read_data[sizeof(test_write_data)];
        EXPECT_EQ(NRF_SUCCESS, flash_read((uint8_t *) test_read_data, FLASH_START, FLASH_SIZE));
        EXPECT_TRUE(memcmp(test_read_data, test_write_data, FLASH_SIZE) == 0);
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
        EXPECT_EQ(NRF_ERROR_INVALID_ADDR, flash_write(FLASH_START, (uint8_t *) test_write_data, sizeof(test_write_data) - 1));
        EXPECT_EQ(NRF_ERROR_INVALID_ADDR, flash_write(FLASH_START + 1, (uint8_t *) test_write_data, sizeof(test_write_data)));
        EXPECT_EQ(NRF_ERROR_INVALID_ADDR, flash_write(FLASH_START + 1, (uint8_t *) test_write_data, sizeof(test_write_data) - 1));
        EXPECT_EQ(NRF_ERROR_INVALID_ADDR, flash_write(FLASH_START + 3, (uint8_t *) test_write_data, sizeof(test_write_data) - 1));
        EXPECT_EQ(NRF_ERROR_INVALID_ADDR, flash_write(FLASH_START, (uint8_t *) test_write_data + 1, sizeof(test_write_data)));

        char test_read_data[8] = {0};
        EXPECT_EQ(NRF_ERROR_INVALID_ADDR, flash_read((uint8_t *) test_read_data, FLASH_START, sizeof(test_write_data) - 1));
        EXPECT_EQ(NRF_ERROR_INVALID_ADDR, flash_read((uint8_t *) test_read_data, FLASH_START + 1, sizeof(test_write_data)));
        EXPECT_EQ(NRF_ERROR_INVALID_ADDR, flash_read((uint8_t *) test_read_data, FLASH_START + 1, sizeof(test_write_data) - 1));
        EXPECT_EQ(NRF_ERROR_INVALID_ADDR, flash_read((uint8_t *) test_read_data, FLASH_START + 3, sizeof(test_write_data) - 1));
        EXPECT_EQ(NRF_ERROR_INVALID_ADDR, flash_read((uint8_t *) test_read_data + 1, FLASH_START, sizeof(test_write_data)));
    }
};



