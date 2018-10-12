#include "processing_lib.h"
#include <stdlib.h>		// For qsort
#include "app_scheduler.h"

#include "sampling_lib.h"
#include "chunk_fifo_lib.h"
#include "storer_lib.h"
#include "systick_lib.h"

#include "chunk_messages.h"

#include "debug_lib.h"


static ScanChunk scan_chunk;	/**< A Scan-chunk structure, needed to convert from ScanSamplingChunk to ScanChunk */

/************************** ACCELEROMETER ***********************/
void processing_process_accelerometer_chunk(void * p_event_data, uint16_t event_size) {
	debug_log("processing_process_accelerometer_chunk...\n");
	AccelerometerChunk* 	accelerometer_chunk;
	while(chunk_fifo_read_open(&accelerometer_chunk_fifo, (void**) &accelerometer_chunk, NULL) == NRF_SUCCESS) {
		
		ret_code_t ret = storer_store_accelerometer_chunk(accelerometer_chunk);
		debug_log("Try to store accelerometer chunk: Ret %d\n", ret);
		if(ret == NRF_ERROR_INTERNAL) {	// E.g. if busy --> reschedule
			app_sched_event_put(NULL, 0, processing_process_accelerometer_chunk);
			break;
		} else {
			chunk_fifo_read_close(&accelerometer_chunk_fifo);	
		}
	}	
}

/********************* ACCELEROMTER INTERRUPT **********************/
void processing_process_accelerometer_interrupt_chunk(void * p_event_data, uint16_t event_size) {
	debug_log("processing_process_accelerometer_interrupt_chunk...\n");
	AccelerometerInterruptChunk* 	accelerometer_interrupt_chunk;
	while(chunk_fifo_read_open(&accelerometer_interrupt_chunk_fifo, (void**) &accelerometer_interrupt_chunk, NULL) == NRF_SUCCESS) {
		
		ret_code_t ret = storer_store_accelerometer_interrupt_chunk(accelerometer_interrupt_chunk);
		debug_log("Try to store accelerometer interrupt chunk: Ret %d\n", ret);
		if(ret == NRF_ERROR_INTERNAL) {	// E.g. if busy --> reschedule
			app_sched_event_put(NULL, 0, processing_process_accelerometer_interrupt_chunk);
			break;
		} else {
			chunk_fifo_read_close(&accelerometer_interrupt_chunk_fifo);	
		}
	}	
}

/******************************** BATTERY *********************************/
void processing_process_battery_chunk(void * p_event_data, uint16_t event_size) {
	debug_log("processing_process_battery_chunk...\n");
	BatteryChunk* battery_chunk;
	while(chunk_fifo_read_open(&battery_chunk_fifo, (void**) &battery_chunk, NULL) == NRF_SUCCESS) {
		// Store the battery chunk
		ret_code_t ret = storer_store_battery_chunk(battery_chunk);		
		debug_log("Try to store battery chunk: Ret %d\n", ret);
		if(ret == NRF_ERROR_INTERNAL) {	// E.g. if busy --> reschedule
			app_sched_event_put(NULL, 0, processing_process_battery_chunk);
			break;
		} else {
			chunk_fifo_read_close(&battery_chunk_fifo);	
		}
	}
	
}


/******************************* MICROPHONE *********************************/
void processing_process_microphone_chunk(void * p_event_data, uint16_t event_size) {
	debug_log("processing_process_microphone_chunk...\n");
	MicrophoneChunk* microphone_chunk;
	while(chunk_fifo_read_open(&microphone_chunk_fifo, (void**) &microphone_chunk, NULL) == NRF_SUCCESS) {
		ret_code_t ret = storer_store_microphone_chunk(microphone_chunk);		
		debug_log("Try to store microphone chunk: Ret %d\n", ret);
		if(ret == NRF_ERROR_INTERNAL) {	// E.g. if busy --> reschedule
			app_sched_event_put(NULL, 0, processing_process_microphone_chunk);
			break;
		} else {
			chunk_fifo_read_close(&microphone_chunk_fifo);	
		}
	}
}


/******************************* SCAN *********************************/

/**@brief Function that checks whether a scan-result is from a beacon or not.
 *
 * @param[in]	scan_result_data	Pointer to scan result data.
 * 
 * @retval	0	If it is no beacon.
 * @retval	1	If it is a beacon.
 */
static uint8_t is_beacon(ScanResultData* scan_result_data) {
	if(scan_result_data->scan_device.ID >= SCAN_BEACON_ID_THRESHOLD) {
		return 1;
	}
	return 0;
}

/**@brief Function that compares two scan-results by beacon.
 *
 * @param[in]	a	Pointer to scan result data of device A.
 * @param[in]	b	Pointer to scan result data of device B.
 *
 * @retval	-1	If device A is a beacon and device B is no beacon --> A should be in front of B.
 * @retval	0	If device A is a beacon and device B is a beacon. Or if A is no beacon and B is no beacon --> Nothing has to be done here.
 * @retval	1	If device B is a beacon and device A is no beacon --> B should be in front of A.
 */
