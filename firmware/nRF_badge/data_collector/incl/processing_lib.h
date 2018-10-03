/**@file
 * @details
 *
 */

#ifndef __PROCESSING_LIB_H
#define __PROCESSING_LIB_H

#include "stdint.h"
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes


#define SCAN_BEACON_ID_THRESHOLD	16000
#define SCAN_PRIORITIZED_BEACONS	4


void processing_process_accelerometer_chunk(void * p_event_data, uint16_t event_size);

void processing_process_accelerometer_interrupt_chunk(void * p_event_data, uint16_t event_size);


void processing_process_battery_chunk(void * p_event_data, uint16_t event_size);

void processing_process_microphone_chunk(void * p_event_data, uint16_t event_size);

void processing_process_scan_sampling_chunk(void * p_event_data, uint16_t event_size);




#endif