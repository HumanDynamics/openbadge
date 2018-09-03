#ifndef __BLE_LIB_H
#define __BLE_LIB_H

#include <stdint.h>
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes
#include "ble_gap.h"	// Needed for ble_gap_evt_adv_report_t-definition



#define ADVERTISING_DEVICE_NAME			"HDBDG"     /**< Name of device. Will be included in the advertising data. */ 
#define ADVERTISING_INTERVAL_MS		  	200   		/**< The advertising interval. */
#define ADVERTISING_TIMEOUT_SECONDS    	6           /**< The advertising timeout interval. */



#define IS_SRVC_CHANGED_CHARACT_PRESENT 0         	/**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/


/**< The BLE-state type. */
typedef enum {
	BLE_STATE_INACTIVE 		= 0,
	BLE_STATE_ADVERTISING 	= (1 << 0),
	BLE_STATE_CONNECTED 	= (1 << 1),
	BLE_STATE_SCANNING	 	= (1 << 2),
} ble_state_t;

typedef void (*ble_on_receive_callback_t)		(uint8_t * p_data, uint16_t length);	/**< The on receive callback function type. */
typedef void (*ble_on_transmit_callback_t)		(void);									/**< The on transmit callback function type. */
typedef void (*ble_on_connect_callback_t)		(void);									/**< The on connect callback function type. */
typedef void (*ble_on_disconnect_callback_t)	(void);									/**< The on disconnect callback function type. */
typedef void (*ble_on_scan_timeout_callback_t)	(void);									/**< The on scan timeout callback function type. */
typedef void (*ble_on_scan_report_callback_t)	(ble_gap_evt_adv_report_t* scan_report);/**< The on scan report callback function type. */


/**@brief	Function to initialize the BLE-Stack.
 *
 * @details The function initializes the advertising-process and the Nordic Uart Service (NUS) for transmitting and receiving data.
 *			Furthermore it sets the security parameters.
 *
 * @retval	NRF_SUCCESS On success, else an error code indicating reason for failure.
 *
 * @note 	Note that the Linker script must have the correct configuration:   RAM (rwx) :  ORIGIN = 0x20001fe8, LENGTH = 0x6018 for 0 central and 1 peripheral! Look at app_ram_base.h.
 * 			Note that the Softdevice has to be initialized (by SOFTDEVICE_HANDLER_INIT) before calling this function
 *			Example:	nrf_clock_lf_cfg_t clock_lf_cfg =  {.source        = NRF_CLOCK_LF_SRC_XTAL,            
 *															.rc_ctiv       = 0,                                
 *															.rc_temp_ctiv  = 0,                                
 *															.xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM};
 *						SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);
 */
ret_code_t 	ble_init(void);


/**@brief 	Function to force the BLE-Stack to disconnect from the current BLE-Connection. 
 */
void 		ble_disconnect(void);


/**@brief		Function to set the custom advertising data.
 *
 * @param[in]	company_identifier		A two byte unofficial company-identifier.
 * @param[in]	custom_advdata			Pointer to memory containing the custom advertising data.
 * @param[in]	custom_advdata_size		The size of the custom advertising data.
 *
 * @retval 		NRF_SUCCESS             If the operation was successful.
 * @retval 		NRF_ERROR_INVALID_PARAM If the operation failed because a wrong parameter was provided internally.
 * @retval 		NRF_ERROR_DATA_SIZE     If the operation failed because not all the requested data could fit into the
 *                                 		advertising packet. The maximum size of the advertisement packet
 *                                 		is @ref BLE_GAP_ADV_MAX_SIZE.
 *
 * @note		The overall maximum advertising data size is BLE_GAP_ADV_MAX_SIZE (= 31 Bytes from ble_gap.h). Therefore, the application needs to take care
 *				to choose the custom_advdata_size small enough that all information could be advertised 
 *				(especially the ADVERTISING_DEVICE_NAME might be cut off, if custom_advdata_size is too big).
 */
ret_code_t 	ble_set_advertising_custom_advdata(uint16_t company_identifier, uint8_t* custom_advdata, uint16_t custom_advdata_size);


/**@brief	Function to start the advertising process with the parameters: ADVERTISING_DEVICE_NAME, ADVERTISING_INTERVAL_MS, ADVERTISING_TIMEOUT_SECONDS.
 *
 * @details Internally the advertising is stopped by the BLE-Stack automatically. 
 *			When this happens the advertising process is started again if it wasn't stopped by ble_stop_advertising().
 *
 * @retval	NRF_SUCCESS On success, else an error code indicating reason for failure.
 */
ret_code_t 	ble_start_advertising(void);


/**@brief Function to stop the advertising process.
 */
void 		ble_stop_advertising(void);


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
ret_code_t 	ble_start_scanning(uint16_t scan_interval_ms, uint16_t scan_window_ms, uint16_t scan_duration_seconds);


/**@brief	Function for stopping any ongoing scan-operation.
 */
void 		ble_stop_scanning(void);


/**@brief Function to retrieve the MAC-address.
 * 
 * @param[out] MAC_address		Pointer to array where to store the MAC-address.
 *
 * @note	The MAC_address-array has to have a size of at least BLE_GAP_ADDR_LEN (= 6 from ble_gap.h) bytes.
 *			The ble_init()-function has to be called, before calling this function.
 */
void 		ble_get_MAC_address(uint8_t* MAC_address);


/**@brief	Function for retrieve the current BLE-state.
 *
 * @retval 	The current BLE-State.
 */
ble_state_t ble_get_state(void);


/**@brief	Function to transmit data via the established BLE-connection.
 *
 * @retval	NRF_SUCCESS If the data were sent successfully. Otherwise, an error code is returned.
 * 
 * @note 	A BLE-connection has to be established before calling this function to work.
 */
ret_code_t 	ble_transmit(uint8_t* data, uint16_t len);



/**@brief	Function to set the on receive callback function.
 *
 * @param [in] 	ble_on_receive_callback		The callback function that should be called.
 */
void 		ble_set_on_receive_callback(ble_on_receive_callback_t 			ble_on_receive_callback);

/**@brief	Function to set the on transmit callback function.
 *
 * @param [in] 	ble_on_transmit_callback		The callback function that should be called.
 */
void 		ble_set_on_transmit_callback(ble_on_transmit_callback_t 		ble_on_transmit_callback);

/**@brief	Function to set the on connect callback function.
 *
 * @param [in] 	ble_on_connect_callback		The callback function that should be called.
 */
void 		ble_set_on_connect_callback(ble_on_connect_callback_t 			ble_on_connect_callback);

/**@brief	Function to set the on disconnect callback function.
 *
 * @param [in] 	ble_on_disconnect_callback		The callback function that should be called.
 */
void 		ble_set_on_disconnect_callback(ble_on_disconnect_callback_t 	ble_on_disconnect_callback);

/**@brief	Function to set the on scan timeout callback function.
 *
 * @param [in] 	ble_on_scan_timeout_callback	The callback function that should be called.
 */
void 		ble_set_on_scan_timeout_callback(ble_on_scan_timeout_callback_t ble_on_scan_timeout_callback);

/**@brief	Function to set the on scan report callback function.
 *
 * @param [in] 	ble_on_scan_report_callback		The callback function that should be called.
 */
void 		ble_set_on_scan_report_callback(ble_on_scan_report_callback_t 	ble_on_scan_report_callback);


#endif