static int compare_by_beacon(const void* a, const void* b) {
	uint8_t is_beacon_a = is_beacon((ScanResultData*) a);
	uint8_t is_beacon_b = is_beacon((ScanResultData*) b);
	if(is_beacon_a > is_beacon_b)
		return -1;	// We want A in front of B in the sorted list
	 else if(is_beacon_a < is_beacon_b)
		return 1; 	// We want B in front of A in the sorted list
	 else 
		return 0;	
}

/**@brief Function that compares two scan-results by RSSI value.
 *
 * @param[in]	a	Pointer to scan result data of device A.
 * @param[in]	b	Pointer to scan result data of device B.
 *
 * @retval	-1	If device A has a greater RSSI value than device B --> A should be in front of B.
 * @retval	0	If device A has the same RSSI value than device B --> Nothing has to be done here.
 * @retval	1	If device B has a greater RSSI value than device A --> B should be in front of A.
 */
static int compare_by_RSSI(const void* a, const void* b) {
	int8_t rssi_a = ((ScanResultData*) a)->scan_device.rssi;
	int8_t rssi_b = ((ScanResultData*) b)->scan_device.rssi;

	if(rssi_a > rssi_b)
		return -1;	// We want A in front of B in the sorted list
	 else if(rssi_a < rssi_b)
		return 1;	// We want B in front of A in the sorted list
	 else 
		return 0;
}

/**@brief Function that sorts the seen devices in the scan_sampling_chunk-structure.
 *
 * @details	As first step the beacons are brought to the begin of the structure and sorted by RSSI-value, 
 *			to get at least SCAN_PRIORITIZED_BEACONS (or less if there are not that much beacons available) in the scan.
 *			After that all remaining seen devices are sorted by RSSI-value.
 *			
 *
 * @param[in]	scan_sampling_chunk		Pointer to the ScanSamplingChunk to be sorted.
 */
static void sort_scan(ScanSamplingChunk* scan_sampling_chunk) {
	// First get the number of beacons:
	uint32_t num_beacons = 0;
	for(uint32_t i = 0; i < scan_sampling_chunk->scan_result_data_count; i++) {
		if(is_beacon(&(scan_sampling_chunk->scan_result_data[i]))) {
			num_beacons++;
		}
	}	
	// Then split Beacons and Badges
	qsort(scan_sampling_chunk->scan_result_data, scan_sampling_chunk->scan_result_data_count, sizeof(ScanResultData), compare_by_beacon);
	
	// Sort the beacons' RSSI values
	qsort(scan_sampling_chunk->scan_result_data, num_beacons, sizeof(ScanResultData), compare_by_RSSI);
	
	// Calculate the number of prioritized beacons
	uint32_t prioritized_beacons = (num_beacons > SCAN_PRIORITIZED_BEACONS) ? SCAN_PRIORITIZED_BEACONS : num_beacons;
	
	// Finally sort all devices, except the prioritized beacons
	qsort(&((scan_sampling_chunk->scan_result_data)[prioritized_beacons]), scan_sampling_chunk->scan_result_data_count - num_beacons, sizeof(ScanResultData), compare_by_RSSI);
}

void processing_process_scan_sampling_chunk(void * p_event_data, uint16_t event_size) {
	debug_log("processing_process_scan_sampling_chunk...\n");
	ScanSamplingChunk* scan_sampling_chunk;
		
	while(chunk_fifo_read_open(&scan_sampling_chunk_fifo, (void**) &scan_sampling_chunk, NULL) == NRF_SUCCESS) {
		// if there are more devices than we can store --> sort the scan chunk to store the "important" devices
		if(scan_sampling_chunk->scan_result_data_count > SCAN_CHUNK_DATA_SIZE) {
			sort_scan(scan_sampling_chunk);
			scan_sampling_chunk->scan_result_data_count = SCAN_CHUNK_DATA_SIZE;
		}
		
		scan_chunk.timestamp = scan_sampling_chunk->timestamp;
		scan_chunk.scan_result_data_count = scan_sampling_chunk->scan_result_data_count;
		for(uint32_t i = 0; i < scan_chunk.scan_result_data_count; i++) {
			scan_chunk.scan_result_data[i] = scan_sampling_chunk->scan_result_data[i];
		}		
		
		ret_code_t ret = storer_store_scan_chunk(&scan_chunk);
		debug_log("Try to store scan chunk: Ret %d\n", ret);
		if(ret == NRF_ERROR_INTERNAL) {	// E.g. if busy --> reschedule
			app_sched_event_put(NULL, 0, processing_process_scan_sampling_chunk);
			break;
		} else {
			chunk_fifo_read_close(&scan_sampling_chunk_fifo);	
		}
	}
}
