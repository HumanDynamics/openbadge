#include "request_handler_lib_01v1.h"

//#define PROTOCOL_01v1

#ifdef PROTOCOL_01v1


#include <string.h>
#include "app_fifo.h"
#include "app_scheduler.h"
#include "sender_lib.h"
#include "systick_lib.h"
#include "storer_lib.h"
#include "sampling_lib.h"
#include "advertiser_lib.h"	// To retrieve the current badge-assignement and set the clock-sync status
#include "battery_lib.h"

#include "debug_lib.h"

#include "tinybuf.h"
#include "protocol_messages_01v1.h"
#include "chunk_messages.h"

#ifndef UNIT_TEST
#include "custom_board.h"
#include "nrf_gpio.h"
#endif


#define RECEIVE_NOTIFICATION_FIFO_SIZE					256		/**< Buffer size for the receive-notification FIFO. Has to be a power of two */
#define AWAIT_DATA_TIMEOUT_MS							1000
#define TRANSMIT_DATA_TIMEOUT_MS						100
#define REQUEST_HANDLER_SERIALIZED_BUFFER_SIZE			512		
#define RESPONSE_MAX_TRANSMIT_RETRIES					50		


#define MICROPHONE_SAMPLING_PERIOD_MS					50
#define SCAN_AGGREGATION_TYPE							SCAN_CHUNK_AGGREGATE_TYPE_MAX

#define MICROPHONE_DATA_RESPONSE_HEADER_SIZE			13	/** Needed for the old protocol */
#define SCAN_DATA_RESPONSE_HEADER_SIZE					9   /** Needed for the old protocol */


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




static request_event_t 	request_event;	/**< Needed a reponse event, to put the timepoint ticks into the structure */
static response_event_t	response_event; /**< Needed a reponse event, to put the reties and the function to call after success into the structure */
static Timestamp		response_timestamp; /**< Needed for the status-, and start-requests */
static uint8_t			response_clock_status;	/**< Needed for the status-, and start-requests */


static app_fifo_t receive_notification_fifo;
static uint8_t receive_notification_buf[RECEIVE_NOTIFICATION_FIFO_SIZE];
static volatile uint8_t processing_receive_notification = 0;			/**< Flag that represents if the processing of a receive notification is still running. */

static uint8_t serialized_buf[REQUEST_HANDLER_SERIALIZED_BUFFER_SIZE];

/**< Store-messages to represent how the data should be stored (global because needed more than one) */
static MicrophoneChunk 	microphone_chunk;
static ScanChunk 		scan_chunk;



static void receive_notification_handler(receive_notification_t receive_notification);
static void process_receive_notification(void * p_event_data, uint16_t event_size);

// TODO: add respone handler function declaration here
static void status_request_handler(void * p_event_data, uint16_t event_size);
static void status_assign_request_handler(void * p_event_data, uint16_t event_size);
static void start_microphone_request_handler(void * p_event_data, uint16_t event_size);
static void stop_microphone_request_handler(void * p_event_data, uint16_t event_size);
static void start_scan_request_handler(void * p_event_data, uint16_t event_size);
static void stop_scan_request_handler(void * p_event_data, uint16_t event_size);
static void microphone_data_request_handler(void * p_event_data, uint16_t event_size);
static void scan_data_request_handler(void * p_event_data, uint16_t event_size);
static void identify_request_handler(void * p_event_data, uint16_t event_size);


static request_handler_for_type_t request_handlers[] = {
        {
                .type = Request_status_request_tag,
                .handler = status_request_handler,
        },
		{
                .type = Request_status_assign_request_tag,
                .handler = status_assign_request_handler,
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
                .type = Request_microphone_data_request_tag,
                .handler = microphone_data_request_handler,
        },
        {
                .type = Request_scan_data_request_tag,
                .handler = scan_data_request_handler,
        },
        {
                .type = Request_identify_request_tag,
                .handler = identify_request_handler,
        }
};



