#ifndef __STORER_LIB_H
#define __STORER_LIB_H

#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes
#include "chunk_messages.h"

ret_code_t storer_init(void);



ret_code_t storer_store_badge_assignement(BadgeAssignement* badge_assignement);

ret_code_t storer_read_badge_assignement(BadgeAssignement* badge_assignement);



ret_code_t storer_store_accelerometer_chunk(AccelerometerChunk* accelerometer_chunk);

ret_code_t storer_find_accelerometer_chunk_from_timestamp(Timestamp timestamp, AccelerometerChunk* accelerometer_chunk);

ret_code_t storer_get_next_accelerometer_chunk(AccelerometerChunk* accelerometer_chunk);



ret_code_t storer_store_accelerometer_interrupt_chunk(AccelerometerInterruptChunk* accelerometer_interrupt_chunk);

ret_code_t storer_find_accelerometer_interrupt_chunk_from_timestamp(Timestamp timestamp, AccelerometerInterruptChunk* accelerometer_interrupt_chunk);

ret_code_t storer_get_next_accelerometer_interrupt_chunk(AccelerometerInterruptChunk* accelerometer_interrupt_chunk);



ret_code_t storer_store_battery_chunk(BatteryChunk* battery_chunk);

ret_code_t storer_find_battery_chunk_from_timestamp(Timestamp timestamp, BatteryChunk* battery_chunk);

ret_code_t storer_get_next_battery_chunk(BatteryChunk* battery_chunk);



ret_code_t storer_store_scan_chunk(ScanChunk* scan_chunk);

ret_code_t storer_find_scan_chunk_from_timestamp(Timestamp timestamp, ScanChunk* scan_chunk);

ret_code_t storer_get_next_scan_chunk(ScanChunk* scan_chunk);



ret_code_t storer_store_microphone_chunk(MicrophoneChunk* microphone_chunk);

ret_code_t storer_find_microphone_chunk_from_timestamp(Timestamp timestamp, MicrophoneChunk* microphone_chunk);

ret_code_t storer_get_next_microphone_chunk(MicrophoneChunk* microphone_chunk);

#endif 

