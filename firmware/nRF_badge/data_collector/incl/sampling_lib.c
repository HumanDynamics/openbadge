#include "sampling_lib.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "chunk_messages.h"
#include "stream_messages.h"
#include "processing_lib.h"
#include "timeout_lib.h"

#include "accel_lib.h"
#include "battery_lib.h"
#include "microphone_lib.h"
#include "scanner_lib.h"
#include "advertiser_lib.h"

#include "systick_lib.h"

// TODO: remove
#include "debug_lib.h"



#define SAMPLING_ACCEL_ENABLED 	ACCELEROMETER_PRESENT




#define ABS(x) (((x) >= 0)? (x) : -(x))


#define AGGREGATE_SCAN_SAMPLE_MAX(sample, aggregated) 	((sample) > (aggregated) ? (sample) : (aggregated))
#define PROCESS_SCAN_SAMPLE_MAX(aggregated, count) 		(aggregated)

#define AGGREGATE_SCAN_SAMPLE_MEAN(sample, aggregated) 	((aggregated) + (sample))
#define PROCESS_SCAN_SAMPLE_MEAN(aggregated, count) 	((aggregated)/(count))

#define MICROPHONE_READING_PERIOD_MS			(1000.0f / 700.0f)
#define MICROPHONE_READING_SLEEP_RATIO          0.075
#define MICROPHONE_READING_WINDOW_MS            (MICROPHONE_READING_PERIOD_MS * MICROPHONE_READING_SLEEP_RATIO)


static sampling_configuration_t sampling_configuration;




#if SAMPLING_ACCEL_ENABLED
chunk_fifo_t 	accelerometer_chunk_fifo;
circular_fifo_t accelerometer_stream_fifo;
static AccelerometerChunk*	accelerometer_chunk = NULL;
static uint32_t accelerometer_timeout_id;
static uint32_t accelerometer_stream_timeout_id;

chunk_fifo_t 	accelerometer_interrupt_chunk_fifo;
circular_fifo_t accelerometer_interrupt_stream_fifo;
static AccelerometerInterruptChunk*	accelerometer_interrupt_chunk = NULL;
static uint32_t	accelerometer_interrupt_ignore_duration_ms = 0;
static uint32_t accelerometer_interrupt_timeout_id;
static uint32_t accelerometer_interrupt_stream_timeout_id;
#endif

chunk_fifo_t 	battery_chunk_fifo;
circular_fifo_t battery_stream_fifo;
static BatteryChunk* battery_chunk = NULL;
static uint32_t battery_timeout_id;
static uint32_t battery_stream_timeout_id;

chunk_fifo_t 	microphone_chunk_fifo;
circular_fifo_t microphone_stream_fifo;
static MicrophoneChunk* microphone_chunk = NULL;
static const float microphone_aggregated_period_ms = MICROPHONE_READING_PERIOD_MS;
static uint16_t microphone_period_ms = 0;
static uint32_t microphone_aggregated = 0;
static uint32_t microphone_aggregated_count = 0;
static uint32_t microphone_timeout_id;
static uint32_t microphone_stream_timeout_id;

chunk_fifo_t 	scan_sampling_chunk_fifo;
circular_fifo_t scan_stream_fifo;
static ScanSamplingChunk* scan_sampling_chunk = NULL;
static uint16_t scan_interval_ms = 0;
static uint16_t scan_window_ms = 0;
static uint16_t scan_duration_seconds = 0;
static uint8_t 	scan_group_filter = 0;
static const uint8_t	scan_no_group_filter_pattern = 0xFF;
static uint8_t	scan_aggregation_type = SCAN_CHUNK_AGGREGATE_TYPE_MAX;	/**< The type of the aggregation: [SCAN_CHUNK_AGGREGATE_TYPE_MAX] == MAX, [SCAN_CHUNK_AGGREGATE_TYPE_MEAN] == MEAN */
static int32_t  scan_aggregated_rssi[SCAN_SAMPLING_CHUNK_DATA_SIZE];		/**< Temporary array to aggregate the rssi-data */
static uint32_t scan_timeout_id;
static uint32_t scan_stream_timeout_id;
/**< The scan:period is directly setted by starting the timer */


APP_TIMER_DEF(sampling_accelerometer_fifo_timer);
void sampling_accelerometer_fifo_callback(void* p_context);
void sampling_setup_accelerometer_chunk(void);
void sampling_finalize_accelerometer_chunk(void);
void sampling_timeout_accelerometer(void);
void sampling_timeout_accelerometer_stream(void);

APP_TIMER_DEF(sampling_accelerometer_interrupt_reset_timer);		/**< The timer that resets the interrupt of the accelerometer. */
void sampling_accelerometer_interrupt_callback(accel_interrupt_event_t const * p_event);
void sampling_accelerometer_interrupt_reset_callback(void* p_context);
void sampling_setup_accelerometer_interrupt_chunk(void * p_event_data, uint16_t event_size); /**< Because it has to be called via the scheduler */
void sampling_finalize_accelerometer_interrupt_chunk(void);
void sampling_timeout_accelerometer_interrupt(void);
void sampling_timeout_accelerometer_interrupt_stream(void);

APP_TIMER_DEF(sampling_battery_timer);
void sampling_battery_callback(void* p_context);
void sampling_setup_battery_chunk(void);
void sampling_finalize_battery_chunk(void);
void sampling_timeout_battery(void);
void sampling_timeout_battery_stream(void);

APP_TIMER_DEF(sampling_microphone_timer);
APP_TIMER_DEF(sampling_microphone_aggregated_timer);
void sampling_microphone_callback(void* p_context);
void sampling_microphone_aggregated_callback(void* p_context);
void sampling_setup_microphone_chunk(void);
void sampling_finalize_microphone_chunk(void);
void sampling_timeout_microphone(void);
void sampling_timeout_microphone_stream(void);

APP_TIMER_DEF(sampling_scan_timer);
void sampling_scan_callback(void* p_context); /**< Starts a scanning cycle */
void sampling_on_scan_timeout_callback(void);
void sampling_on_scan_report_callback(scanner_scan_report_t* scanner_scan_report);
void sampling_setup_scan_sampling_chunk(void);
void sampling_finalize_scan_sampling_chunk(void);
void sampling_timeout_scan(void);
void sampling_timeout_scan_stream(void);


