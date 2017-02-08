//
// Created by Andrew Bartow on 1/26/17.
//

#include "timestamp.h"
#include "ble_setup.h"
#include "rtc_timing.h"

bool mIsTimeSynced = false;

void Timestamp_SetTime(Timestamp_t time) {
    mIsTimeSynced = true;
    setTimeFractional(time.seconds, time.milliseconds);
    updateAdvData();
}

bool Timestamp_IsTimeSynced(void) {
    return mIsTimeSynced;
}

Timestamp_t Timestamp_GetCurrentTime(void) {
    Timestamp_t timestamp = {
            .seconds = (uint32_t) now(),
            .milliseconds = (uint16_t) nowFractional(),
    };

    return timestamp;
}