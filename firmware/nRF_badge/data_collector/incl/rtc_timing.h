//sdfsdf

#ifndef RTC_TIMING_H
#define RTC_TIMING_H

#define APP_PRESCALER          0
#define APP_MAX_TIMERS         12
#define APP_OP_QUEUE_SIZE      6

#include "nrf_drv_config.h"
#include "nrf_soc.h"
#include "app_error.h"

#include "debug_log.h"

volatile bool countdownOver;  //set true when the countdown interrupt triggers
volatile bool sleep;  //whether we should sleep (so actions like data sending can override sleep)

#define CONNECTION_TIMEOUT_MS 6000UL
//#define CONNECTION_TIMEOUT_MS 6000000UL

/**
 * initialize rtc
 */
void rtc_config(void);


/**
 * set a compare interrupt to occur a number of milliseconds from now (i.e. to break from sleep)
 * NOTE: Maximum countdown time is 130 seconds.
 */
void countdown_set(unsigned long ms);


/**
 * similar to countdown_set, but used to keep track of BLE connection timeout.
 */
void ble_timeout_set(unsigned long ms);

/**
 * cancel the ble timeout counter
 */
void ble_timeout_cancel();

/**
 * similar to countdown_set, but used to keep track of LED indicator timeout.
 */
void led_timeout_set(unsigned long ms);

/**
 * cancel the LED timeout counter
 */
void led_timeout_cancel();


/**
 * returns 32768Hz ticks of RTC, extended to 43 bits  (from built-in 24 bits)
 */
unsigned long long ticks(void);

/**
 * emulate functionality of millis() in arduino
 * divide rtc count by 32768=2^15, get number of seconds since timer started
 * x1000 or 1000000 for millis, micros.  (~30.5us precision)
 */
unsigned long millis(void);

/**
 * Returns a timer tick starting point for timer_comparison_millis_since_start comparisons that start at
 *   the current time.
 *  Safe to use from interupt context.
 *
 * @return the current time as a value that can be used as an argument for timer_comparison_millis_since_start
 */
uint32_t timer_comparison_ticks_now(void);

/**
 * Returns the number of timer ticks (in terms of TIMER_TICKS_MS with APP_PRESCALER) since the given timer ticks time.
 *
 * Used to optimize code as the NRF lacks an FPU, so float operations are slow. Generally, only use when profiling
 *   indicates a performance reason to do so. Otherwise, use timer_comparison_millis_since_start.
 */
uint32_t timer_comparison_ticks_since_start(uint32_t ticks_start);

/**
 * Returns the number of millis since the starting point ticks_start (from timer_comparison_ticks_now).
 *   Has percision down to 1/APP_TIMER_FREQS seconds. Can be used for comparisons up to ~512 ms long.
 *  Safe to use from interupt context
 *
 *  @return the number of milliseconds since ticks_start as a float
 */
float timer_comparison_millis_since_start(uint32_t ticks_start);

/**
 * get current timestamp (set with setTime) in seconds (most likely epoch time)
 * Inspired by Arduino Time library
 */
unsigned long now();

/**
 * get fractional part of timestamp in ms, i.e. returns 0-999.
 * For better time precision, while still keeping simple compatibility with epoch time
 * Corresponds to time when now() was last called, not actual current time!
 */
unsigned long nowFractional();

/**
 * (Most consumers should probably use Timestamp_SetTime in timestamp.c, as it manages some additional state.
 *
 * set current timestamp (set with setTime)
 * In seconds, like Unix time, but extra parameter to set it partway through a full second.
 */
void setTimeFractional(unsigned long timestamp, unsigned long fractional);



#endif //TIMING_H