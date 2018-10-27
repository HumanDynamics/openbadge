

// Don't forget gtest.h, which declares the testing framework.


#include "gtest/gtest.h"
#include "ble_lib.h"

#include "callback_generator_lib.h"
#include "data_generator_lib.h"

/** Include some (private) functions only for testing purposes */
extern uint32_t ble_transmit_fifo_get_size(void);
extern ret_code_t ble_transmit_fifo_read(uint8_t* data, uint32_t len);

/** Some variables and (callback) functions to test functionallity) */
static volatile uint8_t connected = 0;
static volatile uint8_t transmitted = 0;
static volatile uint8_t receive_count = 0;
static volatile uint8_t scan_report_count = 0;
static volatile int8_t scan_report_rssi[100];
static volatile uint8_t scan_timeout = 0;

static void get_MAC_address_generator(uint8_t* MAC_address, uint8_t len) {
	for(uint8_t i = 0; i < len; i++)
		MAC_address[i] = i;
	
	EXPECT_EQ(len, 6);
}

static void on_connect(void) {
	connected = 1;
}
static void on_disconnect(void) {
	connected = 0;
}

static void on_transmit_complete(void) {
	transmitted = 1;
}

static void on_receive(uint8_t* data, uint16_t len) {
	
	EXPECT_EQ(callback_generator_ble_on_receive_get_trigger_index(), 0);
	EXPECT_EQ(callback_generator_ble_on_receive_get_timepoint_index(), receive_count);

	EXPECT_EQ(len, 20);
	
	for(uint32_t i = 0; i < len; i++)
		EXPECT_EQ(data[i], (i*i + receive_count) % 256);
	
	receive_count++;
}
static void on_receive_generator(uint8_t* data, uint16_t* length, uint16_t max_len) {
	*length = max_len;
	for(uint32_t i = 0; i < *length; i++)
		data[i] =  (i*i + receive_count) % 256;
}


static void on_scan_report(ble_gap_evt_adv_report_t* scan_report) {
	EXPECT_EQ(callback_generator_ble_on_scan_report_get_trigger_index(), 0);
	EXPECT_EQ(callback_generator_ble_on_scan_report_get_timepoint_index(), scan_report_count);
	
	scan_report_rssi[scan_report_count] = scan_report->rssi;
	scan_report_count ++;
}
static void on_scan_report_generator(ble_gap_evt_adv_report_t* scan_report) {
	scan_report->rssi = -50 - ((int8_t) scan_report_count);
}
static void on_scan_timeout(void) {
	scan_timeout = 1;
}






namespace {

class BLELibMockTest : public ::testing::Test {
	virtual void SetUp() {
		callback_generator_ble_on_connect_reset();
		callback_generator_ble_on_disconnect_reset();
		callback_generator_ble_on_receive_reset();
		callback_generator_ble_on_transmit_complete_reset();
		callback_generator_ble_on_scan_report_reset();
		callback_generator_ble_on_scan_timeout_reset();
		data_generator_ble_get_MAC_address_reset();
	}
};

TEST_F(BLELibMockTest, GetMACAddressTest) {
	data_generator_ble_get_MAC_address_set_generator(get_MAC_address_generator);
	
	uint8_t MAC_address[6];
	ble_get_MAC_address(MAC_address);	
	for(uint8_t i = 0; i < 6; i++)
		EXPECT_EQ(MAC_address[i], i);	
}

TEST_F(BLELibMockTest, ConnectDisconnectTest) {
	uint32_t timepoint_connect = 10;
	uint32_t timepoint_disconnect = 120;
	callback_generator_ble_on_connect_add_trigger_timepoints(&timepoint_connect, 1);
	callback_generator_ble_on_disconnect_add_trigger_timepoints(&timepoint_disconnect, 1);
	
	connected = 0;
	
	ret_code_t ret = ble_init();
	ble_set_on_connect_callback(on_connect);
	ble_set_on_disconnect_callback(on_disconnect);
	
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(connected, 0);
	
	while(	!callback_generator_ble_on_connect_is_ready() );
	
	EXPECT_EQ(connected, 1);
	
	while(	!callback_generator_ble_on_disconnect_is_ready() );
	
	EXPECT_EQ(connected, 0);
	
}


TEST_F(BLELibMockTest, TransmitTest) {
	uint32_t timepoint_connect = 10;
	callback_generator_ble_on_connect_add_trigger_timepoints(&timepoint_connect, 1);
	
	
	connected = 0;
	transmitted = 0;
	uint8_t data[100];
	
	ret_code_t ret = ble_init();
	ble_set_on_connect_callback(on_connect);
	ble_set_on_transmit_callback(on_transmit_complete);
	
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(connected, 0);
	
	// If we try to sent data before it is connected --> Invalid state
	ret = ble_transmit(data, 10);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_STATE);
	
	
	while(	!callback_generator_ble_on_connect_is_ready() );
	
