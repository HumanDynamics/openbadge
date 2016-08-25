/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include <stdint.h>
#include <string.h>

/**
 * From Nordic SDK
 */
#include "nordic_common.h"
#include "nrf.h"
#include "nrf51_bitfields.h"

#include "nrf_drv_rtc.h"        //driver abstraction for real-time counter
#include "app_error.h"          //error handling
#include "nrf_delay.h"          //includes blocking delay functions
#include "nrf_gpio.h"           //abstraction for dealing with gpio
#include "nrf_adc.h"            //abstraction for dealing with adc
#include "ble_flash.h"          //for writing to flash

#include "ble_gap.h"            //basic ble functions (advertising, scans, connecting)

#include "debug_log.h"          //UART debugging logger
//requires app_fifo, app_uart_fifo.c and retarget.c for printf to work properly

#include "nrf_drv_config.h"
#include "boards.h"

//NRF51DK has common cathode LEDs, i.e. gpio LOW turns LED on.
#ifdef BOARD_PCA10028
    #define LED_ON 0
    #define LED_OFF 1
//Badges are common anode, the opposite.
#else
    #define LED_ON 1
    #define LED_OFF 0
#endif


/**
 * Custom libraries/abstractions
 */
#include "analog.h"     //analog inputs, battery reading
#include "rtc_timing.h"  //support millis(), micros(), countdown timer interrupts
#include "ble_setup.h"  //stuff relating to BLE initialization/configuration
//#include "external_flash.h"  //for interfacing to external SPI flash
#include "scanner.h"       //for performing scans and storing scan data
#include "self_test.h"   // for built-in tests
#include "collector.h"  // for collecting data from mic
#include "storer.h"
#include "sender.h"



enum cycleStates {SLEEP, SAMPLE, SCAN, STORE, SEND};
unsigned long cycleStart;       // start of main loop cycle (e.g. sampling cycle)
int cycleState = SAMPLE;     // to keep track of state of main loop
#define MIN_SLEEP 5UL      // ms of sleep, minimum (keep well under SAMPLE_PERIOD - SAMPLE_WINDOW to leave room for sending)
#define MAX_SLEEP 120000UL // ms of sleep, maximum.  (2mins, so that badge periodically cycles thru main loop, even when idle)

// If any module (collecting, storing, sending) has any pending operations, this gets set to true
bool badgeActive = false;   // Otherwise, the badge is inactive and can enter indefinite sleep.


//============================ time-related stuff ======================================
//======================================================================================




//=========================== Global function definitions ==================================
//==========================================================================================

void goToSleep(long ms)  
{
    unsigned long sleepTime = ms;
    if(ms == 0)
    {
        return;  // don't sleep
    }
    sleep = true;
    if(ms == -1)  
    {
        sleepTime = MAX_SLEEP;
    }
    countdown_set(sleepTime);
    while((!countdownOver) && sleep && (!ble_timeout) &&(!led_timeout))  
    {
        sd_app_evt_wait();  //sleep until one of our functions says not to
    }
}




 
/**
 * ============================================== MAIN ====================================================
 */
