#include "sender_lib.h"

#include "ble_lib.h"
#include "systick_lib.h"	// Needed for the timeout-check 
#include "app_fifo.h"
#include "app_util_platform.h"

//#include "stdio.h"
//#include "string.h"
#include "debug_lib.h"

#define TX_FIFO_SIZE			512		/**< Size of the transmit fifo of the BLE (has to be a power of two) */
#define RX_FIFO_SIZE			128		/**< Size of the receive fifo of the BLE (has to be a power of two) */
#define MAX_BYTES_PER_TRANSMIT	20		/**< Number of bytes that could be sent at once via the Nordic Uart Service */		

static void on_connect_callback(void);
static void on_disconnect_callback(void);
static void on_transmit_callback(void);
static void on_receive_callback(uint8_t* data, uint16_t len);
static ret_code_t transmit_queued_bytes(void);


static volatile uint8_t connected = 0;			/**< Flag, if there is a ble-connection. */
static volatile uint8_t transmitting = 0;		/**< Flag, if there is currently an ongoing transmit_queued_bytes()-operation. */
static sender_receive_notification_handler_t receive_notification_handler = NULL; /**< External notification handler, that should be called if sth was received */


static uint8_t 	tx_fifo_buf[TX_FIFO_SIZE];
static uint8_t 	rx_fifo_buf[RX_FIFO_SIZE];
static uint8_t	transmit_buf[MAX_BYTES_PER_TRANSMIT];	/**< Buffer where the actual bytes to send will be buffered */

static app_fifo_t tx_fifo;								/**< The fifo for transmitting */
static app_fifo_t rx_fifo;								/**< The fifo for receiving */

/**@brief Function to reset the sender state.
 */
static void sender_reset(void) {
	connected = 0;
	transmitting = 0;
	// Flush the tx and rx FIFO
	app_fifo_flush(&tx_fifo);
	app_fifo_flush(&rx_fifo);
}


/**@brief Function that is called when a connection was established.
 */
static void on_connect_callback(void) {
	sender_reset();
	connected = 1;
	debug_log_bkgnd("Connected callback\n");
}

/**@brief Function that is called when disconnected event occurs.
 */
static void on_disconnect_callback(void) {
	sender_reset();
	connected = 0;
	debug_log_bkgnd("Disconnected callback\n");
}

/**@brief Function that is called when the transmission was successful.
 *
 * @details transmit_queued_bytes() is called to send the remaining bytes in the FIFO.
 */
static void on_transmit_callback(void) {
	// Reschedule the transmit of the queued bytes
	transmit_queued_bytes();
}

/**@brief Function that is called when data are received.
 *
 * @details It calls the registered notification handler.
 */
static void on_receive_callback(uint8_t* data, uint16_t len) {
	uint64_t timepoint_ticks = systick_get_ticks_since_start();
	uint32_t len_32 = len;
	app_fifo_write(&rx_fifo, data, &len_32);
	//debug_log_bkgnd("Received: %u\n", len_32);
	if(receive_notification_handler != NULL) {
		
		// Prepare a receive_notification-struct and call the notification-handler
		receive_notification_t receive_notification;
		receive_notification.notification_len = len;
		receive_notification.timepoint_ticks = timepoint_ticks;
		receive_notification_handler(receive_notification);
	}
	
}

/**@brief Function transmits the data in tx_fifo in small portions (MAX_BYTES_PER_TRANSMIT).
 *
 * @retval	NRF_SUCCESS If the data were sent successfully. Otherwise, an error code is returned.
 */
static ret_code_t transmit_queued_bytes(void) {
	if(!transmitting)
		return NRF_ERROR_INVALID_STATE;
	
	// Read out how many bytes have to be sent:
	uint32_t remaining_size = 0;
	CRITICAL_REGION_ENTER();
	app_fifo_read(&tx_fifo, NULL, &remaining_size);
	// Calculate the actual number of bytes to sent in this operation
	if(remaining_size == 0) { // Clear the transmitting-flag, if we have nothing to send anymore
		transmitting = 0;
	}
	CRITICAL_REGION_EXIT(); 
	
	uint32_t len = (remaining_size > sizeof(transmit_buf)) ? sizeof(transmit_buf) : remaining_size;	
	
	ret_code_t ret = NRF_SUCCESS;
	if(len > 0) {	
		// Read the bytes manually from the fifo to be efficient:
		for(uint32_t i = 0; i < len; i++) 
			transmit_buf[i] = tx_fifo.p_buf[(tx_fifo.read_pos + i) & tx_fifo.buf_size_mask];	// extracted from app_fifo.c: "static __INLINE void fifo_peek(app_fifo_t * p_fifo, uint16_t index, uint8_t * p_byte)"
		/*
		char out_buf[100];
		sprintf(out_buf, "Transmit (%u): ", (unsigned int) len);
		for(uint8_t i = 0; i < len; i++)  
			sprintf(&out_buf[strlen(out_buf)], "%02X", transmit_buf[i]);
		debug_log_bkgnd("%s\n",out_buf);
		systick_delay_millis(100);
		*/
		// Now send the bytes via bluetooth
		ret = ble_transmit(transmit_buf, len);
		if(ret == NRF_SUCCESS) { // If the transmission was successful, we can "consume" the data in the fifo manually
			tx_fifo.read_pos += len;
		}
	} 
	return ret;
}


