//
// Created by Andrew Bartow on 1/26/17.
//

#include <app_error.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "badge_fs.h"
#include "flash.h"
#include "badge_fs_config.h"
#include "debug_log.h"
#include "calc_macros.h"

#define MAX_NUM_PARTITIONS                2
#define PARTITION_HEADER_SIZE             sizeof(uint32_t)
#define PARTITION_FORMATTED_MAGIC_CODE    0xABABABAB

/**
 * BadgeFS internally uses a 'lookup table' to manage its variable length items. This allows us to store items of
 *   arbitrary length and keep track of the insertion order of these items.
 *
 * Each file system partition consists of two sections, a 'writable area' that contains the data for the items stored in
 * that partition and a 'lookup table' that contains metadata about the items stored in that partition, with each item
 * stored in that partition having a corresponding row in the lookup table.
 *
 * A row in the lookup table contains for the corresponding item:
 *  - The location of that item's data in the partition's writable area
 *  - The length of the item
 *  - A monotonically increasing identifier that is unique to that insertion of that item in that partition
 *
 * Each new item stored in a partition is stored in the lookup table row after the previous item's storage location.
 *   When the end of the table is reached, we wrap around to the beginning to continue storing.
 *
 * Once an item is stored, its metadata will remain in the same row until it is deleted.
 *
 * We can thus scan the lookup table to load all state relating to the file system.
 */

// Internal implementation of the key our clients use to refer to items.
struct BadgeFS_Key_t {
    // Once stored, items won't be repositioned in the lookup table, so its row will always remain the same.
    // Only one item is represented by each row, thus a combination of partition and lookup table row uniquely
    // identifies each item.
    uint8_t partition_id;
    uint16_t table_row;

    // The unique identifier for the item.
    uint32_t item_id;
};

// Define the storage format of the table rows stored in flash.
typedef struct {
    // A monotonically increasing identifier for this item.
    // Because it is monotonically increasing, it can be used to determine which items were inserted first. (See badge_fs.h)
    uint32_t item_id;

    uint32_t item_start_index; // How many bytes into the writable area the item this row refers to begins. (See writable_area_addr)
    uint16_t item_len; // The length of the item stored, in bytes.

    // Slight hack, unwritten flash has value 0xFF, so this will be true if flash is not written.
    bool is_row_erased;
} LookupTableRow_t;

// Type that contains information related to the physical layout of a specific partition in flash.
typedef struct {
    BadgeFS_PartitionLayout_t layout;

    uint32_t base_addr;
    uint32_t header_addr;
    uint32_t table_addr;
    uint32_t table_len;
    uint32_t write_area_addr;
    uint32_t write_area_len;
} PartitionFlashMapping_t;

// Type that contains information related to the current state of a specific partition in flash.
typedef struct {
    uint8_t partition_id;

    uint16_t oldest_row; // The lookup table row for the oldest item in stored the area.
    uint16_t num_stored_items; // Count of items stored in the area. Each item has one lookup table entry.

    uint32_t filled_len; // The number of bytes currently stored in this flash area.
} PartitionStatus_t;

// Contains information related to the mapping of partitions to flash addresses. Should not change after loading.
static PartitionFlashMapping_t mPartitionMappings[MAX_NUM_PARTITIONS];
// Number of partitions in this filesystem configuration. Should not change after loading.
static uint8_t mNumPartitions;

// Contains active state information for each partition in the filesystem.
static PartitionStatus_t mPartitionStatus[MAX_NUM_PARTITIONS];

/* Helper Functions */

