#ifndef __SCANNER_LIB_H
#define __SCANNER_LIB_H


#include "stdint.h"
#include "sdk_errors.h"

#define SCANNER_MINIMUM_RSSI	(-120)


typedef enum {
	SCAN_DEVICE_TYPE_UNKNOWN  = 0,
	SCAN_DEVICE_TYPE_BADGE  = 1,
	SCAN_DEVICE_TYPE_IBEACON = 2,
} scanner_scan_device_type_t;

typedef struct {
	uint16_t 					ID; 	// Here not taken badge_assignement, because it could also be an IBeacon
	uint8_t 					group;
	scanner_scan_device_type_t 	scanner_scan_device_type;
	int8_t 						rssi;
} scanner_scan_report_t;

typedef void (*scanner_on_scan_timeout_callback_t)	(void);				/**< The on scan timeout callback function type. */
typedef void (*scanner_on_scan_report_callback_t)	(scanner_scan_report_t* scanner_scan_report);/**< The on scan report callback function type. */



/**@brief Function to initialize the scanner.
 *
 * @note ble_init() has to be called before.
 */
void 		scanner_init(void);


/**@brief	Function to set the on scan timeout callback function.
 *
 * @param [in] 	scanner_on_scan_timeout_callback	The callback function that should be called.
 */
void 		scanner_set_on_scan_timeout_callback(scanner_on_scan_timeout_callback_t scanner_on_scan_timeout_callback);


/**@brief	Function to set the on scan report callback function.
 *
 * @param [in] 	scanner_on_scan_report_callback		The callback function that should be called.
 */
void 		scanner_set_on_scan_report_callback(scanner_on_scan_report_callback_t 	scanner_on_scan_report_callback);



/**@brief Function to start a scan-operation.
 * 
 * @param[in] scan_interval_ms		The scan interval in milliseconds.
 * @param[in] scan_window_ms		The scan window in milliseconds.
 * @param[in] scan_duration_seconds	The scan duration in seconds.
 *
 * @retval 	NRF_SUCCESS Successfully initiated scanning procedure.
 * @retval 	NRF_ERROR_INVALID_ADDR Invalid pointer supplied.
 * @retval 	NRF_ERROR_INVALID_STATE Invalid state to perform operation.
 * @retval 	NRF_ERROR_INVALID_PARAM Invalid parameter(s) supplied.
 * @retval 	NRF_ERROR_BUSY The stack is busy, process pending events and retry.
 * @retval 	NRF_ERROR_RESOURCES Not enough BLE role slots available.
 *                               Stop one or more currently active roles (Central, Peripheral or Broadcaster) and try again
 *
 * @note		The input-parameters of this function has to be chosen in a way that advertising is still possible.
 */
ret_code_t 	scanner_start_scanning(uint16_t scan_interval_ms, uint16_t scan_window_ms, uint16_t scan_duration_seconds);


/**@brief	Function for stopping any ongoing scan-operation.
 */
void 		scanner_stop_scanning(void);



#endif