ret_code_t sender_init(void) {
	
	ret_code_t ret = app_fifo_init(&tx_fifo, tx_fifo_buf, sizeof(tx_fifo_buf));
	if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
	
	ret = app_fifo_init(&rx_fifo, rx_fifo_buf, sizeof(rx_fifo_buf));
	if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
	
	sender_reset();
	
	receive_notification_handler = NULL;
	ble_set_on_connect_callback(on_connect_callback);
	ble_set_on_disconnect_callback(on_disconnect_callback);
	ble_set_on_transmit_callback(on_transmit_callback);
	ble_set_on_receive_callback(on_receive_callback);
	
	return NRF_SUCCESS;
}





void sender_set_receive_notification_handler(sender_receive_notification_handler_t sender_receive_notification_handler) {
	receive_notification_handler = sender_receive_notification_handler;
}

uint32_t sender_get_received_data_size(void) {
	// Read size of rx-fifo:
	uint32_t available_size = 0;
	app_fifo_read(&rx_fifo, NULL, &available_size);
	return available_size;
}

void sender_get_received_data(uint8_t* data, uint32_t* len) {
	uint32_t available_size = sender_get_received_data_size();
	if(*len > available_size)
		*len = available_size;
	
	app_fifo_read(&rx_fifo, data, len);
}

ret_code_t sender_await_data(uint8_t* data, uint32_t len, uint32_t timeout_ms) {
	if(!connected)
		return NRF_ERROR_INVALID_STATE;
	
	uint64_t start_ms = systick_get_continuous_millis();
	
	// Wait for data or timeout
	while((sender_get_received_data_size() < len) && (systick_get_continuous_millis() <= start_ms + (uint64_t) timeout_ms));
	
	// Check if we timed-out or have enough data received
	if(sender_get_received_data_size() < len)
		return NRF_ERROR_TIMEOUT;
	
	sender_get_received_data(data, &len);
	
	return NRF_SUCCESS;
}

ret_code_t sender_transmit(const uint8_t* data, uint32_t len, uint32_t timeout_ms) {
	if(!connected)
		return NRF_ERROR_INVALID_STATE;
	
	// First check available space in tx-fifo:
	uint64_t start_ms = systick_get_continuous_millis();
	uint32_t available_size = 0;
	// Wait for available space in fifo or timeout
	do {
		app_fifo_write(&tx_fifo, NULL, &available_size);
	} while((len > available_size) && systick_get_continuous_millis() <= start_ms + (uint64_t) timeout_ms);
	
	if(len > available_size) 
		return NRF_ERROR_NO_MEM;
	
	
	
	// If there is enough space in the FIFO, write the data to the FIFO:
	ret_code_t ret;
	CRITICAL_REGION_ENTER();	
	// This is saved by a critical region because:
	// when the transmit_queued_bytes() is called via the ble-ISR, and it reads out that no bytes are left in the TX-FIFO, and before setting the transmitting-flag to 0,
	// the app_fifo_write(&tx_fifo,...)-function is called (only possible in Thread-mode not in normal interrupt-mode), it could happen that the transmit_queued_bytes()
	// is not called again to start the transmitting for the new insert bytes.
	ret	= app_fifo_write(&tx_fifo, data, &len);
	CRITICAL_REGION_EXIT();
	if(ret != NRF_SUCCESS) return ret;
	
	ret = NRF_SUCCESS;
	if(!transmitting) {	// Only start the transmit_queued_bytes()-function, if it is not already transmitting the data of the FIFO
		transmitting = 1;
		ret = transmit_queued_bytes();
	} 
	return ret;
}

// TODO: Timeout check and disconnect automatically!!
void sender_disconnect(void) {
	sender_reset();
	ble_disconnect();
}