// App-scheduler has to be initialized before!
// Sender has to be initialized before!
// Sampling has to be initialized before!
ret_code_t request_handler_init(void) {
	ret_code_t ret = sender_init();
	if(ret != NRF_SUCCESS) return ret;
	
	sender_set_receive_notification_handler(receive_notification_handler);
	
	ret = app_fifo_init(&receive_notification_fifo, receive_notification_buf, sizeof(receive_notification_buf));
	if(ret != NRF_SUCCESS) return ret;
	
	return NRF_SUCCESS;
}


static void receive_notification_handler(receive_notification_t receive_notification) {
	// Put the receive_notification into the fifo
	uint32_t available_len = 0;
	uint32_t notification_size = sizeof(receive_notification);
	app_fifo_write(&receive_notification_fifo, NULL, &available_len);
	if(available_len < notification_size) {
		debug_log("REQUEST_HANDLER: Not enough bytes in Notification FIFO: %u < %u\n", available_len, notification_size);
		return;
	}
	
	app_fifo_write(&receive_notification_fifo, (uint8_t*) &receive_notification, &notification_size);
	
	
	
	// Start the processing of the receive notification (but only if it was not started before/is still running). So it ignores the new notifications until the current
	// processed notification is ready.
	if(!processing_receive_notification) {
		processing_receive_notification = 1;		
		app_sched_event_put(NULL, 0, process_receive_notification);
	}
}

static void finish_request(void) {
	processing_receive_notification = 0;
	app_sched_event_put(NULL, 0, process_receive_notification);
}

// Called when await data failed, or decoding the notification failed, or request does not exist, or transmitting the response failed (because disconnected or something else)
static void finish_request_error(void) {
	app_fifo_flush(&receive_notification_fifo);
	debug_log("REQUEST_HANDLER: Error while processing request/response --> Disconnect!!!\n");
	sender_disconnect();	// To clear the RX- and TX-FIFO
	processing_receive_notification = 0;
	storer_invalidate_iterators();
}

static void process_receive_notification(void * p_event_data, uint16_t event_size) {
	
	receive_notification_t receive_notification;
	uint32_t notification_size = sizeof(receive_notification);
	// Check size of FIFO
	uint32_t available_size;
	app_fifo_read(&receive_notification_fifo, NULL, &available_size);
	if(available_size < notification_size) {
		app_fifo_flush(&receive_notification_fifo);
		processing_receive_notification = 0;
		return;
	}
	
	processing_receive_notification = 1;
	
	// Get the timestamp and clock-sync status before processing the request!
	systick_get_timestamp(&response_timestamp.seconds, &response_timestamp.ms); 
	response_clock_status = systick_is_synced();
	
		
	app_fifo_read(&receive_notification_fifo, (uint8_t*) &receive_notification, &notification_size);
	
	// Read out the received data of the notification
	uint32_t serialized_len = receive_notification.notification_len;
	uint64_t timepoint_ticks = receive_notification.timepoint_ticks;
	ret_code_t ret = sender_await_data(serialized_buf, serialized_len, AWAIT_DATA_TIMEOUT_MS);
	if(ret != NRF_SUCCESS) {
		debug_log("REQUEST_HANDLER: sender_await_data() error\n");
		finish_request_error();
		return;
	}
	
	// Now decode the serialized notification
	memset(&(request_event.request), 0, sizeof(request_event.request));
	tb_istream_t istream = tb_istream_from_buffer(serialized_buf, serialized_len);
	uint8_t decode_status = tb_decode(&istream, Request_fields, &(request_event.request), TB_LITTLE_ENDIAN);
	if(decode_status == 0) {
		debug_log("REQUEST_HANDLER: Error decoding\n");
		finish_request_error();
		return;
	}
	debug_log("REQUEST_HANDLER: Decoded successfully\n");
	
	if(istream.bytes_read < serialized_len) {
		debug_log("REQUEST_HANDLER: Warning decoding: %u bytes have not been read.\n", serialized_len - istream.bytes_read);
	}
	// We need to handle the StatusRequestAssignment message manually, because the old protocol sucks a little bit..
	if(request_event.request.which_type == Request_status_request_tag) {
		istream = tb_istream_from_buffer(&serialized_buf[1], serialized_len - 1);	// Without the which-field
		StatusAssignRequest tmp;
		decode_status = tb_decode(&istream, StatusAssignRequest_fields, &tmp, TB_LITTLE_ENDIAN);
		if(decode_status != 0) {	// If decoding was successful --> we assume that we received a StatusRequestAssign-message and not a StatusRequest-message
			request_event.request.which_type = Request_status_assign_request_tag;
			request_event.request.type.status_assign_request = tmp;
		}
	}
	
	request_event.request_timepoint_ticks = timepoint_ticks;

	
	
	debug_log("REQUEST_HANDLER: Which request type: %u, Ticks: %u\n", request_event.request.which_type, request_event.request_timepoint_ticks);
	
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
		
		request_handler(NULL, 0);
	} else {
		// Should actually not happen, but to be sure...
		debug_log("REQUEST_HANDLER: Have not found a corresponding request handler for which_type: %u\n", request_event.request.which_type);
		finish_request_error();
	}
}


