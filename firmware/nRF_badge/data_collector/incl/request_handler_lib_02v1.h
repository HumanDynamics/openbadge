#ifndef __REQUEST_HANDLER_LIB_H
#define __REQUEST_HANDLER_LIB_H

#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes



/**@brief Function to initialize the request-handler mechanism.
 *
 * @details When sth is received via the ble-interface, the notification handler is called.
 *			The notification is processed (and the ticks at the receive timepoint is saved to set the internal clock correctly).
 *			After the processing of the notification a corresponding request-handler is called, that interprets and processes
 *			the received data. Optionally the request-handler calls a response handler that generates a response and sends
 *			it via the ble-interface.
 *
 *
 * @note app-scheduler has to be initialized before!
 * @note sender_lib has to be initialized before!
 * @note advertiser_lib has to be initialized before!
 * @note storer_lib has to be initialized before!
 * @note sampling_lib has to be initialized before!
 * @note systick_lib has to be initialized before!
 */

ret_code_t request_handler_init(void);




#endif 

