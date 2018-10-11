#include "request_handler_lib_02v1.h"

#ifdef PROTOCOL_02v1


#include <string.h>
#include "app_fifo.h"
#include "app_scheduler.h"
#include "sender_lib.h"
#include "systick_lib.h"
#include "storer_lib.h"
#include "sampling_lib.h"
#include "advertiser_lib.h"	// To retrieve the current badge-assignement and set the clock-sync status
#include "battery_lib.h"
#include "accel_lib.h"	// For ACCELEROMETER_PRESENT

#include "debug_lib.h"

#include "tinybuf.h"
#include "protocol_messages_02v1.h"
#include "chunk_messages.h"
#include "selftest_lib.h"

#ifndef UNIT_TEST
#include "custom_board.h"
#include "nrf_gpio.h"
#endif

#define RECEIVE_NOTIFICATION_FIFO_SIZE					256		/**< Buffer size for the receive-notification FIFO. Has to be a power of two */
#define AWAIT_DATA_TIMEOUT_MS							1000
#define TRANSMIT_DATA_TIMEOUT_MS						100
#define REQUEST_HANDLER_SERIALIZED_BUFFER_SIZE			512		
#define RESPONSE_MAX_TRANSMIT_RETRIES					50		



typedef struct {
	uint64_t 	request_timepoint_ticks;
	Request 	request;
} request_event_t;

typedef struct {
	uint32_t					response_retries;
	app_sched_event_handler_t	response_success_handler;	/**< Scheduler function that should be called, after the reponse was transmitted successfully, to queue some other reponses */
	app_sched_event_handler_t	response_fail_handler;		/**< Scheduler function that should be called, if the response could not be transmitted */
	Response					response;
} response_event_t;


//typedef void (* request_handler_t)(request_event_t* request_event);
typedef void (* request_handler_t)(void * p_event_data, uint16_t event_size);

typedef struct {
    uint8_t type;
    request_handler_t handler;
} request_handler_for_type_t;




static request_event_t request_event;	/**< Needed a reponse event, to put the timepoint ticks into the structure */
static response_event_t	response_event; /**< Needed a reponse event, to put the reties and the function to call after success into the structure */
static Timestamp		response_timestamp; /**< Needed for the status-, and start-requests */
static uint8_t			response_clock_status;	/**< Needed for the status-, and start-requests */

static app_fifo_t receive_notification_fifo;
static uint8_t receive_notification_buf[RECEIVE_NOTIFICATION_FIFO_SIZE];
static volatile uint8_t processing_receive_notification = 0;			/**< Flag that represents if the processing of a receive notification is still running. */
static volatile uint8_t processing_response = 0;						/**< Flag that represents if the processing of response is still running. */
static volatile uint8_t streaming_started = 0;							/**< Flag that represents if streaming is currently running. */

static uint8_t serialized_buf[REQUEST_HANDLER_SERIALIZED_BUFFER_SIZE];

/**< Store-messages to represent how the data should be stored (global because needed more than one) */
static MicrophoneChunk 				microphone_chunk;
static ScanChunk 					scan_chunk;
static AccelerometerChunk 			accelerometer_chunk;
static AccelerometerInterruptChunk 	accelerometer_interrupt_chunk;
static BatteryChunk 				battery_chunk;



static void receive_notification_handler(receive_notification_t receive_notification);
static void process_receive_notification(void * p_event_data, uint16_t event_size);


static void status_request_handler(void * p_event_data, uint16_t event_size);
static void start_microphone_request_handler(void * p_event_data, uint16_t event_size);
static void stop_microphone_request_handler(void * p_event_data, uint16_t event_size);
static void start_scan_request_handler(void * p_event_data, uint16_t event_size);
static void stop_scan_request_handler(void * p_event_data, uint16_t event_size);
static void start_accelerometer_request_handler(void * p_event_data, uint16_t event_size);
static void stop_accelerometer_request_handler(void * p_event_data, uint16_t event_size);
static void start_accelerometer_interrupt_request_handler(void * p_event_data, uint16_t event_size);
static void stop_accelerometer_interrupt_request_handler(void * p_event_data, uint16_t event_size);
static void start_battery_request_handler(void * p_event_data, uint16_t event_size);
static void stop_battery_request_handler(void * p_event_data, uint16_t event_size);
static void microphone_data_request_handler(void * p_event_data, uint16_t event_size);
static void scan_data_request_handler(void * p_event_data, uint16_t event_size);
static void accelerometer_data_request_handler(void * p_event_data, uint16_t event_size);
static void accelerometer_interrupt_data_request_handler(void * p_event_data, uint16_t event_size);
static void battery_data_request_handler(void * p_event_data, uint16_t event_size);
static void start_microphone_stream_request_handler(void * p_event_data, uint16_t event_size);
static void stop_microphone_stream_request_handler(void * p_event_data, uint16_t event_size);
static void start_scan_stream_request_handler(void * p_event_data, uint16_t event_size);
static void stop_scan_stream_request_handler(void * p_event_data, uint16_t event_size);
static void start_accelerometer_stream_request_handler(void * p_event_data, uint16_t event_size);
static void stop_accelerometer_stream_request_handler(void * p_event_data, uint16_t event_size);
static void start_accelerometer_interrupt_stream_request_handler(void * p_event_data, uint16_t event_size);
static void stop_accelerometer_interrupt_stream_request_handler(void * p_event_data, uint16_t event_size);
static void start_battery_stream_request_handler(void * p_event_data, uint16_t event_size);
static void stop_battery_stream_request_handler(void * p_event_data, uint16_t event_size);
static void identify_request_handler(void * p_event_data, uint16_t event_size);
static void test_request_handler(void * p_event_data, uint16_t event_size);


static void status_response_handler(void * p_event_data, uint16_t event_size);
static void start_microphone_response_handler(void * p_event_data, uint16_t event_size);
static void start_scan_response_handler(void * p_event_data, uint16_t event_size);
static void start_accelerometer_response_handler(void * p_event_data, uint16_t event_size);
static void start_accelerometer_interrupt_response_handler(void * p_event_data, uint16_t event_size);
static void start_battery_response_handler(void * p_event_data, uint16_t event_size);
static void microphone_data_response_handler(void * p_event_data, uint16_t event_size);
static void scan_data_response_handler(void * p_event_data, uint16_t event_size);
static void accelerometer_data_response_handler(void * p_event_data, uint16_t event_size);
static void accelerometer_interrupt_data_response_handler(void * p_event_data, uint16_t event_size);
static void battery_data_response_handler(void * p_event_data, uint16_t event_size);
static void stream_response_handler(void * p_event_data, uint16_t event_size);
static void test_response_handler(void * p_event_data, uint16_t event_size);


