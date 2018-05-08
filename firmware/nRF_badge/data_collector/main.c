
#include "sdk_config.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
//#include <nrf51822_peripherals.h> // needed for the peripheral defines!!! (e.g. UART_PRESENT) --> now in sdk_config.h
//#include <nrf51.h> // includes the core_cm0 peripheral (NVIC_SystemReset)
//#include <app_timer.h>
//#include <app_scheduler.h>

//#include <app_uart.h> // requires app_util_platform.h


#include "boards.h"

#include "nrf_drv_uart.h"
#include "nrf_gpio.h"

#include "nrf_delay.h"
/**
 * From Nordic SDK
 */
//#include "nordic_common.h"
//#include "nrf.h"
//#include "nrf51_bitfields.h"

//#include "nrf_drv_rtc.h"        //driver abstraction for real-time counter
//#include "app_error.h"          //error handling
//#include "nrf_delay.h"          //includes blocking delay functions
//#include "nrf_gpio.h"           //abstraction for dealing with gpio
//#include "ble_flash.h"          //for writing to flash

//#include "app_error.h"

//#include "ble_gap.h"            //basic ble functions (advertising, scans, connecting)

//#include "debug_log.h"          //UART debugging logger
//requires app_fifo, app_uart_fifo.c and retarget.c for printf to work properly

//#include "nrf_drv_config.h"
//#include "boards.h"

/**
 * Custom libraries/abstractions
 */
//#include "analog.h"     //analog inputs, battery reading
//#include "battery.h"
//#include "external_flash.h"  //for interfacing to external SPI flash
//#include "scanner.h"       //for performing scans and storing scan data
//#include "self_test.h"   // for built-in tests
/*
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
*/
//=========================== Global function definitions ==================================
//==========================================================================================

#define SCHED_MAX_EVENT_DATA_SIZE sizeof(uint32_t)
#define SCHED_QUEUE_SIZE 100



/*

void uart_event_handle(app_uart_evt_t * p_event) {
    if (p_event->evt_type == APP_UART_DATA_READY) {
        uint8_t rx_byte;
        while (app_uart_get(&rx_byte) == NRF_SUCCESS) {
            
        }
    } else {
       
    }
}

void debug_log_init(void)
{
    uint32_t err_code = NRF_SUCCESS;
    const app_uart_comm_params_t comm_params =  {
        11, 
        10, 
        0, 
        0, 
        APP_UART_FLOW_CONTROL_DISABLED, 
        false, 
        0x01D7E000UL // from nrf51_bitfields.h
    }; 
        
    APP_UART_FIFO_INIT(&comm_params, 
                       32, 
                       256,
                       uart_event_handle,
                       3,
                       err_code);

    UNUSED_VARIABLE(err_code);
}


*/


void handler (nrf_drv_uart_event_t * p_event, void * p_context){

}


// TODO: private printf-function!!
/*
uint8_t buf[200];
void pprintf(args...){
	sprintf(buf, args);
	
}
*/

/**
 * ============================================== MAIN ====================================================
 */
int main(void)
{

	nrf_gpio_pin_dir_set(LED_1,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_1,LED_OFF);  //turn on LED
	
	nrf_gpio_pin_dir_set(LED_2,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_2,LED_ON);  //turn on LED

	nrf_drv_uart_config_t config = NRF_DRV_UART_DEFAULT_CONFIG;
	
	
	
	config.baudrate = (nrf_uart_baudrate_t) 0x01D7E000UL; // from nrf51_bitfields.h 
    config.hwfc = NRF_UART_HWFC_DISABLED;
    config.interrupt_priority = 3;
    config.parity = NRF_UART_PARITY_EXCLUDED;
    config.pselcts = 0;
    config.pselrts = 0;
    config.pselrxd = 11;
    config.pseltxd = 10;
	
	//nrf_drv_uart_t instance = NRF_DRV_UART_INSTANCE(1);	
	nrf_drv_uart_t _instance;
	_instance.drv_inst_idx = UART0_INSTANCE_INDEX;//CONCAT_3(UART, 0, _INSTANCE_INDEX);
	_instance.reg.p_uart = (NRF_UART_Type *) NRF_UART0;//_BASE;
	nrf_drv_uart_init(&_instance, &config, handler);
	
	
	
	char* s = "Hallo!!!\n\r";
	
	while(1) {
		nrf_drv_uart_tx(&_instance, (uint8_t*) s, strlen(s));
		nrf_delay_ms(1000);
	}
	
	
	
	
	
	
	//app_error_save_and_stop(1,2,3);

	while(1);
/*
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
    debug_log("Firmware Version: %s, Branch: %s, Commit: %s\r\n", GIT_TAG, GIT_BRANCH, GIT_COMMIT);


    // Define and set LEDs
    nrf_gpio_pin_dir_set(LED_1,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_1,LED_ON);  //turn on LED
    nrf_gpio_pin_dir_set(LED_2,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_2,LED_OFF);  //turn off LED

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


   

    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);

    collector_init();
    storer_init();
    sender_init();
    scanner_init();
    BatteryMonitor_init();

    BLEsetBadgeAssignment(getStoredBadgeAssignment());
    advertising_init();

    // Blink once on start
	nrf_gpio_pin_write(LED_1,LED_OFF);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LED_1, LED_ON);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LED_1, LED_OFF);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LED_1,LED_ON);
    nrf_gpio_pin_write(LED_2,LED_ON);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LED_1,LED_OFF);
    nrf_gpio_pin_write(LED_2,LED_OFF);


    nrf_delay_ms(1000);

    debug_log("Done with setup.  Entering main loop.\r\n\r\n");

    BLEstartAdvertising();

    nrf_delay_ms(2);

    while (true) {
        app_sched_execute();
        sd_app_evt_wait();
    }
	*/
}

/*
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


void BLEonReceive(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
    if(length > 0)
    {
        pendingCommand = unpackCommand(p_data, length);
        app_sched_event_put(NULL, 0, processPendingCommand);
    }

    ble_timeout_set(CONNECTION_TIMEOUT_MS);
}
*/