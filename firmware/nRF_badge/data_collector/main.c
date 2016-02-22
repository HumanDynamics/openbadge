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
#include "internal_flash.h"  //for managing storage and sending from internal flash
#include "external_flash.h"  //for interfacing to external SPI flash
#include "scanning.h"       //for performing scans and storing scan data







//============================= I/O-related stuff ======================================
//======================================================================================


// Setup expected "zero" for the mic value.
// analog ref is mic VCC, mic is 1/2 VCC biased, input is 1/3 scaled, so zero value is approx. (1023/2)/3 ~= 170
const int zeroValue = 166;


//============================ sound-related stuff =====================================
//======================================================================================
const unsigned long SAMPLE_WINDOW = 100; // how long we should sample
const unsigned long SAMPLE_PERIOD = 250;   // time between samples - must exceed SAMPLE_WINDOW
unsigned int readingsCount = 0;  //number of mic readings taken
unsigned long readingsSum = 0;  //sum of mic readings taken      } for computing average
unsigned long sampleStart = 0;  //beginning of sample period
bool storedSample = false;  //whether we've stored the sample from the current sampling period


//============================ BLE/data-related stuff ==================================
//======================================================================================
volatile bool BLEconnected = false;  //whether we're currently connected
volatile bool sendStatus = false;  //flag signaling that the server asked for status


//============================ time-related stuff ======================================
//======================================================================================
volatile int dateReceived = false; // signifies whether or not the badge has received a date
volatile unsigned long dataTimestamp;    // holds the date

volatile bool sleep = false;  //whether we should sleep (so actions like data sending can override sleep)


//=========================== Global function definitions ==================================
//==========================================================================================

/* Reading mic values */
void addMicReading() {
  int sample = analogRead(MIC_PIN);
  readingsSum += abs(sample - zeroValue);
  readingsCount++;
}

/* read and store data samples */
void sampleData()  {
  unsigned int micValue = readingsSum / readingsCount;
  unsigned char reading = micValue <= 255 ? micValue : 255;  //clip reading
  readingsCount = 0;
  readingsSum = 0;
  if (dateReceived)  {
    storeByte(reading,now());
  }
}

