#ifndef __ADVERTISER_LIB_H
#define __ADVERTISER_LIB_H

#include "stdint.h"
#include "sdk_errors.h"
#include "common_messages.h"	// For BadgeAssignment


#define ADVERTISING_DEVICE_NAME			"HDBDG"     /**< Name of device. Will be included in the advertising data. */ 
#define ADVERTISING_INTERVAL_MS		  	200   		/**< The advertising interval. */
#define ADVERTISING_TIMEOUT_SECONDS    	6           /**< The advertising timeout interval. */


/**@brief Function to initialize the advertiser.
 *
 * @note ble_init() has to be called before.
 */
void advertiser_init(void);

/**@brief	Function to start the advertising process with the parameters: ADVERTISING_DEVICE_NAME, ADVERTISING_INTERVAL_MS, ADVERTISING_TIMEOUT_SECONDS from ble_lib.h.
 *
 * @retval	NRF_SUCCESS On success, else an error code indicating reason for failure.
 */
ret_code_t advertiser_start_advertising(void);

/**@brief Function to stop the advertising process.
 */
void advertiser_stop_advertising(void);

/**@brief Function to set the badge assignement (ID + group) of the advertising-packet.
 *
 * @param[in]	voltage				The battery voltage to set.
 */
void advertiser_set_battery_voltage(float voltage);

/**@brief Function to set the badge assignement (ID + group) of the advertising-packet.
 *
 * @param[in]	badge_assignement		The badge assignement to set.
 */
void advertiser_set_badge_assignement(BadgeAssignement badge_assignement);


/**@brief Function to set the status flags of the advertising-packet.
 *
 * @param[in]	is_clock_synced						Flag if clock is synchronized.
 * @param[in]	microphone_enabled					Flag if microphone is enabled.
 * @param[in]	scan_enabled						Flag if scanner is enabled.
 * @param[in]	accelerometer_data_enabled			Flag if accelerometer data are enabled.
 * @param[in]	accelerometer_interrupt_enabled		Flag if accelerometer interrupt is enabled.
 */
void advertiser_set_status_flags(uint8_t is_clock_synced, uint8_t microphone_enabled, uint8_t scan_enabled, uint8_t accelerometer_data_enabled, uint8_t accelerometer_interrupt_enabled);



/**@brief Function to retrieve the badge assignement from a custom_advdata-packet.
 *
 * @param[out]	badge_assignement	Pointer where to store the badge_assignement.
 * @param[in]	custom_advdata		Pointer to custom_advdata.
 */
void advertiser_get_badge_assignement(BadgeAssignement* badge_assignement, void* custom_advdata);


/**@brief Function to retrieve the length of the manufacture-data.
 *
 * @retval Length of the manufacture data.
 */
uint8_t advertiser_get_manuf_data_len(void);

#endif
