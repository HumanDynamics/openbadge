/**
 * This unittest tests the functionality of the accel_lib_mock.c, parts of the callback_generator_lib.cc and data_generator_lib.cc that are responsible for the accelerometer.
 * The name of the functions for the interrupt-generating is ACCEL_INT1. So all the functions related to the interrupt-generating start with callback_generator_ACCEL_INT1...().
 * The name of the functions for the data-generating for the accel_read_accelerometer-function is accel_read_accelerometer. So all the functions related to the data-generating start with data_generator_accel_read_acceleration...().
 */

// Don't forget gtest.h, which declares the testing framework.

#include "accel_lib.h"
#include "timer_lib.h"			
#include "callback_generator_lib.h"
#include "data_generator_lib.h"
#include "gtest/gtest.h"


static volatile uint32_t accel_interrupt_handler_event_sum = 0; // Just a variable to test how often the accel_interrupt_handler-function is called.
// This is the interrupt-handler that is called by the accelerometer module, when an interrupt occured
void accel_interrupt_handler(accel_interrupt_event_t const * p_event) {
	accel_interrupt_handler_event_sum += *p_event;
}

// This is a function to generate the data for the interrupt-generation of the accelerometer
void correct_accel_interrupt_generator_handler(nrf_drv_gpiote_pin_t* pin, nrf_gpiote_polarity_t* action) {
	*pin = 25;
	*action = NRF_GPIOTE_POLARITY_LOTOHI;
}

void false_accel_interrupt_generator_handler(nrf_drv_gpiote_pin_t* pin, nrf_gpiote_polarity_t* action) {
	// These parameters are invalid for the accelerometer, because the accelerometer-module expects pin == 25 and action == NRF_GPIOTE_POLARITY_LOTOHI.
	// So the accelerometer module won't call the accel_interrupt_handler-function.
	*pin = 24;
	*action = NRF_GPIOTE_POLARITY_HITOLO;
}


// Data-generator function for the accel_read_acceleration()-function of the accelerometer module.
ret_code_t accel_read_acceleration_generator_handler(int16_t* x, int16_t* y, int16_t* z, uint8_t* num_samples) {
	
	for(int16_t i = 0; i < 10; i++) {
		x[i] = i;
		y[i] = -i;
		z[i] = 2*i;
	}
	
	*num_samples = 10;
	return NRF_SUCCESS;
}


namespace {
	
class AccelLibMockTest : public ::testing::Test {
	virtual void SetUp() {
		callback_generator_ACCEL_INT1_reset();
		data_generator_accel_read_acceleration_reset();
	}
};


// Tests for the interrupt-generating
TEST_F(AccelLibMockTest, InterruptNoGeneratorHandlerTest) {
	// Add the timepoints for the callback/interrupt-generation
	uint32_t timepoint_array[5] = {100, 200, 300, 400, 500};
	callback_generator_ACCEL_INT1_add_trigger_timepoints(timepoint_array, 5);
	// Set the event-data generator handler to NULL --> The default implementation in callback_generator_lib.cc will be taken.
	callback_generator_ACCEL_INT1_set_generator(NULL);
	accel_interrupt_handler_event_sum = 0;
	
	accel_init();
	accel_set_interrupt_handler(accel_interrupt_handler);
	// This function triggers the interrupt-generating by calling callback_generator_ACCEL_INT1_trigger()
	accel_set_interrupt(ACCEL_MOTION_INTERRUPT);
	
	while(!callback_generator_ACCEL_INT1_is_ready());	
	
	EXPECT_EQ(accel_interrupt_handler_event_sum, 5);
}



TEST_F(AccelLibMockTest, InterruptCorrectGeneratorHandlerTest) {
	// Add the timepoints for the callback/interrupt-generation
	uint32_t timepoint_array[5] = {100, 200, 300, 400, 500};
	callback_generator_ACCEL_INT1_add_trigger_timepoints(timepoint_array, 5);
	// Set the event-data generator handler to the 
	callback_generator_ACCEL_INT1_set_generator(correct_accel_interrupt_generator_handler);
	accel_interrupt_handler_event_sum = 0;
	
	accel_init();
	accel_set_interrupt_handler(accel_interrupt_handler);
	// This function triggers the interrupt-generating by calling callback_generator_ACCEL_INT1_trigger()
	accel_set_interrupt(ACCEL_MOTION_INTERRUPT);
	
	while(!callback_generator_ACCEL_INT1_is_ready());	
	EXPECT_EQ(accel_interrupt_handler_event_sum, 5);
}

TEST_F(AccelLibMockTest, InterruptFalseGeneratorHandlerTest) {
	// Add the timepoints for the callback/interrupt-generation
	uint32_t timepoint_array[5] = {100, 200, 300, 400, 500};
	callback_generator_ACCEL_INT1_add_trigger_timepoints(timepoint_array, 5);
	// Set the event-data generator handler to the 
	callback_generator_ACCEL_INT1_set_generator(false_accel_interrupt_generator_handler);
	accel_interrupt_handler_event_sum = 0;
	
	accel_init();
	accel_set_interrupt_handler(accel_interrupt_handler);
	// This function triggers the interrupt-generating by calling callback_generator_ACCEL_INT1_trigger()
	accel_set_interrupt(ACCEL_MOTION_INTERRUPT);
	
	while(!callback_generator_ACCEL_INT1_is_ready());
	EXPECT_EQ(accel_interrupt_handler_event_sum, 0);
}


// Test for the data-generating (accel_read_acceleration)
TEST_F(AccelLibMockTest, ReadFunctionNoGeneratorTest) {
	// Read the acceleration from the accelerometer. In this case no data-generator function is set --> the default implementation in data_generator_lib.cc is taken.
	int16_t x[32], y[32], z[32];
	uint8_t num_samples;
	ret_code_t ret = accel_read_acceleration(x, y, z, &num_samples);

	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(num_samples, 1);
	EXPECT_EQ(x[0], 0);
	EXPECT_EQ(y[0], 0);
	EXPECT_EQ(z[0], 0);	
}


TEST_F(AccelLibMockTest, ReadFunctionGeneratorTest) {
	// Read the acceleration from the accelerometer. In this case the data-generator function is set, so the data_generator_lib.cc takes this implementation.
	data_generator_accel_read_acceleration_set_generator(accel_read_acceleration_generator_handler);
	int16_t x[32], y[32], z[32];
	uint8_t num_samples;
	ret_code_t ret = accel_read_acceleration(x, y, z, &num_samples);

	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(num_samples, 10);
	for(int16_t i = 0; i < num_samples; i++) {
		EXPECT_EQ(x[i], i);
		EXPECT_EQ(y[i], -i);
		EXPECT_EQ(z[i], i*2);
	}
}
};  