	EXPECT_EQ(connected, 1);

	// If we try to sent too large data --> Invalid param
	ret = ble_transmit(data, 30);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	ret = ble_transmit(data, 20);
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(transmitted, 0);
	
	
	while(	!callback_generator_ble_on_transmit_complete_is_ready() );
	
	EXPECT_EQ(transmitted, 1);
	EXPECT_EQ(ble_transmit_fifo_get_size(), 20);
	
}

TEST_F(BLELibMockTest, ReceiveTest) {
	uint32_t timepoint_connect = 10;
	uint32_t timepoints_receive[2] = {100, 100};
	callback_generator_ble_on_connect_add_trigger_timepoints(&timepoint_connect, 1);
	callback_generator_ble_on_receive_add_trigger_timepoints(timepoints_receive, 2);
	callback_generator_ble_on_receive_set_generator(on_receive_generator);
	
	
	connected = 0;
	receive_count = 0;
	
	ret_code_t ret = ble_init();
	ble_set_on_connect_callback(on_connect);
	ble_set_on_receive_callback(on_receive);
	
	EXPECT_EQ(ret, NRF_SUCCESS);
	EXPECT_EQ(connected, 0);
	EXPECT_EQ(receive_count, 0);
	
		
	while(	!callback_generator_ble_on_connect_is_ready() );
	
	EXPECT_EQ(connected, 1);
	EXPECT_EQ(receive_count, 0);

	
	while(	!callback_generator_ble_on_receive_is_ready() );
	
	EXPECT_EQ(receive_count, 2);
	

}

TEST_F(BLELibMockTest, ScanTest) {
	uint8_t number_of_scan_reports = 20;
	uint32_t timepoints_scan_report[number_of_scan_reports];
	for(uint8_t i = 0; i < number_of_scan_reports; i++)
		timepoints_scan_report[i] = 25;	
	callback_generator_ble_on_scan_report_add_trigger_timepoints(timepoints_scan_report, number_of_scan_reports);
	callback_generator_ble_on_scan_report_set_generator(on_scan_report_generator);
	
	scan_timeout = 0;
	memset((int8_t*) scan_report_rssi, 0, sizeof(scan_report_rssi));
	scan_report_count = 0;
		
	
	ret_code_t ret = ble_init();
	ble_set_on_scan_report_callback(on_scan_report);
	ble_set_on_scan_timeout_callback(on_scan_timeout);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = ble_start_scanning(0, 200, 2);
	EXPECT_EQ(ret, NRF_ERROR_INVALID_PARAM);
	
	ret = ble_start_scanning(100, 200, 2);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	EXPECT_EQ(scan_timeout, 0);
	EXPECT_EQ(scan_report_count, 0);
	
		
	while(	!callback_generator_ble_on_scan_report_is_ready() || !callback_generator_ble_on_scan_timeout_is_ready() );
	
	EXPECT_EQ(scan_timeout, 1);
	EXPECT_EQ(scan_report_count, number_of_scan_reports);
	for(uint8_t i = 0; i < number_of_scan_reports; i++) {
		EXPECT_EQ(scan_report_rssi[i], -50 - (int8_t) i);
	}

	
}



};  
