
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


void check_init_error(ret_code_t ret, uint8_t identifier);

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
	
	debug_log("MAIN: Start...\n\r");

	
	nrf_clock_lf_cfg_t clock_lf_cfg =  {.source        = NRF_CLOCK_LF_SRC_XTAL,            
										.rc_ctiv       = 0,                                
										.rc_temp_ctiv  = 0,                                
										.xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM};
										

	SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);
	APP_SCHED_INIT(4, 100);
	APP_TIMER_INIT(0, 60, NULL);

	ret = systick_init(0);
	check_init_error(ret, 1);
	
	ret = timeout_init();
	check_init_error(ret, 2);
	
	ret = ble_init();
	check_init_error(ret, 3);
	
	ret = sampling_init();
	check_init_error(ret, 4);
	
	ret = storer_init();
	check_init_error(ret, 5);
	
	ret = uart_commands_init();
	check_init_error(ret, 6);
	
	#ifdef TESTER_ENABLE	
		
	selftest_status_t selftest_status = selftest_test();
	debug_log("MAIN: Ret selftest_test: %u\n\r", selftest_status);
	(void) selftest_status;
	
	ret = storer_clear();
	check_init_error(ret, 7);
	debug_log("MAIN: Storer clear: %u\n\r", ret);
	
	#endif
		
	uint8_t mac[6];
	ble_get_MAC_address(mac);
	debug_log("MAIN: MAC address: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	
	advertiser_init();
	
	ret = advertiser_start_advertising();
	check_init_error(ret, 8);
	
	ret = request_handler_init();
	check_init_error(ret, 9);

	
	// If initialization was successful, blink the green LED 3 times.
	for(uint8_t i = 0; i < 3; i++) {
		nrf_gpio_pin_write(GREEN_LED, LED_ON);  //turn on LED	
		nrf_delay_ms(100);
		nrf_gpio_pin_write(GREEN_LED, LED_OFF);  //turn off LED
		nrf_delay_ms(100);
	}
	
	(void) ret;
	

	while(1) {		
		app_sched_execute();	// Executes the events in the scheduler queue
		sd_app_evt_wait();		// Sleeps until an event/interrupt occurs
	}	
}

/**@brief Function that enters a while-true loop if initialization failed.
 *
 * @param[in]	ret				Error code from an initialization function.
 * @param[in]	identifier		Identifier, represents the number of red LED blinks.
 *
 */
void check_init_error(ret_code_t ret, uint8_t identifier) {
	if(ret == NRF_SUCCESS)
		return;
	while(1) {
		for(uint8_t i = 0; i < identifier; i++) {
			nrf_gpio_pin_write(RED_LED, LED_ON);  //turn on LED	
			nrf_delay_ms(200);
			nrf_gpio_pin_write(RED_LED, LED_OFF);  //turn off LED
			nrf_delay_ms(200);
		}
		nrf_delay_ms(2000);
	}
}
