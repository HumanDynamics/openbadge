#include <badge_fs.h>
#include "gtest/gtest.h"
#include "badge_fs.h"
#include "badge_fs_config.h"
#include "nrf_error.h"
#include "flash_simulator.h"
#include "flash.h"

typedef enum {
    FIRST_PARTITION,
    SECOND_PARTITION,
} TestPartition_t;

// Update this if it is changed in badge_fs.c
#define FLASH_TABLE_ROW_LEN 8

#define TEST_ITEM_A "This is some test data"
#define TEST_ITEM_B "Non-ascii data: \0 ab \3 \12 \155, the alphabet: abcdefghijklmopqrstuvwxyz"
#define TEST_ITEM_C "First test item"
#define TEST_ITEM_D "Second test item"
#define TEST_ITEM_E "Third test item"

#define TEST_FS_READ(EXPECTED_RESULT, KEY)      do {    \
                                                 uint8_t read_buffer[sizeof(EXPECTED_RESULT)]; \
                                                 uint16_t read_len; \
                                                 EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetItem(read_buffer, &read_len, KEY)); \
                                                 EXPECT_EQ(sizeof(EXPECTED_RESULT), read_len); \
                                                 EXPECT_EQ(0, memcmp(EXPECTED_RESULT, read_buffer, sizeof(EXPECTED_RESULT))); \
                                               } while(0)

static BadgeFS_PartitionLayout_t mSinglePartitionTable[] = {
        {
                .partition_id = FIRST_PARTITION,
                .size = 10 * FLASH_PAGE_SIZE,
                .max_num_items = 128,
        },
};

// This partition table is intentionally out of order, because the order of partition tables shouldn't matter.
static BadgeFS_PartitionLayout_t mDoublePartitionTable[] = {
        {
                .partition_id = FIRST_PARTITION,
                .size = 180,
                .max_num_items = 3,
        },
        {
                .partition_id = SECOND_PARTITION,
                .size = FLASH_PAGE_SIZE,
                .max_num_items = 12,
        },
};

bool test_item_checker(uint8_t * item_data, uint16_t item_len) {
    EXPECT_EQ(item_len, strlen((char *) item_data) + 1);
    return strlen((char *) item_data) > 4;
}

// Mock this special function out defined in badge_fs.c
void BadgeFS_Unload(void);

namespace {

    class BadgeFSEmptyFlashTest : public ::testing::Test {
        virtual void SetUp() {
            reset_simulated_flash();
            flash_init();
        }
    };