void goToSleep(long ms)  {
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
    
    
    // Initialize
    sd_power_mode_set(NRF_POWER_MODE_LOWPWR);  //set low power sleep mode
    BLEbegin();
    adc_config();
    rtc_config();
    spi_init();
    
    // LEDs
    nrf_gpio_pin_dir_set(LED_1,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_1,LED_ON);  //turn off LED
    nrf_gpio_pin_dir_set(LED_2,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_2,LED_OFF);  //turn off LED
    
    // Button
    nrf_gpio_cfg_input(BUTTON_1,NRF_GPIO_PIN_PULLUP);  //button 4
    
    // Blink once on start
    nrf_gpio_pin_write(LED_1,LED_ON);
    nrf_delay_ms(2000);
    nrf_gpio_pin_write(LED_1,LED_OFF);
    
    
    /**
     * Reset tracker
     * If the board resets for some reason, an LED will blink.
     * To intentionally reset the board, the button must be held on start.
     */
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
            
    }
    
    nrf_delay_ms(1000);
    
    
    int numDevices = sizeof(masterDeviceList)/sizeof(device_t);
    for(int i = 0; i < numDevices; i++)
    {
        deviceList[i] = masterDeviceList[i];
    }
    
    sortDeviceList(deviceList,numDevices);
    printDeviceList(deviceList,numDevices);
    
    debug_log("\r\n\r\n");
    
    nrf_gpio_pin_write(LED_2,LED_ON);
    scans_init();
    nrf_gpio_pin_write(LED_2,LED_OFF);

    
    if(nrf_gpio_pin_read(BUTTON_1) == 0)
    {
        for(int i = 0; i < 10; i++)
        {
            printScanResult(i);
        }   
    }

    //setTime(0xdead0000UL);
    
    
    
    /*while(1)
    {
        int reading = analogRead(MIC_PIN);
        debug_log("rd: %d\r\n",reading);
        nrf_delay_ms(10);
    }*/
    
    
    

    send.samplePeriod = SAMPLE_PERIOD;  //kludgey, give Flash access to SAMPLE_PERIOD

    //=========================== Resume after reset ================================
    /**
     * We need to find where we left off - what was the last chunk we wrote, and the last sent?
     *   If a chunk is completed but not sent, the last 4 bytes match the first 4 - the timestamp
     *   If a chunk has been sent, the last 4 bytes are all 0
     *   Otherwise, a chunk is not valid (probably reset midway through chunk)
     */
    nrf_gpio_pin_write(LED_2,LED_ON);  //red LED says we're intializing flash
    debug_log("\n\n\nInitializing...\r\n");


    // Find the latest stored chunk, to start storing after it
    unsigned long lastStoredTime = MODERN_TIME;  //ignore obviously false timestamps from way in past
    int lastStoredChunk = 0;
    // and the earliest unsent chunk, to start sending from it
    unsigned long earliestUnsentTime = 0xffffffffUL;  //start at any timestamp found
    int earliestUnsentChunk = 0;

    // Look through whole flash for those two chunks
    for (int c = 0; c <= lastChunk; c++)  
    {
        uint32_t* addr = ADDRESS_OF_CHUNK(c);  //address of chunk
        uint32_t timestamp = *addr;  //timestamp of chunk
        uint32_t chunkCheck = *(addr + WORDS_PER_CHUNK - 1);  //last word of chunk, the check

        if (timestamp != 0xffffffffUL && timestamp > MODERN_TIME)  //is the timestamp possibly valid?
        { 
            if (timestamp == chunkCheck || chunkCheck == 0)  //is it a completely stored chunk?
            { 
                if (timestamp > lastStoredTime)  //is it later than the latest stored one found so far?
                { 
                    lastStoredChunk = c; //keep track of latest stored chunk
                    lastStoredTime = timestamp;
                }
                if (chunkCheck != 0)  //if it's completely stored, is it also unsent?
                { 
                    if (timestamp < earliestUnsentTime)  //is it earlier than the earliest unsent one found so far?
                    {  
                        earliestUnsentChunk = c; //keep track of earliest unsent chunk
                        earliestUnsentTime = timestamp;
                    }
                }
            }  //end of if("completetely stored")
        }  //end of if("timestamp is valid")
    }  //end of for loop
    
    
    debug_log("Last stored chunk: %d at time: %u\r\n", lastStoredChunk, (unsigned int)lastStoredTime);
    //Serial.printf("%d:%d:%d %d-%d-%d\n", hour(), minute(), second(), month(), day(), year());
    
    nrf_delay_ms(100);

    initStorageFromChunk(lastStoredChunk);  //initialize storage handler


    if (earliestUnsentTime == 0xffffffffUL)  
    { //no completely stored, unsent chunks found
        debug_log("No complete unsent chunks\r\n");
        earliestUnsentChunk = lastStoredChunk; //start where we're starting storage
    }
    else  
    {  //toSend is the earliest unsent chunk
        debug_log("Earliest unsent chunk: %d from time: %u\r\n", earliestUnsentChunk, (unsigned int)earliestUnsentTime);
        //Serial.printf("%d:%d:%d %d-%d-%d\n", hour(), minute(), second(), month(), day(), year());
    }

    initSendingFromChunk(earliestUnsentChunk);    
    

    nrf_gpio_pin_write(LED_2,LED_OFF);  //done initializing

    //=========================== End of resume after reset handler ====================


    debug_log("Done with setup().  Entering main loop.\r\n");
    
    //dateReceived = true;
    //enableStorage();
    
    // Enter main loop
    for (;;)  {
        //================ Sampling/Sleep handler ================
        sleep = true;  //default to sleeping at the end of loop()
        
        if (dateReceived)  
        {  //don't start sampling unless we have valid date
            if (millis() - sampleStart <= SAMPLE_WINDOW)  // are we within the sampling window
            {  
                addMicReading();  //add to total, increment count
                sleep = false;  //don't sleep, we're sampling
            }
            else if (millis() - sampleStart >= SAMPLE_PERIOD)  // if not, have we completed the sleep cycle
            {  
                sampleStart = millis();  //mark start of sample period, resetting cycle
                storedSample = false;
                sleep = false;  //we're done sleeping
            }
            else if(storedSample == false)  // are we in sleep period, but haven't yet stored the sample
            {  
                sampleData();  //add average of readings to sample array
                storedSample = true;  //sample has been stored
                sleep = false;  //don't sleep, store.
            }
            else  // otherwise we should be sleeping
            {  
                sleep = true;
            }
        }
        else  {
            sampleStart = millis();  //if not synced, we still need to set this to sleep properly.
        }
        
        
        if (sendStatus)  //triggered from onReceive 's'
        {  
            unsigned char status = 'n';  //not synced
            if(dateReceived)
            {
                if(unsentChunkReady())  {
                    status = 'd';  //synced, and data ready to be sent
                }
                else  {
                    status = 's';  //synced, but no data ready yet
                }
            }
            
            if(BLEwriteChar(status))  //try to send status
            {
                debug_log("Sent Status - %c\r\n",status);
                sendStatus = false;
            }
            else  {
                sleep = false;  //if not able to send status, don't sleep yet.
            }
        }


        //================= Flash storage/sending handler ================
        
        //do flash storage stuff (ensure storage buffer gets stored to flash)
        if (updateStorage())  
        { 
            sleep = false;  //don't sleep if there is buffered data to store to flash
        }

        //do BLE sending stuff (send data if sending enabled)
        if (updateSending())  
        { 
            sleep = false;  //don't sleep if there's data to be sent
        }
        
        updateScanning();


        //============== Sleep, if we're supposed to =================
        if (!dateReceived)  {
            goToSleep(-1);  //sleep infinitely till interrupt
        }
        else if (sleep)  //if not synced, just sleep.
        {
            unsigned long elapsed = millis() - sampleStart;
            if (elapsed < SAMPLE_PERIOD)  
            {  //avoid wraparound in delay time calculation
                //debug_log("Sleeping...\r\n");
                unsigned long sleepDuration = SAMPLE_PERIOD - elapsed;
                //unsigned long sleepStart = millis();
                goToSleep(sleepDuration);
            }
        }
    }
}



