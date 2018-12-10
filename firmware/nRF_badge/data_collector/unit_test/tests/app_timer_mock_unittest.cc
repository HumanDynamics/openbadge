

// Don't forget gtest.h, which declares the testing framework.

#include "app_timer.h"
#include "timer_lib.h"			
#include "gtest/gtest.h"


#define MAX_NUMBER_OF_APP_TIMERS_TEST	MAX_NUMBER_OF_TIMERS

extern volatile timer_node_t timer_nodes[];

extern uint32_t timer_ids[];

extern uint32_t number_of_app_timers;

namespace {
	
class AppTimerTest : public ::testing::Test {
	virtual void SetUp() {
		APP_TIMER_INIT(0, 20, NULL);
		timer_set_start();
		
	}
	
	virtual void TearDown() {
		timer_stop();
	}
	
};

void* p_callback_context_1 = NULL;
void callback_function_1(void* p_context) {
	p_callback_context_1 = p_context;
}

TEST_F(AppTimerTest, SingleShotTimerTest) {
	APP_TIMER_DEF(timer_id);
	ret_code_t ret = app_timer_create(&timer_id, APP_TIMER_MODE_SINGLE_SHOT, callback_function_1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(number_of_app_timers, 1);
	EXPECT_EQ(timer_ids[0], 0);
	
	uint8_t context_data[10];
	p_callback_context_1 = NULL;
	
	uint32_t timeout_ms	= 100;
	uint32_t timeout_ticks = APP_TIMER_TICKS(timeout_ms, 0);
	ret = app_timer_start(timer_id, timeout_ticks, context_data);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	// Check the entries in the timer_lib
	EXPECT_EQ(timer_nodes[0].is_running, 			1);
	EXPECT_EQ(timer_nodes[0].microseconds_interval, timeout_ms*1000);
	EXPECT_TRUE(timer_nodes[0].p_context 			== context_data);
	
	
	// The initial callback state
	EXPECT_TRUE(p_callback_context_1 == NULL);
	
	
	// Sleep for the a little bit smaller than the callback time, to check, whether it is still running
	timer_sleep_milliseconds(timeout_ms - 20);
	
	// Check if the callback was called succesfully
	EXPECT_TRUE(p_callback_context_1 == NULL);
	EXPECT_EQ(timer_nodes[0].is_running, 1);
	
	// Sleep again, to wake up after the callback event
	timer_sleep_milliseconds(20);
	
	// Check if the callback was called succesfully
	EXPECT_TRUE(p_callback_context_1 == context_data);
	EXPECT_EQ(timer_nodes[0].is_running, 0);

}


TEST_F(AppTimerTest, RepeatedTimerTest) {
	APP_TIMER_DEF(timer_id);
	ret_code_t ret = app_timer_create(&timer_id, APP_TIMER_MODE_REPEATED, callback_function_1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(number_of_app_timers, 1);
	EXPECT_EQ(timer_ids[0], 0);
	
	uint8_t context_data[10];
	p_callback_context_1 = NULL;
	
	uint32_t timeout_ms	= 100;
	uint32_t timeout_ticks = APP_TIMER_TICKS(timeout_ms, 0);
	ret = app_timer_start(timer_id, timeout_ticks, context_data);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	// Check the entries in the timer_lib
	EXPECT_EQ(timer_nodes[0].is_running, 			1);
	EXPECT_EQ(timer_nodes[0].microseconds_interval, timeout_ms*1000);
	EXPECT_TRUE(timer_nodes[0].p_context 			== context_data);
	
	
	// The initial callback state
	EXPECT_TRUE(p_callback_context_1 == NULL);
	
	
	// Sleep for the a little bit smaller than the callback time, to check, whether it is still running
	timer_sleep_milliseconds(timeout_ms - 20);
	
	// Check if the callback was called succesfully
	EXPECT_TRUE(p_callback_context_1 == NULL);
	EXPECT_EQ(timer_nodes[0].is_running, 1);
	
	// Sleep again, to wake up after the callback event
	timer_sleep_milliseconds(20);
	
	// Check if the callback was called succesfully
	EXPECT_TRUE(p_callback_context_1 == context_data);
	EXPECT_EQ(timer_nodes[0].is_running, 1);
	
	
}


volatile uint32_t callback_function_2_counter = 0;
void callback_function_2(void* p_context) {
	callback_function_2_counter++;
}



TEST_F(AppTimerTest, StopTimerTest) {
	callback_function_2_counter = 0;
	
	APP_TIMER_DEF(timer_id_0);
	ret_code_t ret = app_timer_create(&timer_id_0, APP_TIMER_MODE_REPEATED, callback_function_2);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	APP_TIMER_DEF(timer_id_1);
	ret = app_timer_create(&timer_id_1, APP_TIMER_MODE_REPEATED, callback_function_2);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(number_of_app_timers, 2);
	EXPECT_EQ(timer_ids[0], 0);
	EXPECT_EQ(timer_ids[1], 1);
	
	
	uint32_t timeout_ms	= 100;
	uint32_t timeout_ticks = APP_TIMER_TICKS(timeout_ms, 0);
	ret = app_timer_start(timer_id_0, timeout_ticks, NULL);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = app_timer_start(timer_id_1, timeout_ticks, NULL);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	
	// Sleep for the a little bit smaller than the callback time, to check, whether it is still running
	timer_sleep_milliseconds(timeout_ms - 20);
	
	// Check if the callback was called succesfully
	EXPECT_EQ(callback_function_2_counter, 0);
	EXPECT_EQ(timer_nodes[0].is_running, 1);
	EXPECT_EQ(timer_nodes[1].is_running, 1);
	
	// Sleep again, to wake up after the callback event
	timer_sleep_milliseconds(20);
	
	// Check if the callback was called succesfully
	EXPECT_EQ(callback_function_2_counter, 2);
	EXPECT_EQ(timer_nodes[0].is_running, 1);
	EXPECT_EQ(timer_nodes[1].is_running, 1);
	
	
	timer_sleep_milliseconds(timeout_ms);
	
	EXPECT_EQ(callback_function_2_counter, 4);
	EXPECT_EQ(timer_nodes[0].is_running, 1);
	EXPECT_EQ(timer_nodes[1].is_running, 1);
	
	// Stopping all the timers
	ret = app_timer_stop(timer_id_0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	ret = app_timer_stop_all();
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	timer_sleep_milliseconds(timeout_ms);
	
	EXPECT_EQ(callback_function_2_counter, 4);
	EXPECT_EQ(timer_nodes[0].is_running, 0);
	EXPECT_EQ(timer_nodes[1].is_running, 0);
}




TEST_F(AppTimerTest, AppTimerCntDiffComputeTest) {
	uint32_t max_ticks = 0x00FFFFFF; // Max rtc ticks
	uint32_t ticks_to = 0;
	uint32_t ticks_from = 0;
	uint32_t ticks_diff = 0;
	
	ret_code_t ret = app_timer_cnt_diff_compute(ticks_to, ticks_from, &ticks_diff);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(ticks_diff, 0);
	
	ticks_to = 10;
	ticks_from = max_ticks - 10;
	ret = app_timer_cnt_diff_compute(ticks_to, ticks_from, &ticks_diff);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(ticks_diff, 21);	
}


TEST_F(AppTimerTest, AppTimerCntGetTest) {
	
	uint32_t ticks_from = app_timer_cnt_get();
	
	uint32_t timeout_ms	= 100;
	uint32_t timeout_ticks = APP_TIMER_TICKS(timeout_ms, 0);
	
	
	timer_sleep_milliseconds(timeout_ms);
	
	uint32_t ticks_to = app_timer_cnt_get();
	
	
	
	
	uint32_t ticks_diff = 0;
	
	
	ret_code_t ret = app_timer_cnt_diff_compute(ticks_to, ticks_from, &ticks_diff);
	
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_GE(ticks_diff, timeout_ticks - 700);
	EXPECT_LE(ticks_diff, timeout_ticks + 700);
	
	
	
	
}
};  
