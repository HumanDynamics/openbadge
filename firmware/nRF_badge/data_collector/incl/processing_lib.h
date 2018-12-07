/**@file
 * @details This module provides the processing of the sampled data-chunks.
 *			It uses the chunk-fifos that are declared in sampling_lib.h for to get the data chunks that should be processed.
 *			All the processing functions are scheduled via the scheduler (and reschedule themself when an error occured).
 */

#ifndef __PROCESSING_LIB_H
#define __PROCESSING_LIB_H

#include "stdint.h"
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes

/**< Processing parameters for the scan chunks */
#define SCAN_BEACON_ID_THRESHOLD	16000
#define SCAN_PRIORITIZED_BEACONS	4

/**@brief Function that processes the accelerometer chunks.
 *
 * @details	It checks for available chunks in the chunk-fifo and tries to store the chunk as it is in the filesystem via the storer-module.
 *
 * @param[in] p_event_data	Pointer to event data (actually always == NULL).
 * @param[in] event_size	Event data size (actually always == 0).
 */
void processing_process_accelerometer_chunk(void * p_event_data, uint16_t event_size);

/**@brief Function that processes the accelerometer interrupt chunks.
 *
 * @details	It checks for available chunks in the chunk-fifo and tries to store the chunk as it is in the filesystem via the storer-module.
 *
 * @param[in] p_event_data	Pointer to event data (actually always == NULL).
 * @param[in] event_size	Event data size (actually always == 0).
 */
void processing_process_accelerometer_interrupt_chunk(void * p_event_data, uint16_t event_size);

/**@brief Function that processes the battery chunks.
 *
 * @details	It checks for available chunks in the chunk-fifo and tries to store the chunk as it is in the filesystem via the storer-module.
 *
 * @param[in] p_event_data	Pointer to event data (actually always == NULL).
 * @param[in] event_size	Event data size (actually always == 0).
 */
void processing_process_battery_chunk(void * p_event_data, uint16_t event_size);

/**@brief Function that processes the microphone chunks.
 *
 * @details	It checks for available chunks in the chunk-fifo and tries to store the chunk as it is in the filesystem via the storer-module.
 *
 * @param[in] p_event_data	Pointer to event data (actually always == NULL).
 * @param[in] event_size	Event data size (actually always == 0).
 */
void processing_process_microphone_chunk(void * p_event_data, uint16_t event_size);

/**@brief Function that processes the scanning chunks.
 *
 * @details	It checks for available chunks in the chunk-fifo. In this case not the ScanSamplingChunk-structure is stored but the ScanChunk-structure.
 *			The ScanSamplingChunk-structure can hold much more devices than the ScanChunk-structure that is used for storing.
 *			To get only the relevant devices a sorting mechanism is performed that sortes for strong RSSI-values and prioritzes beacons.
 *			After the sorting, the ScanChunk is stored in the filesystem via the storer-module.
 *
 * @param[in] p_event_data	Pointer to event data (actually always == NULL).
 * @param[in] event_size	Event data size (actually always == 0).
 */
void processing_process_scan_sampling_chunk(void * p_event_data, uint16_t event_size);




#endif