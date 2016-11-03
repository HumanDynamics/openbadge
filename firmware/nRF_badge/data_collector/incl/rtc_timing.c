#include <app_timer.h>

#include "rtc_timing.h"


#define PRESCALER     0
#define MAX_TIMERS    6
#define OP_QUEUE_SIZE 6

volatile uint64_t extTicks = 0;  //extension of the rtc1 24-bit timer, for millis() etc
                                 // i.e. extTicks+rtcTicks is the number of ticks elapsed since counter initiation
volatile bool countdownOver = false;  //used to give rtc_timing access to sleep from main loop

static uint32_t mCountdownTimer;
static uint32_t mBLETimeoutTimer;
static uint32_t mLEDTimeoutTimer;
static uint32_t mClock;

static uint32_t mClockInMillis;

static void on_countdown_timeout(void * p_context) {
    countdownOver = true;
}

static void on_ble_timeout(void * p_context) {
    ble_timeout = true;
}

static void on_led_timeout(void * p_context) {
    led_timeout = true;
}

static void on_clock_timeout(void * p_context) {
    mClockInMillis++;
}

void rtc_config(void)
{
    APP_TIMER_INIT(PRESCALER, MAX_TIMERS, OP_QUEUE_SIZE, false);

    app_timer_create(&mCountdownTimer, APP_TIMER_MODE_SINGLE_SHOT, on_countdown_timeout);
    app_timer_create(&mBLETimeoutTimer, APP_TIMER_MODE_SINGLE_SHOT, on_ble_timeout);
    app_timer_create(&mLEDTimeoutTimer, APP_TIMER_MODE_SINGLE_SHOT, on_led_timeout);

    app_timer_create(&mClock, APP_TIMER_MODE_REPEATED, on_clock_timeout);
    app_timer_start(mClock, APP_TIMER_TICKS(1, PRESCALER), NULL);
}

static void start_singleshot_timer(uint32_t timer_id, unsigned long ms) {
    if (ms > 130000UL)  {  // 130 seconds.
        ms = 130000UL;  // avoid overflow in calculation of compareTicks below.
    }

    app_timer_stop(timer_id); // Stop the timer if running, new timers preempt old ones.
    app_timer_start(timer_id, APP_TIMER_TICKS(ms, PRESCALER), NULL);
}

void countdown_set(unsigned long ms)
{
    countdownOver = false;
    start_singleshot_timer(mCountdownTimer, ms);
}


void ble_timeout_set(unsigned long ms)
{
    ble_timeout = false;
    start_singleshot_timer(mBLETimeoutTimer, ms);
}

void ble_timeout_cancel()
{
    app_timer_stop(mBLETimeoutTimer);
}

void led_timeout_set(unsigned long ms)
{
    led_timeout = false;
    start_singleshot_timer(mLEDTimeoutTimer, ms);
}

void led_timeout_cancel()
{
    app_timer_stop(mLEDTimeoutTimer);
}

unsigned long long ticks(void)  {
    uint32_t ticks;
    app_timer_cnt_get(&ticks);
    return extTicks+ticks;
}

unsigned long millis(void)  {
    return mClockInMillis;
}

unsigned long micros(void)  {
    return (ticks() * 1000000ULL) >> 15;
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