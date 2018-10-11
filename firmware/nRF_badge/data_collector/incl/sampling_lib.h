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


/**< Declaration of the chunk-fifos and stream-fifos of the different data-sources */
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


/**@brief This function initializes all the data-sources and -structures needed to sample the different data-sources.
 *
 * @details It initalizes all the different data-source modules (accel (if available), battery, scanner, microphone.
 *			For the data-sampling chunk-fifos are used to provide an efficient and clean way to perform fast data-sampling and data-exchange.
 *			For the data-streaming circular-fifos are used.
 *			Furthermore it initializes the timeout-mechanism for the data-sources.
 *
 * @retval 	NRF_SUCCESS		If everything was initialized correctly.
 * @retval	Otherwise an error code is provided.
 *
 * @note app-timer has to be initialized before!
 * @note app-scheduler has to be initialized before!
 * @note timeout_lib has to be initialized before!
 * @note systick_lib has to be initialized before!
 * @note advertiser_lib has to be initialized before!
 */
ret_code_t sampling_init(void);


/**@brief Function to retrieve the current sampling-/streaming-configuration (So which data-sources are currently sampling).
 *
 * @retval 	The current sampling-configuration.
 */
sampling_configuration_t sampling_get_sampling_configuration(void);

/**@brief Function to retrieve the current sampling-/streaming-configuration (So which data-sources are currently sampling).
 *
 * @retval 	The current sampling-configuration.
 */
void sampling_reset_timeouts(void);


/**@brief Function to start the accelerometer data recording or streaming.
 *
 * @param[in]	timeout_ms 					The timeout for the accelerometer in milliseconds (0 --> no timeout).
 * @param[in]	operating_mode 				[0 == power_down, 1 == low_power, 2 == normal, 3 == high_resolution]
 * @param[in]	full_scale					[2, 4, 8, 16]
 * @param[in]	datrate						[1, 10, 25, 50, 100, 200, 400]
 * @param[in]	fifo_sampling_period_ms		The sampling period in ms of the accelerometer output FIFO.
 * @param[in]	streaming					Flag if streaming or data acquisition should be enabled [0 == data-acquisistion, 1 == streaming].
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_start_accelerometer(uint32_t timeout_ms, uint8_t operating_mode, uint8_t full_scale, uint16_t datarate, uint16_t fifo_sampling_period_ms, uint8_t streaming);

/**@brief Function to stop the accelerometer data recording or streaming.
 *
 * @param[in]	streaming					Flag if streaming or data acquisition should be stopped [0 == data-acquisistion, 1 == streaming].
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_stop_accelerometer(uint8_t streaming);




/**@brief Function to start the accelerometer data recording or streaming.
 *
 * @param[in]	timeout_ms 					The timeout for the accelerometer-interrupt in milliseconds (0 --> no timeout).
 * @param[in]	threshold_mg 				The threshold in mg that has to be exceeded to generate an interrupt.
 * @param[in]	minimal_duration_ms 		The minimal duration the acceleration has to exceed the threshold to generate an interrupt.
 * @param[in]	ignore_duration_ms 			The time after an interrupt where new interrupt should be ignored.
 * @param[in]	streaming					Flag if streaming or data acquisition should be enabled [0 == data-acquisistion, 1 == streaming].
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_start_accelerometer_interrupt(uint32_t timeout_ms, uint16_t threshold_mg, uint16_t minimal_duration_ms, uint32_t ignore_duration_ms, uint8_t streaming);

/**@brief Function to stop the accelerometer-interrupt data recording or streaming.
 *
 * @param[in]	streaming					Flag if streaming or data acquisition should be stopped [0 == data-acquisistion, 1 == streaming].
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_stop_accelerometer_interrupt(uint8_t streaming);




/**@brief Function to start the battery data recording or streaming.
 *
 * @param[in]	timeout_ms 					The timeout for the battery in milliseconds  (0 --> no timeout).
 * @param[in]	period_ms 					The period at which the battery-sampling should be done.
 * @param[in]	streaming					Flag if streaming or data acquisition should be enabled [0 == data-acquisistion, 1 == streaming].
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_start_battery(uint32_t timeout_ms, uint32_t period_ms, uint8_t streaming);

/**@brief Function to stop the battery data recording or streaming.
 *
 * @param[in]	streaming					Flag if streaming or data acquisition should be stopped [0 == data-acquisistion, 1 == streaming].
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
void	   sampling_stop_battery(uint8_t streaming);





/**@brief Function to start the microphone data recording or streaming.
 *
 * @param[in]	timeout_ms 					The timeout for the microphone in milliseconds  (0 --> no timeout).
 * @param[in]	period_ms 					The period at which the microphone-average-sampling should be done.
 * @param[in]	streaming					Flag if streaming or data acquisition should be enabled [0 == data-acquisistion, 1 == streaming].
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_start_microphone(uint32_t timeout_ms, uint16_t period_ms, uint8_t streaming);

/**@brief Function to stop the microphone data recording or streaming.
 *
 * @param[in]	streaming					Flag if streaming or data acquisition should be stopped [0 == data-acquisistion, 1 == streaming].
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
void	   sampling_stop_microphone(uint8_t streaming);



/**@brief Function to start the scan data recording or streaming.
 *
 * @param[in]	timeout_ms 					The timeout for the scan in milliseconds  (0 --> no timeout).
 * @param[in]	period_seconds 				The period at which the scan-sampling should be done.
 * @param[in]	interval_ms 				The interval of the scan.
 * @param[in]	window_ms 					The window of the scan.
 * @param[in]	duration_seconds 			The duration of the scan.
 * @param[in]	group_filter 				The group that should be filtered out. If 0xFF nothing is filtered.
 * @param[in]	aggregation_type 			The aggregation type for the seen devices: [SCAN_CHUNK_AGGREGATE_TYPE_MAX] == MAX, [SCAN_CHUNK_AGGREGATE_TYPE_MEAN] == MEAN.
 * @param[in]	streaming					Flag if streaming or data acquisition should be enabled [0 == data-acquisistion, 1 == streaming].
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_start_scan(uint32_t timeout_ms, uint16_t period_seconds, uint16_t interval_ms, uint16_t window_ms, uint16_t duration_seconds, uint8_t group_filter, uint8_t aggregation_type, uint8_t streaming);

/**@brief Function to stop the scan data recording or streaming.
 *
 * @param[in]	streaming					Flag if streaming or data acquisition should be stopped [0 == data-acquisistion, 1 == streaming].
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
void 	   sampling_stop_scan(uint8_t streaming);



#endif