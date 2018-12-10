#include "callback_generator_internal_lib.h"
#include "callback_generator_lib.h"
#include <string.h>



CALLBACK_GENERATOR_IMPLEMENTATION(ACCEL_INT1) {
	nrf_drv_gpiote_pin_t pin = 25;
	nrf_gpiote_polarity_t action = NRF_GPIOTE_POLARITY_LOTOHI;
	
	if(callback_generator_ACCEL_INT1_get_generator() != NULL) {
		callback_generator_ACCEL_INT1_get_generator()(&pin, &action);
	}
	
	if(callback_generator_ACCEL_INT1_get_handler() != NULL) {
		callback_generator_ACCEL_INT1_get_handler()(pin, action);
	}	
}


/** Callback generator implementation for ble_on_connect-callback 
 *
 * @details	There is no generator, because we don't need data to be generated.
 */
CALLBACK_GENERATOR_IMPLEMENTATION(ble_on_connect) {
	// We don't need to generate any data for the ble_on_connect-handler --> So just directly call the handler
	if(callback_generator_ble_on_connect_get_handler() != NULL) {
		callback_generator_ble_on_connect_get_handler()();
	}
}


/** Callback generator implementation for ble_on_disconnect-callback 
 *
 * @details	There is no generator, because we don't need data to be generated.
 */
CALLBACK_GENERATOR_IMPLEMENTATION(ble_on_disconnect) {
	// We don't need to generate any data for the ble_on_disconnect-handler --> So just directly call the handler
	if(callback_generator_ble_on_disconnect_get_handler() != NULL) {
		callback_generator_ble_on_disconnect_get_handler()();
	}
}


/** Callback generator implementation for ble_on_receive-callback */
CALLBACK_GENERATOR_IMPLEMENTATION(ble_on_receive) {
	uint8_t data[20];
	uint16_t len = 0;
	
	if(callback_generator_ble_on_receive_get_generator() != NULL) {
		callback_generator_ble_on_receive_get_generator()(data, &len, sizeof(data));
	}
	
	if(callback_generator_ble_on_receive_get_handler() != NULL) {
		callback_generator_ble_on_receive_get_handler()(data, len);
	}
}

/** Callback generator implementation for ble_on_transmit_complete-callback.
 *
 * @details	The trigger-timepoints are added in ble_lib_mock.c not in the unit-test.
 *			There is no generator, because we don't need data to be generated.
 */
CALLBACK_GENERATOR_IMPLEMENTATION(ble_on_transmit_complete) {
	// We don't need to generate any data for the ble_on_transmit_complete-handler --> So just directly call the handler
	if(callback_generator_ble_on_transmit_complete_get_handler() != NULL) {
		callback_generator_ble_on_transmit_complete_get_handler()();
	}
}



/** Callback generator implementation for ble_on_scan_report-callback */
CALLBACK_GENERATOR_IMPLEMENTATION(ble_on_scan_report) {
	ble_gap_evt_adv_report_t scan_report;
	memset(&scan_report, 0, sizeof(scan_report));
	
	if(callback_generator_ble_on_scan_report_get_generator() != NULL) {
		callback_generator_ble_on_scan_report_get_generator()(&scan_report);
	}
	
	if(callback_generator_ble_on_scan_report_get_handler() != NULL) {
		callback_generator_ble_on_scan_report_get_handler()(&scan_report);
	}
}


/** Callback generator implementation for ble_on_scan_timeout-callback.
 *
 * @details	The trigger-timepoints are added in ble_lib_mock.c not in the unit-test.
 *			There is no generator, because we don't need data to be generated.
 */
CALLBACK_GENERATOR_IMPLEMENTATION(ble_on_scan_timeout) {
	// We don't need to generate any data for the ble_on_scan_timeout-handler --> So just directly call the handler
	if(callback_generator_ble_on_scan_timeout_get_handler() != NULL) {
		callback_generator_ble_on_scan_timeout_get_handler()();
	}
}




