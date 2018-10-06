#include "selftest_lib.h"

#include "microphone_lib.h"
#include "battery_lib.h"
#include "accel_lib.h"
#include "eeprom_lib.h"
#include "flash_lib.h"
#include "custom_board.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

#include "debug_lib.h"


static void test_start_report(uint8_t test_number) {
	for(uint8_t i = 0; i < test_number; i++) {
		nrf_gpio_pin_write(GREEN_LED, LED_ON);  //turn on LED	
		nrf_delay_ms(100);
		nrf_gpio_pin_write(GREEN_LED, LED_OFF);  //turn off LED
		nrf_delay_ms(100);
	}
	nrf_gpio_pin_write(GREEN_LED, LED_ON);  //turn on LED
}

static void test_passed_report(void) {
	nrf_gpio_pin_write(GREEN_LED, LED_OFF);  //turn off LED
	nrf_delay_ms(100);
	nrf_gpio_pin_write(RED_LED, LED_ON);  //turn off LED
	nrf_delay_ms(100);
	nrf_gpio_pin_write(RED_LED, LED_OFF);  //turn off LED
	nrf_delay_ms(500);
}

// All modules need to be initialized before
selftest_status_t selftest_test(void) {
	debug_log("Starting selftest...\n");
	selftest_status_t selftest_status = SELFTEST_PASSED;
	
	/************** ACCELEROMETER *********************/
	#if ACCELEROMETER_PRESENT
	test_start_report(1); 
	
	debug_log("Starting accelerometer selftest: (Please move the badge!)\n");
	if(!accel_selftest()) {
		debug_log("Accelerometer selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_ACCELEROMETER);
		return selftest_status;
	} else {
		debug_log("Accelerometer selftest successful!!\n");
	}
	test_passed_report();

	#endif
	
	
	/*************** MICROPHONE ******************/
	test_start_report(2);
	
	debug_log("Starting microphone selftest:  (Please make some noise!)\n");
	if(!microphone_selftest()) {
		debug_log("Microphone selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_MICROPHONE);
		return selftest_status;
	} else {
		debug_log("Microphone selftest successful!!\n");
	}
	test_passed_report();
	
	
	/********** BATTERY **************/
	test_start_report(3);
	debug_log("Starting battery selftest:\n");
	if(!battery_selftest()) {
		debug_log("Battery selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_BATTERY);
		return selftest_status;
	} else {
		debug_log("Battery selftest successful!!\n");
	}
	test_passed_report();

	
	/************ EEPROM ****************/
	test_start_report(4);
	debug_log("Starting eeprom selftest:\n");
	if(!eeprom_selftest()) {
		debug_log("EEPROM selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_EEPROM);
		return selftest_status;
	} else {
		debug_log("EEPROM selftest successful!!\n");
	}
	test_passed_report();
	
	
	/*********** FLASH *************/
	test_start_report(5);
	debug_log("Starting flash selftest:\n");
	if(!flash_selftest()) {
		debug_log("Flash selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_FLASH);
		return selftest_status;
	} else {
		debug_log("Flash selftest successful!!\n");
	}
	test_passed_report();
	
	return selftest_status;	
}
