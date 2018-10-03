#include "selftest_lib.h"

#include "microphone_lib.h"
#include "battery_lib.h"
#include "accel_lib.h"
#include "eeprom_lib.h"
#include "flash_lib.h"

#include "debug_lib.h"


// All modules need to be initialized before
selftest_status_t selftest_test(void) {
	debug_log("Starting selftest...\n");
	selftest_status_t selftest_status = SELFTEST_PASSED;
	
	#if ACCELEROMETER_PRESENT
	debug_log("Starting accelerometer selftest: (Please move the badge!)\n");
	if(!accel_selftest()) {
		debug_log("Accelerometer selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_ACCELEROMETER);
	} else {
		debug_log("Accelerometer selftest successful!!\n");
	}
	#endif
	
	debug_log("Starting microphone selftest:  (Please make some noise!)\n");
	if(!microphone_selftest()) {
		debug_log("Microphone selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_MICROPHONE);
	} else {
		debug_log("Microphone selftest successful!!\n");
	}
	
	debug_log("Starting battery selftest:\n");
	if(!battery_selftest()) {
		debug_log("Battery selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_BATTERY);
	} else {
		debug_log("Battery selftest successful!!\n");
	}

	debug_log("Starting eeprom selftest:\n");
	if(!eeprom_selftest()) {
		debug_log("EEPROM selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_EEPROM);
	} else {
		debug_log("EEPROM selftest successful!!\n");
	}
	
	debug_log("Starting flash selftest:\n");
	if(!flash_selftest()) {
		debug_log("Flash selftest failed!!\n");
		selftest_status = (selftest_status_t) (selftest_status | SELFTEST_FAILED_FLASH);
	} else {
		debug_log("Flash selftest successful!!\n");
	}
	
	return selftest_status;	
}
