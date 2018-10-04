

// Don't forget gtest.h, which declares the testing framework.
#include <stdio.h>
#include <stdlib.h>

#include "gtest/gtest.h"
#include "app_timer.h"
#include "systick_lib.h"
#include "timeout_lib.h"
#include "ble_lib.h"
#include "advertiser_lib.h"
#include "request_handler_lib_02v1.h"
#include "storer_lib.h"
#include "sampling_lib.h"
#include "app_scheduler.h"
#include "debug_lib.h"
#include "chunk_messages.h"
#include "processing_lib.h"

#include "callback_generator_lib.h"
#include "data_generator_lib.h"


#define SCAN_GROUP_ID	10
#define NUMBER_OF_SCANS	5


static uint32_t number_generated_beacons = 0; 


static void on_scan_report_generator(ble_gap_evt_adv_report_t* scan_report) {
	uint16_t id = (uint16_t) (rand() % 20000);
	/*
	while(id < SCAN_BEACON_ID_THRESHOLD && number_generated_beacons < SCAN_PRIORITIZED_BEACONS) {
		id = (uint16_t) (rand() % 32768);
	}*/
	
	if(id >= SCAN_PRIORITIZED_BEACONS) {
		number_generated_beacons++;
	}
	
	scan_report->rssi = (-1)*((int8_t) (rand() % 120));
	
	// Advertising data (got from sender_lib.c, print out received adv packet data)
	uint8_t data[29] = {0x02, 0x01, 0x06, 0x03, 0x03,
						0x01, 0x00, 0x0E, 0xFF, 0x00,
						0xFF, 0x00, 0x00, 0x64, 0x00,
						0x09, 0xD1, 0x1F, 0xFD, 0xCE,
						0x32, 0xB3, 0x06, 0x09,
						0x48, 0x44, 0x42, 0x44, 0x47};
						
	

	data[13] = (uint8_t)((id) & 0xFF);
	data[14] = (uint8_t)((id >> 8) & 0xFF);
	data[15] = SCAN_GROUP_ID;
	
	
	scan_report->dlen = 29;
	memcpy((uint8_t*) scan_report->data, data, scan_report->dlen);
}




static void check_scan_chunk(ScanChunk* scan_chunk) {
	debug_log("Scan Chunk size %u\n", scan_chunk->scan_result_data_count);
	EXPECT_LE(scan_chunk->scan_result_data_count, SCAN_CHUNK_DATA_SIZE);
	
	
	for(uint32_t i = 0; i < scan_chunk->scan_result_data_count; i++) {
		debug_log("Scan [%u]: %u, %d\n", i, scan_chunk->scan_result_data[i].scan_device.ID, scan_chunk->scan_result_data[i].scan_device.rssi);
	}
	
	
	// Check if the first SCAN_PRIORITIZED_BEACONS are beacons and the RSSI values are getting smaller
	for(uint8_t i = 0; i < SCAN_PRIORITIZED_BEACONS; i++) {
		EXPECT_GE(scan_chunk->scan_result_data[i].scan_device.ID, SCAN_PRIORITIZED_BEACONS);
		if(i > 0) {
			EXPECT_LE(scan_chunk->scan_result_data[i].scan_device.rssi, scan_chunk->scan_result_data[i-1].scan_device.rssi);
		}
	}
	
	for(uint8_t i = SCAN_PRIORITIZED_BEACONS; i < scan_chunk->scan_result_data_count; i++) {
		if(i > SCAN_PRIORITIZED_BEACONS) {
			EXPECT_LE(scan_chunk->scan_result_data[i].scan_device.rssi, scan_chunk->scan_result_data[i-1].scan_device.rssi);
		}
	}
	
	
	
	
}



namespace {

class ScanIntegrationTest : public ::testing::Test {
	virtual void SetUp() {
		
		time_t t;
		srand((unsigned) time(&t));
		
		callback_generator_ble_on_scan_report_reset();
		callback_generator_ble_on_scan_timeout_reset();
	}
};



TEST_F(ScanIntegrationTest, Test) {
	ret_code_t ret;
	
	APP_SCHED_INIT(4, 100);
	APP_TIMER_INIT(0, 60, NULL);
	
	debug_init();
	
	ret = systick_init(0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = timeout_init();
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = ble_init();
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = sampling_init();
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = storer_init();
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	advertiser_init();
	
	ret = advertiser_start_advertising();
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	ret = request_handler_init();
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	//ret = sampling_start_microphone(0, 50, 0);
	//EXPECT_EQ(ret, NRF_SUCCESS);
	number_generated_beacons = 0; 
	
	uint16_t number_of_scan_reports = 300;
	uint32_t timepoints_scan_report[number_of_scan_reports];
	for(uint16_t i = 0; i < number_of_scan_reports; i++)
		timepoints_scan_report[i] = 5;	
	
	for(uint8_t i = 0; i < NUMBER_OF_SCANS; i++)
		callback_generator_ble_on_scan_report_add_trigger_timepoints(timepoints_scan_report, number_of_scan_reports);
	
	callback_generator_ble_on_scan_report_set_generator(on_scan_report_generator);

	
	ret = sampling_start_scan(0, 15, 300, 100, 14, SCAN_GROUP_ID, 0, 0);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	
	while(!callback_generator_ble_on_scan_report_is_ready() || !callback_generator_ble_on_scan_timeout_is_ready()) {
		app_sched_execute();
	}
	// Give some more time for the storing...
	uint64_t end_ms = systick_get_continuous_millis() + (1*1000);
	while(systick_get_continuous_millis() < end_ms) {
		app_sched_execute();
	}
	
	sampling_stop_scan(0);
	
	
	// Now read out the scan chunks...
	uint32_t timestamp_seconds = 0;
	uint16_t timestamp_ms = 0;
	systick_get_timestamp(&timestamp_seconds, &timestamp_ms);
	
	ScanChunk scan_chunk;
	Timestamp timestamp;
	timestamp.seconds = 0;
	timestamp.ms = 0;
	
	ret = storer_find_scan_chunk_from_timestamp(timestamp, &scan_chunk);
	EXPECT_EQ(ret, NRF_SUCCESS);
	
	// Check if we generated at least SCAN_PRIORITIZED_BEACONS beacons. If not (unlikely, but could happen) --> restart/retry
	ASSERT_GE(number_generated_beacons, SCAN_PRIORITIZED_BEACONS);
	
	
	uint8_t number_of_stored_scan_chunks = 0;
	do {
		ret = storer_get_next_scan_chunk(&scan_chunk);
		if(ret != NRF_SUCCESS) break;
		number_of_stored_scan_chunks++;
		check_scan_chunk(&scan_chunk);
	} while(ret == NRF_SUCCESS);
	EXPECT_EQ(number_of_stored_scan_chunks, NUMBER_OF_SCANS);
	
}


};  
