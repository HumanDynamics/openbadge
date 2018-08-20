

// Don't forget gtest.h, which declares the testing framework.

#include "callback_generator_internal_lib.h"
#include "gtest/gtest.h"



#include "timer_lib.h"

// Declaring the handler-function signature
CALLBACK_HANDLER_FUNCTION_DECLARATION(TEST, void, uint32_t val);

// Declaring the generator-function signature
CALLBACK_GENERATOR_FUNCTION_DECLARATION(TEST, void, uint32_t trigger_index, uint32_t timepoint_index, uint32_t* val);

// Declare all the callback-related functions
CALLBACK_GENERATOR_DECLARATION(TEST);

// Implement all the callback-realted functions and implement what should actually be executed when the callback is generated.
CALLBACK_GENERATOR_IMPLEMENTATION(TEST) {
	uint32_t trigger_index = callback_generator_TEST_get_trigger_index();
	uint32_t timepoint_index = callback_generator_TEST_get_timepoint_index();
	//printf("Called: %u, %u\n", trigger_index, timepoint_index);
	
	uint32_t val = 0;
	if(callback_generator_TEST_get_generator() != NULL) {
		callback_generator_TEST_get_generator()(trigger_index, timepoint_index, &val);
	}
	
	if(callback_generator_TEST_get_handler() != NULL) {
		callback_generator_TEST_get_handler()(val);
	}
}




void callback_generator(uint32_t trigger_index, uint32_t timepoint_index, uint32_t* val) {
	*val = trigger_index*10+timepoint_index;
}

volatile uint32_t callback_val_sum = 0;
void callback_handler(uint32_t val) {
	callback_val_sum += val;
//	printf("Callback handler called. val: %u\n", val);
}


namespace {
	
class CallbackGeneratorTest : public ::testing::Test {
	virtual void SetUp() {
		timer_init();
	}
	
	virtual void TearDown() {
		timer_stop();
	}
	
};


TEST_F(CallbackGeneratorTest, FunctionalTest) {
	callback_val_sum = 0;
	
	callback_generator_TEST_reset();
	uint8_t ret = callback_generator_TEST_init();		
	EXPECT_EQ(ret, 1);
	
	callback_generator_TEST_set_handler(callback_handler);
	callback_generator_TEST_set_generator(callback_generator);	
	
	uint32_t trigger_array[5] = {500, 500, 500, 500, 500};
	uint32_t trigger1_array[3] = {100, 100, 100};
	callback_generator_TEST_add_trigger_timepoints(trigger_array, 5);
	callback_generator_TEST_add_trigger_timepoints(trigger1_array, 3);
	
	callback_generator_TEST_trigger();
	
	// Wait until the first trigger is processed completely
	timer_sleep_milliseconds(500*5 + 50);
	EXPECT_EQ(callback_val_sum, 0+1+2+3+4);
	
	// Trigger again for the second trigger-processing
	callback_generator_TEST_trigger();
	
	// Waiting until the callback-generating process is done 
	while(!callback_generator_TEST_is_ready());
	
	EXPECT_EQ(callback_val_sum, 0+1+2+3+4+10+11+12);
	
}


TEST_F(CallbackGeneratorTest, TriggerBeforeAddTest) {
	callback_val_sum = 0;
	
	callback_generator_TEST_reset();
	uint8_t ret = callback_generator_TEST_init();		
	EXPECT_EQ(ret, 1);
	
	callback_generator_TEST_set_handler(callback_handler);
	callback_generator_TEST_set_generator(callback_generator);	
	
	callback_generator_TEST_trigger();
	
	uint32_t trigger_array[5] = {500, 500, 500, 500, 500};
	callback_generator_TEST_add_trigger_timepoints(trigger_array, 5);
	
	
	// Waiting until the callback-generating process is done 
	while(!callback_generator_TEST_is_ready());
	
	EXPECT_EQ(callback_val_sum, 0+1+2+3+4);	
	

	callback_generator_TEST_trigger();
	uint32_t trigger1_array[3] = {100, 100, 100};
	callback_generator_TEST_add_trigger_timepoints(trigger1_array, 3);
	
	// Waiting until the callback-generating process is done 
	while(!callback_generator_TEST_is_ready());
	
	EXPECT_EQ(callback_val_sum, 0+1+2+3+4+10+11+12);	
}

TEST_F(CallbackGeneratorTest, TriggerIgnoreTest) {
	callback_val_sum = 0;
	uint64_t milliseconds_start = timer_get_milliseconds_since_start();
	
	callback_generator_TEST_reset();
	uint8_t ret = callback_generator_TEST_init();		
	EXPECT_EQ(ret, 1);
	
	callback_generator_TEST_set_handler(callback_handler);
	callback_generator_TEST_set_generator(callback_generator);	
	
	callback_generator_TEST_trigger();
	
	uint32_t trigger_array[5] = {500, 500, 500, 500, 500};
	uint32_t trigger1_array[3] = {100, 100, 100};
	callback_generator_TEST_add_trigger_timepoints(trigger_array, 5);
	callback_generator_TEST_add_trigger_timepoints(trigger1_array, 3);
	
	// Trigger again, although the first trigger hasn't been processed
	callback_generator_TEST_trigger();
	
	
	
	
	// Waiting until the callback-generating process is done, it will actually never be done, because the second trigger won't be processed (that's why there is a timeout check)
	while(!callback_generator_TEST_is_ready() && (timer_get_milliseconds_since_start() - milliseconds_start <= 500*5 + 100*3 + 100));
	EXPECT_FALSE(callback_generator_TEST_is_ready());
	EXPECT_EQ(callback_val_sum, 0+1+2+3+4);	
}

};  
