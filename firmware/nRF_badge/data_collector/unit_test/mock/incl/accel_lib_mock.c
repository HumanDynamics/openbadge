#include "accel_lib.h"


#include "callback_generator_lib.h"
#include "data_generator_lib.h"


// TODO: retrieve Pin-numbers from the custom_board-file!
#define ACCEL_INT1_PIN														25			

/**< The default configuration parameters for the accelerometer */
#define ACCEL_INTERRUPT_EVENT_DEFAULT										ACCEL_NO_INTERRUPT



static volatile accel_interrupt_event_t		interrupt_event = ACCEL_INTERRUPT_EVENT_DEFAULT;	/**< The current interrupt event that is generated, when the interrupt-pin switches from low to high */
static volatile accel_interrupt_handler_t 	interrupt_handler = NULL;							/**< The current interrupt handler callback function */




/**@brief   The interrupt handler function of the INT1-interrupt pin.
 *
 * @param[in]	pin		The gpio-pin where the event/interrupt was detected.
 * @param[in]	action	The action of the pin (e.g. NRF_GPIOTE_POLARITY_LOTOHI)
 *
 */
static void accel_int1_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
	if(pin == ACCEL_INT1_PIN && action == NRF_GPIOTE_POLARITY_LOTOHI) {
		if(interrupt_event != ACCEL_NO_INTERRUPT) {
			
			if(interrupt_handler != NULL) {
				accel_interrupt_event_t evt = interrupt_event; // A temporary copy, so the called handler can't change the internal interrupt_event
				interrupt_handler(&evt);
			}
		}
		
	} 	
}


ret_code_t 	accel_init(void) {
	
	
	callback_generator_ACCEL_INT1_init();
	callback_generator_ACCEL_INT1_set_handler(accel_int1_event_handler);
	
	return NRF_SUCCESS;
}



ret_code_t 	accel_set_datarate(accel_datarate_t accel_datarate) {	
	
	
	return NRF_SUCCESS;
}


ret_code_t 	accel_set_operating_mode(accel_operating_mode_t accel_operating_mode) {
	
	
	
	return NRF_SUCCESS;
}

ret_code_t 	accel_set_axis(accel_axis_t accel_axis) {
	
	
	return NRF_SUCCESS;
}



ret_code_t 	accel_set_full_scale(accel_full_scale_t accel_full_scale) {
	
	
	return NRF_SUCCESS;
	
}

ret_code_t 	accel_set_HP_filter(accel_HP_filter_t accel_HP_filter) {
	
	return NRF_SUCCESS;
}


void 		accel_set_interrupt_handler(accel_interrupt_handler_t accel_interrupt_handler) {
	interrupt_handler = accel_interrupt_handler;
}



ret_code_t 	accel_set_motion_interrupt_parameters(uint16_t threshold_milli_gauss, uint16_t minimal_duration_ms) {

	
	return NRF_SUCCESS;
	
}



ret_code_t 	accel_set_interrupt(accel_interrupt_event_t accel_interrupt_event) {
	interrupt_event = accel_interrupt_event;

	if(accel_interrupt_event == ACCEL_MOTION_INTERRUPT) {
		callback_generator_ACCEL_INT1_trigger();
	}
	
	return NRF_SUCCESS;
}

ret_code_t 	accel_reset_interrupt(void) {
	// Every time accel_reset_interrupt is called, the trigger should be called
	if(interrupt_event == ACCEL_MOTION_INTERRUPT) {
		callback_generator_ACCEL_INT1_trigger();
	}
	return NRF_SUCCESS;
}



ret_code_t 	accel_set_fifo(accel_fifo_t accel_fifo) {
	
	
	
	return NRF_SUCCESS;
}


ret_code_t 	accel_read_acceleration(int16_t* accel_x, int16_t* accel_y, int16_t* accel_z, uint8_t* num_samples) {
	
	return data_generator_accel_read_acceleration(accel_x, accel_y, accel_z, num_samples);
	
}



bool 		accel_selftest(void) {
	
	
	return 1;
}