static void send_response(void * p_event_data, uint16_t event_size) {
	// On transmit failure, reschedule itself
	response_event.response_fail_handler = send_response;
	
	// Check if we already had too much retries
	if(response_event.response_retries > RESPONSE_MAX_TRANSMIT_RETRIES) {
		finish_request_error();
		return;
	}
	
	response_event.response_retries++;
	
	// Encoding has not to be done every time, but to don't have an extra function for that, we just do it here..
	tb_ostream_t ostream = tb_ostream_from_buffer(serialized_buf, sizeof(serialized_buf));
	uint8_t encode_status = tb_encode(&ostream, Response_fields, &(response_event.response), TB_LITTLE_ENDIAN);
	uint32_t len = ostream.bytes_written;
	
	
	
	
	if(encode_status == 0) {
		debug_log("REQUEST_HANDLER: Error encoding response!\n");
		finish_request_error();
		return;
	}
	
	
	
	ret_code_t ret = NRF_SUCCESS;
	// This is a fix to be compatible with the old protocol-implementation.
	// We first check that the transmit-FIFO is empty. If not --> reschedule.
	// If it is empty we check if we need to transmit microphone-data or scan-data,
	// because these two types have to be handled separately (first a header has to be sent)
	// and after that the actual data have to be sent.
	
	uint32_t transmit_fifo_size = sender_get_transmit_fifo_size();
	uint64_t end_ms = systick_get_continuous_millis() + TRANSMIT_DATA_TIMEOUT_MS;
	while(transmit_fifo_size != 0 && systick_get_continuous_millis() < end_ms) {
		transmit_fifo_size = sender_get_transmit_fifo_size();
	}
	
	
	if(transmit_fifo_size != 0) {
		ret = NRF_ERROR_NO_MEM;
	} else {
		// In the old Protocol there should be no which_field indicating the response-type, so we ignore the first byte
		uint32_t transmit_len = len-1;				
		uint8_t* transmit_buf = &serialized_buf[1];
		// Split Headers:
		uint32_t header_len = 0;
		// Mic response --> 13 Byte Header
		// Scan response --> 9 Byte Header
		if(response_event.response.which_type == Response_microphone_data_response_tag) {
			header_len = (transmit_len < MICROPHONE_DATA_RESPONSE_HEADER_SIZE) ? transmit_len : MICROPHONE_DATA_RESPONSE_HEADER_SIZE;
		} else if(response_event.response.which_type == Response_scan_data_response_tag) {
			header_len = (transmit_len < SCAN_DATA_RESPONSE_HEADER_SIZE) ? transmit_len : SCAN_DATA_RESPONSE_HEADER_SIZE;
		}	

		ret = sender_transmit(transmit_buf, header_len, TRANSMIT_DATA_TIMEOUT_MS);	
		if(ret == NRF_SUCCESS) { // Wait until the header has been transmitted:
			do {
				transmit_fifo_size = sender_get_transmit_fifo_size();
			// If the connection drops, the FIFO will be cleared automatically --> leaving this loop
			} while(transmit_fifo_size != 0);
		}
		ret = sender_transmit(&transmit_buf[header_len], transmit_len-header_len, TRANSMIT_DATA_TIMEOUT_MS);
	}
	
	
	debug_log("REQUEST_HANDLER: Transmit status %u!\n", ret);
	
	
	if(ret == NRF_SUCCESS) {
		if(response_event.response_success_handler != NULL) {
			app_sched_event_put(NULL, 0, response_event.response_success_handler);
		} else {
			// If we don't need to call a success_handler-function, we are done with the request
			finish_request();
		}
	} else {
		if(ret == NRF_ERROR_NO_MEM)	// Only reschedule a fail handler, if we could not transmit because of memory problems
			if(response_event.response_fail_handler != NULL) {	// This function should actually always be set..
				app_sched_event_put(NULL, 0, response_event.response_fail_handler);
				return;
			}
		// Some BLE-problems occured (e.g. disconnected...)
		finish_request_error();
	}
}


