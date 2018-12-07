#include "scanner_lib.h"
#include "ble_lib.h"
#include "advertiser_lib.h"
#include "string.h"

#include "debug_lib.h"

// Values from iBeacon spec
#define COMPANY_ID_APPLE  		0x004C
#define IBEACON_TYPE_PROXIMITY 	0x1502
#define IBEACON_MANUF_DATA_LEN 	0x19

/**< Structure of iBeacon manufacturer-specific data, after the 0xFF type specifier byte. */
typedef struct
{
    uint16_t company_ID;
    uint16_t type;
    uint8_t UUID[16];
    uint8_t major[2];  // big-endian
    uint8_t minor[2];  // big-endian
    uint8_t measured_power;
} iBeacon_data_t;

/**< The internal handlers for scan-timeout and -report events. */
static void internal_on_scan_report_callback(ble_gap_evt_adv_report_t* scan_report);
static void internal_on_scan_timeout_callback(void);

/**< The external handlers for scan-timeout and -report events. */
static scanner_on_scan_timeout_callback_t	external_on_scan_timeout_callback = NULL;
static scanner_on_scan_report_callback_t	external_on_scan_report_callback  = NULL;





/**@brief Function that is called when a scan timeout occurs.
 */
static void internal_on_scan_timeout_callback(void) {
	if(external_on_scan_timeout_callback != NULL)
		external_on_scan_timeout_callback();
}

/**@brief Function that is called when a scan report is available.
 * 
 * @param[in]	scan_report		The scan report of a BLE device.
 */
static void internal_on_scan_report_callback(ble_gap_evt_adv_report_t* scan_report) {
	
	
	int8_t rssi = scan_report->rssi;
	
	 if (rssi < SCANNER_MINIMUM_RSSI)  {
        return;  // ignore signals that are too weak.
    }
	
	scanner_scan_report_t scanner_scan_report;
	scanner_scan_report.rssi = rssi;
	scanner_scan_report.scanner_scan_device_type = SCAN_DEVICE_TYPE_UNKNOWN;
	
	uint8_t data_len = scan_report->dlen;
	uint8_t* data = (uint8_t*) scan_report->data;
	
	uint8_t* name_ptr = NULL;
	uint8_t* manuf_data_ptr = NULL;
	uint8_t manuf_data_len = 0;
	uint8_t index = 0;
	
	
	
	
	while ((name_ptr == NULL || manuf_data_ptr == NULL) && index < data_len)  {
        uint8_t field_len = data[index];
        index++;
        uint8_t field_type = data[index];
        if (field_type == BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME || field_type == BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME)  {
            name_ptr = &data[index + 1];  // skip field type byte
        }
        else if (field_type == BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA)  {
            manuf_data_ptr = &data[index + 1];  // skip field type byte
            manuf_data_len = field_len - 1;
        }
        index += field_len;
    }
	
	// To get the advertising message structure
	/*
	char tmp[200];
	sprintf(tmp, "Len (%u): ", data_len);
	for(uint8_t i = 0; i < data_len; i++)
		sprintf(&tmp[strlen(tmp)], "%02X, ", data[i]);
	debug_log("SCANNER: Scan: %u, %u, %d, %s\n", name_ptr-data, manuf_data_ptr-data, rssi, tmp);
	*/
	if (manuf_data_len == advertiser_get_manuf_data_len())  {
        if (name_ptr != NULL && memcmp(name_ptr,(const uint8_t *)ADVERTISING_DEVICE_NAME,strlen(ADVERTISING_DEVICE_NAME)) == 0)  {
           
			scanner_scan_report.scanner_scan_device_type = SCAN_DEVICE_TYPE_BADGE;
			BadgeAssignement badge_assignement;
			
			advertiser_get_badge_assignement_from_advdata(&badge_assignement, &manuf_data_ptr[2]);
			scanner_scan_report.ID = badge_assignement.ID;
			scanner_scan_report.group = badge_assignement.group;
			

           //debug_log("SCANNER: ---Badge seen: group %d, ID %.4X, rssi %d.\r\n", scanner_scan_report.group, scanner_scan_report.ID, scanner_scan_report.rssi);
        }
    } else if (manuf_data_len == IBEACON_MANUF_DATA_LEN)  {
        iBeacon_data_t iBeacon_data;
        memcpy(&iBeacon_data, manuf_data_ptr, IBEACON_MANUF_DATA_LEN);  // ensure data is properly aligned
        if (iBeacon_data.company_ID == COMPANY_ID_APPLE && iBeacon_data.type == IBEACON_TYPE_PROXIMITY)  {
			scanner_scan_report.scanner_scan_device_type = SCAN_DEVICE_TYPE_IBEACON;
			
            // major/minor values are big-endian
            uint16_t major = ((uint16_t)iBeacon_data.major[0] * 256) + iBeacon_data.major[1];
            (void) major; // Unused variable
            uint16_t minor = ((uint16_t)iBeacon_data.minor[0] * 256) + iBeacon_data.minor[1];
            
			scanner_scan_report.ID = minor; // take only lower byte of major value
			scanner_scan_report.group = iBeacon_data.major[1];
			//debug_log("SCANNER: ---iBeacon seen: major %d, minor %d, rssi %d.\r\n", major, minor, rssi);
        }
    }
	
	if(scanner_scan_report.scanner_scan_device_type == SCAN_DEVICE_TYPE_UNKNOWN) {
		//debug_log("SCANNER: Unknown device seen: rssi %d.\r\n", rssi);
		return;
	}
	
	if(external_on_scan_report_callback != NULL)
		external_on_scan_report_callback(&scanner_scan_report);
	
}


void scanner_init(void) {
	ble_set_on_scan_timeout_callback(internal_on_scan_timeout_callback);
	ble_set_on_scan_report_callback(internal_on_scan_report_callback);
}


void scanner_set_on_scan_timeout_callback(scanner_on_scan_timeout_callback_t scanner_on_scan_timeout_callback) {
	external_on_scan_timeout_callback = scanner_on_scan_timeout_callback;
}



void scanner_set_on_scan_report_callback(scanner_on_scan_report_callback_t 	scanner_on_scan_report_callback) {
	external_on_scan_report_callback = scanner_on_scan_report_callback;
}


ret_code_t scanner_start_scanning(uint16_t scan_interval_ms, uint16_t scan_window_ms, uint16_t scan_duration_seconds) {
	return ble_start_scanning(scan_interval_ms, scan_window_ms, scan_duration_seconds);
}


void scanner_stop_scanning(void) {
	ble_stop_scanning();
}
