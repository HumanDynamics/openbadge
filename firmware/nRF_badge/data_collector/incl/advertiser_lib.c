#include "advertiser_lib.h"
#include "ble_lib.h"
#include "string.h" // For memset
#include "storer_lib.h"
#include "debug_lib.h"


#define CUSTOM_COMPANY_IDENTIFIER	0xFF00
#define CUSTOM_ADVDATA_LEN			11

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

static void advertiser_get_default_badge_assignement(BadgeAssignement* badge_assignement) {
	badge_assignement->ID = crc16_compute(custom_advdata.MAC, 6, NULL);;
	badge_assignement->group = ADVERTISING_DEFAULT_GROUP;
}



void advertiser_init(void) {	
	memset(&custom_advdata, 0, sizeof(custom_advdata));
	// We need to swap the MAC address to be compatible with old code..
	uint8_t MAC[6];
	ble_get_MAC_address(MAC);
	for(uint8_t i = 0; i < 6; i++) {
		custom_advdata.MAC[i] = MAC[5-i];
	}
	
	// Try to read the badge-assignement from the filesystem/storer:
	BadgeAssignement badge_assignement;
	ret_code_t ret = storer_read_badge_assignement(&badge_assignement);
	if(ret != NRF_SUCCESS) {
		advertiser_get_default_badge_assignement(&badge_assignement);
		debug_log("ADVERTISER: Could not find badge assignement in storage -> Take default: (%u, %u)\n", badge_assignement.ID, badge_assignement.group);	
	} else {
		debug_log("ADVERTISER: Read out badge assignement from storage: (%u, %u)\n", badge_assignement.ID, badge_assignement.group);	
	}
	custom_advdata.group = badge_assignement.group;
	custom_advdata.ID = badge_assignement.ID;
	
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
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
	
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}




ret_code_t advertiser_set_badge_assignement(BadgeAssignement badge_assignement) {
	
	uint8_t reset_badge_assignement = 0;
	if(badge_assignement.ID == ADVERTISING_RESET_ID && badge_assignement.group == ADVERTISING_RESET_GROUP) {
		advertiser_get_default_badge_assignement(&badge_assignement);
		debug_log("ADVERTISER: Reset badge assignement -> Take default: (%u, %u)\n", badge_assignement.ID, badge_assignement.group);
		reset_badge_assignement = 1;
	}
	
	custom_advdata.ID = badge_assignement.ID;
	custom_advdata.group = badge_assignement.group;	
	
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
	
	
	// Check if we need to reset the badge assignement, or if we should store it.
	if(reset_badge_assignement) {
		debug_log("ADVERTISER: Delete badge assignement from storage...\n");
		return storer_clear_badge_assignement();		
	} else {	// We want to store the new badge assignement
		// First read if we have already the correct badge-assignement stored:
		BadgeAssignement stored_badge_assignement;
		ret_code_t ret = storer_read_badge_assignement(&stored_badge_assignement);
		if(ret == NRF_ERROR_INVALID_STATE || ret == NRF_ERROR_INVALID_DATA || (ret == NRF_SUCCESS && (stored_badge_assignement.ID != badge_assignement.ID || stored_badge_assignement.group != badge_assignement.group))) {
			// If we have not found any entry or the badge assignement mismatches --> store it.
			debug_log("ADVERTISER: Badge assignement missmatch: --> setting the new badge assignement: Old (%u, %u), New (%u, %u)\n", stored_badge_assignement.ID, stored_badge_assignement.group, badge_assignement.ID, badge_assignement.group);
			return storer_store_badge_assignement(&badge_assignement);
			/*
			if(ret == NRF_ERROR_INTERNAL) {
				return NRF_ERROR_INTERNAL;
			} else if (ret != NRF_SUCCESS) {	// There is an error in the configuration of the badge-assignement partition
				return NRF_ERROR_NOT_SUPPORTED;
			} 
			*/
		} else if(ret == NRF_ERROR_INTERNAL) {
			return NRF_ERROR_INTERNAL;
		} else {
			debug_log("ADVERTISER: Badge assignement already up to date  (%u, %u)\n", badge_assignement.ID, badge_assignement.group);
		}
	}
	
	return NRF_SUCCESS;
}

void advertiser_set_status_flag_is_clock_synced(uint8_t is_clock_synced) {
	if(is_clock_synced)
		custom_advdata.status_flags |= (1 << 0);
	else
		custom_advdata.status_flags &= ~(1 << 0);
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}


void advertiser_set_status_flag_microphone_enabled(uint8_t microphone_enabled) {
	if(microphone_enabled)
		custom_advdata.status_flags |= (1 << 1);
	else
		custom_advdata.status_flags &= ~(1 << 1);
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}


void advertiser_set_status_flag_scan_enabled(uint8_t scan_enabled) {
	if(scan_enabled)
		custom_advdata.status_flags |= (1 << 2);
	else
		custom_advdata.status_flags &= ~(1 << 2);
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}


void advertiser_set_status_flag_accelerometer_enabled(uint8_t accelerometer_enabled) {
	if(accelerometer_enabled)
		custom_advdata.status_flags |= (1 << 3);
	else
		custom_advdata.status_flags &= ~(1 << 3);
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}

void advertiser_set_status_flag_accelerometer_interrupt_enabled(uint8_t accelerometer_interrupt_enabled) {
	if(accelerometer_interrupt_enabled)
		custom_advdata.status_flags |= (1 << 4);
	else
		custom_advdata.status_flags &= ~(1 << 4);
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}

void advertiser_set_status_flag_battery_enabled(uint8_t battery_enabled) {
	if(battery_enabled)
		custom_advdata.status_flags |= (1 << 5);
	else
		custom_advdata.status_flags &= ~(1 << 5);
	ble_set_advertising_custom_advdata(CUSTOM_COMPANY_IDENTIFIER, (uint8_t*) &custom_advdata, CUSTOM_ADVDATA_LEN);
}




void advertiser_get_badge_assignement(BadgeAssignement* badge_assignement) {
	badge_assignement->ID = custom_advdata.ID;
	badge_assignement->group = custom_advdata.group;
}

void advertiser_get_badge_assignement_from_advdata(BadgeAssignement* badge_assignement, void* custom_advdata) {
	custom_advdata_t tmp;
	memset(&tmp, 0, sizeof(tmp));
	memcpy(&tmp, custom_advdata, CUSTOM_ADVDATA_LEN);
	
	badge_assignement->ID = tmp.ID;
	badge_assignement->group = tmp.group;
}

uint8_t advertiser_get_manuf_data_len(void) {
	return (uint8_t) (CUSTOM_ADVDATA_LEN + 2);	// + 2 for the CUSTOM_COMPANY_IDENTIFIER
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

uint8_t advertiser_get_status_flag_battery_enabled(void) {
	return (custom_advdata.status_flags & (1 << 5)) ? 1 : 0;
}
