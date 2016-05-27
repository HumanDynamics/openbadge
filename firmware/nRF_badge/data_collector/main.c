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
#include "external_flash.h"  //for interfacing to external SPI flash
//#include "scanning.h"       //for performing scans and storing scan data
//#include "self_test.h"   // for built-in tests
#include "collector.h"  // for collecting data from mic
#include "storer.h"
#include "sender.h"



enum cycleStates {SLEEP, SAMPLE, STORE, SEND};
unsigned long cycleStart;       // start of main loop cycle (e.g. sampling cycle)
int cycleState = SLEEP;     // to keep track of state of main loop

// If any module (collecting, storing, sending) has any pending operations, this gets set to true
bool badgeActive = false;   // Otherwise, the badge is inactive and can enter indefinite sleep.


//============================ time-related stuff ======================================
//======================================================================================

volatile bool sleep = false;  //whether we should sleep (so actions like data sending can override sleep)


//=========================== Global function definitions ==================================
//==========================================================================================

void goToSleep(long ms)  {
    if(ms == 0)
    {
        return;  // don't sleep
    }
    sleep = true;
    if(ms == -1)  
    {
        while(sleep)  {  //infinite sleep until an interrupt cancels it
            sd_app_evt_wait();
        }
    }
    else  
    {   
        countdown_set(ms);
        while((!countdownOver) && sleep)  {
            sd_app_evt_wait();  //sleep until one of our functions says not to
        }
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


    // Define and set LEDs
    nrf_gpio_pin_dir_set(LED_1,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_1,LED_ON);  //turn off LED
    nrf_gpio_pin_dir_set(LED_2,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_2,LED_OFF);  //turn off LED

    // Button
    nrf_gpio_cfg_input(BUTTON_1,NRF_GPIO_PIN_PULLUP);  //button

    // Self-test code (incompatible with new code structure)
    /*#if defined(TESTER_ENABLE) // tester mode is enabled
        //////////////////////////////////
        nrf_gpio_pin_write(GREEN_LED,LED_ON);
        nrf_gpio_pin_write(RED_LED,LED_ON);
        nrf_delay_ms(LED_BLINK_MS);
        nrf_gpio_pin_write(GREEN_LED,LED_OFF);
        nrf_gpio_pin_write(RED_LED,LED_OFF);
        nrf_delay_ms(LED_BLINK_MS);

        // BLE start
        debug_log("Starting BLE\r\n");
        nrf_gpio_pin_write(GREEN_LED,LED_ON);
        
        BLE_init();
        
        nrf_delay_ms(LED_BLINK_MS);
        nrf_gpio_pin_write(GREEN_LED,LED_OFF);
        nrf_delay_ms(LED_BLINK_MS);
        //////////////////////////////////
        // other init
        debug_log("Init misc.\r\n");
        nrf_gpio_pin_write(GREEN_LED,LED_ON);

        sd_power_mode_set(NRF_POWER_MODE_LOWPWR);  //set low power sleep mode        
        adc_config();
        rtc_config();
        
        nrf_delay_ms(LED_BLINK_MS);
        nrf_gpio_pin_write(GREEN_LED,LED_OFF);
        nrf_delay_ms(LED_BLINK_MS);

        //////////////////////////////////
        // test internal flash
        debug_log("Testing internal flash\r\n");
        nrf_gpio_pin_write(GREEN_LED,LED_ON);
        
        if (testInternalFlash()) {
            debug_log("Success\r\n");
        }
        else{
            debug_log("Failed\r\n");
            while(1) {};
        }

        nrf_delay_ms(LED_BLINK_MS);
        nrf_gpio_pin_write(GREEN_LED,LED_OFF);
        nrf_delay_ms(LED_BLINK_MS);

        //////////////////////////////////
        // test external flash
        debug_log("Testing external flash\r\n");
        nrf_gpio_pin_write(GREEN_LED,LED_ON);
        
        // init
        spi_init();
        // read/write
        if (testExternalFlash()) {
            debug_log("Success\r\n");
        }
        else{
            debug_log("Failed\r\n");
            while(1) {};
        }

        nrf_delay_ms(LED_BLINK_MS);
        nrf_gpio_pin_write(GREEN_LED,LED_OFF);
        nrf_delay_ms(LED_BLINK_MS);

        //////////////////////////////////
        // test button and mic
        debug_log("Testing button and mic\r\n");

        testMicInit(zeroValue);
        while(1) // stay in infinite loop, spit out mic values
        {
            // update reading
            testMicAddSample();
            
            if (testMicAboveThreshold()) {
                nrf_gpio_pin_write(RED_LED,LED_ON);
                nrf_delay_ms(100);                  
            }
            else {
                nrf_gpio_pin_write(RED_LED,LED_OFF);   
            }
            
            // turn on green light if button is pressed
            if(nrf_gpio_pin_read(BUTTON_1) == 0)
            {
                nrf_gpio_pin_write(GREEN_LED,LED_ON);
            }
            else {
                nrf_gpio_pin_write(GREEN_LED,LED_OFF);   
            }

            nrf_delay_ms(10);
        }
        
        while(1) {};
    #endif    // end of self tests
    */
    
    // Initialize
    sd_power_mode_set(NRF_POWER_MODE_LOWPWR);  //set low power sleep mode
    BLE_init();
    adc_config();
    rtc_config();
    spi_init();
    
    collector_init();
    storer_init();
    sender_init();
    
    // Blink once on start
    nrf_gpio_pin_write(LED_1,LED_ON);
    nrf_delay_ms(2000);
    nrf_gpio_pin_write(LED_1,LED_OFF);
    
    
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
    
    
    /*int numDevices = sizeof(masterDeviceList)/sizeof(device_t);
    for(int i = 0; i < numDevices; i++)
    {
        deviceList[i] = masterDeviceList[i];
    }
    
    sortDeviceList(deviceList,numDevices);
    printDeviceList(deviceList,numDevices);
    
    debug_log("\r\n\r\n");*/
    
    /*
    nrf_gpio_pin_write(LED_2,LED_ON);
    scans_init();
    nrf_gpio_pin_write(LED_2,LED_OFF);

    
    if(nrf_gpio_pin_read(BUTTON_1) == 0)
    {
        for(int i = 0; i < 10; i++)
        {
            printScanResult(i);
        }   
    }*/

    debug_log("Done with setup().  Entering main loop.\r\n");
    
    BLEstartAdvertising();
    
    cycleStart = millis();
    
    // Enter main loop
    for (;;)  {
        //================ Sampling/Sleep handler ================
        
        if(ble_timeout)
        {
            debug_log("Connection timeout.  Disconnecting...\r\n");
            BLEforceDisconnect();
            ble_timeout = false;
        }
        
        switch(cycleState)
        {
            
            
            case SAMPLE:
                if(isCollecting)
                {
                    badgeActive |= true;
                    
                    if(millis() - cycleStart < SAMPLE_WINDOW)
                    {
                        takeMicReading();
                        //sleep = false;
                    }
                    else  {
                        collectSample();
                        cycleState = STORE;
                    }
                }
                else
                {
                    cycleState = STORE;
                }
                break;
                
            case STORE:
                ;// can't put declaration directly after case label.
                bool storerActive = updateStorer();
                //if(storerActive) debug_log("stA\r\n");
                badgeActive |= storerActive;
                cycleState = SEND;
                break;
                
            case SEND:
                ;// can't put declaration directly after case label.
                bool senderActive = updateSender();
                //if(senderActive) debug_log("seA\r\n");
                badgeActive |= senderActive;
                
                if(millis() - cycleStart > 200 || (!senderActive))  // is it time to sleep, or is sending finished?
                {
                    cycleState = SLEEP;
                }
                
                break;
            case SLEEP:
                ;// can't put declaration directly after case label.
                long sleepDuration;
                
                // If none of the modules (collector, storer, sender) is active, then we can sleep indefinitely (until BLE activity)
                if(!badgeActive)
                {
                    sleepDuration = -1;  // infinite sleep
                }
                
                // Else we're actively cycling thru main loop, and should sleep for the remainder of the sampling period
                unsigned long elapsed = millis() - cycleStart;
                if(elapsed < SAMPLE_PERIOD)
                {
                    sleepDuration = SAMPLE_PERIOD - elapsed;
                }
                else
                {
                    sleepDuration = 0;
                }
                
                // Main loop will halt on the following line as long as the badge is sleeping (i.e. until an interrupt wakes it)
                goToSleep(sleepDuration);
                
                // Exit sleep if we've reached the end of the sampling period.
                if(millis() - cycleStart >= SAMPLE_PERIOD)  // did we exit sleep by the countdown event
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
    debug_log("Connected.\r\n");
    sleep = false;

    // for app development. disable if forgotten in prod. version
    nrf_gpio_pin_write(LED_1,LED_ON);
    
    ble_timeout_set(CONNECTION_TIMEOUT_MS);
}

void BLEonDisconnect()
{
    debug_log("Disconnected.\r\n");
    sleep = false;

    // for app development. disable if forgotten in prod. version
    nrf_gpio_pin_write(LED_1,LED_OFF);
    
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
        pendingCommand = unpackCommand(p_data);
    }
    sleep = false;
    
    ble_timeout_set(CONNECTION_TIMEOUT_MS);
}