/**
 Every request-handler has an response-handler!
*/





/**< These are the response handlers, that are called by the request-handlers. */

static void status_response_handler(void * p_event_data, uint16_t event_size) {
	response_event.response.which_type = Response_status_response_tag;
	response_event.response.type.status_response.clock_status = response_clock_status;
	response_event.response.type.status_response.collector_status = (sampling_get_sampling_configuration() & SAMPLING_MICROPHONE) ? 1 : 0;
	response_event.response.type.status_response.scan_status = (sampling_get_sampling_configuration() & SAMPLING_SCAN) ? 1 : 0;
	response_event.response.type.status_response.timestamp = response_timestamp;
	response_event.response.type.status_response.battery_data.voltage = battery_get_voltage();
	
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	
	debug_log("REQUEST_HANDLER: Status response: %u, %u\n", (uint32_t)response_event.response.type.status_response.timestamp.seconds, response_event.response.type.status_response.timestamp.ms);
	
	send_response(NULL, 0);	
}

static void start_microphone_response_handler(void * p_event_data, uint16_t event_size) {
	response_event.response.which_type = Response_start_microphone_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	
	response_event.response.type.start_microphone_response.timestamp = response_timestamp;
	
	
	send_response(NULL, 0);	
}


static void stop_microphone_response_handler(void * p_event_data, uint16_t event_size) {
	response_event.response.which_type = Response_stop_microphone_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	
	send_response(NULL, 0);	

}


static void start_scan_response_handler(void * p_event_data, uint16_t event_size) {
	response_event.response.which_type = Response_start_scan_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	response_event.response.type.start_scan_response.timestamp = response_timestamp;
	
	
	send_response(NULL, 0);	
}


static void stop_scan_response_handler(void * p_event_data, uint16_t event_size) {
	response_event.response.which_type = Response_stop_scan_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	
	send_response(NULL, 0);	

}

static void microphone_data_response_handler(void * p_event_data, uint16_t event_size) {
	response_event.response.which_type = Response_microphone_data_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = microphone_data_response_handler;
	

	ret_code_t ret = storer_get_next_microphone_chunk(&microphone_chunk);
	if(ret == NRF_SUCCESS) {
		debug_log("REQUEST_HANDLER: Found microphone data: %u, %u\n", microphone_chunk.timestamp.seconds, microphone_chunk.timestamp.ms);
		// Send microphone data
		response_event.response.type.microphone_data_response.microphone_data_response_header.timestamp = microphone_chunk.timestamp;
		response_event.response.type.microphone_data_response.microphone_data_response_header.battery_data.voltage = 0;
		response_event.response.type.microphone_data_response.microphone_data_response_header.sample_period_ms = microphone_chunk.sample_period_ms;
		uint32_t microphone_data_count = (microphone_chunk.microphone_data_count > PROTOCOL_MICROPHONE_DATA_SIZE) ? PROTOCOL_MICROPHONE_DATA_SIZE : microphone_chunk.microphone_data_count;
		response_event.response.type.microphone_data_response.microphone_data_count = microphone_data_count;
		memcpy(response_event.response.type.microphone_data_response.microphone_data, microphone_chunk.microphone_data, microphone_data_count*sizeof(MicrophoneData));
		
		send_response(NULL, 0);	
	} else if(ret == NRF_ERROR_NOT_FOUND || ret == NRF_ERROR_INVALID_STATE) {
		debug_log("REQUEST_HANDLER: Could not fetch Mic-data. Sending end Header..\n");
		memset(&(response_event.response.type.microphone_data_response.microphone_data_response_header), 0,  sizeof(response_event.response.type.microphone_data_response.microphone_data_response_header));
		response_event.response.type.microphone_data_response.microphone_data_count = 0;
		
		// Send end-header
		response_event.response_success_handler = NULL;
		send_response(NULL, 0);	
	} else {
		app_sched_event_put(NULL, 0, microphone_data_response_handler);
	}
}

