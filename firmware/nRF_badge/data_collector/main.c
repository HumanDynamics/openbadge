

#include <stdint.h>
#include <string.h>
#include <nrf51.h>

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

#include "app_error.h"

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

typedef struct {
    bool error_occured;
    uint32_t error_code;
    uint32_t line_num;
    char file_name[32];
} AppErrorData_t;

static AppErrorData_t mAppErrorData __attribute((section (".noinit")));

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
    mAppErrorData.error_occured = true;
    mAppErrorData.error_code = error_code;
    mAppErrorData.line_num = line_num;
    strncpy(mAppErrorData.file_name, (char *) p_file_name, sizeof(mAppErrorData.file_name));

    NVIC_SystemReset();
}

enum cycleStates {SLEEP, SAMPLE, SCAN, STORE, SEND};
unsigned long cycleStart;       // start of main loop cycle (e.g. sampling cycle)
int cycleState = SAMPLE;     // to keep track of state of main loop
#define MIN_SLEEP 5UL      // ms of sleep, minimum (keep well under SAMPLE_PERIOD - SAMPLE_WINDOW to leave room for sending)
#define MAX_SLEEP 120000UL // ms of sleep, maximum.  (2mins, so that badge periodically cycles thru main loop, even when idle)

// If any module (collecting, storing, sending) has any pending operations, this gets set to true
bool badgeActive = false;   // Otherwise, the badge is inactive and can enter indefinite sleep.



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

    // TODO: Check the reset reason to make sure our noinit RAM is valid
    if (mAppErrorData.error_occured) {
        debug_log("CRASH! APP ERROR %lu @ %s:%lu\r\n", mAppErrorData.error_code,
                  mAppErrorData.file_name, mAppErrorData.line_num);
        mAppErrorData.error_occured = false;
    }

    debug_log("Name: %.5s\r\n",DEVICE_NAME);


    // Define and set LEDs
    nrf_gpio_pin_dir_set(LED_1,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_1,LED_ON);  //turn on LED
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
    
    
    #if defined(TESTER_ENABLE) // tester mode is enabled
        runSelfTests();
        while(1);
    #endif    // end of self tests
    
    
    /*
    debug_log("=DEVELOPMENT BADGE.  ONLY ERASES EEPROM=\r\n");
    debug_log("=ERASING EEPROM...=\r\n");
    ext_eeprom_wait();
    unsigned char empty[EXT_CHUNK_SIZE + EXT_EEPROM_PADDING];
    memset(empty,0,sizeof(empty));
    for (int i = EXT_FIRST_CHUNK; i <= EXT_LAST_CHUNK; i++)  {
        ext_eeprom_write(EXT_ADDRESS_OF_CHUNK(i),empty,sizeof(empty));
        if (i % 10 == 0)  {
            nrf_gpio_pin_toggle(LED_1);
            nrf_gpio_pin_toggle(LED_2);
        }
        ext_eeprom_wait();
    }
    debug_log("  done.  \r\n");
    nrf_gpio_pin_write(LED_1,LED_ON);
    nrf_gpio_pin_write(LED_2,LED_ON);
    while (1);
    */
    
    
    collector_init();
    storer_init();
    sender_init();
    scanner_init();
    
    BLEsetBadgeAssignment(getStoredBadgeAssignment());
    advertising_init();
    
    // Blink once on start
    nrf_gpio_pin_write(LED_1,LED_OFF);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LED_2,LED_ON);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LED_2,LED_OFF);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LED_1,LED_ON);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LED_1,LED_OFF);

    
    nrf_delay_ms(1000);

    debug_log("Done with setup.  Entering main loop.\r\n\r\n");
    
    BLEstartAdvertising();
    
    cycleStart = millis();
    
    nrf_delay_ms(2);
    
    
    // Enter main loop
    for (;;)  {
        //================ Sampling/Sleep handler ================
        
        if (ble_timeout)  {
            debug_log("Connection timeout.  Disconnecting...\r\n");
            BLEforceDisconnect();
            ble_timeout = false;
        }
        
        if (led_timeout)  {
            nrf_gpio_pin_write(LED_2,LED_OFF);
            led_timeout = false;
        }
        
        switch (cycleState)  {
            
        case SAMPLE:
            if (millis() - lastBatteryUpdate >= MIN_BATTERY_READ_INTERVAL)  {
                //badgeActive |= true;
                if(BLEpause(PAUSE_REQ_COLLECTOR))  {
                    updateBatteryVoltage();
                    BLEresume(PAUSE_REQ_COLLECTOR);
                }
            }
            
            if (isCollecting)  {
                badgeActive |= true;
                
                if (millis() - cycleStart < sampleWindow)  {
                    takeMicReading();
                    //sleep = false;
                }
                else  {
                    collectSample();
                    cycleState = SCAN;
                }
            }
            else  {
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
            
            if (millis() - cycleStart > (samplePeriod - MIN_SLEEP) || (!senderActive))  {  // is it time to sleep, or done sending?
                cycleState = SLEEP;
            }
            
            break;
        case SLEEP:
            ;// can't put declaration directly after case label.
            long sleepDuration;
            unsigned long elapsed = millis() - cycleStart;
            
            
            // If none of the modules (collector, storer, sender) is active, then we can sleep indefinitely (until BLE activity)
            if (!badgeActive)  {
                sleepDuration = -1;  // infinite sleep
            }
            
            // Else we're actively cycling thru main loop, and should sleep for the remainder of the sampling period
            else if (elapsed < samplePeriod)  {
                sleepDuration = samplePeriod - elapsed;
            }
            else  {
                sleepDuration = 0;
            }
            
            // Main loop will halt on the following line as long as the badge is sleeping (i.e. until an interrupt wakes it)
            goToSleep(sleepDuration);
            
            // Exit sleep if we've reached the end of the sampling period, or if we're in idle mode
            if (millis() - cycleStart >= samplePeriod || (!badgeActive))  {  // did we exit sleep by the countdown event
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


