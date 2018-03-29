#include "app_timer.h"

#include "analog.h"
#include "battery.h"
#include "rtc_timing.h"

// Read the battery once every 15 seconds.
//  I profiled the sample operation and determined it took ~120 microseconds to complete one read.
//  So a reading every 15 seconds should pretty negligible and also produce very good data.
#define BATTERY_READ_INTERVAL_MILLIS (15 * 1000)
#define BATTERY_SAMPLES_PER_AVERAGE  5

static float mBatteryAverage;

static uint32_t mBatteryMonitorTimer;

static void on_battery_sample(void * p_context) {
    float vdd = readVDD();
    mBatteryAverage -= mBatteryAverage * (1.f / (float) BATTERY_SAMPLES_PER_AVERAGE);
    mBatteryAverage += vdd * (1.f / (float) BATTERY_SAMPLES_PER_AVERAGE);

    debug_log("  Read battery: %dmV (raw: %dmV).\r\n", (int) (1000.f * mBatteryAverage), (int) (1000.f * vdd));
    updateAdvData();
}

void BatteryMonitor_init(void) {
    app_timer_create(&mBatteryMonitorTimer, APP_TIMER_MODE_REPEATED, on_battery_sample);
    app_timer_start(mBatteryMonitorTimer, APP_TIMER_TICKS(BATTERY_READ_INTERVAL_MILLIS, APP_PRESCALER), NULL);

    mBatteryAverage = readVDD();
    debug_log("Battery: %dmV.\r\n", (int) (1000.0 * mBatteryAverage));
}

float BatteryMonitor_getBatteryVoltage(void) {
    return mBatteryAverage;
}