static request_handler_for_type_t request_handlers[] = {
        {
                .type = Request_status_request_tag,
                .handler = status_request_handler,
        },
        {
                .type = Request_start_microphone_request_tag,
                .handler = start_microphone_request_handler,
        },
		{
                .type = Request_stop_microphone_request_tag,
                .handler = stop_microphone_request_handler,
        },
        {
                .type = Request_start_scan_request_tag,
                .handler = start_scan_request_handler,
        },
        {
                .type = Request_stop_scan_request_tag,
                .handler = stop_scan_request_handler,
        },
		{
                .type = Request_start_accelerometer_request_tag,
                .handler = start_accelerometer_request_handler,
        },
        {
                .type = Request_stop_accelerometer_request_tag,
                .handler = stop_accelerometer_request_handler,
        },
		{
                .type = Request_start_accelerometer_interrupt_request_tag,
                .handler = start_accelerometer_interrupt_request_handler,
        },
        {
                .type = Request_stop_accelerometer_interrupt_request_tag,
                .handler = stop_accelerometer_interrupt_request_handler,
        },
		{
                .type = Request_start_battery_request_tag,
                .handler = start_battery_request_handler,
        },
        {
                .type = Request_stop_battery_request_tag,
                .handler = stop_battery_request_handler,
        },
        {
                .type = Request_microphone_data_request_tag,
                .handler = microphone_data_request_handler,
        },
        {
                .type = Request_scan_data_request_tag,
                .handler = scan_data_request_handler,
        },
		{
                .type = Request_accelerometer_data_request_tag,
                .handler = accelerometer_data_request_handler,
        },
		{
                .type = Request_accelerometer_interrupt_data_request_tag,
                .handler = accelerometer_interrupt_data_request_handler,
        },
		{
                .type = Request_battery_data_request_tag,
                .handler = battery_data_request_handler,
        },		
		{
                .type = Request_start_microphone_stream_request_tag,
                .handler = start_microphone_stream_request_handler,
        },
		{
                .type = Request_stop_microphone_stream_request_tag,
                .handler = stop_microphone_stream_request_handler,
        },
        {
                .type = Request_start_scan_stream_request_tag,
                .handler = start_scan_stream_request_handler,
        },
        {
                .type = Request_stop_scan_stream_request_tag,
                .handler = stop_scan_stream_request_handler,
        },
		{
                .type = Request_start_accelerometer_stream_request_tag,
                .handler = start_accelerometer_stream_request_handler,
        },
        {
                .type = Request_stop_accelerometer_stream_request_tag,
                .handler = stop_accelerometer_stream_request_handler,
        },
		{
                .type = Request_start_accelerometer_interrupt_stream_request_tag,
                .handler = start_accelerometer_interrupt_stream_request_handler,
        },
        {
                .type = Request_stop_accelerometer_interrupt_stream_request_tag,
                .handler = stop_accelerometer_interrupt_stream_request_handler,
        },
		{
                .type = Request_start_battery_stream_request_tag,
                .handler = start_battery_stream_request_handler,
        },
        {
                .type = Request_stop_battery_stream_request_tag,
                .handler = stop_battery_stream_request_handler,
        },
		
		
        {
                .type = Request_identify_request_tag,
                .handler = identify_request_handler,
        },
		{
                .type = Request_test_request_tag,
                .handler = test_request_handler,
        }
};



ret_code_t request_handler_init(void) {
	ret_code_t ret = sender_init();
	if(ret != NRF_SUCCESS) return ret;
	
	sender_set_receive_notification_handler(receive_notification_handler);
	
	ret = app_fifo_init(&receive_notification_fifo, receive_notification_buf, sizeof(receive_notification_buf));
	if(ret != NRF_SUCCESS) return ret;
	
	return NRF_SUCCESS;
}

/**@brief Handler that is called  by the sender-module, when a notification was received.
 * 
 * @details It puts the received notification in the notification-fifo and schedules the process_receive_notification to process the notification.
 */
static void receive_notification_handler(receive_notification_t receive_notification) {
	// Put the receive_notification into the fifo
	uint32_t available_len = 0;
	uint32_t notification_size = sizeof(receive_notification);
	app_fifo_write(&receive_notification_fifo, NULL, &available_len);
	if(available_len < notification_size) {
		debug_log_bkgnd("Not enough bytes in Notification FIFO: %u < %u\n", available_len, notification_size);
		return;
	}
	
	app_fifo_write(&receive_notification_fifo, (uint8_t*) &receive_notification, &notification_size);
	
	app_sched_event_put(NULL, 0, process_receive_notification);

}

static void finish_receive_notification(void) {
	processing_receive_notification = 0;
}

static void finish_and_reschedule_receive_notification(void) {
	processing_receive_notification = 0;
	app_sched_event_put(NULL, 0, process_receive_notification);
}

static ret_code_t start_receive_notification(app_sched_event_handler_t reschedule_handler) {
	if(processing_receive_notification) {
		app_sched_event_put(NULL, 0, reschedule_handler);
		return NRF_ERROR_BUSY;
	}
	processing_receive_notification = 1;
	return NRF_SUCCESS;
}


static void finish_response(void) {
	processing_response = 0;
}

static ret_code_t start_response(app_sched_event_handler_t reschedule_handler) {
	if(processing_response) {	// Check if we are allowed to prepare and send our response, if not reschedule the response-handler again
		app_sched_event_put(NULL, 0, reschedule_handler);
		return NRF_ERROR_BUSY;
	}
	processing_response = 1;
	return NRF_SUCCESS;
}


// Called when await data failed, or decoding the notification failed, or request does not exist, or transmitting the response failed (because disconnected or something else)
static void finish_error(void) {
	app_fifo_flush(&receive_notification_fifo);
	debug_log_bkgnd("Error while processing notification/request --> Disconnect!!!\n");
	sender_disconnect();	// To clear the RX- and TX-FIFO
	finish_receive_notification();
	finish_response();
	streaming_started = 0;
	storer_invalidate_iterators();
}


static ret_code_t receive_notification_fifo_peek(receive_notification_t* receive_notification, uint32_t index) {
	uint32_t notification_size = sizeof(receive_notification_t);
	uint32_t available_size;
	app_fifo_read(&receive_notification_fifo, NULL, &available_size);
	if(available_size < notification_size*(index+1)) {
		return NRF_ERROR_NOT_FOUND;
	}
	
	uint8_t tmp[notification_size];
	for(uint32_t i = 0; i < notification_size; i++) {
		app_fifo_peek(&receive_notification_fifo, i + index*notification_size, &(tmp[i]));
	}
	
	memcpy(receive_notification, tmp, notification_size);
	return NRF_SUCCESS;
}

static void receive_notification_fifo_consume(void) {
	uint32_t notification_size = sizeof(receive_notification_t);
	uint8_t tmp[notification_size];
	app_fifo_read(&receive_notification_fifo, tmp, &notification_size);
}

static void receive_notification_fifo_set_receive_notification(receive_notification_t* receive_notification, uint32_t index) {
	uint32_t notification_size = sizeof(receive_notification_t);
	uint32_t available_size;
	app_fifo_read(&receive_notification_fifo, NULL, &available_size);
	if(available_size < notification_size*(index+1)) {
		return;
	}
	uint8_t tmp[notification_size];
	memcpy(tmp, receive_notification, notification_size);
	// Manually set the bytes of the notification in the FIFO
	uint32_t start_index = receive_notification_fifo.read_pos + index*notification_size;
	for(uint32_t i = 0; i < notification_size; i++) {
		receive_notification_fifo.p_buf[(start_index + i) & receive_notification_fifo.buf_size_mask] = tmp[i];
	}	
}

