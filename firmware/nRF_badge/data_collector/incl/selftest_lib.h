/**@file
 */

#ifndef __SELFTEST_LIB_H
#define __SELFTEST_LIB_H

#include "stdint.h"
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes

typedef enum {
	SELFTEST_PASSED 			  = 0,
	SELFTEST_FAILED_ACCELEROMETER = (1 << 0),
	SELFTEST_FAILED_MICROPHONE = 	(1 << 1),
	SELFTEST_FAILED_BATTERY = 		(1 << 2),
	SELFTEST_FAILED_EEPROM = 		(1 << 3),
	SELFTEST_FAILED_FLASH = 		(1 << 4),	
} selftest_status_t;

/**@brief Function to start the selftests of the peripherals.
 * 
 * @details In the beginning the "simple" tests without user interaction are performed:
 *			The battery, eeprom and flash test. During these tests the red LED is turned on 
 *			before each test and turned off, if the test was successful. 
 *			If all the prior tests were passed tests that need user interventions are performed:
 *			The microphone and the accelerometer. During these tests both LEDs (red and green) are
 *			turned on. If a test was passed, both LEDs blink a couple of times.
 *			When all tests have been passed the green LED goes on for 1sec.
 *
 *			The function will directly return, if one of the test fails --> The user can see which test failed
 *			because the LEDs won't go off. 
 *
 * @retval	The status of the self-test. If one test fails, it should have the value of this failed test.
 */ 
selftest_status_t selftest_test(void);

#endif