ret_code_t sampling_init(void) {
	ret_code_t ret = NRF_SUCCESS;
	(void) ret;
	
	sampling_configuration = (sampling_configuration_t) 0;
	
	#if SAMPLING_ACCEL_ENABLED
	/********************* ACCELEROMETER ***************************/
	ret = accel_init();
	if(ret != NRF_SUCCESS) return ret;	
	
	// Configure the accelerometer
	ret = accel_set_axis((accel_axis_t) (ACCEL_X_AXIS_ENABLE | ACCEL_Y_AXIS_ENABLE | ACCEL_Z_AXIS_ENABLE));
	if(ret != NRF_SUCCESS) return ret;		
	ret = accel_set_fifo(ACCEL_FIFO_STREAM_ENABLE);
	if(ret != NRF_SUCCESS) return ret;	
	ret = accel_set_HP_filter(ACCEL_HP_FILTER_ENABLE);
	if(ret != NRF_SUCCESS) return ret;		
	
	// create a timer for sampling of the accelerometer data
	ret = app_timer_create(&sampling_accelerometer_fifo_timer, APP_TIMER_MODE_REPEATED, sampling_accelerometer_fifo_callback);
	if(ret != NRF_SUCCESS) return ret;
	
	// initialize the chunk-fifo for the accelereometer 
	CHUNK_FIFO_INIT(ret, accelerometer_chunk_fifo, 2, sizeof(AccelerometerChunk), 0);
	if(ret != NRF_SUCCESS) return ret;
	
	// initialize the circular-fifo for the stream of the accelerometer
	CIRCULAR_FIFO_INIT(ret, accelerometer_stream_fifo, sizeof(AccelerometerStream) * STREAM_ACCELEROMETER_FIFO_SIZE);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = timeout_register(&accelerometer_timeout_id, sampling_timeout_accelerometer);
	if(ret != NRF_SUCCESS) return ret;
	ret = timeout_register(&accelerometer_stream_timeout_id, sampling_timeout_accelerometer_stream);
	if(ret != NRF_SUCCESS) return ret;
	
	/********************* ACCELEROMETER INTERRUPT ***************************/
	// create a timer for reset of the interrupt 
	ret = app_timer_create(&sampling_accelerometer_interrupt_reset_timer, APP_TIMER_MODE_SINGLE_SHOT, sampling_accelerometer_interrupt_reset_callback);
	if(ret != NRF_SUCCESS) return ret;
	
	// initialize the chunk-fifo for the accelereometer interrupt
	CHUNK_FIFO_INIT(ret, accelerometer_interrupt_chunk_fifo, 2, sizeof(AccelerometerInterruptChunk), 0);
	if(ret != NRF_SUCCESS) return ret;
	
	// initialize the circular-fifo for the stream of the accelerometer interrupt
	CIRCULAR_FIFO_INIT(ret, accelerometer_interrupt_stream_fifo, sizeof(AccelerometerInterruptStream) * STREAM_ACCELEROMETER_INTERRUPT_FIFO_SIZE);
	if(ret != NRF_SUCCESS) return ret;	
	
	ret = timeout_register(&accelerometer_interrupt_timeout_id, sampling_timeout_accelerometer_interrupt);
	if(ret != NRF_SUCCESS) return ret;
	ret = timeout_register(&accelerometer_interrupt_stream_timeout_id, sampling_timeout_accelerometer_interrupt_stream);
	if(ret != NRF_SUCCESS) return ret;
	
	#endif
	
	
	/************************ BATTERY *************************************/
	
	ret = battery_init();
	if(ret != NRF_SUCCESS) return ret;
	
	// create a timer for battery measurement
	ret = app_timer_create(&sampling_battery_timer, APP_TIMER_MODE_REPEATED, sampling_battery_callback);
	if(ret != NRF_SUCCESS) return ret;
	
	// initialize the chunk-fifo for the battery data
	CHUNK_FIFO_INIT(ret, battery_chunk_fifo, 2, sizeof(BatteryChunk), 0);
	if(ret != NRF_SUCCESS) return ret;
	
	// initialize the circular-fifo for the stream of the battery data
	CIRCULAR_FIFO_INIT(ret, battery_stream_fifo, sizeof(BatteryStream) * STREAM_BATTERY_FIFO_SIZE);
	if(ret != NRF_SUCCESS) return ret;	
	
	ret = timeout_register(&battery_timeout_id, sampling_timeout_battery);
	if(ret != NRF_SUCCESS) return ret;
	ret = timeout_register(&battery_stream_timeout_id, sampling_timeout_battery_stream);
	if(ret != NRF_SUCCESS) return ret;
	

	/********************* MICROPHONE ***********************************/
	microphone_init();
	
	// create a timer for microphone sampling
	ret = app_timer_create(&sampling_microphone_timer, APP_TIMER_MODE_REPEATED, sampling_microphone_callback);
	if(ret != NRF_SUCCESS) return ret;
	
	// create a timer for microphone accumulation
	ret = app_timer_create(&sampling_microphone_aggregated_timer, APP_TIMER_MODE_REPEATED, sampling_microphone_aggregated_callback);
	if(ret != NRF_SUCCESS) return ret;
	
	// initialize the chunk-fifo for the microphone data
	CHUNK_FIFO_INIT(ret, microphone_chunk_fifo, 2, sizeof(MicrophoneChunk), 0);
	if(ret != NRF_SUCCESS) return ret;
	
	// initialize the circular-fifo for the stream of the microphone data
	CIRCULAR_FIFO_INIT(ret, microphone_stream_fifo, sizeof(MicrophoneStream) * STREAM_MICROPHONE_FIFO_SIZE);
	if(ret != NRF_SUCCESS) return ret;	
	
	ret = timeout_register(&microphone_timeout_id, sampling_timeout_microphone);
	if(ret != NRF_SUCCESS) return ret;
	ret = timeout_register(&microphone_stream_timeout_id, sampling_timeout_microphone_stream);
	if(ret != NRF_SUCCESS) return ret;
	
	/********************* SCAN ***************************************/
	scanner_init();
	
	// Set timeout and report callback function
	scanner_set_on_scan_timeout_callback(sampling_on_scan_timeout_callback);
	scanner_set_on_scan_report_callback(sampling_on_scan_report_callback);
	
	// create a timer for scans
	ret = app_timer_create(&sampling_scan_timer, APP_TIMER_MODE_REPEATED, sampling_scan_callback);
	if(ret != NRF_SUCCESS) return ret;
	
	// initialize the chunk-fifo for the scan data
	CHUNK_FIFO_INIT(ret, scan_sampling_chunk_fifo, 1, sizeof(ScanSamplingChunk), 0);
	if(ret != NRF_SUCCESS) return ret;
	
	// initialize the circular-fifo for the stream of the scan data
	CIRCULAR_FIFO_INIT(ret, scan_stream_fifo, sizeof(ScanStream) * STREAM_SCAN_FIFO_SIZE);
	if(ret != NRF_SUCCESS) return ret;	
	
	ret = timeout_register(&scan_timeout_id, sampling_timeout_scan);
	if(ret != NRF_SUCCESS) return ret;
	ret = timeout_register(&scan_stream_timeout_id, sampling_timeout_scan_stream);
	if(ret != NRF_SUCCESS) return ret;
	
	return NRF_SUCCESS;
}