/**
 * This function could handle multiple queued receive notifications.
 * It searches for the length of the packet and then splits the 
 * queued receive notifications that belongs to the actual whole packet
 * and removes the receive notificatins from the queue.
 *
 * Then it calls the corresponding request handler.
 * If the request-handler is done, it finishes the processing of the receive notification, 
 * and reschedules the processing of new receive notifications.
 */
static void process_receive_notification(void * p_event_data, uint16_t event_size) {
	// Check if we can start the processing of the receive notification, otherwise reschedule
	if(start_receive_notification(process_receive_notification) != NRF_SUCCESS)
		return;

	// Get the first receive-notification in the FIFO. If there is no in the queue --> we are finish with processing receive notifications.
	receive_notification_t receive_notification;
	ret_code_t ret = receive_notification_fifo_peek(&receive_notification, 0);
	if(ret == NRF_ERROR_NOT_FOUND) {
		app_fifo_flush(&receive_notification_fifo);
		finish_receive_notification();
		return;
	}
	
	// Get the timestamp and clock-sync status before processing the request!
	systick_get_timestamp(&response_timestamp.seconds, &response_timestamp.ms); 
	response_clock_status = systick_is_synced();
	
	uint64_t timepoint_ticks = receive_notification.timepoint_ticks;
	
	// Wait for the length header bytes
	uint8_t length_header[2];
	ret = sender_await_data(length_header, 2, AWAIT_DATA_TIMEOUT_MS);
	if(ret != NRF_SUCCESS) {
		debug_log("sender_await_data() error for length header\n");
		app_fifo_flush(&receive_notification_fifo);
		finish_error();
		return;
	}
	
	uint16_t len = (((uint16_t)length_header[0]) << 8) | ((uint16_t)length_header[1]);
	
	// Now wait for the actual data
	ret = sender_await_data(serialized_buf, len, AWAIT_DATA_TIMEOUT_MS);
	if(ret != NRF_SUCCESS) {
		debug_log("sender_await_data() error\n");
		app_fifo_flush(&receive_notification_fifo);
		finish_error();
		return;
	}
	
	// Here we assume that we got enough receive notifications to receive all the data,
	// so we need to consume all notifications in the notification-fifo that corresponds to this data
	uint16_t consume_len = len + 2;	// + 2 for the header
	uint32_t consume_index = 0;
	while(consume_len > 0) {
		ret = receive_notification_fifo_peek(&receive_notification, consume_index);
		if(ret == NRF_ERROR_NOT_FOUND) {
			app_fifo_flush(&receive_notification_fifo);
			finish_error();
			return;
		}
		if(receive_notification.notification_len <= consume_len) {
			consume_len -= receive_notification.notification_len;
			// Increment the consume_index here, to consume the right number of notifications in the end
			consume_index++;
		} else {			
			// Adapt the notification-len of the consume_index-th notification
			receive_notification.notification_len = receive_notification.notification_len - consume_len;
			//debug_log("Adapt %u. notification len to %u\n", consume_index, receive_notification.notification_len);
			receive_notification_fifo_set_receive_notification(&receive_notification, consume_index);
			// Set consume len to 0
			consume_len = 0;
		}
	}
	//debug_log("Consume %u notifications\n", consume_index);
	
	// Now manually consume the notifications
	for(uint32_t i = 0; i < consume_index; i++) {
		receive_notification_fifo_consume();
	}
	
	
	// Now decode the serialized notification
	memset(&(request_event.request), 0, sizeof(request_event.request));
	tb_istream_t istream = tb_istream_from_buffer(serialized_buf, len);
	uint8_t decode_status = tb_decode(&istream, Request_fields, &(request_event.request), TB_BIG_ENDIAN);
	if(decode_status == 0) {
		debug_log("Error decoding\n");
		finish_error();
		return;
	}
	debug_log("Decoded successfully\n");
	
	if(istream.bytes_read < len) {
		debug_log("Warning decoding: %u bytes have not been read.\n", len - istream.bytes_read);
	}
	
	request_event.request_timepoint_ticks = timepoint_ticks;

	
	debug_log("Which request type: %u, Ticks: %u\n", request_event.request.which_type, request_event.request_timepoint_ticks);
	
	request_handler_t request_handler = NULL;
	for(uint8_t i = 0; i < sizeof(request_handlers)/sizeof(request_handler_for_type_t); i++) {
		if(request_event.request.which_type == request_handlers[i].type) {
			request_handler = request_handlers[i].handler;
			break;
		}
		
	}
	if(request_handler != NULL) {
		
		// If we have received a request, we know that the hub is still active --> reset all timeouts of the sampling
		sampling_reset_timeouts();
		
		// These request-handlers finish the processing of the receive notification
		request_handler(NULL, 0);
	} else {
		// Should actually not happen, but to be sure...
		debug_log("Have not found a corresponding request handler for which_type: %u\n", request_event.request.which_type);
		finish_error();
	}
}


static void send_response(void * p_event_data, uint16_t event_size) {
	// On transmit failure, reschedule itself
	response_event.response_fail_handler = send_response;
	
	// Check if we already had too much retries
	if(response_event.response_retries > RESPONSE_MAX_TRANSMIT_RETRIES) {
		finish_error();
		return;
	}
	
	response_event.response_retries++;
	
	// Encoding has not to be done every time, but to don't have an extra function for that, we just do it here..


	tb_ostream_t ostream = tb_ostream_from_buffer(&serialized_buf[2], sizeof(serialized_buf) - 2);
	uint8_t encode_status = tb_encode(&ostream, Response_fields, &(response_event.response), TB_BIG_ENDIAN);
	uint32_t len = ostream.bytes_written;
	
	
	
	
	if(encode_status == 0) {
		debug_log("Error encoding response, len: %u!\n", len);
		finish_error();
		return;
	}
	
	
	
	ret_code_t ret = NRF_SUCCESS;

	serialized_buf[0] = (uint8_t)((((uint16_t) len) & 0xFF00) >> 8);
	serialized_buf[1] = (uint8_t)(((uint16_t) len) & 0xFF);
	ret = sender_transmit(serialized_buf, len+2, TRANSMIT_DATA_TIMEOUT_MS);	
	
	debug_log("Transmit status %u!\n", ret);
	
	
	if(ret == NRF_SUCCESS) {
		
		if(response_event.response_success_handler != NULL) {
			app_sched_event_put(NULL, 0, response_event.response_success_handler);
		} 
		
		finish_response();	// We are now done with this reponse
	
	} else {
		if(ret == NRF_ERROR_NO_MEM)	// Only reschedule a fail handler, if we could not transmit because of memory problems
			if(response_event.response_fail_handler != NULL) {	// This function should actually always be set..
				app_sched_event_put(NULL, 0, response_event.response_fail_handler);
				// Here we are not done with the response, so don't call the finish_response()-function
				return;
			}
		// Some BLE-problems occured (e.g. disconnected...)
		finish_error();
	}
}


/**
 Every request-handler has an response-handler!
*/





/**< These are the response handlers, that are called by the request-handlers. */

