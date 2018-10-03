/**@file
 * @details
 *
 */

#ifndef __SAMPLING_LIB_H
#define __SAMPLING_LIB_H

#include "stdint.h"
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes
#include "chunk_fifo_lib.h"
#include "circular_fifo_lib.h"



typedef enum {
	SAMPLING_ACCELEROMETER 				= (1 << 0),
	STREAMING_ACCELEROMETER				= (1 << 1),
	SAMPLING_ACCELEROMETER_INTERRUPT 	= (1 << 2),
	STREAMING_ACCELEROMETER_INTERRUPT 	= (1 << 3),
	SAMPLING_BATTERY 					= (1 << 4),
	STREAMING_BATTERY 					= (1 << 5),
	SAMPLING_MICROPHONE 				= (1 << 6),
	STREAMING_MICROPHONE 				= (1 << 7),
	SAMPLING_SCAN 						= (1 << 8),
	STREAMING_SCAN 						= (1 << 9),
} sampling_configuration_t;



extern chunk_fifo_t 	accelerometer_chunk_fifo;
extern circular_fifo_t 	accelerometer_stream_fifo;
extern chunk_fifo_t 	accelerometer_interrupt_chunk_fifo;
extern circular_fifo_t 	accelerometer_interrupt_stream_fifo;
extern chunk_fifo_t 	battery_chunk_fifo;
extern circular_fifo_t 	battery_stream_fifo;
extern chunk_fifo_t 	microphone_chunk_fifo;
extern circular_fifo_t 	microphone_stream_fifo;
extern chunk_fifo_t 	scan_sampling_chunk_fifo;
extern circular_fifo_t 	scan_stream_fifo;

// app-timer must be initialized
// timeout must be intitalized
// systick
ret_code_t sampling_init(void);

sampling_configuration_t sampling_get_sampling_configuration(void);


void sampling_reset_timeouts(void);

ret_code_t sampling_start_accelerometer(uint32_t timeout_ms, uint8_t operating_mode, uint8_t full_scale, uint16_t datarate, uint16_t fifo_sampling_period_ms, uint8_t streaming);

ret_code_t sampling_stop_accelerometer(uint8_t streaming);


ret_code_t sampling_start_accelerometer_interrupt(uint32_t timeout_ms, uint16_t threshold_mg, uint16_t minimal_duration_ms, uint32_t ignore_duration_ms, uint8_t streaming);

ret_code_t sampling_stop_accelerometer_interrupt(uint8_t streaming);


ret_code_t sampling_start_battery(uint32_t timeout_ms, uint32_t period_ms, uint8_t streaming);

void	   sampling_stop_battery(uint8_t streaming);


ret_code_t sampling_start_microphone(uint32_t timeout_ms, uint16_t period_ms, uint8_t streaming);

void	   sampling_stop_microphone(uint8_t streaming);


ret_code_t sampling_start_scan(uint32_t timeout_ms, uint16_t period_seconds, uint16_t interval_ms, uint16_t window_ms, uint16_t duration_seconds, uint8_t group_filter, uint8_t aggregation_type, uint8_t streaming);

void 	   sampling_stop_scan(uint8_t streaming);



#endif