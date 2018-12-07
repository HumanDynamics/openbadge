#include "ble_lib.h"

#include "callback_generator_lib.h"
#include "data_generator_lib.h"

#include "debug_lib.h"
#include "app_fifo.h"


#define TX_FIFO_SIZE	1024

static uint8_t	ble_state;																/**< The current BLE-state. */ 


static ble_on_receive_callback_t		external_ble_on_receive_callback = NULL;		/**< The external on receive callback function */
static ble_on_transmit_callback_t		external_ble_on_transmit_callback = NULL;		/**< The external on transmit callback function */
static ble_on_connect_callback_t		external_ble_on_connect_callback = NULL;		/**< The external on connect callback function */
static ble_on_disconnect_callback_t		external_ble_on_disconnect_callback = NULL;		/**< The external on disconnect callback function */
static ble_on_scan_timeout_callback_t	external_ble_on_scan_timeout_callback = NULL;	/**< The external on scan timeout callback function */
static ble_on_scan_report_callback_t	external_ble_on_scan_report_callback = NULL;	/**< The external on scan report callback function */


static app_fifo_t tx_fifo;								/**< The transmit FIFO to check functionallity of transmit. */
static uint8_t tx_fifo_buf[TX_FIFO_SIZE];				/**< The buffer of the transmit FIFO. */



static void ble_nus_on_receive_callback(uint8_t * p_data, uint16_t length);
static void ble_nus_on_transmit_complete_callback(void);
static void ble_on_connect_callback(void);
static void ble_on_disconnect_callback(void);
static void ble_on_scan_report_callback(ble_gap_evt_adv_report_t* scan_report);
static void ble_on_scan_timeout_callback(void);



ret_code_t ble_init(void) {
	
	
	callback_generator_ble_on_connect_init();
	callback_generator_ble_on_connect_set_handler(ble_on_connect_callback);
	callback_generator_ble_on_disconnect_init();
	callback_generator_ble_on_disconnect_set_handler(ble_on_disconnect_callback);
	callback_generator_ble_on_receive_init();
	callback_generator_ble_on_receive_set_handler(ble_nus_on_receive_callback);
	callback_generator_ble_on_transmit_complete_init();
	callback_generator_ble_on_transmit_complete_set_handler(ble_nus_on_transmit_complete_callback);
	callback_generator_ble_on_scan_report_init();
	callback_generator_ble_on_scan_report_set_handler(ble_on_scan_report_callback);
	callback_generator_ble_on_scan_timeout_init();
	callback_generator_ble_on_scan_timeout_set_handler(ble_on_scan_timeout_callback);
	
	
	ret_code_t ret = app_fifo_init(&tx_fifo, tx_fifo_buf, sizeof(tx_fifo_buf));
	if(ret != NRF_SUCCESS) return ret;
	
	
	
	// The trigger for the ble_on_connect_callback should be called
	callback_generator_ble_on_connect_trigger();
		
	
	ble_state = BLE_STATE_INACTIVE;

	
	return NRF_SUCCESS;	
}


ret_code_t ble_set_advertising_custom_advdata(uint16_t company_identifier, uint8_t* custom_advdata, uint16_t custom_advdata_size) {
	

	return NRF_SUCCESS;
	
}


ret_code_t ble_start_advertising(void) {
	ble_state |= BLE_STATE_ADVERTISING;
	
	return NRF_SUCCESS;
}

void ble_stop_advertising(void) {
	ble_state &= ~BLE_STATE_ADVERTISING;
}



ret_code_t ble_start_scanning(uint16_t scan_interval_ms, uint16_t scan_window_ms, uint16_t scan_duration_seconds) {
	
	if(scan_interval_ms == 0 || scan_window_ms == 0 || scan_duration_seconds == 0)
		return NRF_ERROR_INVALID_PARAM;
	
	// The trigger function for ble_on_scan_report_callback has to be called
	callback_generator_ble_on_scan_report_trigger();
	
	// The trigger function for ble_on_scan_timeout_callback has to be called + the scan timeout period could be set here
	uint32_t trigger_timepoint = ((uint32_t) scan_duration_seconds) * 1000;
	callback_generator_ble_on_scan_timeout_add_trigger_timepoints(&trigger_timepoint, 1);
	callback_generator_ble_on_scan_timeout_trigger();
	
	ble_state |= BLE_STATE_SCANNING;
	
	return NRF_SUCCESS;
}

