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
 * @note storer_init() has to be called before.
 */
void advertiser_init(void);

/**@brief	Function to start the advertising process with the parameters: ADVERTISING_DEVICE_NAME, ADVERTISING_INTERVAL_MS, ADVERTISING_TIMEOUT_SECONDS.
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


/**@brief Function to set the is_clock_synced-status flag of the advertising-packet.
 *
 * @param[in]	is_clock_synced						Flag if clock is synchronized.
 */
void advertiser_set_status_flag_is_clock_synced(uint8_t is_clock_synced);

/**@brief Function to set the microphone_enabled-status flag of the advertising-packet.
 *
 * @param[in]	microphone_enabled					Flag if microphone is enabled.
 */
void advertiser_set_status_flag_microphone_enabled(uint8_t microphone_enabled);

/**@brief Function to set the scan_enabled-status flag of the advertising-packet.
 *
 * @param[in]	scan_enabled						Flag if scanner is enabled.
 */
void advertiser_set_status_flag_scan_enabled(uint8_t scan_enabled);

/**@brief Function to set the accelerometer_enabled-status flag of the advertising-packet.
 *
 * @param[in]	accelerometer_enabled			Flag if accelerometer is enabled.
 */
void advertiser_set_status_flag_accelerometer_enabled(uint8_t accelerometer_enabled);

/**@brief Function to set the accelerometer_interrupt_enabled-status flag of the advertising-packet.
 *
 * @param[in]	accelerometer_interrupt_enabled		Flag if accelerometer interrupt is enabled.
 */
void advertiser_set_status_flag_accelerometer_interrupt_enabled(uint8_t accelerometer_interrupt_enabled);

/**@brief Function to set the battery_enabled-status flag of the advertising-packet.
 *
 * @param[in]	battery_enabled		Flag if battery is enabled.
 */
void advertiser_set_status_flag_battery_enabled(uint8_t battery_enabled);


/**@brief Function to retrieve the own badge-assignment.
 *
 * @param[out]	badge_assignement	Pointer where to store the badge_assignement.
 */
void advertiser_get_badge_assignement(BadgeAssignement* badge_assignement);


/**@brief Function to retrieve the badge assignement from a custom_advdata-packet.
 *
 * @param[out]	badge_assignement	Pointer where to store the badge_assignement.
 * @param[in]	custom_advdata		Pointer to custom_advdata.
 */
void advertiser_get_badge_assignement_from_advdata(BadgeAssignement* badge_assignement, void* custom_advdata);


/**@brief Function to retrieve the length of the manufacture-data.
 *
 * @retval Length of the manufacture data.
 */
uint8_t advertiser_get_manuf_data_len(void);

/**@brief Function to retrieve the is_clock_synced-status flag of the advertising-packet.
 *
 * @retval 	0			If clock is unsynced.
 * @retval 	1			If clock is synced.
 */
uint8_t advertiser_get_status_flag_is_clock_synced(void);

/**@brief Function to retrieve the microphone_enabled-status flag of the advertising-packet.
 *
 * @retval 	0			If microphone is not enabled.
 * @retval 	1			If microphone is enabled.
 */
uint8_t advertiser_get_status_flag_microphone_enabled(void);

/**@brief Function to retrieve the scan_enabled-status flag of the advertising-packet.
 *
 * @retval 	0			If scanner is not enabled.
 * @retval 	1			If scanner is enabled.
 */
uint8_t advertiser_get_status_flag_scan_enabled(void);

/**@brief Function to retrieve the accelerometer-status flag of the advertising-packet.
 *
 * @retval 	0			If accelerometer is not enabled.
 * @retval 	1			If accelerometer_ is enabled.
 */
uint8_t advertiser_get_status_flag_accelerometer_enabled(void);

/**@brief Function to retrieve the accelerometer_interrupt-status flag of the advertising-packet.
 *
 * @retval 	0			If accelerometer_interrupt is not enabled.
 * @retval 	1			If accelerometer_interrupt is enabled.
 */
uint8_t advertiser_get_status_flag_accelerometer_interrupt_enabled(void);

/**@brief Function to retrieve the battery-status flag of the advertising-packet.
 *
 * @retval 	0			If battery is not enabled.
 * @retval 	1			If battery is enabled.
 */
uint8_t advertiser_get_status_flag_battery_enabled(void);


#endif