static void status_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(status_response_handler) != NRF_SUCCESS)
		return;
	

	response_event.response.which_type = Response_status_response_tag;
	response_event.response.type.status_response.clock_status = response_clock_status;
	response_event.response.type.status_response.microphone_status = (sampling_get_sampling_configuration() & SAMPLING_MICROPHONE) ? 1 : 0;
	response_event.response.type.status_response.scan_status = (sampling_get_sampling_configuration() & SAMPLING_SCAN) ? 1 : 0; 
	response_event.response.type.status_response.accelerometer_status = (sampling_get_sampling_configuration() & SAMPLING_ACCELEROMETER) ? 1 : 0; 
	response_event.response.type.status_response.accelerometer_interrupt_status = (sampling_get_sampling_configuration() & SAMPLING_ACCELEROMETER_INTERRUPT) ? 1 : 0; 
	response_event.response.type.status_response.battery_status = (sampling_get_sampling_configuration() & SAMPLING_BATTERY) ? 1 : 0; 
	response_event.response.type.status_response.timestamp = response_timestamp;
	battery_read_voltage(&(response_event.response.type.status_response.battery_data.voltage));
	
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
	send_response(NULL, 0);	
	
}

static void start_microphone_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(start_microphone_response_handler) != NRF_SUCCESS)
		return;
	
	
	response_event.response.which_type = Response_start_microphone_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	response_event.response.type.start_microphone_response.timestamp = response_timestamp;
	
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
	send_response(NULL, 0);	
}


static void start_scan_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(start_scan_response_handler) != NRF_SUCCESS)
		return;
	
	response_event.response.which_type = Response_start_scan_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	response_event.response.type.start_scan_response.timestamp = response_timestamp;
	
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
	send_response(NULL, 0);	
}

static void start_accelerometer_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(start_accelerometer_response_handler) != NRF_SUCCESS)
		return;
	
	response_event.response.which_type = Response_start_accelerometer_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	response_event.response.type.start_accelerometer_response.timestamp = response_timestamp;
	
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
	send_response(NULL, 0);	
}

static void start_accelerometer_interrupt_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(start_accelerometer_interrupt_response_handler) != NRF_SUCCESS)
		return;
	
	response_event.response.which_type = Response_start_accelerometer_interrupt_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	response_event.response.type.start_accelerometer_interrupt_response.timestamp = response_timestamp;
	
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
	send_response(NULL, 0);	
}


static void start_battery_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(start_battery_response_handler) != NRF_SUCCESS)
		return;
	
	response_event.response.which_type = Response_start_battery_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	response_event.response.type.start_battery_response.timestamp = response_timestamp;
	
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
	send_response(NULL, 0);	
}


static void microphone_data_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(microphone_data_response_handler) != NRF_SUCCESS)
		return;
	
	response_event.response.which_type = Response_microphone_data_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = microphone_data_response_handler;
	

	ret_code_t ret = storer_get_next_microphone_chunk(&microphone_chunk);
	if(ret == NRF_SUCCESS) {
		debug_log("Found microphone data..\n");
		// Send microphone data
		response_event.response.type.microphone_data_response.last_response = 0;
		response_event.response.type.microphone_data_response.timestamp = microphone_chunk.timestamp;
		response_event.response.type.microphone_data_response.sample_period_ms = microphone_chunk.sample_period_ms;
		uint32_t microphone_data_count = (microphone_chunk.microphone_data_count > PROTOCOL_MICROPHONE_DATA_SIZE) ? PROTOCOL_MICROPHONE_DATA_SIZE : microphone_chunk.microphone_data_count;
		response_event.response.type.microphone_data_response.microphone_data_count = microphone_data_count;
		memcpy(response_event.response.type.microphone_data_response.microphone_data, microphone_chunk.microphone_data, microphone_data_count*sizeof(MicrophoneData));
		
		send_response(NULL, 0);	
	} else if(ret == NRF_ERROR_NOT_FOUND || ret == NRF_ERROR_INVALID_STATE) {
		debug_log("Could not fetch Mic-data. Sending end Header..\n");
		response_event.response.type.microphone_data_response.last_response = 1;
		response_event.response.type.microphone_data_response.microphone_data_count = 0;
		
		// Send end-header
		response_event.response_success_handler = NULL;
		send_response(NULL, 0);	
	} else {
		app_sched_event_put(NULL, 0, microphone_data_response_handler);
	}
}


static void scan_data_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(scan_data_response_handler) != NRF_SUCCESS)
		return;
	
	response_event.response.which_type = Response_scan_data_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = scan_data_response_handler;
	

	
	ret_code_t ret = storer_get_next_scan_chunk(&scan_chunk);
	if(ret == NRF_SUCCESS) {
		debug_log("Found scan data..\n");
		// Send scan data
		response_event.response.type.scan_data_response.last_response = 0;
		response_event.response.type.scan_data_response.timestamp = scan_chunk.timestamp;
		uint32_t scan_result_data_count = (scan_chunk.scan_result_data_count > PROTOCOL_SCAN_DATA_SIZE) ? PROTOCOL_SCAN_DATA_SIZE : scan_chunk.scan_result_data_count;
		response_event.response.type.scan_data_response.scan_result_data_count = scan_result_data_count;
		memcpy(response_event.response.type.scan_data_response.scan_result_data, scan_chunk.scan_result_data, scan_result_data_count*sizeof(ScanResultData));
		
		
		send_response(NULL, 0);	
	} else if(ret == NRF_ERROR_NOT_FOUND || ret == NRF_ERROR_INVALID_STATE) {
		debug_log("Could not fetch Scan-data. Sending end Header..\n");
		response_event.response.type.scan_data_response.last_response = 1;
		response_event.response.type.scan_data_response.scan_result_data_count = 0;
		
		// Send end-header
		response_event.response_success_handler = NULL;
		send_response(NULL, 0);	
	} else {
		app_sched_event_put(NULL, 0, scan_data_response_handler);
	}
}


static void accelerometer_data_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(accelerometer_data_response_handler) != NRF_SUCCESS)
		return;
	
	response_event.response.which_type = Response_accelerometer_data_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = accelerometer_data_response_handler;
	
	
	ret_code_t ret = storer_get_next_accelerometer_chunk(&accelerometer_chunk);
	if(ret == NRF_SUCCESS) {
		debug_log("Found accelerometer data..\n");
		// Send accelerometer data
		response_event.response.type.accelerometer_data_response.last_response = 0;
		response_event.response.type.accelerometer_data_response.timestamp = accelerometer_chunk.timestamp;
		uint32_t accelerometer_data_count = (accelerometer_chunk.accelerometer_data_count > PROTOCOL_ACCELEROMETER_DATA_SIZE) ? PROTOCOL_ACCELEROMETER_DATA_SIZE : accelerometer_chunk.accelerometer_data_count;
		response_event.response.type.accelerometer_data_response.accelerometer_data_count = accelerometer_data_count;
		memcpy(response_event.response.type.accelerometer_data_response.accelerometer_data, accelerometer_chunk.accelerometer_data, accelerometer_data_count*sizeof(AccelerometerData));
		
		
		send_response(NULL, 0);	
	} else if(ret == NRF_ERROR_NOT_FOUND || ret == NRF_ERROR_INVALID_STATE) {
		debug_log("Could not fetch accelerometer-data. Sending end Header..\n");
		response_event.response.type.accelerometer_data_response.last_response = 1;
		response_event.response.type.accelerometer_data_response.accelerometer_data_count = 0;
		
		// Send end-header
		response_event.response_success_handler = NULL;
		send_response(NULL, 0);	
	} else {
		app_sched_event_put(NULL, 0, accelerometer_data_response_handler);
	}	
}