void ble_stop_scanning(void) {	

	// TODO: Check if this function is called when scanning is manually stopped?
	//ble_on_scan_timeout_callback();
	
	ble_state &= ~BLE_STATE_SCANNING;
}


// For this a callback-generator has to be implemented
/**@brief Handler function that is called when some data were received via the Nordic Uart Service.
 *
 * @param[in]	p_nus	Pointer to the nus-identifier.
 * @param[in]	p_data	Pointer to the received data.
 * @param[in]	length	Length of the received data.
 */
static void ble_nus_on_receive_callback(uint8_t * p_data, uint16_t length) {
	if(!(ble_state & BLE_STATE_CONNECTED))
		return;
	
	debug_log("BLE: BLE on receive callback. Len = %u\n", length);
	
	// TODO: (I think it is ok, but...) The trigger function for receive has to be called (because we don't want to wait for the next ble_on_connect-event to receive sth again!) This doesn't work!
	// Could not call its own trigger-function in an executing trigger..
	// callback_generator_ble_on_receive_trigger();
	
	if(external_ble_on_receive_callback != NULL) 
		external_ble_on_receive_callback(p_data, length);
}


/**@brief Handler function that is called when data were transmitted via the Nordic Uart Service.
 */
static void ble_nus_on_transmit_complete_callback(void) {
	if(!(ble_state & BLE_STATE_CONNECTED))
		return;
	
	debug_log("BLE: BLE on transmit complete callback\n");
	
	if(external_ble_on_transmit_callback != NULL) 
		external_ble_on_transmit_callback();
}


// For this a callback-generator has to be implemented
/**@brief Handler function that is called when a BLE-connection was established.
 */
static void ble_on_connect_callback(void) {
	if(ble_state & BLE_STATE_CONNECTED)
		return;
	
	debug_log("BLE: BLE on connect callback\n");
	
	// The trigger function for receive has to be called
	callback_generator_ble_on_receive_trigger();
	
	// The trigger function for the ble_on_disconnect_callback has be called (because after connecting we should/could disconnect)
	callback_generator_ble_on_disconnect_trigger();
	
	ble_state |= BLE_STATE_CONNECTED;
	if(external_ble_on_connect_callback != NULL)
		external_ble_on_connect_callback();	
}


// For this a callback-generator has to be implemented
/**@brief Handler function that is called when disconnecting from an exisiting BLE-connection.
 */
static void ble_on_disconnect_callback(void) {
	if(!(ble_state & BLE_STATE_CONNECTED))
		return;
	
	debug_log("BLE: BLE on disconnect callback\n");
	
	// The trigger function for the ble_on_connect_callback has be called
	callback_generator_ble_on_connect_trigger();
	
	ble_state &= ~BLE_STATE_CONNECTED;
	if(external_ble_on_disconnect_callback != NULL)
		external_ble_on_disconnect_callback();
}


// For this a callback-generator has to be implemented
/**@brief Handler function that is called when there is an advertising report event during scanning.
 *
 * @param[in]	scan_report		Pointer to the advertsising report event.
 */
static void ble_on_scan_report_callback(ble_gap_evt_adv_report_t* scan_report) {
	if(!(ble_state & BLE_STATE_SCANNING))
		return;
	if(external_ble_on_scan_report_callback != NULL)
		external_ble_on_scan_report_callback(scan_report);

	debug_log("BLE: BLE on scan report callback. RSSI: %d\n", scan_report->rssi);
}

// For this a callback-generator has to be implemented
/**@brief Handler function that is called when the scan process timed-out.
 */
static void ble_on_scan_timeout_callback(void) {
	if(!(ble_state & BLE_STATE_SCANNING))
		return;

	ble_state &= ~BLE_STATE_SCANNING;
	if(external_ble_on_scan_timeout_callback != NULL)
		external_ble_on_scan_timeout_callback();
	
	debug_log("BLE: BLE on scan timeout callback\n");
}