// Generates a flash address mapping for each partition and outputs it to partition_mapping.
//   Ordering is preserved between mapping and partition_table.
static void Partitions_GenerateMapping(const BadgeFS_PartitionLayout_t partition_table[],
                                       uint8_t num_partitions,
                                       uint32_t fs_base_flash_addr,
                                       PartitionFlashMapping_t partition_mapping[]) {

    uint32_t base_addr = fs_base_flash_addr;

    for (int i = 0; i < num_partitions; i++) {
        partition_mapping[i].layout = partition_table[i];

        partition_mapping[i].base_addr = base_addr;
        partition_mapping[i].header_addr = base_addr;
        partition_mapping[i].table_addr = partition_mapping[i].header_addr + PARTITION_HEADER_SIZE;
        partition_mapping[i].table_len = partition_table[i].max_num_items * sizeof(LookupTableRow_t);
        partition_mapping[i].write_area_addr = partition_mapping[i].table_addr + partition_mapping[i].table_len;
        partition_mapping[i].write_area_len = partition_table[i].size - partition_mapping[i].table_len - PARTITION_HEADER_SIZE;

        // To prevent burnt out flash chips, all lookup tables should be in EEPROM.
        APP_ERROR_CHECK_BOOL(partition_mapping[i].write_area_addr <= NRF_FLASH_START);

        // Check to make sure we're not past the end of our assigned flash.
        APP_ERROR_CHECK_BOOL(partition_mapping[i].write_area_addr + partition_mapping[i].write_area_len - 1 <= BADGE_FS_END);

        base_addr = partition_mapping[i].write_area_addr + partition_mapping[i].write_area_len;
    }
}

// Formats all the partitions that need formatting, marks headers when formatting is complete.
static void Partitions_FormatPartitionsIfNecessary(const PartitionFlashMapping_t partiton_mapping[], uint8_t num_partitions) {
    for (int i = 0; i < num_partitions; i++) {
        uint32_t partition_header;
        flash_read((uint8_t *) &partition_header, partiton_mapping[i].header_addr, PARTITION_HEADER_SIZE);

        if (partition_header != PARTITION_FORMATTED_MAGIC_CODE) {
            uint16_t erase_increment = FLASH_PAGE_SIZE;
            uint8_t erase_buffer[erase_increment];
            memset(erase_buffer, 0xFF, erase_increment);

            uint32_t data_erased = 0;
            while (data_erased != partiton_mapping[i].table_len) {
                uint16_t erase_len = (uint16_t) MIN(erase_increment, partiton_mapping[i].table_len - data_erased);
                flash_write(partiton_mapping[i].table_addr + data_erased, erase_buffer, erase_len);
                data_erased += erase_len;
            }

            uint32_t formatted_partition_header = PARTITION_FORMATTED_MAGIC_CODE;
            flash_write(partiton_mapping[i].header_addr, (uint8_t *) &formatted_partition_header, PARTITION_HEADER_SIZE);
        }
    }
}

// Returns the flash address of a given lookup table row.
static uint32_t lookup_table_row_address(uint8_t partition_id, uint16_t row_index) {
    return mPartitionMappings[partition_id].table_addr + (sizeof(LookupTableRow_t) * row_index);
}

// Wraps around row_index such that it points to a valid row.
static uint16_t wrap_row_index(uint8_t partition_id, uint16_t row_index) {
    return (uint16_t) (row_index % mPartitionMappings[partition_id].layout.max_num_items);
}

// Returns the index of the row after row_index, wrapping around to the start of the table if necessary.
static uint16_t increment_row_index(uint8_t partition_id, uint16_t row_index) {
    return wrap_row_index(partition_id, (uint16_t) (row_index + 1));
}

// Returns the newest row currently stored in 'dst_partition'. Do not call when no items are stored in partition.
static uint16_t newest_row_index(uint8_t partition_id) {
    APP_ERROR_CHECK_BOOL(mPartitionStatus[partition_id].num_stored_items != 0);
    return wrap_row_index(partition_id, (uint16_t) (mPartitionStatus[partition_id].oldest_row +
            mPartitionStatus[partition_id].num_stored_items - 1));
}

// Loads a table row from flash storage and returns it.
static LookupTableRow_t table_row_from_flash(uint8_t partition_id, uint16_t row_index) {
    LookupTableRow_t row;
    flash_read((uint8_t *) &row, lookup_table_row_address(partition_id, row_index), sizeof(row));
    return row;
}

