#ifndef TESTER_ENABLE_H
#define TESTER_ENABLE_H
#define LED_BLINK_MS      500

#include "internal_flash.h"
#include "external_flash.h"

// Tests internal flash. Returns true on success
bool testInternalFlash(void);

// Tests external flash. Returns true on success
bool testExternalFlash(void);

#endif // TESTER_ENABLE_H