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



static void simple_test_start(void) {
	nrf_delay_ms(100);
	nrf_gpio_pin_write(RED_LED, LED_ON);  
}
static void simple_test_passed(void) {
	nrf_delay_ms(100);
	nrf_gpio_pin_write(RED_LED, LED_OFF);  
}
static void user_intervention_test_start(void) {
	nrf_delay_ms(200);
	nrf_gpio_pin_write(RED_LED, LED_ON);  
	nrf_gpio_pin_write(GREEN_LED, LED_ON);  
}
static void user_intervention_test_passed(void) {
	for(uint8_t i = 0; i < 5; i++) {
		nrf_delay_ms(100);
		nrf_gpio_pin_write(RED_LED, LED_ON);
		nrf_gpio_pin_write(GREEN_LED, LED_ON);		
		nrf_delay_ms(100);
		nrf_gpio_pin_write(RED_LED, LED_OFF);  
		nrf_gpio_pin_write(GREEN_LED, LED_OFF);  
	}
}

// All modules need to be initialized before using this function
selftest_status_t selftest_test(void) {
	
	debug_log("SELFTEST: Starting selftest...\n");
	selftest_status_t selftest_status = SELFTEST_PASSED;
	
	
	/********** BATTERY **************/
	simple_test_start();
	debug_log("SELFTEST: Starting battery selftest:\n");
	if(!battery_selftest()) {
		debug_log("SELFTEST: Battery selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_BATTERY);
		return selftest_status;
	} else {
		debug_log("SELFTEST: Battery selftest successful!!\n");
	}
	simple_test_passed();
	
	/************ EEPROM ****************/
	simple_test_start();
	debug_log("SELFTEST: Starting eeprom selftest:\n");
	if(!eeprom_selftest()) {
		debug_log("SELFTEST: EEPROM selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_EEPROM);
		return selftest_status;
	} else {
		debug_log("SELFTEST: EEPROM selftest successful!!\n");
	}
	simple_test_passed();
	
	
	/*********** FLASH *************/
	simple_test_start();
	debug_log("SELFTEST: Starting flash selftest:\n");
	if(!flash_selftest()) {
		debug_log("SELFTEST: Flash selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_FLASH);
		return selftest_status;
	} else {
		debug_log("SELFTEST: Flash selftest successful!!\n");
	}
	simple_test_passed();
	
	/*************** MICROPHONE ******************/
	user_intervention_test_start();
	debug_log("SELFTEST: Starting microphone selftest:  (Please make some noise!)\n");
	if(!microphone_selftest()) {
		debug_log("SELFTEST: Microphone selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_MICROPHONE);
		return selftest_status;
	} else {
		debug_log("SELFTEST: Microphone selftest successful!!\n");
	}
	user_intervention_test_passed();
	
	/************** ACCELEROMETER *********************/	
	#if ACCELEROMETER_PRESENT
	user_intervention_test_start();	
	debug_log("SELFTEST: Starting accelerometer selftest: (Please move the badge!)\n");
	if(!accel_selftest()) {
		debug_log("SELFTEST: Accelerometer selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_ACCELEROMETER);
		return selftest_status;
	} else {
		debug_log("SELFTEST: Accelerometer selftest successful!!\n");
	}
	user_intervention_test_passed();

	#endif
	
	nrf_gpio_pin_write(GREEN_LED, LED_ON);		
	nrf_delay_ms(1000);
	nrf_gpio_pin_write(GREEN_LED, LED_OFF);  
	
	return selftest_status;	
}
