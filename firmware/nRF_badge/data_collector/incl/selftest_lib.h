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


selftest_status_t selftest_test(void);

#endif