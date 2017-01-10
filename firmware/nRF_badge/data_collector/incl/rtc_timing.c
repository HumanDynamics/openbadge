#include <app_timer.h>
#include <nrf51.h>

#include "rtc_timing.h"
#include "ble_setup.h"
#include "custom_board.h"

// This is a slight optimization. Our timer wakes our chip whenever the timer goes off,
// so we want to wake it as little as necessary. Thus, we want out clock timer to rarely tick, but we needs millisecond
// precision. This is tricky, because the RTC timer only has 24 bits and app timers can only last <= 512 seconds.
//
// So, we set a timer that ticks every 100 (<=512) seconds, and then we calculate the current time based on time
// since last clock tick using our RTC timer through app_timer_cnt_get().
#define CLOCK_TICK_MILLIS  (100 * 1000)

#define MILLIS_PER_TICK    (1000.f / ((APP_PRESCALER + 1) * APP_TIMER_CLOCK_FREQ))

volatile bool countdownOver = false;  //used to give rtc_timing access to sleep from main loop

static uint32_t mCountdownTimer;
static uint32_t mBLETimeoutTimer;
static uint32_t mLEDTimeoutTimer;
static uint32_t mClock;

static uint32_t mClockInMillis;
static uint32_t mLastClockTickTimerCount;

static void on_countdown_timeout(void * p_context) {
    countdownOver = true;
}

static void on_ble_timeout(void * p_context) {
    debug_log("Connection timeout.  Disconnecting...\r\n");
    BLEforceDisconnect();
}

static void on_led_timeout(void * p_context) {
    nrf_gpio_pin_write(LED_2,LED_OFF);
}

static void on_clock_timeout(void * p_context) {
    app_timer_cnt_get(&mLastClockTickTimerCount);
    mClockInMillis += CLOCK_TICK_MILLIS;
}

void rtc_config(void)
{
    APP_TIMER_INIT(APP_PRESCALER, APP_MAX_TIMERS, APP_OP_QUEUE_SIZE, false);

    app_timer_create(&mCountdownTimer, APP_TIMER_MODE_SINGLE_SHOT, on_countdown_timeout);
    app_timer_create(&mBLETimeoutTimer, APP_TIMER_MODE_SINGLE_SHOT, on_ble_timeout);
    app_timer_create(&mLEDTimeoutTimer, APP_TIMER_MODE_SINGLE_SHOT, on_led_timeout);

    app_timer_create(&mClock, APP_TIMER_MODE_REPEATED, on_clock_timeout);
    app_timer_start(mClock, APP_TIMER_TICKS(CLOCK_TICK_MILLIS, APP_PRESCALER), NULL);
}

static void start_singleshot_timer(uint32_t timer_id, unsigned long ms) {
    if (ms > 130000UL)  {  // 130 seconds.
        ms = 130000UL;  // avoid overflow in calculation of compareTicks below.
    }

    app_timer_stop(timer_id); // Stop the timer if running, new timers preempt old ones.
    app_timer_start(timer_id, APP_TIMER_TICKS(ms, APP_PRESCALER), NULL);
}

void countdown_set(unsigned long ms)
{
    countdownOver = false;
    start_singleshot_timer(mCountdownTimer, ms);
}


void ble_timeout_set(unsigned long ms)
{
#ifndef DEBUG_LOG_ENABLE
    start_singleshot_timer(mBLETimeoutTimer, ms);
#endif
}

void ble_timeout_cancel()
{
    app_timer_stop(mBLETimeoutTimer);
}

void led_timeout_set(unsigned long ms)
{
    start_singleshot_timer(mLEDTimeoutTimer, ms);
}

void led_timeout_cancel()
{
    app_timer_stop(mLEDTimeoutTimer);
}

uint32_t timer_comparison_ticks_now(void) {
    uint32_t timer_ticks;
    app_timer_cnt_get(&timer_ticks);
    return timer_ticks;
}

uint32_t timer_comparison_ticks_since_start(uint32_t ticks_start) {
    uint32_t current_time;
    app_timer_cnt_get(&current_time);

    uint32_t ticks_since_start;
    app_timer_cnt_diff_compute(current_time, ticks_start, &ticks_since_start);

    return ticks_since_start;
}

float timer_comparison_millis_since_start(uint32_t ticks_start) {
    return timer_comparison_ticks_since_start(ticks_start) * MILLIS_PER_TICK;
}

unsigned long millis(void)  {
    // We ensure that millis() calls are atomic operations, so that the clock does not tick during out calculations.
    //   If we do not ensure this, in rare cases, a clock tick interrupt will cause mClockInMillis and
    //   mLastClockTickTimerCount to be mismatched.
    unsigned long millis;
    CRITICAL_REGION_ENTER();
    millis = mClockInMillis + (unsigned long) timer_comparison_millis_since_start(mLastClockTickTimerCount);
    CRITICAL_REGION_EXIT();

    return millis;
}


unsigned long lastMillis;  //last time now() was called

struct
{
    unsigned long s;
    unsigned long ms;
} masterTime;

unsigned long now()
{
    unsigned long difference = millis() - lastMillis;
    while (difference >= 1000)  {
        masterTime.s++;
        lastMillis += 1000;
        difference = millis() - lastMillis;
    }
    //difference is now the fractional part of the timestamp in ms, from 0-999
    masterTime.ms = difference;
    return masterTime.s;
}

unsigned long nowFractional()
{
    return masterTime.ms;
}

void setTimeFractional(unsigned long timestamp, unsigned long fractional)
{
    masterTime.s = timestamp;
    lastMillis = millis() - fractional;  //we want difference=millis()-lastMillis to be the fractional part
}

void setTime(unsigned long timestamp)
{
    setTimeFractional(timestamp, 0);
}