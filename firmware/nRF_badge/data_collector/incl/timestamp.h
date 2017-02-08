//
// Created by Andrew Bartow on 1/26/17.
//

#ifndef OPENBADGE_TIMESTAMP_H
#define OPENBADGE_TIMESTAMP_H

#include <stdbool.h>
#include <stdint.h>

// Represents the time since Unix epoch.
//   The time since epoch for a given timestamp is timestamp.seconds + (timestamp.milliseconds / 1000) seconds.
//   That is, timestamp.milliseconds is how many full milliseconds it's been since the epoch and timestamp.microseconds
//     is how many additional full microseconds have elapsed on top of that.
typedef struct {
    uint32_t seconds;
    uint16_t milliseconds;
} Timestamp_t;

// Wrapper functions that manage time sync. - Use these rather than rtc_timing.h

/**
 * Synchronizes the current time to timestamp.
 * @param timestamp Timestamp containing time to synchronize this to.
 */
void Timestamp_SetTime(Timestamp_t timestamp);

/**
 * @return The timestamp for the current time.
 */
Timestamp_t Timestamp_GetCurrentTime(void);

/**
 * @return True if the time has been synchronized.
 */
bool Timestamp_IsTimeSynced(void);

#endif //OPENBADGE_TIMESTAMP_H