int main(void)
{    
    #if defined(BOARD_PCA10028)  //NRF51DK
        //If button 4 is pressed on startup, do nothing (mostly so that UART lines are freed on the DK board)
        nrf_gpio_cfg_input(BUTTON_4,NRF_GPIO_PIN_PULLUP);  //button 4
        if(nrf_gpio_pin_read(BUTTON_4) == 0)  //button pressed
        {
            nrf_gpio_pin_dir_set(LED_4,NRF_GPIO_PIN_DIR_OUTPUT);
            nrf_gpio_pin_write(LED_4,LED_ON);
            while(1);
        }
        nrf_gpio_cfg_default(BUTTON_4);
    #endif
    
    debug_log_init();
    debug_log("\r\n\r\n\r\n\r\nUART trace initialized.\r\n\r\n");
    debug_log("Name: %.5s\r\n",DEVICE_NAME);


    // Define and set LEDs
    nrf_gpio_pin_dir_set(LED_1,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_1,LED_ON);  //turn off LED
    nrf_gpio_pin_dir_set(LED_2,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_2,LED_OFF);  //turn off LED

    // Button
    nrf_gpio_cfg_input(BUTTON_1,NRF_GPIO_PIN_PULLUP);  //button
    
    // Initialize
    BLE_init();
    sd_power_mode_set(NRF_POWER_MODE_LOWPWR);  //set low power sleep mode
    adc_config();
    rtc_config();
    spi_init();
    
    /*scan_chunk_t scanChunk;
    for(int i = EXT_FIRST_DATA_CHUNK; i < 64; i++)
    {
        uint32_t result = ext_flash_read(EXT_ADDRESS_OF_CHUNK(i), scanChunk.buf, sizeof(scanChunk.buf));
        if(result != NRF_SUCCESS)
        {
            debug_log("ERR: err reading ext flash.\r\n");
            while(1);
        }
        ext_flash_wait();
        unsigned long ts = scanChunk.timestamp;
        if(ts == scanChunk.check && ts > MODERN_TIME && ts < FUTURE_TIME)  // not looking at trunc/continue chunks yet.
        {
            debug_log("Ext chunk %d, time 0x%lX num %d\r\n",i,scanChunk.timestamp,scanChunk.num);
            for(int j=0; j<scanChunk.num; j++)
            {
                debug_log("  bdg ID#%.4hX, rssi %d, count %d\r\n", scanChunk.devices[j].ID,
                                                                   (int)scanChunk.devices[j].rssi,
                                                                   (int)scanChunk.devices[j].count );
                nrf_delay_ms(5);
            }
        }
        nrf_delay_ms(10);
    }
    while(1);*/
    
    #if defined(TESTER_ENABLE) // tester mode is enabled
        runSelfTests();
        while(1);
    #endif    // end of self tests
    
    
    collector_init();
    storer_init();
    sender_init();
    scanner_init();
    
    BLEsetBadgeAssignment(getStoredBadgeAssignment());
    advertising_init();
    
    /*#ifdef DEBUG_LOG_ENABLE
        ble_gap_addr_t MAC;
        sd_ble_gap_address_get(&MAC);
        debug_log("MAC address: %X:%X:%X:%X:%X:%X\r\n", MAC.addr[5],MAC.addr[4],MAC.addr[3],
                                                    MAC.addr[2],MAC.addr[1],MAC.addr[0]);
    
        //uint32_t* deviceAddrPtr = (uint32_t*)NRF_FICR->DEVICEADDR;
        //debug_log("MAC address address: 0x%X\r\n",(unsigned int)deviceAddrPtr);
    #endif*/
    
    // Blink once on start
    nrf_gpio_pin_write(LED_1,LED_ON);
    nrf_delay_ms(2000);
    nrf_gpio_pin_write(LED_1,LED_OFF);
    
    
    //setTime(MODERN_TIME+100);
    //startCollector();
    //dateReceived = true;
    
    /**
     * Reset tracker
     * If the board resets for some reason, an LED will blink.
     * To intentionally reset the board, the button must be held on start.
     
    if(nrf_gpio_pin_read(BUTTON_1) != 0)
    {
        nrf_gpio_pin_write(LED_1,LED_ON);
        nrf_delay_ms(1000);
        nrf_gpio_pin_write(LED_1,LED_OFF);
        while(1)  {
            nrf_gpio_pin_write(LED_2,LED_ON);
            nrf_delay_ms(5);
            nrf_gpio_pin_write(LED_2,LED_OFF);
            //nrf_delay_ms(1000);
            sleep = true;
            goToSleep(1000);
        }
            
    }*/
    
    nrf_delay_ms(1000);

    debug_log("Done with setup.  Entering main loop.\r\n\r\n");
    
    BLEstartAdvertising();
    
    cycleStart = millis();
    
    nrf_delay_ms(2);
    
    
    /*scan_params.active = 0;  //passive scanning, only looking for advertising packets
    scan_params.selective = 0;  //non-selective, don't use whitelist
    scan_params.p_whitelist = NULL;  //no whitelist
    scan_params.interval = (SCAN_INTERVAL * 1000) / 625;   //scan_params uses interval in units of 0.625ms
    scan_params.window = (SCAN_WINDOW * 1000) / 625;       //window also in units of 0.625ms
    scan_params.timeout = SCAN_TIMEOUT;                    //timeout is in s
    
    debug_log("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
    debug_log("====================================================\r\n");
    debug_log("===== DEVELOPMENT BADGE.  PERFORMS SCANS ONLY. =====\r\n");
    debug_log("  Press button to start scanning.\r\n\r\n");
    
    while(1)
    {
        uint32_t result;
        
        // wait till button pressed
        while(nrf_gpio_pin_read(BUTTON_1) == 0);
        nrf_delay_ms(200);
        while(nrf_gpio_pin_read(BUTTON_1) != 0);
        nrf_delay_ms(200);
        
        debug_log("Starting scans...");
        result = startScan();
        if(result == NRF_SUCCESS)
        {
            debug_log(" Scan started.\r\n");
        }
        else
        {
            debug_log(" Error starting scan.\r\n");
            while(1);
        }
    }*/
    
    /*
    debug_log("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
    debug_log("====================================================\r\n");
    debug_log("===== DEVELOPMENT BADGE.  DUMPS EXT FLASH ONLY. =====\r\n");
    debug_log("  Press button to start scanning.\r\n\r\n");
        
    for(int i = 0; i <= EXT_LAST_CHUNK; i++)
    {
        scan_chunk_t scanChunk;
        getScanChunk(&scanChunk,i);
        for(int j=0; j<8; j++)
        {
            for(int k=0; k<8; k++)
            {
                debug_log("%.2X "dfkjngskfjngkjgndfkjgns
            }
        }
    }
    while(1);
    */
    
    
    // Enter main loop
    for (;;)  {
        //================ Sampling/Sleep handler ================
        
        if(ble_timeout)
        {
            debug_log("Connection timeout.  Disconnecting...\r\n");
            BLEforceDisconnect();
            ble_timeout = false;
        }
        
        if(led_timeout)
        {
            nrf_gpio_pin_write(LED_2,LED_OFF);
            led_timeout = false;
        }
        
        switch(cycleState)
        {
            
            case SAMPLE:
                if(millis() - lastBatteryUpdate >= MIN_BATTERY_READ_INTERVAL)
                {
                    //badgeActive |= true;
                    if(BLEpause(PAUSE_REQ_COLLECTOR))
                    {
                        updateBatteryVoltage();
                        BLEresume(PAUSE_REQ_COLLECTOR);
                    }
                }
                
                if(isCollecting)
                {
                    badgeActive |= true;
                    
                    if(millis() - cycleStart < sampleWindow)
                    {
                        takeMicReading();
                        //sleep = false;
                    }
                    else  {
                        collectSample();
                        cycleState = SCAN;
                    }
                }
                else
                {
                    cycleState = SCAN;
                }
                break;
            
            case SCAN:
                ;
                bool scannerActive = updateScanner();
                badgeActive |= scannerActive;
                cycleState = STORE;
                break;
                
            case STORE:
                ;
                bool storerActive = updateStorer();
                badgeActive |= storerActive;
                cycleState = SEND;
                break;
                
            case SEND:
                ;
                bool senderActive = updateSender();
                badgeActive |= senderActive;
                
                if(millis() - cycleStart > (samplePeriod - MIN_SLEEP) || (!senderActive))  // is it time to sleep, or done sending?
                {
                    cycleState = SLEEP;
                }
                
                break;
            case SLEEP:
                ;// can't put declaration directly after case label.
                long sleepDuration;
                unsigned long elapsed = millis() - cycleStart;
                
                
                // If none of the modules (collector, storer, sender) is active, then we can sleep indefinitely (until BLE activity)
                if(!badgeActive)
                {
                    sleepDuration = -1;  // infinite sleep
                }
                
                // Else we're actively cycling thru main loop, and should sleep for the remainder of the sampling period
                else if(elapsed < samplePeriod)
                {
                    sleepDuration = samplePeriod - elapsed;
                }
                else
                {
                    sleepDuration = 0;
                }
                
                // Main loop will halt on the following line as long as the badge is sleeping (i.e. until an interrupt wakes it)
                goToSleep(sleepDuration);
                
                // Exit sleep if we've reached the end of the sampling period, or if we're in idle mode
                if(millis() - cycleStart >= samplePeriod || (!badgeActive))  // did we exit sleep by the countdown event
                {
                    cycleState = SAMPLE;
                    cycleStart = millis();
                    badgeActive = false;
                }
                
                break;
            default:
                break;
        }

    }
}



void BLEonConnect()
{
    debug_log("--CONNECTED--\r\n");
    sleep = false;

    // for app development. disable if forgotten in prod. version
    #ifdef DEBUG_LOG_ENABLE
        nrf_gpio_pin_write(LED_1,LED_ON);
    #endif
    
    ble_timeout_set(CONNECTION_TIMEOUT_MS);
}

void BLEonDisconnect()
{
    debug_log("--DISCONNECTED--\r\n");
    sleep = false;

    // for app development. disable if forgotten in prod. version
    #ifdef DEBUG_LOG_ENABLE
        nrf_gpio_pin_write(LED_1,LED_OFF);
    #endif
    
    ble_timeout_cancel();
}

// Convert chars to long (expects little endian)
unsigned long readLong(uint8_t *a) {
  unsigned long retval;
  retval  = (unsigned long) a[3] << 24 | (unsigned long) a[2] << 16;
  retval |= (unsigned long) a[1] << 8 | a[0];
  return retval;
}


/** Function for handling incoming data from the BLE UART service
 */
void BLEonReceive(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)  
{
    if(length > 0)
    {
        pendingCommand = unpackCommand(p_data, length);
    }
    sleep = false;
    
    ble_timeout_set(CONNECTION_TIMEOUT_MS);
}