static void accelerometer_interrupt_data_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(accelerometer_interrupt_data_response_handler) != NRF_SUCCESS)
		return;
	
	response_event.response.which_type = Response_accelerometer_interrupt_data_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = accelerometer_interrupt_data_response_handler;
	
	
	ret_code_t ret = storer_get_next_accelerometer_interrupt_chunk(&accelerometer_interrupt_chunk);
	if(ret == NRF_SUCCESS) {
		debug_log("Found accelerometer interrupt data..\n");
		// Send accelerometer interrupt data
		response_event.response.type.accelerometer_interrupt_data_response.last_response = 0;
		response_event.response.type.accelerometer_interrupt_data_response.timestamp = accelerometer_interrupt_chunk.timestamp;
		
		send_response(NULL, 0);	
	} else if(ret == NRF_ERROR_NOT_FOUND || ret == NRF_ERROR_INVALID_STATE) {
		debug_log("Could not fetch accelerometer interrupt-data. Sending end Header..\n");
		response_event.response.type.accelerometer_interrupt_data_response.last_response = 1;
		
		// Send end-header
		response_event.response_success_handler = NULL;
		send_response(NULL, 0);	
	} else {
		app_sched_event_put(NULL, 0, accelerometer_interrupt_data_response_handler);
	}	
}


static void battery_data_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(battery_data_response_handler) != NRF_SUCCESS)
		return;

	response_event.response.which_type = Response_battery_data_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = battery_data_response_handler;
	
	ret_code_t ret = storer_get_next_battery_chunk(&battery_chunk);
	if(ret == NRF_SUCCESS) {
		debug_log("Found battery data..\n");
		// Send accelerometer interrupt data
		response_event.response.type.battery_data_response.last_response = 0;
		response_event.response.type.battery_data_response.timestamp = battery_chunk.timestamp;
		response_event.response.type.battery_data_response.battery_data = battery_chunk.battery_data;
		
		send_response(NULL, 0);	
	} else if(ret == NRF_ERROR_NOT_FOUND || ret == NRF_ERROR_INVALID_STATE) {
		debug_log("Could not fetch battery-data. Sending end Header..\n");
		response_event.response.type.battery_data_response.last_response = 1;
		
		// Send end-header
		response_event.response_success_handler = NULL;
		send_response(NULL, 0);	
	} else {
		app_sched_event_put(NULL, 0, battery_data_response_handler);
	}	
}

static void stream_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(stream_response_handler) != NRF_SUCCESS)
		return;
	
		
	response_event.response.which_type = Response_stream_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	
	response_event.response.type.stream_response.battery_stream_count = 0;
	response_event.response.type.stream_response.microphone_stream_count = 0;
	response_event.response.type.stream_response.scan_stream_count = 0;
	response_event.response.type.stream_response.accelerometer_stream_count = 0;
	response_event.response.type.stream_response.accelerometer_interrupt_stream_count = 0;
	
	systick_get_timestamp(&response_event.response.type.stream_response.timestamp.seconds, &response_event.response.type.stream_response.timestamp.ms);
	
	sampling_configuration_t sampling_configuration = sampling_get_sampling_configuration();
	#if ACCELEROMETER_PRESENT
	if(sampling_configuration & STREAMING_ACCELEROMETER) {
		uint32_t n = circular_fifo_get_size(&accelerometer_stream_fifo) / sizeof(AccelerometerStream);
		n = n > PROTOCOL_ACCELEROMETER_STREAM_SIZE ? PROTOCOL_ACCELEROMETER_STREAM_SIZE : n;
		response_event.response.type.stream_response.accelerometer_stream_count = n;
		uint32_t len = n*sizeof(AccelerometerStream);
		circular_fifo_read(&accelerometer_stream_fifo, (uint8_t*) response_event.response.type.stream_response.accelerometer_stream, &len);
	}
	
	if(sampling_configuration & STREAMING_ACCELEROMETER_INTERRUPT) {
		uint32_t n = circular_fifo_get_size(&accelerometer_interrupt_stream_fifo) / sizeof(AccelerometerInterruptStream);
		n = n > PROTOCOL_ACCELEROMETER_INTERRUPT_STREAM_SIZE ? PROTOCOL_ACCELEROMETER_INTERRUPT_STREAM_SIZE : n;
		response_event.response.type.stream_response.accelerometer_interrupt_stream_count = n;
		uint32_t len = n*sizeof(AccelerometerInterruptStream);
		circular_fifo_read(&accelerometer_interrupt_stream_fifo, (uint8_t*) response_event.response.type.stream_response.accelerometer_interrupt_stream, &len);
	}
	#endif
	
	if(sampling_configuration & STREAMING_BATTERY) {
		uint32_t n = circular_fifo_get_size(&battery_stream_fifo) / sizeof(BatteryStream);
		n = n > PROTOCOL_BATTERY_STREAM_SIZE ? PROTOCOL_BATTERY_STREAM_SIZE : n;
		response_event.response.type.stream_response.battery_stream_count = n;
		uint32_t len = n*sizeof(BatteryStream);
		circular_fifo_read(&battery_stream_fifo, (uint8_t*) response_event.response.type.stream_response.battery_stream, &len);
	}
	
	if(sampling_configuration & STREAMING_MICROPHONE) {
		uint32_t n = circular_fifo_get_size(&microphone_stream_fifo) / sizeof(MicrophoneStream);
		n = n > PROTOCOL_MICROPHONE_STREAM_SIZE ? PROTOCOL_MICROPHONE_STREAM_SIZE : n;
		response_event.response.type.stream_response.microphone_stream_count = n;
		uint32_t len = n*sizeof(MicrophoneStream);
		circular_fifo_read(&microphone_stream_fifo, (uint8_t*) response_event.response.type.stream_response.microphone_stream, &len);
	}
	
	if(sampling_configuration & STREAMING_SCAN) {
		uint32_t n = circular_fifo_get_size(&scan_stream_fifo) / sizeof(ScanStream);
		n = n > PROTOCOL_SCAN_STREAM_SIZE ? PROTOCOL_SCAN_STREAM_SIZE : n;
		response_event.response.type.stream_response.scan_stream_count = n;
		uint32_t len = n*sizeof(ScanStream);
		circular_fifo_read(&scan_stream_fifo, (uint8_t*) response_event.response.type.stream_response.scan_stream, &len);
	}
	
	
	
	// Only send the response if we have sth to send
	if(	response_event.response.type.stream_response.battery_stream_count ||
		response_event.response.type.stream_response.microphone_stream_count ||
		response_event.response.type.stream_response.scan_stream_count ||
		response_event.response.type.stream_response.accelerometer_stream_count ||
		response_event.response.type.stream_response.accelerometer_interrupt_stream_count) {
	
		send_response(NULL, 0);	
	} else {
		finish_response();
	}
	
	// E.g. when disconnected or any other error occured
	if(!streaming_started) {
		// Stop all streamings
		sampling_stop_accelerometer(1);
		sampling_stop_accelerometer_interrupt(1);
		sampling_stop_battery(1);
		sampling_stop_microphone(1);
		sampling_stop_scan(1);
	}
	
	// Check if we need to cancel the streaming
	if((!(sampling_configuration & STREAMING_ACCELEROMETER || sampling_configuration & STREAMING_ACCELEROMETER_INTERRUPT ||
		sampling_configuration & STREAMING_BATTERY || sampling_configuration & STREAMING_MICROPHONE || sampling_configuration & STREAMING_SCAN)) || !streaming_started) {
			streaming_started = 0;
	} else {
		// Put itself manually on the scheduler again --> if not ready with sending the response start_response() will reschedule
		// It is done this way (not with the response_event.response_success_handler) because if sth wents wrong during sending
		// this function won't be called anymore to disable the streaming for all peripherals.
		// Another reason is that we probably don't want to send everytime this stream_response_handler is called
		// because we don't have anything to send (all counts = 0)
		app_sched_event_put(NULL, 0, stream_response_handler);
	}	
}



