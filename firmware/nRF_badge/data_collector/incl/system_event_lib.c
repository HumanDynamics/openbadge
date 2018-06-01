#include "system_event_lib.h"

#include "softdevice_handler.h"


static volatile system_event_handler_t event_handlers[MAX_NUMBER_SYSTEM_EVENT_HANDLER];
static volatile uint32_t number_system_event_handler = 0;




static void system_event_dispatch(uint32_t sys_evt)
{	
	// Call all the registered event handlers
	for(uint32_t i = 0; i < number_system_event_handler; i++){
		event_handlers[i](sys_evt);
	}
}




void system_event_init(void) {
	// Only initialize the module once!
	static uint8_t init_done = 0;
	if(init_done == 0) {
		// Register with the SoftDevice handler module for system events.
		uint32_t err_code = softdevice_sys_evt_handler_set(system_event_dispatch);
		
		(void)(err_code);
		//APP_ERROR_CHECK(err_code);
		
		number_system_event_handler = 0;
		init_done = 1;
	}
}




ret_code_t system_event_register_handler(system_event_handler_t event_handler) {
	if(number_system_event_handler >= MAX_NUMBER_SYSTEM_EVENT_HANDLER) {
		return NRF_ERROR_NO_MEM;
	}
	
	event_handlers[number_system_event_handler] = event_handler;
	number_system_event_handler++;
	
	return NRF_SUCCESS;
}