// Scans each partition to determine its status. Outputs the status of all partitions to partition_status. Partitions
//   must be formatted first.
static void scan_for_status(PartitionStatus_t *partition_status) {
    for (int i = 0; i < mNumPartitions; i++) {
        uint8_t partition_id = mPartitionMappings[i].layout.partition_id;

        uint32_t oldest_row_item_id = UINT32_MAX;
        uint16_t oldest_row_index = 0;
        uint16_t num_data_items_seen = 0;
        uint32_t filled_len = 0;

        for (uint16_t row_index = 0; row_index < mPartitionMappings[i].layout.max_num_items; row_index++) {
            LookupTableRow_t row = table_row_from_flash(partition_id, row_index);

            if (!row.is_row_erased) {
                if (row.item_id < oldest_row_item_id) {
                    oldest_row_item_id = row.item_id;
                    oldest_row_index = row_index;
                }

                filled_len += row.item_len;
                num_data_items_seen++;
            }
        }

        partition_status[i].partition_id = partition_id;
        partition_status[i].oldest_row = oldest_row_index;
        partition_status[i].num_stored_items = num_data_items_seen;
        partition_status[i].filled_len = filled_len;
    }
}

// Returns the flash address for the index_in_area-th byte in the writable area of the partition.
static uint32_t writable_area_addr(uint8_t partition_id, uint32_t index_in_area) {
    return (index_in_area % mPartitionMappings[partition_id].write_area_len)
           + mPartitionMappings[partition_id].write_area_addr;
}

// Read len bytes from the dst_index-th position in the specified partition's writable area.
// Does not update any partition status state.
static void read_from_partition(uint8_t partition_id,
                                uint8_t * p_dst,
                                uint32_t src_index,
                                uint16_t len) {

    // Calculate how much past the end of our circular buffer we will read.
    uint16_t overflow_len = (uint16_t) MAX((int32_t) src_index + (int32_t) len - (int32_t) mPartitionMappings[partition_id].write_area_len, 0);

    // Break the read up into two portions and read it in.
    uint32_t err_code = flash_read(p_dst, writable_area_addr(partition_id, src_index), len - overflow_len);
    APP_ERROR_CHECK(err_code);

    if (overflow_len > 0) {
        err_code = flash_read(&p_dst[len - overflow_len], writable_area_addr(partition_id, 0), overflow_len);
        APP_ERROR_CHECK(err_code);
    }
}

// Write len bytes from the p_src to the dst_index-th position in the specified partition's writable area.
// Does not update any partition status state.
static void write_to_partition(uint8_t partition_id,
                                uint32_t dst_index,
                                uint8_t * p_src,
                                uint16_t len) {

    // Calculate how much past the end of our circular buffer we will read.
    uint16_t overflow_len = (uint16_t) MAX((int32_t) dst_index + (int32_t) len - (int32_t) mPartitionMappings[partition_id].write_area_len, 0);

    // Break the read up into two portions and read it in.
    uint32_t err_code = flash_write(writable_area_addr(partition_id, dst_index), p_src, len - overflow_len);
    APP_ERROR_CHECK(err_code);

    if (overflow_len > 0) {
        err_code = flash_write(writable_area_addr(partition_id, 0), &p_src[len - overflow_len], overflow_len);
        APP_ERROR_CHECK(err_code);
    }
}

static uint32_t consume_next_item_id(uint8_t partition_id) {
    uint32_t next_item_id = 0;

    if (mPartitionStatus[partition_id].num_stored_items != 0) {
        next_item_id = table_row_from_flash(partition_id, newest_row_index(partition_id)).item_id + 1;
    }

    return next_item_id;
}

// Erase the oldest lookup item's row in the specified partition's lookup table. Updates the partition's status
// accordingly.
static void EraseOldestLookupTableRow(uint8_t partition_id) {
    // Grab the row from flash before we erase it.
    LookupTableRow_t row_to_erase = table_row_from_flash(partition_id, mPartitionStatus[partition_id].oldest_row);

    // Overwrite contents of row in flash.
    LookupTableRow_t erased_row;
    memset(&erased_row, 0xFF, sizeof(erased_row));
    flash_write(lookup_table_row_address(partition_id, mPartitionStatus[partition_id].oldest_row),
                (uint8_t *) &erased_row,
                sizeof(LookupTableRow_t));

    // Update partition status to account for newly erased row.
    mPartitionStatus[partition_id].num_stored_items--;
    mPartitionStatus[partition_id].filled_len -= row_to_erase.item_len;
    mPartitionStatus[partition_id].oldest_row = increment_row_index(partition_id,
                                                                    mPartitionStatus[partition_id].oldest_row);
}