static void test_response_handler(void * p_event_data, uint16_t event_size) {
	if(start_response(test_response_handler) != NRF_SUCCESS)
		return;
	
	response_event.response.which_type = Response_test_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	
	selftest_status_t status = selftest_test();
	
	response_event.response.type.test_response.test_passed = (uint8_t) status;
	
	send_response(NULL, 0);	
}
































/**< These are the request handlers that actually call the response-handlers via the scheduler */



static void status_request_handler(void * p_event_data, uint16_t event_size) {

	// Set the timestamp:
	Timestamp timestamp = request_event.request.type.status_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	if(request_event.request.type.status_request.has_badge_assignement) {		
		
		BadgeAssignement badge_assignement;
		badge_assignement = request_event.request.type.status_request.badge_assignement;
		
		advertiser_set_badge_assignement(badge_assignement);
		
		// First read if we have already the correct badge-assignement stored:
		BadgeAssignement stored_badge_assignement;
		ret_code_t ret = storer_read_badge_assignement(&stored_badge_assignement);
		if(ret == NRF_ERROR_INVALID_STATE || ret == NRF_ERROR_INVALID_DATA || (ret == NRF_SUCCESS && (stored_badge_assignement.ID != badge_assignement.ID || stored_badge_assignement.group != badge_assignement.group))) {
			debug_log("Badge assignements missmatch: --> setting the new badge assignement: Old (%u, %u), New (%u, %u)\n", stored_badge_assignement.ID, stored_badge_assignement.group, badge_assignement.ID, badge_assignement.group);
			ret = storer_store_badge_assignement(&badge_assignement);
			if(ret == NRF_ERROR_INTERNAL) {
				// TODO: Error counter for rescheduling 
				app_sched_event_put(NULL, 0, status_request_handler);
				return;
			} else if (ret != NRF_SUCCESS) {	// There is an error in the configuration of the badge-assignement partition
				finish_error();
				return;
			} 
		} else if(ret == NRF_ERROR_INTERNAL) {
			// TODO: Error counter for rescheduling 
			app_sched_event_put(NULL, 0, status_request_handler);
			return;
		}
	}
	
	app_sched_event_put(NULL, 0, status_response_handler);
	// Don't finish it here, but in the response-handler (because of the response_timestamp and response_clock_status)
	//finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
}


static void start_microphone_request_handler(void * p_event_data, uint16_t event_size) {
	// Set the timestamp:
	Timestamp timestamp = request_event.request.type.start_microphone_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	uint32_t timeout = (request_event.request).type.start_microphone_request.timeout;
	uint16_t period_ms = (request_event.request).type.start_microphone_request.period_ms;

	debug_log("Start microphone with timeout: %u, period ms %u\n", timeout, period_ms);
	
	ret_code_t ret = sampling_start_microphone(timeout*60*1000, period_ms, 0);
	debug_log("Ret sampling_start_microphone: %d\n\r", ret);
	
	if(ret == NRF_SUCCESS) {
		app_sched_event_put(NULL, 0, start_microphone_response_handler);
		// Don't finish it here, but in the response-handler (because of the response_timestamp and response_clock_status)
		//finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_microphone_request_handler);
	}
}

static void stop_microphone_request_handler(void * p_event_data, uint16_t event_size) {

	sampling_stop_microphone(0);
	
	debug_log("Stop microphone\n");
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}

static void start_scan_request_handler(void * p_event_data, uint16_t event_size) {

	// Set the timestamp:
	Timestamp timestamp = (request_event.request).type.start_scan_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	uint32_t timeout	= (request_event.request).type.start_scan_request.timeout;
	uint16_t window	 	= (request_event.request).type.start_scan_request.window;
	uint16_t interval 	= (request_event.request).type.start_scan_request.interval;
	uint16_t duration 	= (request_event.request).type.start_scan_request.duration;
	uint16_t period 	= (request_event.request).type.start_scan_request.period;
	uint8_t  aggregation_type 	= (request_event.request).type.start_scan_request.aggregation_type;
	
	if(duration > period)
		period = duration; 
	
	debug_log("Start scanning with timeout: %u, window: %u, interval: %u, duration: %u, period: %u, aggregation_type: %u\n", timeout, window, interval, duration, period, aggregation_type);
	BadgeAssignement badge_assignement;
	advertiser_get_badge_assignement(&badge_assignement);
	ret_code_t ret = sampling_start_scan(timeout*60*1000, period, interval, window, duration, badge_assignement.group, aggregation_type, 0);
	debug_log("Ret sampling_start_scan: %d\n\r", ret);
	
	
	if(ret == NRF_SUCCESS) {
		app_sched_event_put(NULL, 0, start_scan_response_handler);
		// Don't finish it here, but in the response-handler (because of the response_timestamp and response_clock_status)
		//finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_scan_request_handler);
	}
}

static void stop_scan_request_handler(void * p_event_data, uint16_t event_size) {
	sampling_stop_scan(0);	
	debug_log("Stop scan\n");
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}

static void start_accelerometer_request_handler(void * p_event_data, uint16_t event_size) {
	// Set the timestamp:
	Timestamp timestamp = (request_event.request).type.start_accelerometer_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	uint32_t timeout					= (request_event.request).type.start_accelerometer_request.timeout;
	uint8_t  operating_mode				= (request_event.request).type.start_accelerometer_request.operating_mode;
	uint8_t  full_scale					= (request_event.request).type.start_accelerometer_request.full_scale;
	uint16_t datarate				 	= (request_event.request).type.start_accelerometer_request.datarate;
	uint16_t fifo_sampling_period_ms	= (request_event.request).type.start_accelerometer_request.fifo_sampling_period_ms;
	
	debug_log("Start accelerometer with timeout: %u, operating_mode: %u, full_scale: %u, datarate: %u, fifo_sampling_period_ms: %u\n", timeout, operating_mode, full_scale, datarate, fifo_sampling_period_ms);
	
	ret_code_t ret = sampling_start_accelerometer(timeout*60*1000, operating_mode, full_scale, datarate, fifo_sampling_period_ms, 0);
	debug_log("Ret sampling_start_accelerometer: %d\n\r", ret);
	
	if(ret == NRF_SUCCESS) {
		app_sched_event_put(NULL, 0, start_accelerometer_response_handler);
		// Don't finish it here, but in the response-handler (because of the response_timestamp and response_clock_status)
		//finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_accelerometer_request_handler);
	}
}

