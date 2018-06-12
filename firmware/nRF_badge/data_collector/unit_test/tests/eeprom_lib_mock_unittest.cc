

// Don't forget gtest.h, which declares the testing framework.

#include "eeprom_lib.h"
#include "gtest/gtest.h"

#define EEPROM_SIZE_TEST	(256*1024)

namespace {


// Test for init the module
TEST(EEPROMInitTest, ReturnValue) {
	ret_code_t ret = eeprom_init();
	ASSERT_EQ(ret, NRF_SUCCESS);
}

TEST(EEPROMInitTest, SizeCheck) {
	uint32_t size = eeprom_get_size();
	ASSERT_EQ(size, EEPROM_SIZE_TEST);
}



TEST(EEPROMStoreTest, ReturnValues) {
	uint8_t data[10];	
	ret_code_t ret = eeprom_store(0, data, 10);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = eeprom_store_bkgnd(0, data, EEPROM_SIZE_TEST+1);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
}






};  // namespace

// Step 3. Call RUN_ALL_TESTS() in main().
//
// We do this by linking in src/gtest_main.cc file, which consists of
// a main() function which calls RUN_ALL_TESTS() for us.
//
// This runs all the tests you've defined, prints the result, and
// returns 0 if successful, or 1 otherwise.
//
// Did you notice that we didn't register the tests?  The
// RUN_ALL_TESTS() macro magically knows about all the tests we
// defined.  Isn't this convenient?
