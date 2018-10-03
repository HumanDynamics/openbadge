
#include "sdk_config.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "custom_board.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "softdevice_handler.h"
#include "debug_lib.h"
#include "app_timer.h"
#include "systick_lib.h"
#include "timeout_lib.h"
#include "ble_lib.h"
#include "advertiser_lib.h"
#include "request_handler_lib_01v1.h"
#include "request_handler_lib_02v1.h"
#include "storer_lib.h"
#include "sampling_lib.h"
#include "app_scheduler.h"
#include "selftest_lib.h"
#include "uart_commands_lib.h"


#define BATTERY_MEASUREMENT_PERIOD_MS	15000

/**
 * ============================================== MAIN ====================================================
 */
int main(void)
{


	nrf_gpio_pin_dir_set(GREEN_LED, NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
	nrf_gpio_pin_dir_set(RED_LED,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
	
	// Do some nice pattern here:
    nrf_gpio_pin_write(RED_LED, LED_ON);  //turn on LED
	
	nrf_delay_ms(100);
    nrf_gpio_pin_write(RED_LED, LED_OFF);  //turn off LED
	
	
	ret_code_t ret;
	
	
	debug_init();
	
	debug_log("Start...\n\r");

	
	nrf_clock_lf_cfg_t clock_lf_cfg =  {.source        = NRF_CLOCK_LF_SRC_XTAL,            
										.rc_ctiv       = 0,                                
										.rc_temp_ctiv  = 0,                                
										.xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM};
										

	SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);
	APP_SCHED_INIT(4, 100);
	APP_TIMER_INIT(0, 60, NULL);
	
	ret = systick_init(0);
	
	ret = timeout_init();
	
	ret = ble_init();
	
	ret = sampling_init();
	
	ret = storer_init();
	
	#ifdef TESTER_ENABLE
	
	ret = uart_commands_init();
		
	selftest_status_t selftest_status = selftest_test();
	debug_log("Ret selftest_test: %u\n\r", selftest_status);
	(void) selftest_status;
	
	#endif
		
	uint8_t mac[6];
	ble_get_MAC_address(mac);
	debug_log("MAC address: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	
	advertiser_init();
	
	ret = advertiser_start_advertising();	
	
	ret = request_handler_init();
	
	// Directly start the battery-sampling
	ret = sampling_start_battery(0, BATTERY_MEASUREMENT_PERIOD_MS, 0);
	
	(void) ret;
	

	while(1) {
		sd_app_evt_wait();
		app_sched_execute();
	}	
}
