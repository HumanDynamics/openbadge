//
// Created by Andrew Bartow on 1/26/17.
//

#ifndef OPENBADGE_FLASH_MANAGER_CONFIG_H
#define OPENBADGE_FLASH_MANAGER_CONFIG_H

#include <stdbool.h>

#include "badge_fs.h"
#include "flash_layout.h"

#define NUM_FS_PARTITIONS          (sizeof(mBadgeFSConfig_AreaTable) / sizeof(FSPartitionConfig_t))

// We generate approximately:
//   808 bytes per minute for audio collection
//   256 bytes per minute for proximity collection
// So we allocate:
//   (808 / (808 + 256)) = 75% of our space for audio
//   (256 / (808 + 256)) = 25% of our space for proximity

#define AUDIO_PARTITION_SIZE     (3 * BADGE_FS_SIZE / 4)
#define PROXIMITY_PARTITION_SIZE (1 * BADGE_FS_SIZE / 4)

#define MAX_NUM_AUDIO_ITEMS      ((7 * 60 * 60) / 6)
#define MAX_NUM_PROXIMITY_ITEMS  ((7 * 60 * 60) / 30)

typedef enum {
    FS_PARTITION_AUDIO,
    FS_PARTITION_PROXIMITY,
} FSConfig_Partition_t;

static BadgeFS_PartitionLayout_t mBadgeFSConfig_PartitionTable[] = {
        {
                .partition_id = FS_PARTITION_PROXIMITY,
                .size = PROXIMITY_PARTITION_SIZE,
                .max_num_items = MAX_NUM_PROXIMITY_ITEMS,
        },
        {
                .partition_id = FS_PARTITION_AUDIO,
                .size = AUDIO_PARTITION_SIZE,
                .max_num_items = MAX_NUM_AUDIO_ITEMS
        },
};

#endif //OPENBADGE_FLASH_MANAGER_CONFIG_H
