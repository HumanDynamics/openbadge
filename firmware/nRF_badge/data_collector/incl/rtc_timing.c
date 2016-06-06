
#include "rtc_timing.h"


const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(1);  //instance for RTC1 driver (RTC0 is used by BLE)
volatile uint64_t extTicks = 0;  //extension of the rtc1 24-bit timer, for millis() etc
                                 // i.e. extTicks+rtcTicks is the number of ticks elapsed since counter initiation

volatile bool countdownOver = false;  //used to give rtc_timing access to sleep from main loop

void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    if (int_type == NRF_DRV_RTC_INT_OVERFLOW)  {
        extTicks += 0x1000000ULL;  //increment rtc extension by 2^24
        extTicks &= 0x7ffffffffffULL;  //clip it to 43bits (so doesn't overflow when *1000000 in micros()
    }
    else if(int_type == NRF_DRV_RTC_INT_COMPARE0)  //countdown timer interrupt
    {
        nrf_drv_rtc_cc_disable(&rtc, 0);  //disable the compare channel - countdown is finished
        //debug_log("Countdown end\r\n");
        countdownOver = true;
    }
    else if(int_type == NRF_DRV_RTC_INT_COMPARE1)  //countdown timer interrupt
    {
        nrf_drv_rtc_cc_disable(&rtc, 1);  //disable the compare channel - ble timeout
        //debug_log("BLE connection timeout.\r\n");
        ble_timeout = true;
    }
    else if(int_type == NRF_DRV_RTC_INT_COMPARE2)  //countdown timer interrupt
    {
        nrf_drv_rtc_cc_disable(&rtc, 2);  //disable the compare channel - led timeout
        led_timeout = true;
    }
}
 
void rtc_config(void)
{
    uint32_t err_code;
    
    nrf_drv_rtc_config_t rtc_config;
    rtc_config.prescaler = 0;  //no prescaler.  RTC ticks at 32768Hz
    rtc_config.interrupt_priority = NRF_APP_PRIORITY_LOW;  //low priority interrupt
    rtc_config.reliable = 0;  //don't need reliable mode because the countdown will be setting compares ~200ms away
    rtc_config.tick_latency = 200;  //used for compare interrupts

    //Initialize RTC instance
    err_code = nrf_drv_rtc_init(&rtc, &rtc_config, rtc_handler);
    APP_ERROR_CHECK(err_code);

    //Enable overflow event & interrupt
    nrf_drv_rtc_overflow_enable(&rtc,true);
    
    //nrf_drv_rtc_tick_disable(&rtc);

    //Power on RTC instance
    nrf_drv_rtc_enable(&rtc);
}

void countdown_set(unsigned long ms)
{
    if(ms > 130000UL)  {  // 130 seconds.
        ms = 130000UL;  // avoid overflow in calculation of compareTicks below.
    }
    //Set compare value so that an interrupt will occur ms milliseconds from now
    countdownOver = false;
    unsigned long compareTicks = (nrf_drv_rtc_counter_get(&rtc) + (32768UL * ms / 1000UL));  //convert ms to ticks
    compareTicks &= 0xffffff; //clip to 24bits
    nrf_drv_rtc_cc_set(&rtc,0,compareTicks,true);  //set compare channel 0 to interrupt when counter hits compareTicks
}


void ble_timeout_set(unsigned long ms)
{
    if(ms > 130000UL)  {  // 130 seconds.
        ms = 130000UL;  // avoid overflow in calculation of compareTicks below.
    }
    //Set compare value so that an interrupt will occur ms milliseconds from now
    ble_timeout = false;
    unsigned long compareTicks = (nrf_drv_rtc_counter_get(&rtc) + (32768UL * ms / 1000UL));  //convert ms to ticks
    compareTicks &= 0xffffff; //clip to 24bits
    nrf_drv_rtc_cc_set(&rtc,1,compareTicks,true);  //set compare channel 1 to interrupt when counter hits compareTicks
}

void ble_timeout_cancel()
{
    nrf_drv_rtc_cc_disable(&rtc, 1);  //disable the compare channel - timeout canceled
}

void led_timeout_set(unsigned long ms)
{
    if(ms > 130000UL)  {  // 130 seconds.
        ms = 130000UL;  // avoid overflow in calculation of compareTicks below.
    }
    //Set compare value so that an interrupt will occur ms milliseconds from now
    led_timeout = false;
    unsigned long compareTicks = (nrf_drv_rtc_counter_get(&rtc) + (32768UL * ms / 1000UL));  //convert ms to ticks
    compareTicks &= 0xffffff; //clip to 24bits
    nrf_drv_rtc_cc_set(&rtc,2,compareTicks,true);  //set compare channel 1 to interrupt when counter hits compareTicks
}

void led_timeout_cancel()
{
    nrf_drv_rtc_cc_disable(&rtc, 2);  //disable the compare channel - timeout canceled
}

unsigned long long ticks(void)  {
    return extTicks+nrf_drv_rtc_counter_get(&rtc);
}

unsigned long millis(void)  {
    return (ticks() * 1000ULL) >> 15;
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
    while(difference >= 1000)
    {
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