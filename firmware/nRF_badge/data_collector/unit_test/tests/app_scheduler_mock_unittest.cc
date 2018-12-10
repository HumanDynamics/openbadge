

// Don't forget gtest.h, which declares the testing framework.

#include "app_scheduler.h"
#include "app_scheduler.h"
#include "gtest/gtest.h"

#define SCHED_MAX_EVENT_DATA_SIZE sizeof(uint32_t)
#define SCHED_QUEUE_SIZE 100


namespace {
	
class AppSchedulerTest : public ::testing::Test {
	virtual void SetUp() {
		APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);	
	}
};


uint32_t callback_event_size_sum = 0;
uint32_t callback_event_data_sum = 0;
void callback_function(void * p_event_data, uint16_t event_size) {
	callback_event_size_sum += event_size;
	for(uint8_t i = 0; i < event_size; i++)
		callback_event_data_sum += ((uint8_t*)p_event_data)[i];
}

TEST_F(AppSchedulerTest, FunctionalTest) {
	callback_event_size_sum = 0;
	callback_event_data_sum = 0;
	uint8_t event_data[10];
	for(uint8_t i = 0; i < 10; i++)
		event_data[i] = i;
	
	app_sched_event_put(event_data, 1, callback_function);
	
	EXPECT_EQ(callback_event_size_sum, 0);
	EXPECT_EQ(callback_event_data_sum, 0);
	
	app_sched_execute();	
	EXPECT_EQ(callback_event_size_sum, 1);
	EXPECT_EQ(callback_event_data_sum, 0);
	
	app_sched_event_put(event_data, 2, callback_function);
	
	app_sched_execute();	
	EXPECT_EQ(callback_event_size_sum, 3);
	EXPECT_EQ(callback_event_data_sum, 1);
	
	
	callback_event_size_sum = 0;
	callback_event_data_sum = 0;
	
	app_sched_event_put(event_data, 1, callback_function);
	app_sched_event_put(event_data, 2, callback_function);
	app_sched_event_put(event_data, 3, callback_function);
	
	uint32_t space = app_sched_queue_space_get();
	EXPECT_EQ(space, SCHED_QUEUE_SIZE - 3);
	
	EXPECT_EQ(callback_event_size_sum, 0);
	EXPECT_EQ(callback_event_data_sum, 0);
	app_sched_execute();	
	EXPECT_EQ(callback_event_size_sum, 1+2+3);
	EXPECT_EQ(callback_event_data_sum, 0 + (0+1) + (0+1+2));
	
	space = app_sched_queue_space_get();
	EXPECT_EQ(space, SCHED_QUEUE_SIZE);
	
}

TEST_F(AppSchedulerTest, ExceptionTest) {
	uint8_t event_data[10];
	ret_code_t ret = app_sched_event_put(event_data, 10, callback_function);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_LENGTH);
	
	ret = app_sched_event_put(event_data, 0, callback_function);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = app_sched_event_put(NULL, 1, callback_function);
	EXPECT_EQ(ret, NRF_SUCCESS);
		
	for(uint8_t i = 0; i < SCHED_QUEUE_SIZE - 2; i++) {
		ret = app_sched_event_put(event_data, 1, callback_function);
		EXPECT_EQ(ret, NRF_SUCCESS);
	}
	
	ret = app_sched_event_put(event_data, 1, callback_function);
	EXPECT_EQ(ret, NRF_ERROR_NO_MEM);
}


};  