void BadgeFS_Init(BadgeFS_PartitionLayout_t partition_table[], uint8_t num_partitions, uint32_t fs_base_flash_addr) {
    APP_ERROR_CHECK_BOOL(partition_table != NULL);
    APP_ERROR_CHECK_BOOL(sizeof(LookupTableRow_t) == PER_ITEM_OVERHEAD_BYTES);
    APP_ERROR_CHECK_BOOL(PARTITION_HEADER_SIZE == PER_PARTITION_OVERHEAD_BYTES);

    // Map out and format our filesystem's partitions.
    PartitionFlashMapping_t partition_mapping[MAX_NUM_PARTITIONS];
    Partitions_GenerateMapping(partition_table, num_partitions, fs_base_flash_addr, partition_mapping);

    // Load the partition_mapping into mPartitionMapping, should only be read from here on out.
    memcpy(&mNumPartitions, &num_partitions, sizeof(mNumPartitions));
    memcpy(mPartitionMappings, partition_mapping, sizeof(mPartitionMappings));

    Partitions_FormatPartitionsIfNecessary(mPartitionMappings, mNumPartitions);
    scan_for_status(mPartitionStatus);
}

uint32_t BadgeFS_FindKey(uint8_t partition, BadgeFS_ItemConditionChecker_t condition_checker, BadgeFS_Key_t ** p_key) {
    BadgeFS_Key_t * search_key;

    uint32_t err_code = BadgeFS_GetFirstKey(partition, &search_key);

    while (err_code == NRF_SUCCESS) {
        uint16_t item_len;
        if (BadgeFS_GetItemSize(&item_len, search_key) == NRF_ERROR_NOT_FOUND) {
            // Item was overwritten while we were looking through flash. Check the next one.
            err_code = BadgeFS_IncrementKey(search_key);
            continue;
        }

        // Load up the next item.
        uint8_t item_data[item_len];
        BadgeFS_GetItem(item_data, &item_len, search_key);
        if (condition_checker(item_data, item_len) == true) {
            // We found the item we were looking for, return it.
            *p_key = search_key;
            return NRF_SUCCESS;
        } else {
            // Not the item we were after, keep looking for it.
            err_code = BadgeFS_IncrementKey(search_key);
        }
    }

    return NRF_ERROR_NOT_FOUND;
}

uint32_t BadgeFS_GetFirstKey(uint8_t partition, BadgeFS_Key_t ** p_key) {
    APP_ERROR_CHECK_BOOL(p_key != NULL);
    if (mPartitionStatus[partition].num_stored_items == 0) return NRF_ERROR_NOT_FOUND;

    BadgeFS_Key_t * key = (BadgeFS_Key_t *) malloc(sizeof(BadgeFS_Key_t));
    key->partition_id = partition;
    key->table_row = mPartitionStatus[partition].oldest_row;
    key->item_id = table_row_from_flash(key->partition_id, key->table_row).item_id;

    *p_key = key;

    return NRF_SUCCESS;
}

uint32_t BadgeFS_IncrementKey(BadgeFS_Key_t *p_key) {
    APP_ERROR_CHECK_BOOL(p_key != NULL);
    LookupTableRow_t row_for_key = table_row_from_flash(p_key->partition_id, p_key->table_row);
    if (row_for_key.item_id == p_key->item_id) {
        // Item pointed to by this key still exists. See if another item exists after the one pointed to by this key.
        uint16_t next_row_index = increment_row_index(p_key->partition_id, p_key->table_row);
        LookupTableRow_t next_row = table_row_from_flash(p_key->partition_id, next_row_index);

        if (next_row.is_row_erased || next_row.item_id < row_for_key.item_id) return NRF_ERROR_NOT_FOUND;

        p_key->table_row = next_row_index;
        p_key->item_id = next_row.item_id;
    } else {
        // The row that was pointed to by this key as been overwritten, key should now point to oldest item
        //  in this partition. (See specification for this method)
        BadgeFS_Key_t * first_key = {0};
        uint32_t err_code = BadgeFS_GetFirstKey(p_key->partition_id, &first_key);
        APP_ERROR_CHECK(err_code);

        memcpy(p_key, first_key, sizeof(BadgeFS_Key_t));
    }

    return NRF_SUCCESS;
}