// Implement a data-generator function for that
// BLE_GAP_ADDR_LEN = 6 from ble_gap.h
void ble_get_MAC_address(uint8_t* MAC_address) {
	data_generator_ble_get_MAC_address(MAC_address, BLE_GAP_ADDR_LEN);
}



ble_state_t ble_get_state(void) {
	if(ble_state & BLE_STATE_CONNECTED) {	// BLE_STATE_CONNECTED has higher priority than BLE_STATE_SCANNING and BLE_STATE_ADVERTISING!
		return BLE_STATE_CONNECTED;
	} else if (ble_state & BLE_STATE_SCANNING) {	// BLE_STATE_SCANNING has higher priority than BLE_STATE_ADVERTISING!
		return BLE_STATE_SCANNING;
	} else if(ble_state & BLE_STATE_ADVERTISING) {
		return BLE_STATE_ADVERTISING;
	} 
	return BLE_STATE_INACTIVE;
}



ret_code_t ble_transmit(uint8_t* data, uint16_t len) {
	// If not connected, return invalid state
	if(!(ble_state & BLE_STATE_CONNECTED))
		return NRF_ERROR_INVALID_STATE;
	
	if(len > 20) 
		return NRF_ERROR_INVALID_PARAM;
	

	uint32_t len_32 = len;
	ret_code_t ret = app_fifo_write(&tx_fifo, data, &len_32);
	if(ret != NRF_SUCCESS) return ret;
	
	// The trigger function for ble_on_transmit_complete_callback has to be called + the "transmit-time" could be set here
	uint32_t trigger_timepoint = 10;
	callback_generator_ble_on_transmit_complete_add_trigger_timepoints(&trigger_timepoint, 1);
	callback_generator_ble_on_transmit_complete_trigger();
	
	
	
	return NRF_SUCCESS;
}

/**@brief (Private) Function to retrive the current number of bytes in the transmit-fifo of the simulated ble-interface (only for testing purposes).
 *
 * @retval	Number of bytes in the transmit-fifo.
 */
uint32_t ble_transmit_fifo_get_size(void) {
	uint32_t len = 0;
	app_fifo_read(&tx_fifo, NULL, &len);
	return len;
}

/**@brief (Private) Function to retrive the read a certain number of bytes from the transmit-fifo of the simulated ble-interface (only for testing purposes).
 *
 * @retval	NRF_SUCCESS				If the bytes could be read successfully.
 *			NRF_ERROR_NOT_FOUND		If len bytes were not found
 */
ret_code_t ble_transmit_fifo_read(uint8_t* data, uint32_t len) {
	uint32_t available_len = ble_transmit_fifo_get_size();
	if(available_len < len)
		return NRF_ERROR_NOT_FOUND;
	
	return app_fifo_read(&tx_fifo, data, &len);
}


void ble_disconnect(void) {
	ble_on_disconnect_callback();
}



void ble_set_on_receive_callback(ble_on_receive_callback_t ble_on_receive_callback) {
	external_ble_on_receive_callback = ble_on_receive_callback;
}

void ble_set_on_transmit_callback(ble_on_transmit_callback_t ble_on_transmit_callback) {
	external_ble_on_transmit_callback = ble_on_transmit_callback;
}

void ble_set_on_connect_callback(ble_on_connect_callback_t ble_on_connect_callback) {
	external_ble_on_connect_callback = ble_on_connect_callback;
}

void ble_set_on_disconnect_callback(ble_on_disconnect_callback_t ble_on_disconnect_callback) {
	external_ble_on_disconnect_callback = ble_on_disconnect_callback;
}

void ble_set_on_scan_timeout_callback(ble_on_scan_timeout_callback_t ble_on_scan_timeout_callback) {
	external_ble_on_scan_timeout_callback = ble_on_scan_timeout_callback;
}

void ble_set_on_scan_report_callback(ble_on_scan_report_callback_t ble_on_scan_report_callback) {
	external_ble_on_scan_report_callback = ble_on_scan_report_callback;
}