void BLEonConnect()
{
    debug_log("Connected\r\n");
    sleep = false;
    disableStorage();  //don't manipulate flash while BLE is busy
}

void BLEonDisconnect()
{
    debug_log("Disconnected\r\n");
    sleep = false;
    //sendStatus = false;
    disableSending();  // stop sending
    if(getScanState() == SCAN_IDLE)  {
        enableStorage();  //continue storage operations now that connection is ended, if scans aren't active
    }
}

/** Function for handling incoming data from the BLE UART service
 */
void BLEonReceive(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)  
{
    debug_log("Received: ");
    if (length > 2)  
    {  //is it long enough to be a timestamp.
        debug_log("sync timestamp.\r\n");
        unsigned long f = readLong(p_data);
        setTime(f);
        //Serial.printf("%d:%d:%d %d-%d-%d\n", hour(), minute(), second(), month(), day(), year());
        disableSending();
        dateReceived = true;
    }
    else if (p_data[0] == 's')  
    {
        debug_log("status request.\r\n");
        sendStatus = true;
    }
    else if (p_data[0] == 'd')  
    {
        debug_log("data request.\r\n");
        if (dateReceived && unsentChunkReady())  
        {
            enableSending();
        }
    }
    else  
    {
        debug_log("unknown receive!\r\n");
    }
    sleep = false;  //break from sleep
}