void BadgeFS_DestroyKey(BadgeFS_Key_t *p_item_key) {
    free(p_item_key);
}

uint32_t BadgeFS_GetItemSize(uint16_t * p_item_len, const BadgeFS_Key_t * p_src_key) {
    LookupTableRow_t row = table_row_from_flash(p_src_key->partition_id, p_src_key->table_row);
    if (row.is_row_erased == true || row.item_id != p_src_key->item_id) return NRF_ERROR_NOT_FOUND;

    *p_item_len = row.item_len;

    return NRF_SUCCESS;
}

uint32_t BadgeFS_GetItem(uint8_t *p_dst, uint16_t * p_item_len, const BadgeFS_Key_t *p_src_key) {
    LookupTableRow_t row = table_row_from_flash(p_src_key->partition_id, p_src_key->table_row);
    if (row.is_row_erased == true || row.item_id != p_src_key->item_id) return NRF_ERROR_NOT_FOUND;

    read_from_partition(p_src_key->partition_id, p_dst, row.item_start_index, row.item_len);

    *p_item_len = row.item_len;

    return NRF_SUCCESS;
}

void BadgeFS_StoreItem(uint8_t dst_partition, uint8_t *p_src, uint16_t len) {
    if (len > mPartitionMappings[dst_partition].write_area_len) APP_ERROR_CHECK(NRF_ERROR_NO_MEM);

    // Free space in the writeable area if needed.
    while (mPartitionMappings[dst_partition].write_area_len < (len + mPartitionStatus[dst_partition].filled_len)) {
        EraseOldestLookupTableRow(dst_partition);
    }

    // Free up a lookup table row to store the new item in if needed.
    if (mPartitionStatus[dst_partition].num_stored_items == mPartitionMappings[dst_partition].layout.max_num_items) {
        EraseOldestLookupTableRow(dst_partition);
    }

    uint32_t dst_index = 0; // The index of to write this item to in the writable area.

    if (mPartitionStatus[dst_partition].num_stored_items != 0) {
        LookupTableRow_t newest_row =  table_row_from_flash(dst_partition, newest_row_index(dst_partition));
        dst_index = (newest_row.item_start_index + newest_row.item_len) % mPartitionMappings[dst_partition].write_area_len;
    }

    LookupTableRow_t new_item_table_entry = {
            .is_row_erased = false,
            .item_id = consume_next_item_id(dst_partition),
            .item_start_index = dst_index,
            .item_len = len
    };

    uint16_t row_index = (mPartitionStatus[dst_partition].num_stored_items == 0) ?
                         (uint16_t) 0 : increment_row_index(dst_partition, newest_row_index(dst_partition));

    flash_write(lookup_table_row_address(dst_partition, row_index), (uint8_t *) &new_item_table_entry, sizeof(new_item_table_entry));

    write_to_partition(dst_partition, dst_index, p_src, len);

    mPartitionStatus[dst_partition].num_stored_items++;
    if (mPartitionStatus->num_stored_items == 1) mPartitionStatus[dst_partition].oldest_row = row_index;
    mPartitionStatus[dst_partition].filled_len += new_item_table_entry.item_len;
}

#ifdef UNITTEST
/**
 * THIS METHOD IS ONLY TO BE USED IN UNIT TESTS AND SHOULD NOT BE INCLUDED IN PRODUCTION CODE.
 *
 * This method simulates a chip reset by clearing all state that is stored in RAM. This allows BadgeFS to be
 *   reinitialized as though the chip had just rebooted with the contents of flash.
 */

void BadgeFS_Unload(void) {
    memset(mPartitionMappings, 0x00, sizeof(mPartitionMappings));
    memset(mPartitionStatus, 0x00, sizeof(mPartitionStatus));
}
#endif