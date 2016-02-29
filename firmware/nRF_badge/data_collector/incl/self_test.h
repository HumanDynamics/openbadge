#ifndef TESTER_ENABLE_H
#define TESTER_ENABLE_H
#define LED_BLINK_MS      500

#include "internal_flash.h"
#include "external_flash.h"

#define THRESH_BUFSIZE 32  //size of the thresholding buffer, in bytes
#define THRESH_SAMPLES 64 // number of samples averaged to create a single reading
#define THRESH_SD 1.2 // 
#define THRESH_MAGIC_NUMBER 15 // value to add to average to reduce sensitivity
struct
{
    uint8_t bytes[THRESH_BUFSIZE];
	int pos;
	int zeroValue;
} threshold_buffer;

// Tests internal flash. Returns true on success
bool testInternalFlash(void);

// Tests external flash. Returns true on success
bool testExternalFlash(void);

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