static void stop_accelerometer_request_handler(void * p_event_data, uint16_t event_size) {
	sampling_stop_accelerometer(0);
	debug_log("Stop accelerometer\n");
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}


static void start_accelerometer_interrupt_request_handler(void * p_event_data, uint16_t event_size) {
	// Set the timestamp:
	Timestamp timestamp = (request_event.request).type.start_accelerometer_interrupt_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	uint32_t timeout				= (request_event.request).type.start_accelerometer_interrupt_request.timeout;
	uint16_t threshold_mg			= (request_event.request).type.start_accelerometer_interrupt_request.threshold_mg;
	uint16_t minimal_duration_ms	= (request_event.request).type.start_accelerometer_interrupt_request.minimal_duration_ms;
	uint16_t ignore_duration_ms		= (request_event.request).type.start_accelerometer_interrupt_request.ignore_duration_ms;
	
	debug_log("Start accelerometer interrupt with timeout: %u, threshold_mg: %u, minimal_duration_ms: %u, ignore_duration_ms: %u\n", timeout, threshold_mg, minimal_duration_ms, ignore_duration_ms);
	
	ret_code_t ret = sampling_start_accelerometer_interrupt(timeout*60*1000, threshold_mg, minimal_duration_ms, ignore_duration_ms, 0);
	debug_log("Ret sampling_start_accelerometer_interrupt: %d\n\r", ret);
	
	if(ret == NRF_SUCCESS) {
		app_sched_event_put(NULL, 0, start_accelerometer_interrupt_response_handler);
		// Don't finish it here, but in the response-handler (because of the response_timestamp and response_clock_status)
		//finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_accelerometer_interrupt_request_handler);
	}
}

static void stop_accelerometer_interrupt_request_handler(void * p_event_data, uint16_t event_size) {
	sampling_stop_accelerometer_interrupt(0);
	debug_log("Stop accelerometer interrupt\n");
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}

static void start_battery_request_handler(void * p_event_data, uint16_t event_size) {
	// Set the timestamp:
	Timestamp timestamp = (request_event.request).type.start_battery_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	uint32_t timeout   	= (request_event.request).type.start_battery_request.timeout;
	uint16_t period_ms 	= (request_event.request).type.start_battery_request.period_ms;
	
	debug_log("Start battery with timeout %u, period_ms: %u\n", timeout, period_ms);
	
	ret_code_t ret = sampling_start_battery(timeout*60*1000, period_ms, 0);
	debug_log("Ret sampling_start_battery: %d\n\r", ret);
	
	if(ret == NRF_SUCCESS) {
		app_sched_event_put(NULL, 0, start_battery_response_handler);
		// Don't finish it here, but in the response-handler (because of the response_timestamp and response_clock_status)
		//finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification. 
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_battery_request_handler);
	}
}
static void stop_battery_request_handler(void * p_event_data, uint16_t event_size) {
	sampling_stop_battery(0);
	debug_log("Stop battery\n");
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}

static void microphone_data_request_handler(void * p_event_data, uint16_t event_size) {
	Timestamp timestamp = request_event.request.type.microphone_data_request.timestamp;
	debug_log("Pull microphone data since: %u s, %u ms\n", timestamp.seconds, timestamp.ms);
	
	ret_code_t ret = storer_find_microphone_chunk_from_timestamp(timestamp, &microphone_chunk);
	if(ret == NRF_SUCCESS || ret == NRF_ERROR_INVALID_STATE) {
		app_sched_event_put(NULL, 0, microphone_data_response_handler);
		finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, microphone_data_request_handler);
	}
}


static void scan_data_request_handler(void * p_event_data, uint16_t event_size) {	
	Timestamp timestamp =  request_event.request.type.scan_data_request.timestamp;	
	debug_log("Pull scan data since: %u s, %u ms\n", timestamp.seconds, timestamp.ms);
	
	ret_code_t ret = storer_find_scan_chunk_from_timestamp(timestamp, &scan_chunk);
	if(ret == NRF_SUCCESS || ret == NRF_ERROR_INVALID_STATE) {
		app_sched_event_put(NULL, 0, scan_data_response_handler);
		finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, scan_data_request_handler);
	}	
}


static void accelerometer_data_request_handler(void * p_event_data, uint16_t event_size) {
	
	Timestamp timestamp =  request_event.request.type.accelerometer_data_request.timestamp;	
	debug_log("Pull accelerometer data since: %u s, %u ms\n", timestamp.seconds, timestamp.ms);
	
	ret_code_t ret = storer_find_accelerometer_chunk_from_timestamp(timestamp, &accelerometer_chunk);
	if(ret == NRF_SUCCESS || ret == NRF_ERROR_INVALID_STATE) {
		app_sched_event_put(NULL, 0, accelerometer_data_response_handler);
		finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, accelerometer_data_request_handler);
	}	
}


static void accelerometer_interrupt_data_request_handler(void * p_event_data, uint16_t event_size) {
	
	Timestamp timestamp =  request_event.request.type.accelerometer_interrupt_data_request.timestamp;	
	debug_log("Pull accelerometer interrupt data since: %u s, %u ms\n", timestamp.seconds, timestamp.ms);
	
	ret_code_t ret = storer_find_accelerometer_interrupt_chunk_from_timestamp(timestamp, &accelerometer_interrupt_chunk);
	if(ret == NRF_SUCCESS || ret == NRF_ERROR_INVALID_STATE) {
		app_sched_event_put(NULL, 0, accelerometer_interrupt_data_response_handler);
		finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, accelerometer_interrupt_data_request_handler);
	}	
}


static void battery_data_request_handler(void * p_event_data, uint16_t event_size) {
	Timestamp timestamp =  request_event.request.type.battery_data_request.timestamp;	
	debug_log("Pull battery data since: %u s, %u ms\n", timestamp.seconds, timestamp.ms);
	
	ret_code_t ret = storer_find_battery_chunk_from_timestamp(timestamp, &battery_chunk);
	if(ret == NRF_SUCCESS || ret == NRF_ERROR_INVALID_STATE) {
		app_sched_event_put(NULL, 0, battery_data_response_handler);
		finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, battery_data_request_handler);
	}	
}

static void start_microphone_stream_request_handler(void * p_event_data, uint16_t event_size) {
	// Set the timestamp:
	Timestamp timestamp = request_event.request.type.start_microphone_stream_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	uint32_t timeout = (request_event.request).type.start_microphone_stream_request.timeout;
	uint16_t period_ms = (request_event.request).type.start_microphone_stream_request.period_ms;

	debug_log("Start microphone stream with timeout: %u, period ms %u\n", timeout, period_ms);
	
	ret_code_t ret = sampling_start_microphone(timeout*60*1000, period_ms, 1);
	debug_log("Ret sampling_start_microphone stream: %d\n\r", ret);
	
	if(ret == NRF_SUCCESS) {
		if(!streaming_started) 	{	
			streaming_started = 1;
			app_sched_event_put(NULL, 0, stream_response_handler);
		}
		finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_microphone_stream_request_handler);
	}	
}

static void stop_microphone_stream_request_handler(void * p_event_data, uint16_t event_size) {
	sampling_stop_microphone(1);
	
	debug_log("Stop microphone stream\n");
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}

