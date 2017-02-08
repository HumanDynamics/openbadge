//
// Created by Andrew Bartow on 1/26/17.
//

#ifndef OPENBADGE_FLASH_MANAGER_H
#define OPENBADGE_FLASH_MANAGER_H

#include <stdint.h>

// Do not include this 'badge_fs_private.h' header from any other file.
#include "badge_fs_private.h"

#define PER_ITEM_OVERHEAD_BYTES      12
#define PER_PARTITION_OVERHEAD_BYTES 4

#define CALCULATE_WRITEABLE_AREA(PARTITION_SIZE, MAX_NUM_ITEMS)               \
                         (PARTITION_SIZE - (PER_ITEM_OVERHEAD_BYTES * MAX_NUM_ITEMS) - PER_PARTITION_OVERHEAD_BYTES)

typedef struct BadgeFS_Key_t BadgeFS_Key_t;

typedef struct {
    uint8_t partition_id; // A unique identifier for the partition.
    uint32_t size; // The size of the partition in bytes.

    // The maximum number of items that can be held in this partition.
    // This partition will only hold up to this many items, regardless of how large each individual item is.
    // Each allowed item consumes a fixed amount of space as overhead, so this number should be minimized.
    // If this limit is exceeded, older items will be discarded to make room for newer items regardless of the
    //  size of those items.
    uint32_t max_num_items;
} BadgeFS_PartitionLayout_t;

typedef bool (*BadgeFS_ItemConditionChecker_t)(uint8_t * item_data, uint16_t item_len);

/**
 * BadgeFS is a very simple filesystem optimized for the needs of the OpenBadge firmware.
 *
 * It supports the storage and retrieval of arbitrary variable length byte arrays, with a special focus on the quick
 *   iterative retrieval of items based on insertion order. Storage into BadgeFS is also optimized to automatically
 *   make room for new items when a partition fills up by erasing the oldest items first.
 *
 * Use of the filesystem is based on the exchange of "keys", which each uniquely identify an item, for the stored data,
 *   which we call "items". The file system also has a concept of "partitions". Partitions are separate slices of flash
 *   that operate independently from each other. When older items must be pruned to make room for newer items,
 *   only items in the partition that is being inserted to shall be deleted. Each key uniquely identifies both the item
 *   and the partition that the item is stored in. This is useful to separate different kinds of data.
 *
 * A key for the first item inserted into an partition can be retrieved by calling BadgeFS_GetFirstKey() and that key
 *   can be iterated upon to get the keys for any other item by calling BadgeFS_IncrementItemKey(). Keys cardinality is
 *   based on insertion order. (Keys for items inserted first come first, keys for items inserted last come last).
 *
 * We also offer a BadgeFS_FindKey to easily search the filesystem for specified items.
 *
 * To store a new item, call BadgeFS_StoreItem and specify the partition to store the item in. To retrieve an item,
 *   call BadgeFS_GetItem.
 */

/**
 * Initializes the filesystem. Must be called before any methods may be used. Will load the existing filesystem
 *   at the indicated address if a valid one already exists, or will format the memory to create a new filesystem if
 *   one does not exist.
 *
 * @param partition_table The partition layout table for the filesystem. This table must be statically allocated.
 * @param fs_base_flash_addr The address of the first byte in flash that the filesystem is located at.
 */
void BadgeFS_Init(BadgeFS_PartitionLayout_t partition_table[], uint8_t num_partitions, uint32_t fs_base_flash_addr);

/**
 * Searches for the first item item in a given partition that matches the given condition (i.e., returns true).
 * Constructs a new key that points to that item.
 *
 * IMPORTANT NOTE: This operation will allocate memory to store the new key, and so the consumer MUST call
 *   BadgeFS_DestroyKey when this key is no longer used to avoid memory leaks.
 *
 * @param partition Partition to search.
 * @param condition_checker Method that checks if the given item matches a condition.
 *      If the item matches the condition, should return True.
 * @param p_key Destination to copy pointer to the key for the found item. Must not be null.
 * @return NRF_SUCCESS if the item was found and a key created, NRF_ERROR_NOT_FOUND if the item does not exist or
 *   partition is empty.
 */
uint32_t BadgeFS_FindKey(uint8_t partition, BadgeFS_ItemConditionChecker_t condition_checker, BadgeFS_Key_t ** p_key);

