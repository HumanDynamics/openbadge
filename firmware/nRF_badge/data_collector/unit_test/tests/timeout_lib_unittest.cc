

// Don't forget gtest.h, which declares the testing framework.


#include "gtest/gtest.h"
#include "timeout_lib.h"
#include "app_timer.h"
#include "systick_lib.h"


extern volatile uint8_t timeout_timer_running;

namespace {

TEST(TimeoutTest, InitTest) {
	APP_TIMER_INIT(0, 20, NULL);
	systick_init(0);
	ret_code_t ret = timeout_init();
	EXPECT_EQ(ret, NRF_SUCCESS);
}

volatile uint32_t timeout_handler_1_count = 0;
void timeout_handler_1(void) {
	timeout_handler_1_count++;
}

volatile uint32_t timeout_handler_2_count = 0;
void timeout_handler_2(void) {
	timeout_handler_2_count++;
}

TEST(TimeoutTest, RegisterOne) {
	ret_code_t ret;
	
	uint32_t timeout_id_1;
	ret = timeout_register(&timeout_id_1, timeout_handler_1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	
	EXPECT_EQ(timeout_timer_running, 0);
	
	timeout_handler_1_count = 0;
	timeout_start(timeout_id_1, 200);
	
	EXPECT_EQ(timeout_timer_running, 1);
	
	EXPECT_EQ(timeout_handler_1_count, 0);
	systick_delay_millis(180);
	EXPECT_EQ(timeout_handler_1_count, 0);
	systick_delay_millis(40);
	EXPECT_EQ(timeout_handler_1_count, 1);	
	
	EXPECT_EQ(timeout_timer_running, 0);
}


TEST(TimeoutTest, RegisterTwo) {
	ret_code_t ret;
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	uint32_t timeout_id_1, timeout_id_2;
	ret = timeout_register(&timeout_id_1, timeout_handler_1);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = timeout_register(&timeout_id_2, timeout_handler_2);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(timeout_timer_running, 0);
	
	timeout_handler_1_count = 0;
	timeout_handler_2_count = 0;
	timeout_start(timeout_id_1, 200);
	EXPECT_EQ(timeout_timer_running, 1);
	EXPECT_EQ(timeout_handler_1_count, 0);
	EXPECT_EQ(timeout_handler_2_count, 0);
	systick_delay_millis(100);
	EXPECT_EQ(timeout_handler_1_count, 0);
	EXPECT_EQ(timeout_handler_2_count, 0);
	
	timeout_start(timeout_id_2, 50);
	
	EXPECT_EQ(timeout_handler_1_count, 0);
	EXPECT_EQ(timeout_handler_2_count, 0);
	
	systick_delay_millis(40);
	
	EXPECT_EQ(timeout_handler_1_count, 0);
	EXPECT_EQ(timeout_handler_2_count, 0);
	
	systick_delay_millis(20);
	
	EXPECT_EQ(timeout_handler_1_count, 0);
	EXPECT_EQ(timeout_handler_2_count, 1);
	
	systick_delay_millis(50);
	EXPECT_EQ(timeout_handler_1_count, 1);	
	EXPECT_EQ(timeout_handler_2_count, 1);

	EXPECT_EQ(timeout_timer_running, 0);	
}

TEST(TimeoutTest, ResetTwo) {
	ret_code_t ret;
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	uint32_t timeout_id_1, timeout_id_2;
	ret = timeout_register(&timeout_id_1, timeout_handler_1);
	EXPECT_EQ(ret, NRF_SUCCESS);

	ret = timeout_register(&timeout_id_2, timeout_handler_2);
	EXPECT_EQ(ret, NRF_SUCCESS);

	EXPECT_EQ(timeout_timer_running, 0);
	
	timeout_handler_1_count = 0;
	timeout_handler_2_count = 0;
	
	
	
	timeout_start(timeout_id_1, 200);

	
	EXPECT_EQ(timeout_timer_running, 1);
	EXPECT_EQ(timeout_handler_1_count, 0);
	EXPECT_EQ(timeout_handler_2_count, 0);
	systick_delay_millis(100);
	EXPECT_EQ(timeout_handler_1_count, 0);
	EXPECT_EQ(timeout_handler_2_count, 0);

	timeout_start(timeout_id_2, 50);

	
	EXPECT_EQ(timeout_handler_1_count, 0);
	EXPECT_EQ(timeout_handler_2_count, 0);
	
	systick_delay_millis(40);	// 140

	timeout_reset(timeout_id_2);
	
	EXPECT_EQ(timeout_handler_1_count, 0);
	EXPECT_EQ(timeout_handler_2_count, 0);
	
	systick_delay_millis(20);	// 160
	
	timeout_reset(timeout_id_1);
	
	EXPECT_EQ(timeout_handler_1_count, 0);
	EXPECT_EQ(timeout_handler_2_count, 0);
	
	systick_delay_millis(50);	// 210
	
	EXPECT_EQ(timeout_handler_1_count, 0);	
	EXPECT_EQ(timeout_handler_2_count, 1);
	EXPECT_EQ(timeout_timer_running, 1);
	
	systick_delay_millis(140);	// 350
	
	EXPECT_EQ(timeout_handler_1_count, 0);	
	EXPECT_EQ(timeout_handler_2_count, 1);
	
	systick_delay_millis(20);	// 370

	EXPECT_EQ(timeout_handler_1_count, 1);	
	EXPECT_EQ(timeout_handler_2_count, 1);

	EXPECT_EQ(timeout_timer_running, 0);
}


};  