sampling_configuration_t sampling_get_sampling_configuration(void) {
	return sampling_configuration;	
}

void sampling_reset_timeouts(void) {
	#if SAMPLING_ACCEL_ENABLED
	timeout_reset(accelerometer_timeout_id);
	timeout_reset(accelerometer_stream_timeout_id);
	timeout_reset(accelerometer_interrupt_timeout_id);
	timeout_reset(accelerometer_interrupt_stream_timeout_id);
	#endif
	timeout_reset(battery_timeout_id);
	timeout_reset(battery_stream_timeout_id);
	timeout_reset(microphone_timeout_id);
	timeout_reset(microphone_stream_timeout_id);
	timeout_reset(scan_timeout_id);
	timeout_reset(scan_stream_timeout_id);
}




ret_code_t sampling_start_accelerometer(uint32_t timeout_ms, uint8_t operating_mode, uint8_t full_scale, uint16_t datarate, uint16_t fifo_sampling_period_ms, uint8_t streaming) {
	ret_code_t ret = NRF_SUCCESS;
	
	#if SAMPLING_ACCEL_ENABLED
	
	accel_datarate_t		accelerometer_datarate;
	accel_operating_mode_t	accelerometer_operating_mode;
	accel_full_scale_t		accelerometer_full_scale;

	
	if(operating_mode == 0) 
		accelerometer_operating_mode = ACCEL_POWER_DOWN_MODE;
	else if(operating_mode == 1)
		accelerometer_operating_mode = ACCEL_LOW_POWER_MODE;
	else if(operating_mode == 2)
		accelerometer_operating_mode = ACCEL_NORMAL_MODE;
	else if(operating_mode == 3)
		accelerometer_operating_mode = ACCEL_HIGH_RESOLUTION_MODE;
	else
		accelerometer_operating_mode = ACCEL_LOW_POWER_MODE;
	
	
	
	if(full_scale <= 2)
		accelerometer_full_scale = ACCEL_FULL_SCALE_2G;
	else if(full_scale <= 4)
		accelerometer_full_scale = ACCEL_FULL_SCALE_4G;
	else if(full_scale <= 8)
		accelerometer_full_scale = ACCEL_FULL_SCALE_8G;
	else if(full_scale <= 16)
		accelerometer_full_scale = ACCEL_FULL_SCALE_16G;
	else
		accelerometer_full_scale = ACCEL_FULL_SCALE_4G;
	
	
	
	if(datarate <= 1)
		accelerometer_datarate = ACCEL_DATARATE_1_HZ;
	else if(datarate <= 10)
		accelerometer_datarate = ACCEL_DATARATE_10_HZ;
	else if(datarate <= 25)
		accelerometer_datarate = ACCEL_DATARATE_25_HZ;
	else if(datarate <= 50)
		accelerometer_datarate = ACCEL_DATARATE_50_HZ;
	else if(datarate <= 100)
		accelerometer_datarate = ACCEL_DATARATE_100_HZ;
	else if(datarate <= 200)
		accelerometer_datarate = ACCEL_DATARATE_200_HZ;
	else if(datarate <= 400)
		accelerometer_datarate = ACCEL_DATARATE_400_HZ;
	else 
		accelerometer_datarate = ACCEL_DATARATE_10_HZ;
	
	
	ret = accel_set_full_scale(accelerometer_full_scale);
	if(ret != NRF_SUCCESS) return ret;	
	
	ret = accel_set_datarate(accelerometer_datarate);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = accel_set_operating_mode(accelerometer_operating_mode);
	if(ret != NRF_SUCCESS) return ret;
	
	if(!streaming) {
		if((sampling_configuration & SAMPLING_ACCELEROMETER) == 0) { // Only stop and start the sampling if it is not already running			
			// Stop the sampling-timer that was probably already started
			app_timer_stop(sampling_accelerometer_fifo_timer);
			
			// If we want to start normal sampling
			sampling_setup_accelerometer_chunk();
			
			// Now start the sampling-timer
			ret = app_timer_start(sampling_accelerometer_fifo_timer, APP_TIMER_TICKS(fifo_sampling_period_ms, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;	
			
			sampling_configuration = (sampling_configuration_t) (sampling_configuration | SAMPLING_ACCELEROMETER);
			advertiser_set_status_flag_accelerometer_enabled(1);
			
			timeout_start(accelerometer_timeout_id, timeout_ms);
		}
		
	} else {
		// If we are not already sampling the accelerometer, we have to start the sampling-timer
		if((sampling_configuration & SAMPLING_ACCELEROMETER) == 0) {
			// Stop the sampling-timer that was probably already started
			app_timer_stop(sampling_accelerometer_fifo_timer);
			
			// Now start the sampling-timer
			ret = app_timer_start(sampling_accelerometer_fifo_timer, APP_TIMER_TICKS(fifo_sampling_period_ms, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;	
		}
			
		sampling_configuration = (sampling_configuration_t) (sampling_configuration | STREAMING_ACCELEROMETER);
		
		timeout_start(accelerometer_stream_timeout_id, timeout_ms);
	}
	
	
	#endif
	
	
	
	return ret;
}

ret_code_t sampling_stop_accelerometer(uint8_t streaming) {
	ret_code_t ret = NRF_SUCCESS;
	#if SAMPLING_ACCEL_ENABLED
	// Check if we are allowed to stop the accelerometer-timer, and to disable the accelerometer
	if(!streaming) {
		if((sampling_configuration & STREAMING_ACCELEROMETER) == 0) {	// We are only allowed to stop the accelerometer-timer, if we don't stream
			app_timer_stop(sampling_accelerometer_fifo_timer);
			// Only disable the accelerometer, if no interrupts should be generated
			if((sampling_configuration & SAMPLING_ACCELEROMETER_INTERRUPT) == 0 && (sampling_configuration & STREAMING_ACCELEROMETER_INTERRUPT) == 0) {
				ret = accel_set_operating_mode(ACCEL_POWER_DOWN_MODE);
				if(ret != NRF_SUCCESS) return ret;
			}				
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(SAMPLING_ACCELEROMETER));
		advertiser_set_status_flag_accelerometer_enabled(0);
	} else {
		if((sampling_configuration & SAMPLING_ACCELEROMETER) == 0) {	// We are only allowed to stop the accelerometer-timer, if we don't sample
			app_timer_stop(sampling_accelerometer_fifo_timer);
			// Only disable the accelerometer, if no interrupts should be generated
			if((sampling_configuration & SAMPLING_ACCELEROMETER_INTERRUPT) == 0 && (sampling_configuration & STREAMING_ACCELEROMETER_INTERRUPT) == 0) {
				ret = accel_set_operating_mode(ACCEL_POWER_DOWN_MODE);
				if(ret != NRF_SUCCESS) return ret;
			}
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(STREAMING_ACCELEROMETER));
	}
	
	
	#endif
	
	
	
	return ret;
}

void sampling_timeout_accelerometer(void) {
	debug_log("SAMPLING: Accelerometer timed out --> stopping\n");
	sampling_stop_accelerometer(0);
}
void sampling_timeout_accelerometer_stream(void) {
	debug_log("SAMPLING: Accelerometer stream timed out --> stopping\n");
	sampling_stop_accelerometer(1);
}


void sampling_setup_accelerometer_chunk(void) {
	#if SAMPLING_ACCEL_ENABLED
	// Open a chunk in the FIFO
	chunk_fifo_write_open(&accelerometer_chunk_fifo, (void**) &accelerometer_chunk, NULL);
	
	systick_get_timestamp(&((accelerometer_chunk->timestamp).seconds), &((accelerometer_chunk->timestamp).ms));
	accelerometer_chunk->accelerometer_data_count = 0;
	
	debug_log("SAMPLING: Setup accelerometer chunk\n");
	#endif
}

void sampling_finalize_accelerometer_chunk(void) {	
	#if SAMPLING_ACCEL_ENABLED
	debug_log("SAMPLING: Finalize accelerometer chunk\n");
	
	// Close the chunk in the FIFO
	chunk_fifo_write_close(&accelerometer_chunk_fifo);
	
	sampling_setup_accelerometer_chunk();		// Setup a new chunk
	
	app_sched_event_put(NULL, 0, processing_process_accelerometer_chunk);
	#endif
}



/**@brief This callback funcion is called when the sampling-timer of the accelerometer is invoked.
 * 
 * @param[in]	p_context 	Pointer to context that is provided by the timer. Should be NULL.
 */
void sampling_accelerometer_fifo_callback(void* p_context) {
	#if SAMPLING_ACCEL_ENABLED
	if((sampling_configuration & SAMPLING_ACCELEROMETER) == 0 && (sampling_configuration & STREAMING_ACCELEROMETER) == 0)
		return;
	
	int16_t x[32], y[32], z[32];
	
	uint32_t num =  accelerometer_chunk->accelerometer_data_count;
	uint32_t remaining_num_samples = ACCELEROMETER_CHUNK_DATA_SIZE - num;
	uint8_t num_samples = 0;
	// Read the accelerometer
	ret_code_t ret = accel_read_acceleration(x, y, z, &num_samples, remaining_num_samples);
	if(ret != NRF_SUCCESS)
		return;
	//debug_log("SAMPLING: Read accel fifo: n=%u, remain=%u, ms=%u\n", num_samples, remaining_num_samples, (uint32_t) systick_get_millis());
	if(sampling_configuration & SAMPLING_ACCELEROMETER) {	// Fill the chunk if we want to
		for(uint8_t i = 0; i < num_samples; i++) {	
			accelerometer_chunk->accelerometer_data[num + i].acceleration = (ABS(x[i]) + ABS(y[i]) + ABS(z[i]));	
		}	
		
		
		// Increment the number of read samples
		accelerometer_chunk->accelerometer_data_count = num + ((uint32_t) num_samples);
		
		// Check if the max number of samples has been reached
		if(accelerometer_chunk->accelerometer_data_count >= ACCELEROMETER_CHUNK_DATA_SIZE) {
			sampling_finalize_accelerometer_chunk();	// Finalize the opened chunk
		}
	}
	
	if(sampling_configuration & STREAMING_ACCELEROMETER) {	// Put the elements on the stream if we want to
		AccelerometerStream accelerometer_stream;
		for(uint8_t i = 0; i < num_samples; i++) {
			accelerometer_stream.accelerometer_raw_data.raw_acceleration[0] = x[i];
			accelerometer_stream.accelerometer_raw_data.raw_acceleration[1] = y[i];
			accelerometer_stream.accelerometer_raw_data.raw_acceleration[2] = z[i];
			circular_fifo_write(&accelerometer_stream_fifo, (uint8_t*) &accelerometer_stream, sizeof(accelerometer_stream));
		}
	}	
	#endif
}






/********************************* ACCELEROMETER INTERRUPT ************************/


ret_code_t sampling_start_accelerometer_interrupt(uint32_t timeout_ms, uint16_t threshold_mg, uint16_t minimal_duration_ms, uint32_t ignore_duration_ms, uint8_t streaming) { 
	ret_code_t ret = NRF_SUCCESS;
	#if SAMPLING_ACCEL_ENABLED
	// Check if we need to set up some default parameters for the accelerometer
	if((sampling_configuration & SAMPLING_ACCELEROMETER) == 0 && (sampling_configuration & STREAMING_ACCELEROMETER) == 0) {
		ret = accel_set_full_scale(ACCEL_FULL_SCALE_4G);
		if(ret != NRF_SUCCESS) return ret;		
		ret = accel_set_datarate(ACCEL_DATARATE_10_HZ);
		if(ret != NRF_SUCCESS) return ret;	
		ret = accel_set_operating_mode(ACCEL_LOW_POWER_MODE);
		if(ret != NRF_SUCCESS) return ret;
	}
	
	
	ret = accel_set_motion_interrupt_parameters(threshold_mg, minimal_duration_ms);
	if(ret != NRF_SUCCESS) return ret;	
	
	accel_set_interrupt_handler(sampling_accelerometer_interrupt_callback);
	
	ret = accel_set_interrupt(ACCEL_MOTION_INTERRUPT);
	if(ret != NRF_SUCCESS) return ret;	
	
	
	accelerometer_interrupt_ignore_duration_ms = ignore_duration_ms;
	
	
	if(!streaming) {
		sampling_setup_accelerometer_interrupt_chunk(NULL, 0);
		
		sampling_configuration = (sampling_configuration_t) (sampling_configuration | SAMPLING_ACCELEROMETER_INTERRUPT);
		advertiser_set_status_flag_accelerometer_interrupt_enabled(1);
		
		timeout_start(accelerometer_interrupt_timeout_id, timeout_ms);
	} else {
		sampling_configuration = (sampling_configuration_t) (sampling_configuration | STREAMING_ACCELEROMETER_INTERRUPT);
		timeout_start(accelerometer_interrupt_stream_timeout_id, timeout_ms);
	}
	
	#endif
	return ret;
}


ret_code_t sampling_stop_accelerometer_interrupt(uint8_t streaming) {
	ret_code_t ret = NRF_SUCCESS;
	#if SAMPLING_ACCEL_ENABLED
	// Check if we are allowed to disable interrupts and to disable the accelerometer
	if(!streaming) {
		if((sampling_configuration & STREAMING_ACCELEROMETER_INTERRUPT) == 0) {
			ret = accel_set_interrupt(ACCEL_NO_INTERRUPT);
			if(ret != NRF_SUCCESS) return ret;	
			if((sampling_configuration & SAMPLING_ACCELEROMETER) == 0 && (sampling_configuration & STREAMING_ACCELEROMETER) == 0) {	
				ret = accel_set_operating_mode(ACCEL_POWER_DOWN_MODE);
				if(ret != NRF_SUCCESS) return ret;
			}
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(SAMPLING_ACCELEROMETER_INTERRUPT));
		advertiser_set_status_flag_accelerometer_interrupt_enabled(0);
	} else {
		if((sampling_configuration & SAMPLING_ACCELEROMETER_INTERRUPT) == 0) {
			ret = accel_set_interrupt(ACCEL_NO_INTERRUPT);
			if(ret != NRF_SUCCESS) return ret;	
			if((sampling_configuration & SAMPLING_ACCELEROMETER) == 0 && (sampling_configuration & STREAMING_ACCELEROMETER) == 0) {
				ret = accel_set_operating_mode(ACCEL_POWER_DOWN_MODE);
				if(ret != NRF_SUCCESS) return ret;
			}
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(STREAMING_ACCELEROMETER_INTERRUPT));
	}
	
	#endif
	return ret;
}

void sampling_timeout_accelerometer_interrupt(void) {
	debug_log("SAMPLING: Accelerometer interrupt timed out --> stopping\n");
	sampling_stop_accelerometer_interrupt(0);
}
void sampling_timeout_accelerometer_interrupt_stream(void) {
	debug_log("SAMPLING: Accelerometer interrupt stream timed out --> stopping\n");
	sampling_stop_accelerometer_interrupt(1);
}

void sampling_setup_accelerometer_interrupt_chunk(void * p_event_data, uint16_t event_size) {
	#if SAMPLING_ACCEL_ENABLED
	debug_log("SAMPLING: Setup accelerometer interrupt chunk\n");
	
	// Open a chunk in the FIFO
	chunk_fifo_write_open(&accelerometer_interrupt_chunk_fifo, (void**) &accelerometer_interrupt_chunk, NULL);
	
	// Reset the interrupt
	accel_reset_interrupt();
	#endif
}

void sampling_finalize_accelerometer_interrupt_chunk(void) {	
	#if SAMPLING_ACCEL_ENABLED
	debug_log("SAMPLING: Finalize accelerometer interrupt chunk\n");
	
	// Close the chunk in the FIFO
	chunk_fifo_write_close(&accelerometer_interrupt_chunk_fifo);
	
	app_sched_event_put(NULL, 0, processing_process_accelerometer_interrupt_chunk);
	
	// Don't call sampling_setup_accelerometer_interrupt_chunk() here because it should be called via the reset-timer
	#endif
	
}

void sampling_accelerometer_interrupt_reset_callback(void* p_context) {
	#if SAMPLING_ACCEL_ENABLED
	// We have to call setup_accelerometer_interrupt_chunk via the scheduler,
	// because the SPI-transfer might have the same IRQ-Priority like this function (called by the timer)
	// and so the SPI-transfer would never terminate.
	
	app_sched_event_put(NULL, 0, sampling_setup_accelerometer_interrupt_chunk);
	#endif
}


/**@brief This callback funcion is called when there is an interrupt event generated by the accelerometer.
 *
 * @note Keep in mind that accel_reset_interrupt() has to be called every time an interrupt was generated (otherwise there won't be new interrupts).
 *			But it is important to not call this function in this callback function because of the interrupt priorities of the spi-interface and the INT1-Pin.
 * 
 * @param[in]	p_event 	Pointer to event that is provided by the accelerometer interrupt handler. Should be ACCEL_MOTION_INTERRUPT.
 */
void sampling_accelerometer_interrupt_callback(accel_interrupt_event_t const * p_event) {
	#if SAMPLING_ACCEL_ENABLED
	debug_log("SAMPLING: sampling_accelerometer_interrupt_callback\n");
	
	if((sampling_configuration & SAMPLING_ACCELEROMETER_INTERRUPT) == 0 && (sampling_configuration & STREAMING_ACCELEROMETER_INTERRUPT) == 0)
		return;
	
	
	if(sampling_configuration & SAMPLING_ACCELEROMETER_INTERRUPT) {	
		systick_get_timestamp(&(accelerometer_interrupt_chunk->timestamp.seconds), &(accelerometer_interrupt_chunk->timestamp.ms));
		
		sampling_finalize_accelerometer_interrupt_chunk();
	}
	
	
	if(sampling_configuration & STREAMING_ACCELEROMETER_INTERRUPT) {	// Put the elements on the stream if we want to
		AccelerometerInterruptStream accelerometer_interrupt_stream;
	
		systick_get_timestamp(&(accelerometer_interrupt_stream.timestamp.seconds), &(accelerometer_interrupt_stream.timestamp.ms));

		circular_fifo_write(&accelerometer_interrupt_stream_fifo, (uint8_t*) &accelerometer_interrupt_stream, sizeof(accelerometer_interrupt_stream));

	}	
	
	
	// Now start the reset timer that should reset the interrupt after a certain period of time.
	app_timer_start(sampling_accelerometer_interrupt_reset_timer, APP_TIMER_TICKS(accelerometer_interrupt_ignore_duration_ms, 0), NULL);
	#endif
}


/***************************** BATTERY ************************************/
ret_code_t sampling_start_battery(uint32_t timeout_ms, uint32_t period_ms, uint8_t streaming) {
	ret_code_t ret = NRF_SUCCESS;
	
	
	if(!streaming) {
		if((sampling_configuration & SAMPLING_BATTERY) == 0) { // Only stop and start the sampling if it is not already running			
			// Stop the sampling-timer that was probably already started
			app_timer_stop(sampling_battery_timer);
			
			sampling_setup_battery_chunk();
			
			// Now start the sampling-timer
			ret = app_timer_start(sampling_battery_timer, APP_TIMER_TICKS(period_ms, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;	
			
			sampling_configuration = (sampling_configuration_t) (sampling_configuration | SAMPLING_BATTERY);
			advertiser_set_status_flag_battery_enabled(1);
			
			timeout_start(battery_timeout_id, timeout_ms);
		}
	} else {
		// If we are not already sampling the battery, we have to start the sampling-timer
		if((sampling_configuration & SAMPLING_BATTERY) == 0) {
			// Stop the sampling-timer that was probably already started
			app_timer_stop(sampling_battery_timer);
			
			// Now start the sampling-timer
			ret = app_timer_start(sampling_battery_timer, APP_TIMER_TICKS(period_ms, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;	
		}
			
		sampling_configuration = (sampling_configuration_t) (sampling_configuration | STREAMING_BATTERY);
		timeout_start(battery_stream_timeout_id, timeout_ms);
	}
	
	return ret;
}

void sampling_stop_battery(uint8_t streaming) {
	
	// Check if we are allowed to disable the battery timer
	if(!streaming) {
		if((sampling_configuration & STREAMING_BATTERY) == 0) {
			app_timer_stop(sampling_battery_timer);
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(SAMPLING_BATTERY));
		advertiser_set_status_flag_battery_enabled(0);
	} else {
		if((sampling_configuration & SAMPLING_BATTERY) == 0) {
			app_timer_stop(sampling_battery_timer);			
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(STREAMING_BATTERY));
	}
	
}

void sampling_timeout_battery(void) {
	debug_log("SAMPLING: Battery timed out --> stopping\n");
	sampling_stop_battery(0);
}
void sampling_timeout_battery_stream(void) {
	debug_log("SAMPLING: Battery stream timed out --> stopping\n");
	sampling_stop_battery(1);
}

void sampling_battery_callback(void* p_context) {
	//debug_log("SAMPLING: sampling_battery_callback\n");
	
	if((sampling_configuration & SAMPLING_BATTERY) == 0 && (sampling_configuration & STREAMING_BATTERY) == 0)
		return;
	
	
	float voltage = battery_get_voltage();
	
	if(sampling_configuration & SAMPLING_BATTERY) {	
		systick_get_timestamp(&(battery_chunk->timestamp.seconds), &(battery_chunk->timestamp.ms));
		battery_chunk->battery_data.voltage = voltage;
		sampling_finalize_battery_chunk();
	}
	
	if(sampling_configuration & STREAMING_BATTERY) {	// Put the elements on the stream if we want to
		BatteryStream battery_stream;
		battery_stream.battery_data.voltage = voltage;		
		circular_fifo_write(&battery_stream_fifo, (uint8_t*) &battery_stream, sizeof(battery_stream));
	}		
}

void sampling_setup_battery_chunk(void) {
	// Open a chunk in the FIFO
	chunk_fifo_write_open(&battery_chunk_fifo, (void**) &battery_chunk, NULL);
	
}

void sampling_finalize_battery_chunk(void) {
	// Close the chunk in the FIFO
	chunk_fifo_write_close(&battery_chunk_fifo);
	
	sampling_setup_battery_chunk();
	
	app_sched_event_put(NULL, 0, processing_process_battery_chunk);
}



/************************** MICROPHONE ****************************/
ret_code_t sampling_start_microphone(uint32_t timeout_ms, uint16_t period_ms, uint8_t streaming) {
	ret_code_t ret = NRF_SUCCESS;
	
	// This needs to be setted, because sampling_setup_microphone_chunk() needs it
	microphone_period_ms = period_ms;
	
	if(!streaming) {
		if((sampling_configuration & SAMPLING_MICROPHONE) == 0) { // Only stop and start the sampling if it is not already running			
			// Stop the sampling-timer that was probably already started
			app_timer_stop(sampling_microphone_timer);
			app_timer_stop(sampling_microphone_aggregated_timer);
			
			sampling_setup_microphone_chunk();

			// Now start the sampling-timer
			ret = app_timer_start(sampling_microphone_timer, APP_TIMER_TICKS(period_ms, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;

			// Now start the average-sampling-timer
			ret = app_timer_start(sampling_microphone_aggregated_timer, APP_TIMER_TICKS(microphone_aggregated_period_ms, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;

			
			
			sampling_configuration = (sampling_configuration_t) (sampling_configuration | SAMPLING_MICROPHONE);
			advertiser_set_status_flag_microphone_enabled(1);
			
			timeout_start(microphone_timeout_id, timeout_ms);
		}
	} else {
		// If we are not already sampling the microphone, we have to start the sampling-timer
		if((sampling_configuration & SAMPLING_MICROPHONE) == 0) {
			
			// Stop the sampling-timer that was probably already started
			app_timer_stop(sampling_microphone_timer);
			app_timer_stop(sampling_microphone_aggregated_timer);
		
			// Now start the sampling-timer
			ret = app_timer_start(sampling_microphone_timer, APP_TIMER_TICKS(period_ms, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;

			// Now start the average-sampling-timer
			ret = app_timer_start(sampling_microphone_aggregated_timer, APP_TIMER_TICKS(microphone_aggregated_period_ms, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;
		}
			
		sampling_configuration = (sampling_configuration_t) (sampling_configuration | STREAMING_MICROPHONE);
		timeout_start(microphone_stream_timeout_id, timeout_ms);
	}
	
	
	return ret;
}

void sampling_stop_microphone(uint8_t streaming) {

	// Check if we are allowed to disable the microphone timers
	if(!streaming) {
		if((sampling_configuration & STREAMING_MICROPHONE) == 0) {
			app_timer_stop(sampling_microphone_timer);
			app_timer_stop(sampling_microphone_aggregated_timer);
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(SAMPLING_MICROPHONE));
		advertiser_set_status_flag_microphone_enabled(0);
	} else {
		if((sampling_configuration & SAMPLING_MICROPHONE) == 0) {
			app_timer_stop(sampling_microphone_timer);
			app_timer_stop(sampling_microphone_aggregated_timer);
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(STREAMING_MICROPHONE));
	}	
	
}

void sampling_timeout_microphone(void) {
	debug_log("SAMPLING: Microphone timed out --> stopping\n");
	sampling_stop_microphone(0);
}
void sampling_timeout_microphone_stream(void) {
	debug_log("SAMPLING: Microphone stream timed out --> stopping\n");
	sampling_stop_microphone(1);
}

void sampling_microphone_callback(void* p_context) {
	if(microphone_aggregated_count == 0) {
		debug_log("SAMPLING: Microphone aggregated count == 0!\n");
		return;
	}
	
	if(microphone_aggregated_count <= 5) {
		debug_log("SAMPLING: Microphone aggregated count <= 5. We need more samples!\n");
	}

	uint32_t tmp = (microphone_aggregated/(microphone_aggregated_count/2));
	uint8_t value = (tmp > 255) ? 255 : ((uint8_t) tmp);

	
	microphone_aggregated = 0;
	microphone_aggregated_count = 0;
	
	if(sampling_configuration & SAMPLING_MICROPHONE) {
		microphone_chunk->microphone_data[microphone_chunk->microphone_data_count].value = value;
		microphone_chunk->microphone_data_count++;
		//debug_log("SAMPLING: Mic value: %u, %u\n", value, microphone_chunk->microphone_data_count);
		if(microphone_chunk->microphone_data_count >= MICROPHONE_CHUNK_DATA_SIZE) {
			sampling_finalize_microphone_chunk();
		}
	}
	
	if(sampling_configuration & STREAMING_MICROPHONE) {
		MicrophoneStream microphone_stream;
		microphone_stream.microphone_data.value = value;
		circular_fifo_write(&microphone_stream_fifo, (uint8_t*) &microphone_stream, sizeof(microphone_stream));
	}
	
	
}

void sampling_microphone_aggregated_callback(void* p_context) {
	uint64_t end_ticks = systick_get_ticks_since_start() + APP_TIMER_TICKS(MICROPHONE_READING_WINDOW_MS, 0);
	while(end_ticks > systick_get_ticks_since_start()) {
		uint8_t value = 0;
		ret_code_t ret = microphone_read(&value);	
		if(ret != NRF_SUCCESS) continue;
		microphone_aggregated += value;
		microphone_aggregated_count++;
	}		
}

void sampling_setup_microphone_chunk(void) {
	debug_log("SAMPLING: sampling_setup_microphone_chunk\n");
	
	microphone_aggregated = 0;
	microphone_aggregated_count = 0;
	
	// Open a chunk in the FIFO
	chunk_fifo_write_open(&microphone_chunk_fifo, (void**) &microphone_chunk, NULL);
	
	systick_get_timestamp(&(microphone_chunk->timestamp.seconds), &(microphone_chunk->timestamp.ms));
	microphone_chunk->sample_period_ms = microphone_period_ms;
	microphone_chunk->microphone_data_count = 0;	
}

void sampling_finalize_microphone_chunk(void) {
	debug_log("SAMPLING: sampling_finalize_microphone_chunk\n");
	// Close the chunk in the FIFO
	chunk_fifo_write_close(&microphone_chunk_fifo);
	
	sampling_setup_microphone_chunk();		// Setup a new chunk
	
	app_sched_event_put(NULL, 0, processing_process_microphone_chunk);
}

/************************** SCAN *********************************/
ret_code_t sampling_start_scan(uint32_t timeout_ms, uint16_t period_seconds, uint16_t interval_ms, uint16_t window_ms, uint16_t duration_seconds, uint8_t group_filter, uint8_t aggregation_type, uint8_t streaming) {
	ret_code_t ret = NRF_SUCCESS;
	
	
	if(!streaming) {
		if((sampling_configuration & SAMPLING_SCAN) == 0) { // Only stop and start the sampling if it is not already running			
			// Stop the sampling-timer that was probably already started
			app_timer_stop(sampling_scan_timer);
			
			// Now start the sampling-timer
			ret = app_timer_start(sampling_scan_timer, APP_TIMER_TICKS(((uint32_t)period_seconds)*1000, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;	
			
			sampling_configuration = (sampling_configuration_t) (sampling_configuration | SAMPLING_SCAN);
			advertiser_set_status_flag_scan_enabled(1);
			
			timeout_start(scan_timeout_id, timeout_ms);
		}
	} else {
		// If we are not already sampling the scanner, we have to start the sampling-timer
		if((sampling_configuration & SAMPLING_SCAN) == 0) {
			// Stop the sampling-timer that was probably already started
			app_timer_stop(sampling_scan_timer);
			
			// Now start the sampling-timer
			ret = app_timer_start(sampling_scan_timer, APP_TIMER_TICKS(((uint32_t)period_seconds)*1000, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;	
		}
			
		sampling_configuration = (sampling_configuration_t) (sampling_configuration | STREAMING_SCAN);
		
		timeout_start(scan_stream_timeout_id, timeout_ms);
	}
	
	scan_interval_ms = interval_ms;
	scan_window_ms = window_ms;
	scan_duration_seconds = duration_seconds;
	scan_group_filter = group_filter;
	scan_aggregation_type = aggregation_type;
	
	
	
	return ret;	
}

void sampling_stop_scan(uint8_t streaming) {
	
	// Check if we are allowed to disable the scan timer
	if(!streaming) {
		if((sampling_configuration & STREAMING_SCAN) == 0) {
			app_timer_stop(sampling_scan_timer);
			scanner_stop_scanning();
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(SAMPLING_SCAN));
		advertiser_set_status_flag_scan_enabled(0);
	} else {
		if((sampling_configuration & SAMPLING_SCAN) == 0) {
			app_timer_stop(sampling_scan_timer);	
			scanner_stop_scanning();
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(STREAMING_SCAN));
	}
	
}

void sampling_timeout_scan(void) {
	debug_log("SAMPLING: Sampling timed out --> stopping\n");
	sampling_stop_scan(0);
}
void sampling_timeout_scan_stream(void) {
	debug_log("SAMPLING: Sampling stream timed out --> stopping\n");
	sampling_stop_scan(1);
}

void sampling_scan_callback(void* p_context) {
	if(sampling_configuration & SAMPLING_SCAN) {
		sampling_setup_scan_sampling_chunk();
	}
	// Start the scan procedure:
	scanner_start_scanning(scan_interval_ms, scan_window_ms, scan_duration_seconds);
}

void sampling_on_scan_timeout_callback(void) {
	if(sampling_configuration & SAMPLING_SCAN) {
		sampling_finalize_scan_sampling_chunk();
	}
}

void sampling_on_scan_report_callback(scanner_scan_report_t* scanner_scan_report) {
	debug_log("SAMPLING: Scan report: ID %u, RSSI: %d\n", scanner_scan_report->ID, scanner_scan_report->rssi);

	if(scan_group_filter != scan_no_group_filter_pattern) { // If we need to check the group
		if(scanner_scan_report->group != scan_group_filter) {
			return;
		}
	}
	
	if(sampling_configuration & SAMPLING_SCAN) {
	
		uint8_t prev_seen = 0;
		for(uint32_t i = 0; i < scan_sampling_chunk->scan_result_data_count; i++) {
			if(scan_sampling_chunk->scan_result_data[i].scan_device.ID == scanner_scan_report->ID) { // We already added it
				if(scan_sampling_chunk->scan_result_data[i].count < 255) { // Check if we haven't 255 counts for this device
					
					// Check which aggregation type to use:
					if(scan_aggregation_type == SCAN_CHUNK_AGGREGATE_TYPE_MAX) {
						scan_aggregated_rssi[i] = AGGREGATE_SCAN_SAMPLE_MAX(scan_aggregated_rssi[i], scanner_scan_report->rssi);
					} else {	// Use mean
						scan_aggregated_rssi[i] = AGGREGATE_SCAN_SAMPLE_MEAN(scan_aggregated_rssi[i], scanner_scan_report->rssi);
					}				
					scan_sampling_chunk->scan_result_data[i].count++;
				}
				prev_seen = 1;
				break;
			}
		}
		
		if(!prev_seen) {
			if(scan_sampling_chunk->scan_result_data_count < SCAN_SAMPLING_CHUNK_DATA_SIZE) {
				scan_aggregated_rssi[scan_sampling_chunk->scan_result_data_count] = scanner_scan_report->rssi;
				scan_sampling_chunk->scan_result_data[scan_sampling_chunk->scan_result_data_count].scan_device.ID = scanner_scan_report->ID;
				scan_sampling_chunk->scan_result_data[scan_sampling_chunk->scan_result_data_count].scan_device.rssi = 0;	// We could not set it here (it is aggregated)
				scan_sampling_chunk->scan_result_data[scan_sampling_chunk->scan_result_data_count].count = 1;
				scan_sampling_chunk->scan_result_data_count++;	
			}
		}
	}
	
	if(sampling_configuration & STREAMING_SCAN) {
		ScanStream scan_stream;
		scan_stream.scan_device.ID = scanner_scan_report->ID;
		scan_stream.scan_device.rssi = scanner_scan_report->rssi;
		circular_fifo_write(&scan_stream_fifo, (uint8_t*) &scan_stream, sizeof(scan_stream));
	}
	
	
}

void sampling_setup_scan_sampling_chunk(void) {
	
	debug_log("SAMPLING: sampling_setup_scan_sampling_chunk\n");
	// Open a chunk in the FIFO
	chunk_fifo_write_open(&scan_sampling_chunk_fifo, (void**) &scan_sampling_chunk, NULL);
	
	systick_get_timestamp(&(scan_sampling_chunk->timestamp.seconds), &(scan_sampling_chunk->timestamp.ms)); 
	scan_sampling_chunk->scan_result_data_count = 0;

	
}

void sampling_finalize_scan_sampling_chunk(void) {

	debug_log("SAMPLING: sampling_finalize_scan_sampling_chunk\n");
	// Now interpret/Process the aggregated rssi values:
	for(uint32_t i = 0; i < scan_sampling_chunk->scan_result_data_count; i++) {
		if(scan_aggregation_type == SCAN_CHUNK_AGGREGATE_TYPE_MAX) { // Use max
			scan_sampling_chunk->scan_result_data[i].scan_device.rssi = (int8_t) PROCESS_SCAN_SAMPLE_MAX(scan_aggregated_rssi[i], (int32_t)scan_sampling_chunk->scan_result_data[i].count);
		} else {	// Use mean
			scan_sampling_chunk->scan_result_data[i].scan_device.rssi = (int8_t) PROCESS_SCAN_SAMPLE_MEAN(scan_aggregated_rssi[i], (int32_t)scan_sampling_chunk->scan_result_data[i].count);
		}
		

		//debug_log("SAMPLING: Scanning ended: ID %u, RSSI %d, count %u\n", scan_sampling_chunk->scan_result_data[i].scan_device.ID, scan_sampling_chunk->scan_result_data[i].scan_device.rssi, scan_sampling_chunk->scan_result_data[i].count);
		//systick_delay_millis(100);
	}
	//debug_log("SAMPLING: Scanning ended. Seen devices: %u\n", scan_sampling_chunk->scan_result_data_count);
	
	// Close the chunk in the FIFO
	chunk_fifo_write_close(&scan_sampling_chunk_fifo);
	
	
	app_sched_event_put(NULL, 0, processing_process_scan_sampling_chunk);

}