/**
 * Returns the key for the oldest item stored in the given partition. (That is, the item that was inserted first.)
 *
 * IMPORTANT NOTE: This operation will allocate memory to store the new key, and so the consumer MUST call
 *   BadgeFS_DestroyKey when this key is no longer used to avoid memory leaks.
 *
 * This operation takes constant time and does read from flash.
 *
 * @param partition Partition to get the key for the first item stored.
 * @param p_key Destination to copy pointer to the key for the first item. Must not be null.
 * @return NRF_SUCCESS if the key was created and copied successfully, NRF_ERROR_NOT_FOUND if the key does not exist
 *   (i.e., that partition contains no stored items).
 */
uint32_t BadgeFS_GetFirstKey(uint8_t partition, BadgeFS_Key_t ** p_key);

/**
 * Increments the given key so that it points to the key for the next item that was stored in the same partition.
 *
 * If the item for the input key no longer exists (i.e, it was overwritten by a store operation), p_key will be
 *   updated to be a key for the oldest item that still exists in the same partition.
 *
 * This operation takes constant time and does read from flash.
 *
 * @param p_key Pointer to the key to increment. Must not be null. Will be mutated to be resulting key.
 * @return NRF_SUCCESS if the key was successfully incremented, NRF_ERROR_NOT_FOUND if no key exists after the provided
 *   one. (i.e., provided key was to newest item in its partition). Key will not be mutated on failure.
 */
uint32_t BadgeFS_IncrementKey(BadgeFS_Key_t *p_key);

/**
 * Frees the memory used by the specified key. The key cannot be used after this method is called. Does not effect
 *   anything in the filesystem, just frees the memory.
 *
 * IMPORTANT NOTE: This MUST be called once a key is no longer used to avoid memory leaks.
 *
 * This operation takes constant time and does not read from flash.
 *
 * @param p_item_key Pointer to key to free memory used by.
 */
void BadgeFS_DestroyKey(BadgeFS_Key_t *p_item_key);

/**
 * Retrieves the length of the item given by the key and stores the length into p_item_len.
 *
 * @param p_item_len Will be set to the length in bytes of the item retrieved from the file system.
 * @param p_src_key The identifier for the item to retrieve from the file system. Must not be null.
 *
 * This operation takes constant time in terms of its inputs and does read from flash.
 *
 * @return NRF_SUCCESS if the size of the item was successfully read, or NRF_ERROR_NOT_FOUND if the item could not be
 *  located. (i.e., the item indicated by that key has been overwritten by another item.)
 */
uint32_t BadgeFS_GetItemSize(uint16_t * p_item_len, const BadgeFS_Key_t * p_src_key);

/**
 * Retrieves the item identified by the given key and stores it into p_dst.
 *
 * @param p_dst The destination buffer to write the retrieved item into. It is the caller's responsibility to ensure
 *    this buffer is large enough for any item they might retrieve.
 * @param p_item_len Will be set to the length in bytes of the item retrieved from the filesystem.
 * @param p_src_key The identifying the item to retrieve from the filesystem. Must not be null.
 *
 * This operation takes amortized linear time in terms of its inputs and does read from flash.
 *
 * @return NRF_SUCCESS if the item was successfully retrieved, or NRF_ERROR_NOT_FOUND if the item could not be located.
 *   (i.e., the item indicated by that key has been overwritten by another item.)
 */
uint32_t BadgeFS_GetItem(uint8_t *p_dst, uint16_t * p_item_len, const BadgeFS_Key_t *p_src_key);

/**
 * Stores a new item into the filesystem. Will be appended to end of the partition it is stored in. (Thus, item will
 *   have the last key of any item in its partition) The oldest items in the given partition (determined by time of storage to
 *   the filesystem) may be overwritten to make room for this new item if not enough room exists to store the new item.
 *
 * This operation takes amortized linear time in terms of its inputs and does write to and read from flash.
 *
 * @param dst_partition The partition to store the new item to.
 * @param p_src Pointer to bytes to write to filesystem as new item.
 * @param len The length of the item, in bytes.
 */
void BadgeFS_StoreItem(uint8_t dst_partition, uint8_t *p_src, uint16_t len);


#endif //OPENBADGE_FLASH_MANAGER_H
