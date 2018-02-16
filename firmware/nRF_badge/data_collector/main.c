#include <stdint.h>
#include <string.h>
#include <nrf51.h>
#include <app_timer.h>
#include <app_scheduler.h>

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
#include "ble_flash.h"          //for writing to flash

#include "app_error.h"

#include "ble_gap.h"            //basic ble functions (advertising, scans, connecting)

#include "debug_log.h"          //UART debugging logger
//requires app_fifo, app_uart_fifo.c and retarget.c for printf to work properly

#include "nrf_drv_config.h"
#include "boards.h"

/**
 * Custom libraries/abstractions
 */
#include "analog.h"     //analog inputs, battery reading
#include "battery.h"
//#include "external_flash.h"  //for interfacing to external SPI flash
#include "scanner.h"       //for performing scans and storing scan data
#include "self_test.h"   // for built-in tests

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

//=========================== Global function definitions ==================================
//==========================================================================================

#define SCHED_MAX_EVENT_DATA_SIZE sizeof(uint32_t)
#define SCHED_QUEUE_SIZE 100
 
/**
 * ============================================== MAIN ====================================================
 */
int main(void){

    debug_log_init();
    debug_log("Name: %.5s\r\n",DEVICE_NAME);
    debug_log("Firmware Version: %s, Branch: %s, Commit: %s\r\n", GIT_TAG, GIT_BRANCH, GIT_COMMIT);
    
	// Initialize
    BLE_init();
    sd_power_mode_set(NRF_POWER_MODE_LOWPWR);
    adc_config();
    rtc_config();
    spi_init();
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
    collector_init();
    storer_init();
    sender_init();
    scanner_init();
    BLEsetBadgeAssignment(getStoredBadgeAssignment());
    advertising_init();
    debug_log("Done with setup.\r\nSet the log location and press Button 1 to begin printing RSSI values...\r\n");

    // setup button
    nrf_gpio_cfg_input(BUTTON_1,NRF_GPIO_PIN_PULLUP);
	while(nrf_gpio_pin_read(BUTTON_1) != 0);
	nrf_gpio_cfg_default(BUTTON_1);
	
	// start scanning and printing values
    BLEstartAdvertising();
    while (true) {
   		startScan(); 
    	nrf_delay_ms(4000);
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

static void processPendingCommand(void * p_event_data, uint16_t event_size) {
    bool sendOperationsRemaining = updateSender();
    if (sendOperationsRemaining) {
        app_sched_event_put(NULL, 0, processPendingCommand);
    }
}

/** Function for handling incoming data from the BLE UART service
 */
void BLEonReceive(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)  
{
    if(length > 0)
    {
        pendingCommand = unpackCommand(p_data, length);
        app_sched_event_put(NULL, 0, processPendingCommand);
    }

    ble_timeout_set(CONNECTION_TIMEOUT_MS);
}