static void scan_data_response_handler(void * p_event_data, uint16_t event_size) {
	response_event.response.which_type = Response_scan_data_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = scan_data_response_handler;
	

	
	ret_code_t ret = storer_get_next_scan_chunk(&scan_chunk);
	if(ret == NRF_SUCCESS) {
		debug_log("REQUEST_HANDLER: Found scan data..\n");
		// Send scan data
		response_event.response.type.scan_data_response.scan_data_response_header.timestamp_seconds = scan_chunk.timestamp.seconds;
		response_event.response.type.scan_data_response.scan_data_response_header.battery_data.voltage = 0;
		uint32_t scan_result_data_count = (scan_chunk.scan_result_data_count > PROTOCOL_SCAN_DATA_SIZE) ? PROTOCOL_SCAN_DATA_SIZE : scan_chunk.scan_result_data_count;
		response_event.response.type.scan_data_response.scan_result_data_count = scan_result_data_count;
		memcpy(response_event.response.type.scan_data_response.scan_result_data, scan_chunk.scan_result_data, scan_result_data_count*sizeof(ScanResultData));
		
		
		send_response(NULL, 0);	
	} else if(ret == NRF_ERROR_NOT_FOUND || ret == NRF_ERROR_INVALID_STATE) {
		debug_log("REQUEST_HANDLER: Could not fetch Scan-data. Sending end Header..\n");
		memset(&(response_event.response.type.scan_data_response.scan_data_response_header), 0,  sizeof(response_event.response.type.scan_data_response.scan_data_response_header));
		response_event.response.type.scan_data_response.scan_result_data_count = 0;
		
		// Send end-header
		response_event.response_success_handler = NULL;
		send_response(NULL, 0);	
	} else {
		app_sched_event_put(NULL, 0, scan_data_response_handler);
	}
}


static void identify_response_handler(void * p_event_data, uint16_t event_size) {
	response_event.response.which_type = Response_identify_response_tag;
	response_event.response_retries = 0;
	response_event.response_success_handler = NULL;
	
	send_response(NULL, 0);	

}





/**< These are the request handlers that actually call the response-handlers via the scheduler */

static void status_request_handler(void * p_event_data, uint16_t event_size) {

	// Set the timestamp:
	Timestamp timestamp = (request_event.request).type.status_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	app_sched_event_put(NULL, 0, status_response_handler);
}

static void status_assign_request_handler(void * p_event_data, uint16_t event_size) {
	
	// Set the timestamp:
	Timestamp timestamp = (request_event.request).type.status_assign_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	

	BadgeAssignement badge_assignement;
	badge_assignement = request_event.request.type.status_assign_request.badge_assignement;
	
	ret_code_t ret = advertiser_set_badge_assignement(badge_assignement);
	if(ret == NRF_ERROR_INTERNAL) {
		app_sched_event_put(NULL, 0, status_assign_request_handler);
	} else if(ret != NRF_SUCCESS) {
		finish_request_error();
	} else { // ret should be NRF_SUCCESS here
		app_sched_event_put(NULL, 0, status_response_handler);
	}
}