    TEST_F(BadgeFSEmptyFlashTest, TestEmptyPartition) {
        BadgeFS_Init(mSinglePartitionTable, 1, FLASH_START);

        BadgeFS_Key_t * key = NULL;
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_GetFirstKey(FIRST_PARTITION, &key));
        EXPECT_EQ(NULL, key);
    }

    TEST_F(BadgeFSEmptyFlashTest, TestWriteRead) {
        BadgeFS_Init(mSinglePartitionTable, 1, BADGE_FS_START);

        // Try to write to flash.
        uint8_t data_to_write[] = "This is some test data.\n";
        BadgeFS_StoreItem(FIRST_PARTITION, data_to_write, sizeof(data_to_write));

        // Try to get the key of the thing we just wrote.
        BadgeFS_Key_t * key = NULL;
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetFirstKey(FIRST_PARTITION, &key));

        // Try to read from flash.
        uint8_t read_buffer[sizeof(data_to_write) + 1];
        // Initialize read_buffer to all canary values so we can check for overwrite.
        memset(read_buffer, 0xAB, sizeof(read_buffer));
        uint16_t read_len;
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetItem(read_buffer, &read_len, key));

        // Verify output was correct bytes, length, and did not overflow destination buffer.
        EXPECT_EQ(read_len, sizeof(data_to_write));
        EXPECT_STREQ((char *) data_to_write, (char *) read_buffer);
        EXPECT_EQ(0xAB, read_buffer[sizeof(data_to_write)]);

        // Check that we cannot increment our key. (That we only wrote one copy to the filesystem.)
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_IncrementKey(key));

        BadgeFS_DestroyKey(key);
    }


    TEST_F(BadgeFSEmptyFlashTest, TestApplicationPartitionTable) {
        // Check that we can load the Partition Table the application uses.
        BadgeFS_Init(mBadgeFSConfig_PartitionTable, 2, BADGE_FS_START);

        // Write/read some stuff to each partition to make sure it's all kosher.
        BadgeFS_StoreItem(FS_PARTITION_AUDIO, (uint8_t *) TEST_ITEM_A, sizeof(TEST_ITEM_A));
        BadgeFS_Key_t * key;
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetFirstKey(FS_PARTITION_AUDIO, &key));
        TEST_FS_READ(TEST_ITEM_A, key);

        BadgeFS_DestroyKey(key);

        BadgeFS_StoreItem(FS_PARTITION_PROXIMITY, (uint8_t *) TEST_ITEM_A, sizeof(TEST_ITEM_A));
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetFirstKey(FS_PARTITION_PROXIMITY, &key));
        TEST_FS_READ(TEST_ITEM_A, key);

        BadgeFS_DestroyKey(key);
    }



    TEST_F(BadgeFSEmptyFlashTest, TestManyWrites) {
        // Set up our filesystem with an item in it.
        BadgeFS_Init(mSinglePartitionTable, 1, BADGE_FS_START);
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) TEST_ITEM_A, sizeof(TEST_ITEM_A));
        BadgeFS_Key_t * key;
        ASSERT_EQ(NRF_SUCCESS, BadgeFS_GetFirstKey(FIRST_PARTITION, &key));

        // Write many items to flash to check that we can.
        for (int i = 0; i < (4 * UINT16_MAX); i++) {
            // Write a test string
            char test_string[128] = {0};
            sprintf(test_string, "This is write #%d.", i);
            BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) test_string, (uint16_t) (strlen(test_string) + 1));

            // Read it back.
            char read_string[128] = {0};
            uint16_t read_len;
            ASSERT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
            ASSERT_EQ(NRF_SUCCESS, BadgeFS_GetItem((uint8_t *) read_string, &read_len, key));
            ASSERT_EQ(strlen(test_string) + 1, read_len);
            ASSERT_STREQ(test_string, read_string);
        }

        BadgeFS_DestroyKey(key);

        // Unload and try and reinitialize flash.
        BadgeFS_Unload();
        BadgeFS_Init(mSinglePartitionTable, 1, BADGE_FS_START);

        // Check that we can read all the items we expected to.
        uint32_t err_code = BadgeFS_GetFirstKey(FIRST_PARTITION, &key);
        EXPECT_EQ(NRF_SUCCESS, err_code);
        int num_items_read = 0;
        while (err_code == NRF_SUCCESS) {
            int expected_write_num = (4 * UINT16_MAX) - mSinglePartitionTable[0].max_num_items  + num_items_read;
            char expected_string[128] = {0};
            sprintf(expected_string, "This is write #%d.", expected_write_num);

            char read_string[128] = {0};
            uint16_t read_len;
            ASSERT_EQ(NRF_SUCCESS, BadgeFS_GetItem((uint8_t *) read_string, &read_len, key));
            ASSERT_EQ(strlen(expected_string) + 1, read_len);
            ASSERT_STREQ(expected_string, read_string);
            err_code = BadgeFS_IncrementKey(key);

            num_items_read++;
        }

        EXPECT_EQ(mSinglePartitionTable[0].max_num_items, num_items_read);

        BadgeFS_DestroyKey(key);
    }

    TEST_F(BadgeFSEmptyFlashTest, TestFindItemExists) {
        BadgeFS_Init(mSinglePartitionTable, 1, BADGE_FS_START);

        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "a", sizeof("a"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "abc", sizeof("abc"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "cat", sizeof("cat"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "abcd", sizeof("abcd"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "abcde", sizeof("abcde"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "dog", sizeof("a"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "asdfa", sizeof("asdfa"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "this is a test", sizeof("this is a test"));

        BadgeFS_Key_t * key;
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_FindKey(FIRST_PARTITION, test_item_checker, &key));
        TEST_FS_READ("abcde", key);
    }

    TEST_F(BadgeFSEmptyFlashTest, TestFindItemEmptyPartition) {
        BadgeFS_Init(mSinglePartitionTable, 1, BADGE_FS_START);

        BadgeFS_Key_t * key;
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_FindKey(FIRST_PARTITION, test_item_checker, &key));
    }

    TEST_F(BadgeFSEmptyFlashTest, TestFindItemNotFound) {
        BadgeFS_Init(mSinglePartitionTable, 1, BADGE_FS_START);

        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "a", sizeof("a"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "abc", sizeof("abc"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "cat", sizeof("cat"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "abcd", sizeof("abcd"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "dog", sizeof("dog"));
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) "asdf", sizeof("asdf"));

        BadgeFS_Key_t * key;
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_FindKey(FIRST_PARTITION, test_item_checker, &key));
    }
}

namespace {
    class BadgeFSExistingData : public ::testing::Test {
        virtual void SetUp() {
            reset_simulated_flash();
            flash_init();

            BadgeFS_Init(mDoublePartitionTable, 2, BADGE_FS_START);

            BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) TEST_ITEM_A, sizeof(TEST_ITEM_A));
            BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) TEST_ITEM_B, sizeof(TEST_ITEM_B));
            BadgeFS_StoreItem(SECOND_PARTITION, (uint8_t *) TEST_ITEM_C, sizeof(TEST_ITEM_C));
            BadgeFS_StoreItem(SECOND_PARTITION, (uint8_t *) TEST_ITEM_D, sizeof(TEST_ITEM_D));
            BadgeFS_StoreItem(SECOND_PARTITION, (uint8_t *) TEST_ITEM_E, sizeof(TEST_ITEM_E));

            BadgeFS_Unload();
        }
    };

    TEST_F(BadgeFSExistingData, TestExistingPartitions) {
        // Test that we can load an existing partition.
        BadgeFS_Init(mDoublePartitionTable, 2, BADGE_FS_START);

        // Check that we can read the all items from the first partition.
        BadgeFS_Key_t * key;
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetFirstKey(FIRST_PARTITION, &key));

        TEST_FS_READ(TEST_ITEM_A, key);
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        TEST_FS_READ(TEST_ITEM_B, key);
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_IncrementKey(key));

        BadgeFS_DestroyKey(key);

        // Check that we can read the last item from the second partition.
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetFirstKey(SECOND_PARTITION, &key));
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));

        TEST_FS_READ(TEST_ITEM_E, key);

        BadgeFS_DestroyKey(key);
    }

    TEST_F(BadgeFSExistingData, TestOverflowData) {
        BadgeFS_Init(mDoublePartitionTable, 2, BADGE_FS_START);

        // Grab the first key from the second partition.
        BadgeFS_Key_t * key;
        BadgeFS_GetFirstKey(SECOND_PARTITION, &key);

        size_t second_partition_current_len = sizeof(TEST_ITEM_C) + sizeof(TEST_ITEM_D) + sizeof(TEST_ITEM_E);
        size_t second_partition_writable_area = CALCULATE_WRITEABLE_AREA(mDoublePartitionTable[SECOND_PARTITION].size,
                                                                         mDoublePartitionTable[SECOND_PARTITION].max_num_items);
        size_t second_partition_space_remaining = second_partition_writable_area - second_partition_current_len;

        uint8_t almost_overflow_write[second_partition_space_remaining];
        memset(almost_overflow_write, 0xAB, sizeof(almost_overflow_write));
        // Fill up the partition.
        BadgeFS_StoreItem(SECOND_PARTITION, almost_overflow_write, sizeof(almost_overflow_write));

        // Test that we can still read the first item
        TEST_FS_READ(TEST_ITEM_C, key);

        // Overwrite the first and second items in the second partition.
        BadgeFS_StoreItem(SECOND_PARTITION, (uint8_t *) TEST_ITEM_A, sizeof(TEST_ITEM_A));

        uint8_t read_buffer = 0xAB;
        uint16_t read_len = 13;
        // The first key should have been overwritten and no longer valid.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_GetItem(&read_buffer, &read_len, key));

        // Double check BadgeFS_GetItem didn't mutate its inputs
        EXPECT_EQ(0xAB, read_buffer);
        EXPECT_EQ(13, read_len);

        // This should jump to the third flash item, which wasn't overwritten.
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));

        TEST_FS_READ(TEST_ITEM_E, key);

        // This should now be the almost_overflow_write.
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        uint8_t big_read_buffer[sizeof(almost_overflow_write)];
        uint16_t big_read_len;
        EXPECT_EQ(NRF_SUCCESS,  BadgeFS_GetItem(big_read_buffer, &big_read_len, key));
        EXPECT_EQ(sizeof(almost_overflow_write), big_read_len);
        EXPECT_EQ(0, memcmp(almost_overflow_write, big_read_buffer, sizeof(almost_overflow_write)));

        // Try to read the final item that overwrote the first two items
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        TEST_FS_READ(TEST_ITEM_A, key);

        // That should have been the last key in our partition.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_IncrementKey(key));

        // Make another small write to overwrite the almost_overflow_write
        BadgeFS_StoreItem(SECOND_PARTITION, (uint8_t *) TEST_ITEM_B, sizeof(TEST_ITEM_B));

        // This should now read back the thing we just wrote.
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        TEST_FS_READ(TEST_ITEM_B, key);

        // And that should be the last thing in this partition.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_IncrementKey(key));

        BadgeFS_DestroyKey(key);

        // Check that nothing in the first partition was overwritten.
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetFirstKey(FIRST_PARTITION, &key));

        TEST_FS_READ(TEST_ITEM_A, key);
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        TEST_FS_READ(TEST_ITEM_B, key);
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_IncrementKey(key));

        BadgeFS_DestroyKey(key);
    }

    TEST_F(BadgeFSExistingData, TestOverflowItems) {
        BadgeFS_Init(mDoublePartitionTable, 2, BADGE_FS_START);

        // Grab the current key of the first item.
        BadgeFS_Key_t * key;
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetFirstKey(FIRST_PARTITION, &key));

        // Append an item to fill the partition
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) TEST_ITEM_C, sizeof(TEST_ITEM_C));

        // Should still be able to read the first item.
        TEST_FS_READ(TEST_ITEM_A, key);

        // Overflow the maximum number of keys
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) TEST_ITEM_D, sizeof(TEST_ITEM_D));

        // First item should have been overwritten.
        uint8_t read_buffer = 0xAB;
        uint16_t read_len = 13;
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_GetItem(&read_buffer, &read_len, key));

        // Original second and third items should still be there, as well as the new one.
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        TEST_FS_READ(TEST_ITEM_B, key);
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        TEST_FS_READ(TEST_ITEM_C, key);
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        TEST_FS_READ(TEST_ITEM_D, key);

        // Should be no other items in this partition.
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_IncrementKey(key));

        // Should be able to overwrite another one too.
        BadgeFS_StoreItem(FIRST_PARTITION, (uint8_t *) TEST_ITEM_E, sizeof(TEST_ITEM_E));
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        TEST_FS_READ(TEST_ITEM_E, key);
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_IncrementKey(key));

        BadgeFS_DestroyKey(key);

        // Other partition should not have been effected.
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetFirstKey(SECOND_PARTITION, &key));
        TEST_FS_READ(TEST_ITEM_C, key);
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        TEST_FS_READ(TEST_ITEM_D, key);
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_IncrementKey(key));
        TEST_FS_READ(TEST_ITEM_E, key);
        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_IncrementKey(key));

        BadgeFS_DestroyKey(key);
    }

    TEST_F(BadgeFSExistingData, TestWriteEntirePartition) {
        BadgeFS_Init(mDoublePartitionTable, 2, BADGE_FS_START);


        uint32_t largest_possible_write = CALCULATE_WRITEABLE_AREA(mDoublePartitionTable[FIRST_PARTITION].size,
                                                                   mDoublePartitionTable[FIRST_PARTITION].max_num_items);
        // Attempt to write an entire partition all at once.
        uint8_t entire_partition_write[largest_possible_write];
        memset(entire_partition_write, sizeof(entire_partition_write), 0xAB);
        BadgeFS_StoreItem(FIRST_PARTITION, entire_partition_write, sizeof(entire_partition_write));

        BadgeFS_Key_t * key;
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetFirstKey(FIRST_PARTITION, &key));

        uint8_t entire_parition_read[sizeof(entire_partition_write)];
        uint16_t read_len;
        EXPECT_EQ(NRF_SUCCESS, BadgeFS_GetItem(entire_parition_read, &read_len, key));
        EXPECT_EQ(sizeof(entire_partition_write), read_len);
        EXPECT_EQ(0, memcmp(entire_partition_write, entire_parition_read, sizeof(entire_partition_write)));

        EXPECT_EQ(NRF_ERROR_NOT_FOUND, BadgeFS_IncrementKey(key));
        BadgeFS_DestroyKey(key);
    }
}

