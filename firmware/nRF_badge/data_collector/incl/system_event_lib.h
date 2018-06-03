#ifndef __SYSTEM_EVENT_LIB_H
#define __SYSTEM_EVENT_LIB_H


/** @file
 *
 * @brief System event abstraction library.
 *
 * @details This module enables the application to register for system-events that are called when a softdevice system event occured.
 *
 */


#include "sdk_common.h"	// Needed for the definition of ret_code_t and the error-codes

#define MAX_NUMBER_SYSTEM_EVENT_HANDLER	4 			/**<  Max number of system events that could be registered */


/**
 * @brief System event handler type. 
 */
typedef void (*system_event_handler_t)(uint32_t sys_evt);


/**@example Initializing the Softdevice
 *
 *		nrf_clock_lf_cfg_t clock_lf_cfg =  {.source        = NRF_CLOCK_LF_SRC_XTAL,            
 *											.rc_ctiv       = 0,                                
 *											.rc_temp_ctiv  = 0,                                
 *											.xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM};
 *
 *
 *		SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);
 */
	

/**@brief   Function for initializing the system event module.
 *
 * @details This function should only called once. But there is an internal mechanism 
 *			that prevents from multiple initialization.
 *
 * @note The Softdevice has to be initialized to use this module.
 *			
 */
void system_event_init(void);


/**@brief   Function to register am application system event handler.
 *
 * @retval NRF_SUCCESS				If the registration was succesful.
 * @retval NRF_ERROR_NO_MEM			If the registration failed because there are too many registered system_event_handlers.	    	
 */
ret_code_t system_event_register_handler(system_event_handler_t event_handler);





#endif