static void start_microphone_request_handler(void * p_event_data, uint16_t event_size) {
	
	// Set the timestamp:
	Timestamp timestamp = (request_event.request).type.start_microphone_request.timestamp;
	systick_set_timestamp(request_event.request_timepoint_ticks, timestamp.seconds, timestamp.ms);
	advertiser_set_status_flag_is_clock_synced(1);
	
	uint32_t timeout = (request_event.request).type.start_microphone_request.timeout;

	debug_log("REQUEST_HANDLER: Start microphone with timeout: %u \n", timeout);
	
	ret_code_t ret = sampling_start_microphone(timeout*60*1000, MICROPHONE_SAMPLING_PERIOD_MS, 0);
	debug_log("REQUEST_HANDLER: Ret sampling_start_microphone: %d\n\r", ret);
	
	if(ret == NRF_SUCCESS) {
		app_sched_event_put(NULL, 0, start_microphone_response_handler);
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_microphone_request_handler);
	}
}

static void stop_microphone_request_handler(void * p_event_data, uint16_t event_size) {
	sampling_stop_microphone(0);
	
	debug_log("REQUEST_HANDLER: Stop microphone\n");
	app_sched_event_put(NULL, 0, stop_microphone_response_handler);
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
	
	if(duration > period)
		period = duration; 
	
	debug_log("REQUEST_HANDLER: Start scanning with timeout: %u, window: %u, interval: %u, duration: %u, period: %u\n", timeout, window, interval, duration, period);
	BadgeAssignement badge_assignement;
	advertiser_get_badge_assignement(&badge_assignement);
	ret_code_t ret = sampling_start_scan(timeout*60*1000, period, interval, window, duration, badge_assignement.group, SCAN_AGGREGATION_TYPE, 0);
	debug_log("REQUEST_HANDLER: Ret sampling_start_scan: %d\n\r", ret);
	
	
	
	if(ret == NRF_SUCCESS) {
		app_sched_event_put(NULL, 0, start_scan_response_handler);
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, start_scan_request_handler);
	}
}

static void stop_scan_request_handler(void * p_event_data, uint16_t event_size) {
	sampling_stop_scan(0);
	
	debug_log("REQUEST_HANDLER: Stop scan\n");
	app_sched_event_put(NULL, 0, stop_scan_response_handler);
}


static void microphone_data_request_handler(void * p_event_data, uint16_t event_size) {
	Timestamp timestamp = request_event.request.type.microphone_data_request.timestamp;
	debug_log("REQUEST_HANDLER: Pull microphone data since: %u s, %u ms\n", timestamp.seconds, timestamp.ms);
	
	ret_code_t ret = storer_find_microphone_chunk_from_timestamp(timestamp, &microphone_chunk);
	if(ret == NRF_SUCCESS || ret == NRF_ERROR_INVALID_STATE) {
		app_sched_event_put(NULL, 0, microphone_data_response_handler);
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, microphone_data_request_handler);
	}	
}


static void scan_data_request_handler(void * p_event_data, uint16_t event_size) {
	
	Timestamp timestamp;
	timestamp.seconds	= request_event.request.type.scan_data_request.seconds;
	timestamp.ms		= 0;
	debug_log("REQUEST_HANDLER: Pull scan data since: %u s, %u ms\n", timestamp.seconds, timestamp.ms);
	
	ret_code_t ret = storer_find_scan_chunk_from_timestamp(timestamp, &scan_chunk);
	if(ret == NRF_SUCCESS || ret == NRF_ERROR_INVALID_STATE) {
		app_sched_event_put(NULL, 0, scan_data_response_handler);
	} else {
		// TODO: Error counter for rescheduling 
		app_sched_event_put(NULL, 0, scan_data_request_handler);
	}	

}


static void identify_request_handler(void * p_event_data, uint16_t event_size) {
	uint16_t timeout = (request_event.request).type.identify_request.timeout;
	(void) timeout;
	debug_log("REQUEST_HANDLER: Identify request with timeout: %u\n", timeout);
	#ifndef UNIT_TEST
	nrf_gpio_pin_write(RED_LED, LED_ON); 
	systick_delay_millis(timeout*1000);
	nrf_gpio_pin_write(RED_LED, LED_OFF); 
	#endif
	app_sched_event_put(NULL, 0, identify_response_handler);
}

#endif
