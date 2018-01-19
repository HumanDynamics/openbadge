#ifndef TESTER_ENABLE_H
#define TESTER_ENABLE_H

#define LED_BLINK_MS      500

//NRF51DK has common cathode LEDs, i.e. gpio LOW turns LED on.
#ifdef BOARD_PCA10028
    #define LED_ON 0
    #define LED_OFF 1
//Badges are common anode, the opposite.
#else
    #define LED_ON 1
    #define LED_OFF 0
#endif

#include "nrf_delay.h"          //includes blocking delay functions

//#include "internal_flash.h"
#include "analog.h"
#include "storer.h"
#include "ext_eeprom.h"

#define THRESH_BUFSIZE 32  //size of the thresholding buffer, in bytes
#define THRESH_SAMPLES 64 // number of samples averaged to create a single reading
#define THRESH_SD 1.2 //
//#define THRESH_MAGIC_NUMBER 15 // value to add to average to reduce sensitivity
#define THRESH_MAGIC_NUMBER 10 // value to add to average to reduce sensitivity, with 8bit conversion.

struct
{
    uint8_t bytes[THRESH_BUFSIZE];
	int pos;
	int zeroValue;
} threshold_buffer;

/*
// Tests internal flash. Returns true on success
bool testInternalFlash(void);
*/

// adds a mic sample to the thresholding array. It will make
// THRESH_SAMPLES samples and average them to create a single reading
void testMicAddSample();

// init the sampling threshold buffer
void testMicInit(int zeroValue);
#endif // TESTER_ENABLE_H

// returns the average of mic thresholding buffer
uint8_t testMicAvg();

// return true if last sample is above threshold
bool testMicAboveThreshold();


// tests the accelerator
void accel_spi_evt_handler(spi_master_evt_t spi_master_evt);
void accel_test();


void runSelfTests();
