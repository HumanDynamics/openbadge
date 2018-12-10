

// Don't forget gtest.h, which declares the testing framework.

#include "timer_lib.h"
#include "gtest/gtest.h"

#define MAX_NUMBER_OF_TIMERS_TEST	100

#define TIMER_PRESCALER_TEST		1


extern volatile timer_node_t timer_nodes[];

extern uint32_t number_of_timers;



namespace {
class TimerTest : public ::testing::Test {
	virtual void SetUp() {
		timer_init();
		timer_set_start();
	}
	
	virtual void TearDown() {
		timer_stop();
	}
	
};

TEST_F(TimerTest, SleepTest) {
	
	ASSERT_EQ(MAX_NUMBER_OF_TIMERS, MAX_NUMBER_OF_TIMERS_TEST);
	ASSERT_EQ(TIMER_PRESCALER, TIMER_PRESCALER_TEST);
	
	// Just re-init the timer-module for test reasons (LCOV)
	timer_init();

	uint32_t sleep_time_ms = 100;
	
	uint64_t start_time_ms = timer_get_milliseconds_since_start();
	timer_sleep_milliseconds(sleep_time_ms);
	uint64_t end_time_ms = timer_get_milliseconds_since_start();
	
	uint32_t delta_time_ms = (uint32_t) (end_time_ms - start_time_ms);
	
	EXPECT_LE(delta_time_ms, sleep_time_ms + 50);
	EXPECT_GE(delta_time_ms, sleep_time_ms);
	
}

void* p_callback_context_1 = NULL;
void callback_function_1(void* p_context) {
	p_callback_context_1 = p_context;
}

TEST_F(TimerTest, SingleShotTimerTest) {
	
	uint32_t timer_id;
	uint8_t ret = timer_create_timer(&timer_id, TIMER_MODE_SINGLE_SHOT, callback_function_1, 5);
	EXPECT_EQ(ret, 1);
	EXPECT_EQ(number_of_timers, 1);
	
	EXPECT_EQ(timer_nodes[0].timer_id,				0);
	EXPECT_EQ(timer_nodes[0].timer_priority, 		5);
	EXPECT_EQ(timer_nodes[0].is_running, 			0);
	EXPECT_EQ(timer_nodes[0].mode, 					TIMER_MODE_SINGLE_SHOT);
	EXPECT_TRUE(timer_nodes[0].next 				== NULL);
	EXPECT_TRUE(timer_nodes[0].p_timeout_handler 	== (timer_timeout_handler_t) callback_function_1);
	EXPECT_TRUE(timer_nodes[0].p_context 			== NULL);
	
	
	uint64_t timeout_microseconds 		= 100*1000;
	
	uint64_t start_time_microseconds = timer_get_microseconds_since_start();
	
	uint8_t context_data[10];
	p_callback_context_1 = NULL;
	// Start the timer
	ret = timer_start_timer(timer_id, timeout_microseconds, context_data);
	
	EXPECT_EQ(ret, 1);
	
	EXPECT_EQ(timer_nodes[0].is_running, 			1);
	EXPECT_EQ(timer_nodes[0].microseconds_interval, timeout_microseconds);
	EXPECT_TRUE(timer_nodes[0].p_context 			== context_data);
	
	EXPECT_LE(timer_nodes[0].microseconds_at_start, start_time_microseconds + 10*1000);
	EXPECT_GE(timer_nodes[0].microseconds_at_start, start_time_microseconds);
	EXPECT_EQ(timer_nodes[0].microseconds_at_end, 	timer_nodes[0].microseconds_at_start + timer_nodes[0].microseconds_interval);
	
	EXPECT_TRUE(timer_nodes[0].next 				== NULL);
	
	// The initial callback state
	EXPECT_TRUE(p_callback_context_1 == NULL);
	
	// Sleep for the a little bit smaller than the callback time, to check, whether it is still running
	timer_sleep_microseconds(timeout_microseconds - 20*1000);
	
	// Check if the callback was called succesfully
	EXPECT_TRUE(p_callback_context_1 == NULL);
	EXPECT_EQ(timer_nodes[0].is_running, 1);
	
	// Sleep again, to wake up after the callback event
	timer_sleep_microseconds(40*1000);
	
	// Check if the callback was called succesfully
	EXPECT_TRUE(p_callback_context_1 == context_data);
	EXPECT_EQ(timer_nodes[0].is_running, 0);
	
}



TEST_F(TimerTest, RepeatedTimerTest) {
	
	uint32_t timer_id;
	uint8_t ret = timer_create_timer(&timer_id, TIMER_MODE_REPEATED, callback_function_1, 5);
	EXPECT_EQ(ret, 1);
	EXPECT_EQ(number_of_timers, 1);
	
	EXPECT_EQ(timer_nodes[0].timer_id,				0);
	EXPECT_EQ(timer_nodes[0].timer_priority, 		5);
	EXPECT_EQ(timer_nodes[0].is_running, 			0);
	EXPECT_EQ(timer_nodes[0].mode, 					TIMER_MODE_REPEATED);
	EXPECT_TRUE(timer_nodes[0].next 				== NULL);
	EXPECT_TRUE(timer_nodes[0].p_timeout_handler 	== (timer_timeout_handler_t) callback_function_1);
	EXPECT_TRUE(timer_nodes[0].p_context 			== NULL);
	
	
	uint64_t timeout_microseconds 		= 100*1000;
	
	uint64_t start_time_microseconds = timer_get_microseconds_since_start();
	
	uint8_t context_data[10];
	p_callback_context_1 = NULL;
	
	// Start the timer
	ret = timer_start_timer(timer_id, timeout_microseconds, context_data);
	
	EXPECT_EQ(ret, 1);
	
	EXPECT_EQ(timer_nodes[0].is_running, 			1);
	EXPECT_EQ(timer_nodes[0].microseconds_interval, timeout_microseconds);
	EXPECT_TRUE(timer_nodes[0].p_context 			== context_data);
	
	EXPECT_LE(timer_nodes[0].microseconds_at_start, start_time_microseconds + 10*1000);
	EXPECT_GE(timer_nodes[0].microseconds_at_start, start_time_microseconds);
	EXPECT_EQ(timer_nodes[0].microseconds_at_end, 	timer_nodes[0].microseconds_at_start + timer_nodes[0].microseconds_interval);
	
	EXPECT_TRUE(timer_nodes[0].next 				== NULL);
	
	// The initial callback state
	EXPECT_TRUE(p_callback_context_1 == NULL);
	
	// Sleep for the a little bit smaller than the callback time, to check, whether it is still running
	timer_sleep_microseconds(timeout_microseconds - 20*1000);
	
	// Check if the callback was called succesfully
	EXPECT_TRUE(p_callback_context_1 == NULL);
	EXPECT_EQ(timer_nodes[0].is_running, 1);
	
	// Sleep again, to wake up after the callback event
	timer_sleep_microseconds(40*1000);
	
	// Check if the callback was called succesfully
	EXPECT_TRUE(p_callback_context_1 == context_data);
	EXPECT_EQ(timer_nodes[0].is_running, 1);
	
	// Because it is repeated, we should now be able to get another timer event after another wait time
	p_callback_context_1 = NULL;
	timer_sleep_microseconds(timeout_microseconds);
	
	// Check if the callback was called succesfully
	EXPECT_TRUE(p_callback_context_1 == context_data);
	EXPECT_EQ(timer_nodes[0].is_running, 1);	
}


volatile uint32_t callback_function_2_counter = 0;
void callback_function_2(void* p_context) {
	callback_function_2_counter++;
}

volatile uint32_t callback_function_3_counter = 0;
void callback_function_3(void* p_context) {
	callback_function_3_counter++;
}

TEST_F(TimerTest, TwoTimerTest) {
	
	uint32_t timer_id_2, timer_id_3;
	uint8_t ret = timer_create_timer(&timer_id_2, TIMER_MODE_SINGLE_SHOT, callback_function_2, 5);
	EXPECT_EQ(ret, 1);
	EXPECT_EQ(number_of_timers, 1);
	
	ret = timer_create_timer(&timer_id_3, TIMER_MODE_REPEATED, callback_function_3, 4);
	EXPECT_EQ(ret, 1);
	EXPECT_EQ(number_of_timers, 2);
	

	
	
	uint64_t timeout_2_microseconds 		= 100*1000;
	uint64_t timeout_3_microseconds 		= 100*1000;
	
	callback_function_2_counter = 0;
	callback_function_3_counter = 0;
	
	
	// Start the timer_2 and _3
	ret = timer_start_timer(timer_id_2, timeout_2_microseconds, NULL);
	EXPECT_EQ(ret, 1);
	ret = timer_start_timer(timer_id_3, timeout_3_microseconds, NULL);
	EXPECT_EQ(ret, 1);
	
	EXPECT_TRUE(timer_nodes[0].next 				== &timer_nodes[1]);
	EXPECT_TRUE(timer_nodes[1].next 				== NULL);
	
	// Sleep until shortly before timer expires the first time
	timer_sleep_microseconds(80*1000);
	
	EXPECT_EQ(timer_nodes[0].is_running, 			1);
	EXPECT_EQ(timer_nodes[1].is_running, 			1);
	
	// Sleep until shortly after timer expires the first time
	timer_sleep_microseconds(40*1000);
	
	// The first timer shouldn't run anymore, the second (REPEATED) should run
	EXPECT_EQ(timer_nodes[0].is_running, 			0);
	EXPECT_EQ(timer_nodes[1].is_running, 			1);
	
	EXPECT_EQ(callback_function_2_counter, 1);
	EXPECT_EQ(callback_function_3_counter, 1);
	
	
	timer_sleep_microseconds(5*timeout_3_microseconds);
	
	// Check if the number of callbacks is correct
	EXPECT_EQ(callback_function_2_counter, 1);
	EXPECT_EQ(callback_function_3_counter, 1 + 5);
	
}



volatile uint32_t callback_function_4_counter = 0;
void callback_function_4(void* p_context) {
	callback_function_4_counter++;
}

uint32_t next = 1;
uint32_t rand(void) {
    return ( ( next = next * 1103515245L + 12345L ) % 2147483647L );
}

TEST_F(TimerTest, MultipleSingleShotTimerTest) {
	
	uint32_t timer_id;
	uint8_t ret;
	callback_function_4_counter = 0;
	
	for(uint8_t i = 0; i < MAX_NUMBER_OF_TIMERS_TEST; i++) {
		ret = timer_create_timer(&timer_id, TIMER_MODE_SINGLE_SHOT, callback_function_4, i);
		EXPECT_EQ(ret, 1);
		ret = timer_start_timer(timer_id, (rand() % 100 + 1)*1000, NULL);
		EXPECT_EQ(ret, 1);
	}
	
	// Sleep again, to wake up after the callback events
	timer_sleep_microseconds(120*1000);
	
	EXPECT_EQ(callback_function_4_counter, 100);
	EXPECT_EQ(number_of_timers, 100);
	// Create again a timer --> number_of_timers is too big
	ret = timer_create_timer(&timer_id, TIMER_MODE_SINGLE_SHOT, callback_function_4, 0);
	EXPECT_EQ(ret, 0);
	
	// Start again timer 0
	ret = timer_start_timer(0, 10*1000, NULL);
	EXPECT_EQ(ret, 1);
	ret = timer_start_timer(0, 10*1000, NULL);
	EXPECT_EQ(ret, 0);
	
	// Sleep again, to wake up after the callback events
	timer_sleep_microseconds(20*1000);
	
	EXPECT_EQ(callback_function_4_counter, 101);
	
	
}

TEST_F(TimerTest, StopTimerTest) {
	
	uint32_t timer_id;
	uint8_t ret;
	callback_function_4_counter = 0;
	for(uint8_t i = 0; i < MAX_NUMBER_OF_TIMERS_TEST; i++) {
		ret = timer_create_timer(&timer_id, TIMER_MODE_SINGLE_SHOT, callback_function_4, i);
		EXPECT_EQ(ret, 1);
		ret = timer_start_timer(timer_id, (i+10)*1000, NULL);
		EXPECT_EQ(ret, 1);
	}
	
	// Stop a timer somewhere in between and start it again with a higher time
	timer_stop_timer(10);
	ret = timer_start_timer(10, (100)*1000, NULL);
	EXPECT_EQ(ret, 1);
	
	
	
	// Sleep again, to wake up after the callback events
	timer_sleep_microseconds((100 + 20) *1000);
	
	EXPECT_EQ(callback_function_4_counter, 100);
	EXPECT_EQ(number_of_timers, 100);

	
}

TEST_F(TimerTest, MultipleRepeatedTimerTest) {
	
	uint32_t timer_id;
	uint8_t ret;
	callback_function_4_counter = 0;
	for(uint8_t i = 0; i < MAX_NUMBER_OF_TIMERS_TEST; i++) {
		ret = timer_create_timer(&timer_id, TIMER_MODE_REPEATED, callback_function_4, i);
		EXPECT_EQ(ret, 1);
		ret = timer_start_timer(timer_id, 100*1000, NULL);
		EXPECT_EQ(ret, 1);
	}
	
	// Sleep again, to wake up after the callback events
	timer_sleep_microseconds((5*100 + 20) *1000);
	
	EXPECT_EQ(callback_function_4_counter, 500);
	EXPECT_EQ(number_of_timers, 100);

	for(uint8_t i = 0; i < MAX_NUMBER_OF_TIMERS_TEST; i++) {
		timer_stop_timer(i);
	}
	
	// Sleep again, to wake up after the callback events
	timer_sleep_microseconds(100 *1000);
	EXPECT_EQ(callback_function_4_counter, 500);
	
	
	// Start again timer 0
	ret = timer_start_timer(0, 100*1000, NULL);
	EXPECT_EQ(ret, 1);
		
	// Sleep again, to wake up after the callback events
	timer_sleep_microseconds(120*1000);
	
	EXPECT_EQ(callback_function_4_counter, 501);
	
}




uint32_t callback_function_5_counter = 0;
uint32_t timer_id;
uint32_t timeout_microseconds;
void callback_function_5(void* p_context) {
	callback_function_5_counter ++;	
	// Restart the timer withih the Callback
	uint8_t ret = timer_start_timer(timer_id, timeout_microseconds, NULL);
	EXPECT_EQ(ret, 1);
}

TEST_F(TimerTest, CallbackRestartTest) {
	
	uint8_t ret;
	callback_function_5_counter = 0;
	timeout_microseconds = 100*1000;
	
	ret = timer_create_timer(&timer_id, TIMER_MODE_SINGLE_SHOT, callback_function_5, 0);
	EXPECT_EQ(ret, 1);
	
	ret = timer_start_timer(timer_id, timeout_microseconds, NULL);
	EXPECT_EQ(ret, 1);
	
	// Sleep to wake up after the callback events
	timer_sleep_microseconds((5*100 + 20)*1000);
	EXPECT_EQ(callback_function_5_counter, 5);	
}
};  
