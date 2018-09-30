#include "advertiser_lib.h"
#include "ble_lib.h"
#include "string.h" // For memset
#include "storer_lib.h"



#define CUSTOM_COMPANY_IDENTIFIER	0xFF00

/**< Structure for organizing custom badge advertising data */
typedef struct
{
    uint8_t battery;   // scaled so that voltage = 1 + 0.01 * battery
    uint8_t status_flags;
    uint16_t ID;
    uint8_t group;
    uint8_t MAC[6];
} custom_advdata_t;


static custom_advdata_t custom_advdata;	/**< The custom advertising data structure where the current configuration is stored. */


extern uint16_t crc16_compute(uint8_t const * p_data, uint32_t size, uint16_t const * p_crc); /**< In filesystem_lib.c */


void advertiser_init(void) {	
	memset(&custom_advdata, 0, sizeof(custom_advdata));
	
	ble_get_MAC_address(custom_advdata.MAC);
	
	// Try to read the badge-assignement from the filesystem/storer:
	BadgeAssignement badge_assignement;
	ret_code_t ret = storer_read_badge_assignement(&badge_assignement);
	if(ret == NRF_SUCCESS) {
		custom_advdata.group = badge_assignement.group;
		custom_advdata.ID = badge_assignement.ID;
	} else {
		custom_advdata.group = 0;
		custom_advdata.ID = crc16_compute(custom_advdata.MAC, 6, NULL);
	}
	
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, sizeof(custom_advdata));
}

ret_code_t advertiser_start_advertising(void) {
	return ble_start_advertising();
}

void advertiser_stop_advertising(void) {
	ble_stop_advertising();
}


void advertiser_set_battery_voltage(float voltage) {
	int32_t scaled_battery_voltage = ((int32_t)(100.0*voltage)) - 100;
	scaled_battery_voltage = (scaled_battery_voltage < 0) ? 0 : scaled_battery_voltage;
	scaled_battery_voltage = (scaled_battery_voltage > 255) ? 255 : scaled_battery_voltage;
	custom_advdata.battery = (uint8_t) scaled_battery_voltage;
	
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, sizeof(custom_advdata));
}


void advertiser_set_badge_assignement(BadgeAssignement badge_assignement) {
	custom_advdata.ID = badge_assignement.ID;
	custom_advdata.group = badge_assignement.group;	
	
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, sizeof(custom_advdata));
}

void advertiser_set_status_flag_is_clock_synced(uint8_t is_clock_synced) {
	custom_advdata.status_flags |= (is_clock_synced) ? 					(1 << 0) : 0;
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, sizeof(custom_advdata));
}


void advertiser_set_status_flag_microphone_enabled(uint8_t microphone_enabled) {
	custom_advdata.status_flags |= (microphone_enabled) ? 				(1 << 1) : 0;
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, sizeof(custom_advdata));
}


void advertiser_set_status_flag_scan_enabled(uint8_t scan_enabled) {
	custom_advdata.status_flags |= (scan_enabled) ? 					(1 << 2) : 0;
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, sizeof(custom_advdata));
}


void advertiser_set_status_flag_accelerometer_enabled(uint8_t accelerometer_enabled) {
	custom_advdata.status_flags |= (accelerometer_enabled) ? 			(1 << 3) : 0;
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, sizeof(custom_advdata));
}

void advertiser_set_status_flag_accelerometer_interrupt_enabled(uint8_t accelerometer_interrupt_enabled) {
	custom_advdata.status_flags |= (accelerometer_interrupt_enabled) ? (1 << 4) : 0;
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, sizeof(custom_advdata));
}




void advertiser_get_badge_assignement(BadgeAssignement* badge_assignement) {
	badge_assignement->ID = custom_advdata.ID;
	badge_assignement->group = custom_advdata.group;
}

void advertiser_get_badge_assignement_from_advdata(BadgeAssignement* badge_assignement, void* custom_advdata) {
	custom_advdata_t* tmp = (custom_advdata_t*) custom_advdata;
	badge_assignement->ID = tmp->ID;
	badge_assignement->group = tmp->group;
}

uint8_t advertiser_get_manuf_data_len(void) {
	return (uint8_t) (sizeof(custom_advdata)) + 2;	// + 2 for the CUSTOM_COMPANY_IDENTIFIER
}


uint8_t advertiser_get_status_flag_is_clock_synced(void) {
	return (custom_advdata.status_flags & (1 << 0)) ? 1 : 0;
}


uint8_t advertiser_get_status_flag_microphone_enabled(void) {
	return (custom_advdata.status_flags & (1 << 1)) ? 1 : 0;
}


uint8_t advertiser_get_status_flag_scan_enabled(void) {
	return (custom_advdata.status_flags & (1 << 2)) ? 1 : 0;
}


uint8_t advertiser_get_status_flag_accelerometer_enabled(void) {
	return (custom_advdata.status_flags & (1 << 3)) ? 1 : 0;
}

uint8_t advertiser_get_status_flag_accelerometer_interrupt_enabled(void) {
	return (custom_advdata.status_flags & (1 << 4)) ? 1 : 0;
}