static void start_scan_stream_request_handler(void * p_event_data, uint16_t event_size) {
	// Set the timestamp:
	Timestamp timestamp = (request_event.request).type.start_scan_stream_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	uint32_t timeout	= (request_event.request).type.start_scan_stream_request.timeout;
	uint16_t window	 	= (request_event.request).type.start_scan_stream_request.window;
	uint16_t interval 	= (request_event.request).type.start_scan_stream_request.interval;
	uint16_t duration 	= (request_event.request).type.start_scan_stream_request.duration;
	uint16_t period 	= (request_event.request).type.start_scan_stream_request.period;
	uint8_t  aggregation_type 	= (request_event.request).type.start_scan_stream_request.aggregation_type;
	
	if(duration > period)
		period = duration; 
	
	debug_log("Start scanning stream with timeout: %u, window: %u, interval: %u, duration: %u, period: %u\n", timeout, window, interval, duration, period);
	BadgeAssignement badge_assignement;
	advertiser_get_badge_assignement(&badge_assignement);
	ret_code_t ret = sampling_start_scan(timeout*60*1000, period, interval, window, duration, badge_assignement.group, aggregation_type, 1);
	debug_log("Ret sampling_start_scan stream: %d\n\r", ret);
	
	
	if(ret == NRF_SUCCESS) {
		if(!streaming_started) 	{	
			streaming_started = 1;
			app_sched_event_put(NULL, 0, stream_response_handler);
		}
		finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_scan_stream_request_handler);
	}
}

static void stop_scan_stream_request_handler(void * p_event_data, uint16_t event_size) {
	sampling_stop_scan(1);
	
	debug_log("Stop scan stream\n");
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}



static void start_accelerometer_stream_request_handler(void * p_event_data, uint16_t event_size) {
	
	// Set the timestamp:
	Timestamp timestamp = (request_event.request).type.start_accelerometer_stream_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	uint32_t timeout					= (request_event.request).type.start_accelerometer_stream_request.timeout;
	uint8_t  operating_mode				= (request_event.request).type.start_accelerometer_stream_request.operating_mode;
	uint8_t  full_scale					= (request_event.request).type.start_accelerometer_stream_request.full_scale;
	uint16_t datarate				 	= (request_event.request).type.start_accelerometer_stream_request.datarate;
	uint16_t fifo_sampling_period_ms	= (request_event.request).type.start_accelerometer_stream_request.fifo_sampling_period_ms;
	
	debug_log("Start accelerometer stream with timeout: %u, operating_mode: %u, full_scale: %u, datarate: %u, fifo_sampling_period_ms: %u\n", timeout, operating_mode, full_scale, datarate, fifo_sampling_period_ms);
	
	ret_code_t ret = sampling_start_accelerometer(timeout*60*1000, operating_mode, full_scale, datarate, fifo_sampling_period_ms, 1);
	debug_log("Ret sampling_start_accelerometer stream: %d\n\r", ret);
	
	if(ret == NRF_SUCCESS) {
		if(!streaming_started) 	{	
			streaming_started = 1;
			app_sched_event_put(NULL, 0, stream_response_handler);
		}
		finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_accelerometer_stream_request_handler);
	}
}
static void stop_accelerometer_stream_request_handler(void * p_event_data, uint16_t event_size) {
	sampling_stop_accelerometer(1);
	
	debug_log("Stop accelerometer stream\n");
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}



static void start_accelerometer_interrupt_stream_request_handler(void * p_event_data, uint16_t event_size) {
	// Set the timestamp:
	Timestamp timestamp = (request_event.request).type.start_accelerometer_interrupt_stream_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	uint32_t timeout				= (request_event.request).type.start_accelerometer_interrupt_stream_request.timeout;
	uint16_t threshold_mg			= (request_event.request).type.start_accelerometer_interrupt_stream_request.threshold_mg;
	uint16_t minimal_duration_ms	= (request_event.request).type.start_accelerometer_interrupt_stream_request.minimal_duration_ms;
	uint16_t ignore_duration_ms		= (request_event.request).type.start_accelerometer_interrupt_stream_request.ignore_duration_ms;
	
	debug_log("Start accelerometer interrupt stream with timeout: %u, threshold_mg: %u, minimal_duration_ms: %u, ignore_duration_ms: %u\n", timeout, threshold_mg, minimal_duration_ms, ignore_duration_ms);
	
	ret_code_t ret = sampling_start_accelerometer_interrupt(timeout*60*1000, threshold_mg, minimal_duration_ms, ignore_duration_ms, 1);
	debug_log("Ret sampling_start_accelerometer_interrupt stream: %d\n\r", ret);
	
	if(ret == NRF_SUCCESS) {
		if(!streaming_started) 	{	
			streaming_started = 1;
			app_sched_event_put(NULL, 0, stream_response_handler);
		}
		finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_accelerometer_interrupt_stream_request_handler);
	}
}
static void stop_accelerometer_interrupt_stream_request_handler(void * p_event_data, uint16_t event_size) {
	sampling_stop_accelerometer_interrupt(1);
	
	debug_log("Stop accelerometer interrupt stream\n");
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}




static void start_battery_stream_request_handler(void * p_event_data, uint16_t event_size) {
	// Set the timestamp:
	Timestamp timestamp = (request_event.request).type.start_battery_stream_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	uint32_t timeout   	= (request_event.request).type.start_battery_stream_request.timeout;
	uint16_t period_ms	= (request_event.request).type.start_battery_stream_request.period_ms;
	
	debug_log("Start battery stream with timeout %u, period_ms: %u\n", timeout, period_ms);
	
	ret_code_t ret = sampling_start_battery(timeout*60*1000, period_ms, 1);
	debug_log("Ret sampling_start_battery stream: %d\n\r", ret);
	
	if(ret == NRF_SUCCESS) {
		if(!streaming_started) 	{	
			streaming_started = 1;
			app_sched_event_put(NULL, 0, stream_response_handler);
		}
		finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_battery_stream_request_handler);
	}
}
static void stop_battery_stream_request_handler(void * p_event_data, uint16_t event_size) {
	sampling_stop_battery(1);
	
	debug_log("Stop battery stream\n");
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}

static void identify_request_handler(void * p_event_data, uint16_t event_size) {
	uint16_t timeout = (request_event.request).type.identify_request.timeout;
	(void) timeout;
	debug_log("Identify request with timeout: %u\n", timeout);
	#ifndef UNIT_TEST
	nrf_gpio_pin_write(RED_LED, LED_ON); 
	systick_delay_millis(timeout*1000);
	nrf_gpio_pin_write(RED_LED, LED_OFF); 
	#endif
	
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}
static void test_request_handler(void * p_event_data, uint16_t event_size) {
	debug_log("Test request handler\n");
	
	app_sched_event_put(NULL, 0, test_response_handler);
	finish_and_reschedule_receive_notification();	// Now we are done with processing the request --> we can now advance to the next receive-notification
}























// Storer-Modul macht folgendes: read/write Funktion für jede Data-Source
// Die read-Funktion hat auch eine get_next-Funktion, welche dann zum nächsten element geht. Aber auch eine search funktion
// Die Funktionen bekommen einen Pointer auf eine Struktur mit übergeben, welche Sie dann decoden.
// 



#endif
