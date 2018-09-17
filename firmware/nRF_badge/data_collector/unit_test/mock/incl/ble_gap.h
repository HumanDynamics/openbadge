/**@file 
  @details Partial copy of the ble_gap.h library 
 */

#ifndef BLE_GAP_H__
#define BLE_GAP_H__

#include "ble_types.h"
#include "ble_ranges.h"
//#include "nrf_svc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@addtogroup BLE_GAP_ENUMERATIONS Enumerations
 * @{ */

/**@brief GAP API SVC numbers.
 */
enum BLE_GAP_SVCS
{
  SD_BLE_GAP_ADDRESS_SET  = BLE_GAP_SVC_BASE,  /**< Set own Bluetooth Address. */
  SD_BLE_GAP_ADDRESS_GET,                      /**< Get own Bluetooth Address. */
  SD_BLE_GAP_ADV_DATA_SET,                     /**< Set Advertising Data. */
  SD_BLE_GAP_ADV_START,                        /**< Start Advertising. */
  SD_BLE_GAP_ADV_STOP,                         /**< Stop Advertising. */
  SD_BLE_GAP_CONN_PARAM_UPDATE,                /**< Connection Parameter Update. */
  SD_BLE_GAP_DISCONNECT,                       /**< Disconnect. */
  SD_BLE_GAP_TX_POWER_SET,                     /**< Set TX Power. */
  SD_BLE_GAP_APPEARANCE_SET,                   /**< Set Appearance. */
  SD_BLE_GAP_APPEARANCE_GET,                   /**< Get Appearance. */
  SD_BLE_GAP_PPCP_SET,                         /**< Set PPCP. */
  SD_BLE_GAP_PPCP_GET,                         /**< Get PPCP. */
  SD_BLE_GAP_DEVICE_NAME_SET,                  /**< Set Device Name. */
  SD_BLE_GAP_DEVICE_NAME_GET,                  /**< Get Device Name. */
  SD_BLE_GAP_AUTHENTICATE,                     /**< Initiate Pairing/Bonding. */
  SD_BLE_GAP_SEC_PARAMS_REPLY,                 /**< Reply with Security Parameters. */
  SD_BLE_GAP_AUTH_KEY_REPLY,                   /**< Reply with an authentication key. */
  SD_BLE_GAP_LESC_DHKEY_REPLY,                 /**< Reply with an LE Secure Connections DHKey. */
  SD_BLE_GAP_KEYPRESS_NOTIFY,                  /**< Notify of a keypress during an authentication procedure. */
  SD_BLE_GAP_LESC_OOB_DATA_GET,                /**< Get the local LE Secure Connections OOB data. */
  SD_BLE_GAP_LESC_OOB_DATA_SET,                /**< Set the remote LE Secure Connections OOB data. */
  SD_BLE_GAP_ENCRYPT,                          /**< Initiate encryption procedure. */
  SD_BLE_GAP_SEC_INFO_REPLY,                   /**< Reply with Security Information. */
  SD_BLE_GAP_CONN_SEC_GET,                     /**< Obtain connection security level. */
  SD_BLE_GAP_RSSI_START,                       /**< Start reporting of changes in RSSI. */
  SD_BLE_GAP_RSSI_STOP,                        /**< Stop reporting of changes in RSSI. */
  SD_BLE_GAP_SCAN_START,                       /**< Start Scanning. */
  SD_BLE_GAP_SCAN_STOP,                        /**< Stop Scanning. */
  SD_BLE_GAP_CONNECT,                          /**< Connect. */
  SD_BLE_GAP_CONNECT_CANCEL,                   /**< Cancel ongoing connection procedure. */
  SD_BLE_GAP_RSSI_GET,                         /**< Get the last RSSI sample. */
};

/**@brief GAP Event IDs.
 * IDs that uniquely identify an event coming from the stack to the application.
 */
enum BLE_GAP_EVTS
{
  BLE_GAP_EVT_CONNECTED  = BLE_GAP_EVT_BASE,    /**< Connection established.                         \n See @ref ble_gap_evt_connected_t.            */
  BLE_GAP_EVT_DISCONNECTED,                     /**< Disconnected from peer.                         \n See @ref ble_gap_evt_disconnected_t.         */
  BLE_GAP_EVT_CONN_PARAM_UPDATE,                /**< Connection Parameters updated.                  \n See @ref ble_gap_evt_conn_param_update_t.    */
  BLE_GAP_EVT_SEC_PARAMS_REQUEST,               /**< Request to provide security parameters.         \n Reply with @ref sd_ble_gap_sec_params_reply.  \n See @ref ble_gap_evt_sec_params_request_t. */
  BLE_GAP_EVT_SEC_INFO_REQUEST,                 /**< Request to provide security information.        \n Reply with @ref sd_ble_gap_sec_info_reply.    \n See @ref ble_gap_evt_sec_info_request_t.   */
  BLE_GAP_EVT_PASSKEY_DISPLAY,                  /**< Request to display a passkey to the user.       \n In LESC Numeric Comparison, reply with @ref sd_ble_gap_auth_key_reply. \n See @ref ble_gap_evt_passkey_display_t. */
  BLE_GAP_EVT_KEY_PRESSED,                      /**< Notification of a keypress on the remote device.\n See @ref ble_gap_evt_key_pressed_t           */
  BLE_GAP_EVT_AUTH_KEY_REQUEST,                 /**< Request to provide an authentication key.       \n Reply with @ref sd_ble_gap_auth_key_reply.    \n See @ref ble_gap_evt_auth_key_request_t.   */
  BLE_GAP_EVT_LESC_DHKEY_REQUEST,               /**< Request to calculate an LE Secure Connections DHKey. \n Reply with @ref sd_ble_gap_lesc_dhkey_reply.  \n See @ref ble_gap_evt_lesc_dhkey_request_t */
  BLE_GAP_EVT_AUTH_STATUS,                      /**< Authentication procedure completed with status. \n See @ref ble_gap_evt_auth_status_t.          */
  BLE_GAP_EVT_CONN_SEC_UPDATE,                  /**< Connection security updated.                    \n See @ref ble_gap_evt_conn_sec_update_t.      */
  BLE_GAP_EVT_TIMEOUT,                          /**< Timeout expired.                                \n See @ref ble_gap_evt_timeout_t.              */
  BLE_GAP_EVT_RSSI_CHANGED,                     /**< RSSI report.                                    \n See @ref ble_gap_evt_rssi_changed_t.         */
  BLE_GAP_EVT_ADV_REPORT,                       /**< Advertising report.                             \n See @ref ble_gap_evt_adv_report_t.           */
  BLE_GAP_EVT_SEC_REQUEST,                      /**< Security Request.                               \n See @ref ble_gap_evt_sec_request_t.          */
  BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST,        /**< Connection Parameter Update Request.            \n Reply with @ref sd_ble_gap_conn_param_update. \n See @ref ble_gap_evt_conn_param_update_request_t. */
  BLE_GAP_EVT_SCAN_REQ_REPORT,                  /**< Scan request report.                            \n See @ref ble_gap_evt_scan_req_report_t.      */
};

/**@brief GAP Option IDs.
 * IDs that uniquely identify a GAP option.
 */
enum BLE_GAP_OPTS
{
  BLE_GAP_OPT_CH_MAP  = BLE_GAP_OPT_BASE,       /**< Channel Map. @ref ble_gap_opt_ch_map_t  */
  BLE_GAP_OPT_LOCAL_CONN_LATENCY,               /**< Local connection latency. @ref ble_gap_opt_local_conn_latency_t */
  BLE_GAP_OPT_PASSKEY,                          /**< Set passkey. @ref ble_gap_opt_passkey_t */
  BLE_GAP_OPT_PRIVACY,                          /**< Custom privacy. @ref ble_gap_opt_privacy_t */
  BLE_GAP_OPT_SCAN_REQ_REPORT,                  /**< Scan request report. @ref ble_gap_opt_scan_req_report_t */
  BLE_GAP_OPT_COMPAT_MODE                       /**< Compatibility mode. @ref ble_gap_opt_compat_mode_t */
};

/** @} */

/**@addtogroup BLE_GAP_DEFINES Defines
 * @{ */

/**@defgroup BLE_ERRORS_GAP SVC return values specific to GAP
 * @{ */
#define BLE_ERROR_GAP_UUID_LIST_MISMATCH            (NRF_GAP_ERR_BASE + 0x000)  /**< UUID list does not contain an integral number of UUIDs. */
#define BLE_ERROR_GAP_DISCOVERABLE_WITH_WHITELIST   (NRF_GAP_ERR_BASE + 0x001)  /**< Use of Whitelist not permitted with discoverable advertising. */
#define BLE_ERROR_GAP_INVALID_BLE_ADDR              (NRF_GAP_ERR_BASE + 0x002)  /**< The upper two bits of the address do not correspond to the specified address type. */
#define BLE_ERROR_GAP_WHITELIST_IN_USE              (NRF_GAP_ERR_BASE + 0x003)  /**< Attempt to overwrite the whitelist while already in use by another operation. */
/**@} */


/**@defgroup BLE_GAP_ROLES GAP Roles
 * @note Not explicitly used in peripheral API, but will be relevant for central API.
 * @{ */
#define BLE_GAP_ROLE_INVALID     0x0            /**< Invalid Role. */
#define BLE_GAP_ROLE_PERIPH      0x1            /**< Peripheral Role. */
#define BLE_GAP_ROLE_CENTRAL     0x2            /**< Central Role. */
/**@} */


/**@defgroup BLE_GAP_TIMEOUT_SOURCES GAP Timeout sources
 * @{ */
#define BLE_GAP_TIMEOUT_SRC_ADVERTISING                0x00 /**< Advertising timeout. */
#define BLE_GAP_TIMEOUT_SRC_SECURITY_REQUEST           0x01 /**< Security request timeout. */
#define BLE_GAP_TIMEOUT_SRC_SCAN                       0x02 /**< Scanning timeout. */
#define BLE_GAP_TIMEOUT_SRC_CONN                       0x03 /**< Connection timeout. */
/**@} */


/**@defgroup BLE_GAP_ADDR_TYPES GAP Address types
 * @{ */
#define BLE_GAP_ADDR_TYPE_PUBLIC                        0x00 /**< Public address. */
#define BLE_GAP_ADDR_TYPE_RANDOM_STATIC                 0x01 /**< Random Static address. */
#define BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE     0x02 /**< Private Resolvable address. */
#define BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE 0x03 /**< Private Non-Resolvable address. */
/**@} */

/**@defgroup BLE_GAP_ADDR_CYCLE_MODES GAP Address cycle modes
 * @{ */
#define BLE_GAP_ADDR_CYCLE_MODE_NONE      0x00 /**< Set addresses directly, no automatic address cycling. */
#define BLE_GAP_ADDR_CYCLE_MODE_AUTO      0x01 /**< Automatically generate and update private addresses. */
/** @} */

/**@brief The default interval in seconds at which a private address is refreshed when address cycle mode is @ref BLE_GAP_ADDR_CYCLE_MODE_AUTO.  */
#define BLE_GAP_DEFAULT_PRIVATE_ADDR_CYCLE_INTERVAL_S (60 * 15)

/** @brief BLE address length. */
#define BLE_GAP_ADDR_LEN            6


/**@defgroup BLE_GAP_AD_TYPE_DEFINITIONS GAP Advertising and Scan Response Data format
 * @note Found at https://www.bluetooth.org/Technical/AssignedNumbers/generic_access_profile.htm
 * @{ */
#define BLE_GAP_AD_TYPE_FLAGS                               0x01 /**< Flags for discoverability. */
#define BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE   0x02 /**< Partial list of 16 bit service UUIDs. */
#define BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE         0x03 /**< Complete list of 16 bit service UUIDs. */
#define BLE_GAP_AD_TYPE_32BIT_SERVICE_UUID_MORE_AVAILABLE   0x04 /**< Partial list of 32 bit service UUIDs. */
#define BLE_GAP_AD_TYPE_32BIT_SERVICE_UUID_COMPLETE         0x05 /**< Complete list of 32 bit service UUIDs. */
#define BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE  0x06 /**< Partial list of 128 bit service UUIDs. */
#define BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE        0x07 /**< Complete list of 128 bit service UUIDs. */
#define BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME                    0x08 /**< Short local device name. */
#define BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME                 0x09 /**< Complete local device name. */
#define BLE_GAP_AD_TYPE_TX_POWER_LEVEL                      0x0A /**< Transmit power level. */
#define BLE_GAP_AD_TYPE_CLASS_OF_DEVICE                     0x0D /**< Class of device. */
#define BLE_GAP_AD_TYPE_SIMPLE_PAIRING_HASH_C               0x0E /**< Simple Pairing Hash C. */
#define BLE_GAP_AD_TYPE_SIMPLE_PAIRING_RANDOMIZER_R         0x0F /**< Simple Pairing Randomizer R. */
#define BLE_GAP_AD_TYPE_SECURITY_MANAGER_TK_VALUE           0x10 /**< Security Manager TK Value. */
#define BLE_GAP_AD_TYPE_SECURITY_MANAGER_OOB_FLAGS          0x11 /**< Security Manager Out Of Band Flags. */
#define BLE_GAP_AD_TYPE_SLAVE_CONNECTION_INTERVAL_RANGE     0x12 /**< Slave Connection Interval Range. */
#define BLE_GAP_AD_TYPE_SOLICITED_SERVICE_UUIDS_16BIT       0x14 /**< List of 16-bit Service Solicitation UUIDs. */
#define BLE_GAP_AD_TYPE_SOLICITED_SERVICE_UUIDS_128BIT      0x15 /**< List of 128-bit Service Solicitation UUIDs. */
#define BLE_GAP_AD_TYPE_SERVICE_DATA                        0x16 /**< Service Data - 16-bit UUID. */
#define BLE_GAP_AD_TYPE_PUBLIC_TARGET_ADDRESS               0x17 /**< Public Target Address. */
#define BLE_GAP_AD_TYPE_RANDOM_TARGET_ADDRESS               0x18 /**< Random Target Address. */
#define BLE_GAP_AD_TYPE_APPEARANCE                          0x19 /**< Appearance. */
#define BLE_GAP_AD_TYPE_ADVERTISING_INTERVAL                0x1A /**< Advertising Interval. */
#define BLE_GAP_AD_TYPE_LE_BLUETOOTH_DEVICE_ADDRESS         0x1B /**< LE Bluetooth Device Address. */
#define BLE_GAP_AD_TYPE_LE_ROLE                             0x1C /**< LE Role. */
#define BLE_GAP_AD_TYPE_SIMPLE_PAIRING_HASH_C256            0x1D /**< Simple Pairing Hash C-256. */
#define BLE_GAP_AD_TYPE_SIMPLE_PAIRING_RANDOMIZER_R256      0x1E /**< Simple Pairing Randomizer R-256. */
#define BLE_GAP_AD_TYPE_SERVICE_DATA_32BIT_UUID             0x20 /**< Service Data - 32-bit UUID. */
#define BLE_GAP_AD_TYPE_SERVICE_DATA_128BIT_UUID            0x21 /**< Service Data - 128-bit UUID. */
#define BLE_GAP_AD_TYPE_URI                                 0x24 /**< URI */
#define BLE_GAP_AD_TYPE_3D_INFORMATION_DATA                 0x3D /**< 3D Information Data. */
#define BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA          0xFF /**< Manufacturer Specific Data. */
/**@} */


/**@defgroup BLE_GAP_ADV_FLAGS GAP Advertisement Flags
 * @{ */
#define BLE_GAP_ADV_FLAG_LE_LIMITED_DISC_MODE         (0x01)   /**< LE Limited Discoverable Mode. */
#define BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE         (0x02)   /**< LE General Discoverable Mode. */
#define BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED         (0x04)   /**< BR/EDR not supported. */
#define BLE_GAP_ADV_FLAG_LE_BR_EDR_CONTROLLER         (0x08)   /**< Simultaneous LE and BR/EDR, Controller. */
#define BLE_GAP_ADV_FLAG_LE_BR_EDR_HOST               (0x10)   /**< Simultaneous LE and BR/EDR, Host. */
#define BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE   (BLE_GAP_ADV_FLAG_LE_LIMITED_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED)   /**< LE Limited Discoverable Mode, BR/EDR not supported. */
#define BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE   (BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE | BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED)   /**< LE General Discoverable Mode, BR/EDR not supported. */
/**@} */


/**@defgroup BLE_GAP_ADV_INTERVALS GAP Advertising interval max and min
 * @{ */
#define BLE_GAP_ADV_INTERVAL_MIN        0x0020 /**< Minimum Advertising interval in 625 us units, i.e. 20 ms. */
#define BLE_GAP_ADV_NONCON_INTERVAL_MIN 0x00A0 /**< Minimum Advertising interval in 625 us units for non connectable mode, i.e. 100 ms. */
#define BLE_GAP_ADV_INTERVAL_MAX        0x4000 /**< Maximum Advertising interval in 625 us units, i.e. 10.24 s. */
 /**@}  */


/**@defgroup BLE_GAP_SCAN_INTERVALS GAP Scan interval max and min
 * @{ */
#define BLE_GAP_SCAN_INTERVAL_MIN       0x0004 /**< Minimum Scan interval in 625 us units, i.e. 2.5 ms. */
#define BLE_GAP_SCAN_INTERVAL_MAX       0x4000 /**< Maximum Scan interval in 625 us units, i.e. 10.24 s. */
 /** @}  */


/**@defgroup BLE_GAP_SCAN_WINDOW GAP Scan window max and min
 * @{ */
#define BLE_GAP_SCAN_WINDOW_MIN         0x0004 /**< Minimum Scan window in 625 us units, i.e. 2.5 ms. */
#define BLE_GAP_SCAN_WINDOW_MAX         0x4000 /**< Maximum Scan window in 625 us units, i.e. 10.24 s. */
 /** @}  */


/**@defgroup BLE_GAP_SCAN_TIMEOUT GAP Scan timeout max and min
 * @{ */
#define BLE_GAP_SCAN_TIMEOUT_MIN        0x0001 /**< Minimum Scan timeout in seconds. */
#define BLE_GAP_SCAN_TIMEOUT_MAX        0xFFFF /**< Maximum Scan timeout in seconds. */
 /** @}  */


/**@brief Maximum size of advertising data in octets. */
#define  BLE_GAP_ADV_MAX_SIZE           31


/**@defgroup BLE_GAP_ADV_TYPES GAP Advertising types
 * @{ */
#define BLE_GAP_ADV_TYPE_ADV_IND          0x00   /**< Connectable undirected. */
#define BLE_GAP_ADV_TYPE_ADV_DIRECT_IND   0x01   /**< Connectable directed. */
#define BLE_GAP_ADV_TYPE_ADV_SCAN_IND     0x02   /**< Scannable undirected. */
#define BLE_GAP_ADV_TYPE_ADV_NONCONN_IND  0x03   /**< Non connectable undirected. */
/**@} */


/**@defgroup BLE_GAP_ADV_FILTER_POLICIES GAP Advertising filter policies
 * @{ */
#define BLE_GAP_ADV_FP_ANY                0x00   /**< Allow scan requests and connect requests from any device. */
#define BLE_GAP_ADV_FP_FILTER_SCANREQ     0x01   /**< Filter scan requests with whitelist. */
#define BLE_GAP_ADV_FP_FILTER_CONNREQ     0x02   /**< Filter connect requests with whitelist. */
#define BLE_GAP_ADV_FP_FILTER_BOTH        0x03   /**< Filter both scan and connect requests with whitelist. */
/**@} */


/**@defgroup BLE_GAP_ADV_TIMEOUT_VALUES GAP Advertising timeout values
 * @{ */
#define BLE_GAP_ADV_TIMEOUT_LIMITED_MAX      180 /**< Maximum advertising time in limited discoverable mode (TGAP(lim_adv_timeout) = 180s). */
#define BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED  0 /**< Unlimited advertising in general discoverable mode. */
/**@} */


/**@defgroup BLE_GAP_DISC_MODES GAP Discovery modes
 * @{ */
#define BLE_GAP_DISC_MODE_NOT_DISCOVERABLE  0x00   /**< Not discoverable discovery Mode. */
#define BLE_GAP_DISC_MODE_LIMITED           0x01   /**< Limited Discovery Mode. */
#define BLE_GAP_DISC_MODE_GENERAL           0x02   /**< General Discovery Mode. */
/**@} */

/**@defgroup BLE_GAP_IO_CAPS GAP IO Capabilities
 * @{ */
#define BLE_GAP_IO_CAPS_DISPLAY_ONLY      0x00   /**< Display Only. */
#define BLE_GAP_IO_CAPS_DISPLAY_YESNO     0x01   /**< Display and Yes/No entry. */
#define BLE_GAP_IO_CAPS_KEYBOARD_ONLY     0x02   /**< Keyboard Only. */
#define BLE_GAP_IO_CAPS_NONE              0x03   /**< No I/O capabilities. */
#define BLE_GAP_IO_CAPS_KEYBOARD_DISPLAY  0x04   /**< Keyboard and Display. */
/**@} */

/**@defgroup BLE_GAP_AUTH_KEY_TYPES GAP Authentication Key Types
 * @{ */
#define BLE_GAP_AUTH_KEY_TYPE_NONE        0x00   /**< No key (may be used to reject). */
#define BLE_GAP_AUTH_KEY_TYPE_PASSKEY     0x01   /**< 6-digit Passkey. */
#define BLE_GAP_AUTH_KEY_TYPE_OOB         0x02   /**< Out Of Band data. */
/**@} */

/**@defgroup BLE_GAP_KP_NOT_TYPES GAP Keypress Notification Types
 * @{ */
#define BLE_GAP_KP_NOT_TYPE_PASSKEY_START       0x00   /**< Passkey entry started. */
#define BLE_GAP_KP_NOT_TYPE_PASSKEY_DIGIT_IN    0x01   /**< Passkey digit entered. */
#define BLE_GAP_KP_NOT_TYPE_PASSKEY_DIGIT_OUT   0x02   /**< Passkey digit erased. */
#define BLE_GAP_KP_NOT_TYPE_PASSKEY_CLEAR       0x03   /**< Passkey cleared. */
#define BLE_GAP_KP_NOT_TYPE_PASSKEY_END         0x04   /**< Passkey entry completed. */
/**@} */

/**@defgroup BLE_GAP_SEC_STATUS GAP Security status
 * @{ */
#define BLE_GAP_SEC_STATUS_SUCCESS                0x00  /**< Procedure completed with success. */
#define BLE_GAP_SEC_STATUS_TIMEOUT                0x01  /**< Procedure timed out. */
#define BLE_GAP_SEC_STATUS_PDU_INVALID            0x02  /**< Invalid PDU received. */
#define BLE_GAP_SEC_STATUS_RFU_RANGE1_BEGIN       0x03  /**< Reserved for Future Use range #1 begin. */
#define BLE_GAP_SEC_STATUS_RFU_RANGE1_END         0x80  /**< Reserved for Future Use range #1 end. */
#define BLE_GAP_SEC_STATUS_PASSKEY_ENTRY_FAILED   0x81  /**< Passkey entry failed (user cancelled or other). */
#define BLE_GAP_SEC_STATUS_OOB_NOT_AVAILABLE      0x82  /**< Out of Band Key not available. */
#define BLE_GAP_SEC_STATUS_AUTH_REQ               0x83  /**< Authentication requirements not met. */
#define BLE_GAP_SEC_STATUS_CONFIRM_VALUE          0x84  /**< Confirm value failed. */
#define BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP       0x85  /**< Pairing not supported.  */
#define BLE_GAP_SEC_STATUS_ENC_KEY_SIZE           0x86  /**< Encryption key size. */
#define BLE_GAP_SEC_STATUS_SMP_CMD_UNSUPPORTED    0x87  /**< Unsupported SMP command. */
#define BLE_GAP_SEC_STATUS_UNSPECIFIED            0x88  /**< Unspecified reason. */
#define BLE_GAP_SEC_STATUS_REPEATED_ATTEMPTS      0x89  /**< Too little time elapsed since last attempt. */
#define BLE_GAP_SEC_STATUS_INVALID_PARAMS         0x8A  /**< Invalid parameters. */
#define BLE_GAP_SEC_STATUS_DHKEY_FAILURE          0x8B  /**< DHKey check failure. */
#define BLE_GAP_SEC_STATUS_NUM_COMP_FAILURE       0x8C  /**< Numeric Comparison failure. */
#define BLE_GAP_SEC_STATUS_BR_EDR_IN_PROG         0x8D  /**< BR/EDR pairing in progress. */
#define BLE_GAP_SEC_STATUS_X_TRANS_KEY_DISALLOWED 0x8E  /**< BR/EDR Link Key cannot be used for LE keys. */
#define BLE_GAP_SEC_STATUS_RFU_RANGE2_BEGIN       0x8F  /**< Reserved for Future Use range #2 begin. */
#define BLE_GAP_SEC_STATUS_RFU_RANGE2_END         0xFF  /**< Reserved for Future Use range #2 end. */
/**@} */

/**@defgroup BLE_GAP_SEC_STATUS_SOURCES GAP Security status sources
 * @{ */
#define BLE_GAP_SEC_STATUS_SOURCE_LOCAL           0x00  /**< Local failure. */
#define BLE_GAP_SEC_STATUS_SOURCE_REMOTE          0x01  /**< Remote failure. */
/**@} */

/**@defgroup BLE_GAP_CP_LIMITS GAP Connection Parameters Limits
 * @{ */
#define BLE_GAP_CP_MIN_CONN_INTVL_NONE           0xFFFF  /**< No new minimum connection interval specified in connect parameters. */
#define BLE_GAP_CP_MIN_CONN_INTVL_MIN            0x0006  /**< Lowest minimum connection interval permitted, in units of 1.25 ms, i.e. 7.5 ms. */
#define BLE_GAP_CP_MIN_CONN_INTVL_MAX            0x0C80  /**< Highest minimum connection interval permitted, in units of 1.25 ms, i.e. 4 s. */
#define BLE_GAP_CP_MAX_CONN_INTVL_NONE           0xFFFF  /**< No new maximum connection interval specified in connect parameters. */
#define BLE_GAP_CP_MAX_CONN_INTVL_MIN            0x0006  /**< Lowest maximum connection interval permitted, in units of 1.25 ms, i.e. 7.5 ms. */
#define BLE_GAP_CP_MAX_CONN_INTVL_MAX            0x0C80  /**< Highest maximum connection interval permitted, in units of 1.25 ms, i.e. 4 s. */
#define BLE_GAP_CP_SLAVE_LATENCY_MAX             0x01F3  /**< Highest slave latency permitted, in connection events. */
#define BLE_GAP_CP_CONN_SUP_TIMEOUT_NONE         0xFFFF  /**< No new supervision timeout specified in connect parameters. */
#define BLE_GAP_CP_CONN_SUP_TIMEOUT_MIN          0x000A  /**< Lowest supervision timeout permitted, in units of 10 ms, i.e. 100 ms. */
#define BLE_GAP_CP_CONN_SUP_TIMEOUT_MAX          0x0C80  /**< Highest supervision timeout permitted, in units of 10 ms, i.e. 32 s. */
/**@} */


/**@brief GAP device name maximum length. */
#define BLE_GAP_DEVNAME_MAX_LEN           31

/**@brief Disable RSSI events for connections */
#define BLE_GAP_RSSI_THRESHOLD_INVALID    0xFF

/**@defgroup BLE_GAP_CONN_SEC_MODE_SET_MACROS GAP attribute security requirement setters
 *
 * See @ref ble_gap_conn_sec_mode_t.
 * @{ */
/**@brief Set sec_mode pointed to by ptr to have no access rights.*/
#define BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(ptr)          do {(ptr)->sm = 0; (ptr)->lv = 0;} while(0)
/**@brief Set sec_mode pointed to by ptr to require no protection, open link.*/
#define BLE_GAP_CONN_SEC_MODE_SET_OPEN(ptr)               do {(ptr)->sm = 1; (ptr)->lv = 1;} while(0)
/**@brief Set sec_mode pointed to by ptr to require encryption, but no MITM protection.*/
#define BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(ptr)        do {(ptr)->sm = 1; (ptr)->lv = 2;} while(0)
/**@brief Set sec_mode pointed to by ptr to require encryption and MITM protection.*/
#define BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(ptr)      do {(ptr)->sm = 1; (ptr)->lv = 3;} while(0)
/**@brief Set sec_mode pointed to by ptr to require LESC encryption and MITM protection.*/
#define BLE_GAP_CONN_SEC_MODE_SET_LESC_ENC_WITH_MITM(ptr) do {(ptr)->sm = 1; (ptr)->lv = 4;} while(0)
/**@brief Set sec_mode pointed to by ptr to require signing or encryption, no MITM protection needed.*/
#define BLE_GAP_CONN_SEC_MODE_SET_SIGNED_NO_MITM(ptr)     do {(ptr)->sm = 2; (ptr)->lv = 1;} while(0)
/**@brief Set sec_mode pointed to by ptr to require signing or encryption with MITM protection.*/
#define BLE_GAP_CONN_SEC_MODE_SET_SIGNED_WITH_MITM(ptr)   do {(ptr)->sm = 2; (ptr)->lv = 2;} while(0)
/**@} */


/**@brief GAP Security Random Number Length. */
#define BLE_GAP_SEC_RAND_LEN 8

/**@brief GAP Security Key Length. */
#define BLE_GAP_SEC_KEY_LEN 16

/**@brief GAP LE Secure Connections Elliptic Curve Diffie-Hellman P-256 Public Key Length. */
#define BLE_GAP_LESC_P256_PK_LEN 64

/**@brief GAP LE Secure Connections Elliptic Curve Diffie-Hellman DHKey Length. */
#define BLE_GAP_LESC_DHKEY_LEN   32

/**@brief GAP Passkey Length. */
#define BLE_GAP_PASSKEY_LEN 6

/**@brief Maximum amount of addresses in a whitelist. */
#define BLE_GAP_WHITELIST_ADDR_MAX_COUNT (8)

/**@brief Maximum amount of IRKs in a whitelist.
 * @note  The number of IRKs is limited to 8, even if the hardware supports more.
 */
#define BLE_GAP_WHITELIST_IRK_MAX_COUNT (8)

/**@defgroup GAP_SEC_MODES GAP Security Modes
 * @{ */
#define BLE_GAP_SEC_MODE 0x00 /**< No key (may be used to reject). */
/**@} */
/** @} */

/**@addtogroup BLE_GAP_STRUCTURES Structures
 * @{ */

/**
 * @brief BLE GAP initialization parameters.
 */
typedef struct
{
  uint8_t                   periph_conn_count;  /**< Number of connections acting as a peripheral  */
  uint8_t                   central_conn_count; /**< Number of connections acting as a central */
  uint8_t                   central_sec_count;  /**< Number of SMP instances for all connections acting as a central. */
} ble_gap_enable_params_t;

/**@brief Bluetooth Low Energy address. */
typedef struct
{
  uint8_t addr_type;                    /**< See @ref BLE_GAP_ADDR_TYPES. */
  uint8_t addr[BLE_GAP_ADDR_LEN];       /**< 48-bit address, LSB format. */
} ble_gap_addr_t;


/**@brief GAP connection parameters.
 *
 * @note  When ble_conn_params_t is received in an event, both min_conn_interval and
 *        max_conn_interval will be equal to the connection interval set by the central.
 *
 * @note If both conn_sup_timeout and max_conn_interval are specified, then the following constraint applies:
 *       conn_sup_timeout * 4 > (1 + slave_latency) * max_conn_interval
 *       that corresponds to the following Bluetooth Spec requirement:
 *       The Supervision_Timeout in milliseconds shall be larger than
 *       (1 + Conn_Latency) * Conn_Interval_Max * 2, where Conn_Interval_Max is given in milliseconds.
 */
typedef struct
{
  uint16_t min_conn_interval;         /**< Minimum Connection Interval in 1.25 ms units, see @ref BLE_GAP_CP_LIMITS.*/
  uint16_t max_conn_interval;         /**< Maximum Connection Interval in 1.25 ms units, see @ref BLE_GAP_CP_LIMITS.*/
  uint16_t slave_latency;             /**< Slave Latency in number of connection events, see @ref BLE_GAP_CP_LIMITS.*/
  uint16_t conn_sup_timeout;          /**< Connection Supervision Timeout in 10 ms units, see @ref BLE_GAP_CP_LIMITS.*/
} ble_gap_conn_params_t;


/**@brief GAP connection security modes.
 *
 * Security Mode 0 Level 0: No access permissions at all (this level is not defined by the Bluetooth Core specification).\n
 * Security Mode 1 Level 1: No security is needed (aka open link).\n
 * Security Mode 1 Level 2: Encrypted link required, MITM protection not necessary.\n
 * Security Mode 1 Level 3: MITM protected encrypted link required.\n
 * Security Mode 1 Level 4: LESC MITM protected encrypted link required.\n
 * Security Mode 2 Level 1: Signing or encryption required, MITM protection not necessary.\n
 * Security Mode 2 Level 2: MITM protected signing required, unless link is MITM protected encrypted.\n
 */
typedef struct
{
  uint8_t sm : 4;                     /**< Security Mode (1 or 2), 0 for no permissions at all. */
  uint8_t lv : 4;                     /**< Level (1, 2, 3 or 4), 0 for no permissions at all. */

} ble_gap_conn_sec_mode_t;


/**@brief GAP connection security status.*/
typedef struct
{
  ble_gap_conn_sec_mode_t sec_mode;           /**< Currently active security mode for this connection.*/
  uint8_t                 encr_key_size;      /**< Length of currently active encryption key, 7 to 16 octets (only applicable for bonding procedures). */
} ble_gap_conn_sec_t;


/**@brief Identity Resolving Key. */
typedef struct
{
  uint8_t irk[BLE_GAP_SEC_KEY_LEN];   /**< Array containing IRK. */
} ble_gap_irk_t;


/**@brief Whitelist structure. */
typedef struct
{
  ble_gap_addr_t    **pp_addrs;        /**< Pointer to an array of device address pointers, pointing to addresses to be used in whitelist. NULL if none are given. */
  uint8_t             addr_count;      /**< Count of device addresses in array, up to @ref BLE_GAP_WHITELIST_ADDR_MAX_COUNT. */
  ble_gap_irk_t     **pp_irks;         /**< Pointer to an array of Identity Resolving Key (IRK) pointers, each pointing to an IRK in the whitelist. NULL if none are given. */
  uint8_t             irk_count;       /**< Count of IRKs in array, up to @ref BLE_GAP_WHITELIST_IRK_MAX_COUNT. */
} ble_gap_whitelist_t;

/**@brief Channel mask for RF channels used in advertising. */
typedef struct
{
  uint8_t ch_37_off : 1;  /**< Setting this bit to 1 will turn off advertising on channel 37 */
  uint8_t ch_38_off : 1;  /**< Setting this bit to 1 will turn off advertising on channel 38 */
  uint8_t ch_39_off : 1;  /**< Setting this bit to 1 will turn off advertising on channel 39 */
} ble_gap_adv_ch_mask_t;

/**@brief GAP advertising parameters.*/
typedef struct
{
  uint8_t               type;                 /**< See @ref BLE_GAP_ADV_TYPES. */
  ble_gap_addr_t       *p_peer_addr;          /**< For @ref BLE_GAP_ADV_TYPE_ADV_DIRECT_IND mode only, known peer address. */
  uint8_t               fp;                   /**< Filter Policy, see @ref BLE_GAP_ADV_FILTER_POLICIES. */
  ble_gap_whitelist_t  *p_whitelist;          /**< Pointer to whitelist, NULL if no whitelist or the current active whitelist is to be used. */
  uint16_t              interval;             /**< Advertising interval between 0x0020 and 0x4000 in 0.625 ms units (20ms to 10.24s), see @ref BLE_GAP_ADV_INTERVALS.
                                                   - If type equals @ref BLE_GAP_ADV_TYPE_ADV_DIRECT_IND, this parameter must be set to 0 for high duty cycle directed advertising.
                                                   - If type equals @ref BLE_GAP_ADV_TYPE_ADV_DIRECT_IND, set @ref BLE_GAP_ADV_INTERVAL_MIN <= interval <= @ref BLE_GAP_ADV_INTERVAL_MAX for low duty cycle advertising.*/
  uint16_t              timeout;              /**< Advertising timeout between 0x0001 and 0x3FFF in seconds, 0x0000 disables timeout. See also @ref BLE_GAP_ADV_TIMEOUT_VALUES. If type equals @ref BLE_GAP_ADV_TYPE_ADV_DIRECT_IND, this parameter must be set to 0 for High duty cycle directed advertising. */
  ble_gap_adv_ch_mask_t channel_mask;         /**< Advertising channel mask. See @ref ble_gap_adv_ch_mask_t. */
} ble_gap_adv_params_t;


/**@brief GAP scanning parameters. */
typedef struct
{
  uint8_t                 active    : 1;        /**< If 1, perform active scanning (scan requests). */
  uint8_t                 selective : 1;        /**< If 1, ignore unknown devices (non whitelisted). */
  ble_gap_whitelist_t *   p_whitelist;          /**< Pointer to whitelist, NULL if no whitelist or the current active whitelist is to be used. */
  uint16_t                interval;             /**< Scan interval between 0x0004 and 0x4000 in 0.625ms units (2.5ms to 10.24s). */
  uint16_t                window;               /**< Scan window between 0x0004 and 0x4000 in 0.625ms units (2.5ms to 10.24s). */
  uint16_t                timeout;              /**< Scan timeout between 0x0001 and 0xFFFF in seconds, 0x0000 disables timeout. */
} ble_gap_scan_params_t;


/** @brief Keys that can be exchanged during a bonding procedure. */
typedef struct
{
  uint8_t enc     : 1;                        /**< Long Term Key and Master Identification. */
  uint8_t id      : 1;                        /**< Identity Resolving Key and Identity Address Information. */
  uint8_t sign    : 1;                        /**< Connection Signature Resolving Key. */
  uint8_t link    : 1;                        /**< Derive the Link Key from the LTK. */
} ble_gap_sec_kdist_t;


/**@brief GAP security parameters. */
typedef struct
{
  uint8_t               bond      : 1;             /**< Perform bonding. */
  uint8_t               mitm      : 1;             /**< Enable Man In The Middle protection. */
  uint8_t               lesc      : 1;             /**< Enable LE Secure Connection pairing. */
  uint8_t               keypress  : 1;             /**< Enable generation of keypress notifications. */
  uint8_t               io_caps   : 3;             /**< IO capabilities, see @ref BLE_GAP_IO_CAPS. */
  uint8_t               oob       : 1;             /**< Out Of Band data available. */
  uint8_t               min_key_size;              /**< Minimum encryption key size in octets between 7 and 16. If 0 then not applicable in this instance. */
  uint8_t               max_key_size;              /**< Maximum encryption key size in octets between min_key_size and 16. */
  ble_gap_sec_kdist_t   kdist_own;                 /**< Key distribution bitmap: keys that the local device will distribute. */
  ble_gap_sec_kdist_t   kdist_peer;                /**< Key distribution bitmap: keys that the remote device will distribute. */
} ble_gap_sec_params_t;


/**@brief GAP Encryption Information. */
typedef struct
{
  uint8_t   ltk[BLE_GAP_SEC_KEY_LEN];   /**< Long Term Key. */
  uint8_t   lesc : 1;                   /**< Key generated using LE Secure Connections. */
  uint8_t   auth : 1;                   /**< Authenticated Key. */
  uint8_t   ltk_len : 6;                /**< LTK length in octets. */
} ble_gap_enc_info_t;


/**@brief GAP Master Identification. */
typedef struct
{
  uint16_t  ediv;                       /**< Encrypted Diversifier. */
  uint8_t   rand[BLE_GAP_SEC_RAND_LEN]; /**< Random Number. */
} ble_gap_master_id_t;


/**@brief GAP Signing Information. */
typedef struct
{
  uint8_t   csrk[BLE_GAP_SEC_KEY_LEN];        /**< Connection Signature Resolving Key. */
} ble_gap_sign_info_t;

/**@brief GAP LE Secure Connections P-256 Public Key. */
typedef struct
{
  uint8_t   pk[BLE_GAP_LESC_P256_PK_LEN];        /**< LE Secure Connections Elliptic Curve Diffie-Hellman P-256 Public Key. Stored in the standard SMP protocol format: {X,Y} both in little-endian. */
} ble_gap_lesc_p256_pk_t;

/**@brief GAP LE Secure Connections DHKey. */
typedef struct
{
  uint8_t   key[BLE_GAP_LESC_DHKEY_LEN];        /**< LE Secure Connections Elliptic Curve Diffie-Hellman Key. Stored in little-endian. */
} ble_gap_lesc_dhkey_t;

/**@brief GAP LE Secure Connections OOB data. */
typedef struct
{
  ble_gap_addr_t  addr;                          /**< Bluetooth address of the device. */
  uint8_t         r[BLE_GAP_SEC_KEY_LEN];        /**< Random Number. */
  uint8_t         c[BLE_GAP_SEC_KEY_LEN];        /**< Confirm Value. */
} ble_gap_lesc_oob_data_t;

/**@brief Event structure for @ref BLE_GAP_EVT_CONNECTED. */
typedef struct
{
  ble_gap_addr_t        peer_addr;              /**< Bluetooth address of the peer device. */
  ble_gap_addr_t        own_addr;               /**< Bluetooth address of the local device used during connection setup. */
  uint8_t               role;                   /**< BLE role for this connection, see @ref BLE_GAP_ROLES */
  uint8_t               irk_match :1;           /**< If 1, peer device's address resolved using an IRK. */
  uint8_t               irk_match_idx  :7;      /**< Index in IRK list where the address was matched. */
  ble_gap_conn_params_t conn_params;            /**< GAP Connection Parameters. */
} ble_gap_evt_connected_t;


/**@brief Event structure for @ref BLE_GAP_EVT_DISCONNECTED. */
typedef struct
{
  uint8_t reason;                               /**< HCI error code, see @ref BLE_HCI_STATUS_CODES. */
} ble_gap_evt_disconnected_t;


/**@brief Event structure for @ref BLE_GAP_EVT_CONN_PARAM_UPDATE. */
typedef struct
{
  ble_gap_conn_params_t conn_params;            /**<  GAP Connection Parameters. */
} ble_gap_evt_conn_param_update_t;


/**@brief Event structure for @ref BLE_GAP_EVT_SEC_PARAMS_REQUEST. */
typedef struct
{
  ble_gap_sec_params_t peer_params;             /**< Initiator Security Parameters. */
} ble_gap_evt_sec_params_request_t;


/**@brief Event structure for @ref BLE_GAP_EVT_SEC_INFO_REQUEST. */
typedef struct
{
  ble_gap_addr_t      peer_addr;                     /**< Bluetooth address of the peer device. */
  ble_gap_master_id_t master_id;                     /**< Master Identification for LTK lookup. */
  uint8_t             enc_info  : 1;                 /**< If 1, Encryption Information required. */
  uint8_t             id_info   : 1;                 /**< If 1, Identity Information required. */
  uint8_t             sign_info : 1;                 /**< If 1, Signing Information required. */
} ble_gap_evt_sec_info_request_t;


/**@brief Event structure for @ref BLE_GAP_EVT_PASSKEY_DISPLAY. */
typedef struct
{
  uint8_t passkey[BLE_GAP_PASSKEY_LEN];         /**< 6-digit passkey in ASCII ('0'-'9' digits only). */
  uint8_t match_request : 1;                    /**< If 1 requires the application to report the match using @ref sd_ble_gap_auth_key_reply 
                                                     with either @ref BLE_GAP_AUTH_KEY_TYPE_NONE if there is no match or 
                                                     @ref BLE_GAP_AUTH_KEY_TYPE_PASSKEY if there is a match. */
} ble_gap_evt_passkey_display_t;

/**@brief Event structure for @ref BLE_GAP_EVT_KEY_PRESSED. */
typedef struct
{
  uint8_t kp_not;         /**< Keypress notification type, see @ref BLE_GAP_KP_NOT_TYPES. */
} ble_gap_evt_key_pressed_t;


/**@brief Event structure for @ref BLE_GAP_EVT_AUTH_KEY_REQUEST. */
typedef struct
{
  uint8_t key_type;                             /**< See @ref BLE_GAP_AUTH_KEY_TYPES. */
} ble_gap_evt_auth_key_request_t;

/**@brief Event structure for @ref BLE_GAP_EVT_LESC_DHKEY_REQUEST. */
typedef struct
{
  ble_gap_lesc_p256_pk_t *p_pk_peer;  /**< LE Secure Connections remote P-256 Public Key. This will point to the application-supplied memory 
                                           inside the keyset during the call to @ref sd_ble_gap_sec_params_reply. */
  uint8_t oobd_req       :1;          /**< LESC OOB data required. A call to @ref sd_ble_gap_lesc_oob_data_set is required to complete the procedure. */
} ble_gap_evt_lesc_dhkey_request_t;


/**@brief Security levels supported.
 * @note See Bluetooth Specification Version 4.2 Volume 3, Part C, Chapter 10, Section 10.2.1.
*/
typedef struct
{
  uint8_t lv1 : 1;                              /**< If 1: Level 1 is supported. */
  uint8_t lv2 : 1;                              /**< If 1: Level 2 is supported. */
  uint8_t lv3 : 1;                              /**< If 1: Level 3 is supported. */
  uint8_t lv4 : 1;                              /**< If 1: Level 4 is supported. */
} ble_gap_sec_levels_t;


/**@brief Encryption Key. */
typedef struct
{
  ble_gap_enc_info_t    enc_info;             /**< Encryption Information. */
  ble_gap_master_id_t   master_id;            /**< Master Identification. */
} ble_gap_enc_key_t;


/**@brief Identity Key. */
typedef struct
{
  ble_gap_irk_t         id_info;              /**< Identity Information. */
  ble_gap_addr_t        id_addr_info;         /**< Identity Address Information. */
} ble_gap_id_key_t;


/**@brief Security Keys. */
typedef struct
{
  ble_gap_enc_key_t      *p_enc_key;           /**< Encryption Key, or NULL. */
  ble_gap_id_key_t       *p_id_key;            /**< Identity Key, or NULL. */
  ble_gap_sign_info_t    *p_sign_key;          /**< Signing Key, or NULL. */
  ble_gap_lesc_p256_pk_t *p_pk;                /**< LE Secure Connections P-256 Public Key. When in debug mode the application must use the value defined 
                                                    in the Core Bluetooth Specification v4.2 Vol.3, Part H, Section 2.3.5.6.1 */
} ble_gap_sec_keys_t;


/**@brief Security key set for both local and peer keys. */
typedef struct
{
  ble_gap_sec_keys_t            keys_own;     /**< Keys distributed by the local device. For LE Secure Connections the encryption key will be generated locally and will always be stored if bonding. */
  ble_gap_sec_keys_t            keys_peer;    /**< Keys distributed by the remote device. For LE Secure Connections, p_enc_key must always be NULL. */
} ble_gap_sec_keyset_t;


/**@brief Event structure for @ref BLE_GAP_EVT_AUTH_STATUS. */
typedef struct
{
  uint8_t               auth_status;            /**< Authentication status, see @ref BLE_GAP_SEC_STATUS. */
  uint8_t               error_src : 2;          /**< On error, source that caused the failure, see @ref BLE_GAP_SEC_STATUS_SOURCES. */
  uint8_t               bonded : 1;             /**< Procedure resulted in a bond. */
  ble_gap_sec_levels_t  sm1_levels;             /**< Levels supported in Security Mode 1. */
  ble_gap_sec_levels_t  sm2_levels;             /**< Levels supported in Security Mode 2. */
  ble_gap_sec_kdist_t   kdist_own;              /**< Bitmap stating which keys were exchanged (distributed) by the local device. If bonding with LE Secure Connections, the enc bit will be always set. */
  ble_gap_sec_kdist_t   kdist_peer;             /**< Bitmap stating which keys were exchanged (distributed) by the remote device. If bonding with LE Secure Connections, the enc bit will never be set. */
} ble_gap_evt_auth_status_t;


/**@brief Event structure for @ref BLE_GAP_EVT_CONN_SEC_UPDATE. */
typedef struct
{
  ble_gap_conn_sec_t conn_sec;                  /**< Connection security level. */
} ble_gap_evt_conn_sec_update_t;


/**@brief Event structure for @ref BLE_GAP_EVT_TIMEOUT. */
typedef struct
{
  uint8_t src;                                  /**< Source of timeout event, see @ref BLE_GAP_TIMEOUT_SOURCES. */
} ble_gap_evt_timeout_t;


/**@brief Event structure for @ref BLE_GAP_EVT_RSSI_CHANGED. */
typedef struct
{
  int8_t  rssi;                               /**< Received Signal Strength Indication in dBm. */
} ble_gap_evt_rssi_changed_t;


/**@brief Event structure for @ref BLE_GAP_EVT_ADV_REPORT. */
typedef struct
{
  ble_gap_addr_t peer_addr;                     /**< Bluetooth address of the peer device. */
  int8_t         rssi;                          /**< Received Signal Strength Indication in dBm. */
  uint8_t        scan_rsp : 1;                  /**< If 1, the report corresponds to a scan response and the type field may be ignored. */
  uint8_t        type     : 2;                  /**< See @ref BLE_GAP_ADV_TYPES. Only valid if the scan_rsp field is 0. */
  uint8_t        dlen     : 5;                  /**< Advertising or scan response data length. */
  uint8_t        data[BLE_GAP_ADV_MAX_SIZE];    /**< Advertising or scan response data. */
} ble_gap_evt_adv_report_t;


/**@brief Event structure for @ref BLE_GAP_EVT_SEC_REQUEST. */
typedef struct
{
  uint8_t    bond       : 1;                       /**< Perform bonding. */
  uint8_t    mitm       : 1;                       /**< Man In The Middle protection requested. */
  uint8_t    lesc       : 1;                       /**< LE Secure Connections requested. */
  uint8_t    keypress   : 1;                       /**< Generation of keypress notifications requested. */
} ble_gap_evt_sec_request_t;


/**@brief Event structure for @ref BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST. */
typedef struct
{
  ble_gap_conn_params_t conn_params;            /**<  GAP Connection Parameters. */
} ble_gap_evt_conn_param_update_request_t;


/**@brief Event structure for @ref BLE_GAP_EVT_SCAN_REQ_REPORT. */
typedef struct
{
  int8_t                  rssi;              /**< Received Signal Strength Indication in dBm. */
  ble_gap_addr_t          peer_addr;         /**< Bluetooth address of the peer device. */
} ble_gap_evt_scan_req_report_t;



/**@brief GAP event structure. */
typedef struct
{
  uint16_t conn_handle;                                     /**< Connection Handle on which event occurred. */
  union                                                     /**< union alternative identified by evt_id in enclosing struct. */
  {
    ble_gap_evt_connected_t                   connected;                    /**< Connected Event Parameters. */
    ble_gap_evt_disconnected_t                disconnected;                 /**< Disconnected Event Parameters. */
    ble_gap_evt_conn_param_update_t           conn_param_update;            /**< Connection Parameter Update Parameters. */
    ble_gap_evt_sec_params_request_t          sec_params_request;           /**< Security Parameters Request Event Parameters. */
    ble_gap_evt_sec_info_request_t            sec_info_request;             /**< Security Information Request Event Parameters. */
    ble_gap_evt_passkey_display_t             passkey_display;              /**< Passkey Display Event Parameters. */
    ble_gap_evt_key_pressed_t                 key_pressed;                  /**< Key Pressed Event Parameters. */
    ble_gap_evt_auth_key_request_t            auth_key_request;             /**< Authentication Key Request Event Parameters. */
    ble_gap_evt_lesc_dhkey_request_t          lesc_dhkey_request;           /**< LE Secure Connections DHKey calculation request. */
    ble_gap_evt_auth_status_t                 auth_status;                  /**< Authentication Status Event Parameters. */
    ble_gap_evt_conn_sec_update_t             conn_sec_update;              /**< Connection Security Update Event Parameters. */
    ble_gap_evt_timeout_t                     timeout;                      /**< Timeout Event Parameters. */
    ble_gap_evt_rssi_changed_t                rssi_changed;                 /**< RSSI Event parameters. */
    ble_gap_evt_adv_report_t                  adv_report;                   /**< Advertising Report Event Parameters. */
    ble_gap_evt_sec_request_t                 sec_request;                  /**< Security Request Event Parameters. */
    ble_gap_evt_conn_param_update_request_t   conn_param_update_request;    /**< Connection Parameter Update Parameters. */
    ble_gap_evt_scan_req_report_t             scan_req_report;              /**< Scan Request Report parameters. */
  } params;                                                                 /**< Event Parameters. */

} ble_gap_evt_t;


/**@brief Channel Map option.
 *        Used with @ref sd_ble_opt_get to get the current channel map
 *        or @ref sd_ble_opt_set to set a new channel map. When setting the
 *        channel map, it applies to all current and future connections. When getting the
 *        current channel map, it applies to a single connection and the connection handle
 *        must be supplied.
 *
 * @note  Setting the channel map may take some time, depending on connection parameters.
 *        The time taken may be different for each connection and the get operation will
 *        return the previous channel map until the new one has taken effect.
 *
 * @note  After setting the channel map, by spec it can not be set again until at least 1 s has passed.
 *        See Bluetooth Specification Version 4.1 Volume 2, Part E, Section 7.3.46.
 *
 * @retval ::NRF_SUCCESS Get or set successful.
 * @retval ::NRF_ERROR_BUSY Channel map was set again before enough time had passed.
 * @retval ::NRF_ERROR_INVALID_STATE Invalid state to perform operation.
 * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid connection handle supplied for get.
 * @retval ::NRF_ERROR_NOT_SUPPORTED Returned by sd_ble_opt_set in peripheral-only SoftDevices.
 *
 */
typedef struct
{
  uint16_t conn_handle;                   /**< Connection Handle (only applicable for get) */
  uint8_t ch_map[5];                      /**< Channel Map (37-bit). */
} ble_gap_opt_ch_map_t;


/**@brief Local connection latency option.
 *
 *        Local connection latency is a feature which enables the slave to improve
 *        current consumption by ignoring the slave latency set by the peer. The
 *        local connection latency can only be set to a multiple of the slave latency,
 *        and cannot be longer than half of the supervision timeout.
 *
 *        Used with @ref sd_ble_opt_set to set the local connection latency. The
 *        @ref sd_ble_opt_get is not supported for this option, but the actual
 *        local connection latency (unless set to NULL) is set as a return parameter
 *        when setting the option.
 *
 * @note  The latency set will be truncated down to the closest slave latency event
 *        multiple, or the nearest multiple before half of the supervision timeout.
 *
 * @note  The local connection latency is disabled by default, and needs to be enabled for new
 *        connections and whenever the connection is updated.
 *
 * @retval ::NRF_SUCCESS Set successfully.
 * @retval ::NRF_ERROR_NOT_SUPPORTED Get is not supported.
 * @retval ::BLE_ERROR_INVALID_CONN_HANDLE Invalid connection handle parameter.
 */
typedef struct
{
  uint16_t   conn_handle;                       /**< Connection Handle */
  uint16_t   requested_latency;                 /**< Requested local connection latency. */
  uint16_t * p_actual_latency;                  /**< Pointer to storage for the actual local connection latency (can be set to NULL to skip return value). */
} ble_gap_opt_local_conn_latency_t;


/**@brief Passkey Option.
 *
 *        Structure containing the passkey to be used during pairing. This can be used with @ref
 *        sd_ble_opt_set to make the SoftDevice use a pre-programmed passkey for authentication
 *        instead of generating a random one.
 *
 * @note  @ref sd_ble_opt_get is not supported for this option.
 *
 */
typedef struct
{
  uint8_t * p_passkey;                          /**< Pointer to 6-digit ASCII string (digit 0..9 only, no NULL termination) passkey to be used during pairing. If this is NULL, the SoftDevice will generate a random passkey if required.*/
} ble_gap_opt_passkey_t;


/**@brief Custom Privacy Option.
 *
 *        This structure is used with both @ref sd_ble_opt_set (as input) and with
 *        @ref sd_ble_opt_get (as output).
 *
 *        Structure containing:
 *        - A pointer to an IRK to set (if input), or a place to store a read IRK (if output).
 *        - A private address refresh cycle.
 *
 * @note  The specified address cycle interval is used when the address cycle mode is
 *        @ref BLE_GAP_ADDR_CYCLE_MODE_AUTO. If 0 is given, the address will not be automatically
 *        refreshed at all. The default interval is @ref BLE_GAP_DEFAULT_PRIVATE_ADDR_CYCLE_INTERVAL_S.
 *
 * @note  If the current address cycle mode is @ref BLE_GAP_ADDR_CYCLE_MODE_AUTO, the address will immediately be
 *        refreshed when a custom privacy option is set. A new address can be generated manually by calling
 *        @ref sd_ble_gap_address_set with the same type again.
 *
 * @note  If the IRK is updated, the new IRK becomes the one to be distributed in all
 *        bonding procedures performed after @ref sd_ble_opt_set returns.
 *
 * @retval ::NRF_SUCCESS Set or read successfully.
 * @retval ::NRF_ERROR_INVALID_ADDR The pointer to IRK storage is invalid.
 */
typedef struct
{
  ble_gap_irk_t * p_irk;        /**< When input: Pointer to custom IRK, or NULL to use/reset to the device's default IRK. When output: Pointer to where the current IRK is to be stored, or NULL to not read out the IRK. */
  uint16_t        interval_s;   /**< When input: Custom private address cycle interval in seconds. When output: The current private address cycle interval. */
} ble_gap_opt_privacy_t;


/**@brief Scan request report option.
 *
 *        This can be used with @ref sd_ble_opt_set to make the SoftDevice send
 *        @ref BLE_GAP_EVT_SCAN_REQ_REPORT events.
 *
 *  @note   Due to the limited space reserved for scan request report events,
 *          not all received scan requests will be reported.
 *
 *  @note   If whitelisting is used, only whitelisted requests are reported.
 *
 *  @retval ::NRF_SUCCESS Set successfully.
 *  @retval ::NRF_ERROR_INVALID_STATE When advertising is ongoing while the option is set.
 */
typedef struct
{
   uint8_t enable : 1;                           /**< Enable scan request reports. */
} ble_gap_opt_scan_req_report_t;

/**@brief Compatibility mode option.
 *
 *        This can be used with @ref sd_ble_opt_set to enable and disable
 *        compatibility modes. Compatibility modes are disabled by default.
 *
 *  @note  Compatibility mode 1 enables interoperability with devices that do not support
 *         a value of 0 for the WinOffset parameter in the Link Layer CONNECT_REQ packet.
 *
 *  @retval ::NRF_SUCCESS Set successfully.
 *  @retval ::NRF_ERROR_INVALID_STATE When connection creation is ongoing while mode 1 is set.
 */
typedef struct
{
   uint8_t mode_1_enable : 1;                           /**< Enable compatibility mode 1.*/
} ble_gap_opt_compat_mode_t;

/**@brief Option structure for GAP options. */
typedef union
{
  ble_gap_opt_ch_map_t                  ch_map;                    /**< Parameters for the Channel Map option. */
  ble_gap_opt_local_conn_latency_t      local_conn_latency;        /**< Parameters for the Local connection latency option */
  ble_gap_opt_passkey_t                 passkey;                   /**< Parameters for the Passkey option.*/
  ble_gap_opt_privacy_t                 privacy;                   /**< Parameters for the Custom privacy option. */
  ble_gap_opt_scan_req_report_t         scan_req_report;           /**< Parameters for the scan request report option.*/
  ble_gap_opt_compat_mode_t             compat_mode;               /**< Parameters for the compatibility mode option.*/
} ble_gap_opt_t;
/**@} */



#ifdef __cplusplus
}
#endif
#endif // BLE_GAP_H__

/**
  @}
*/
