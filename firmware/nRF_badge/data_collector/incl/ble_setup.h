#ifndef BLE_SETUP_H
#define BLE_SETUP_H

#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "softdevice_handler.h"

#include "crc16.h"

#include "nrf_drv_config.h"

#include "debug_log.h"          //UART debugging logger

#include "ble_bas.h"  //battery service
#include "ble_nus.h"  //Nordic UART service
 
 
#include "storer.h" 
#include "scanner.h"
 
#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#ifdef BOARD_PCA10028
    #define DEVICE_NAME                     "NRF51DK"
#else
    #define DEVICE_NAME                     "HDBDG"                           /**< Name of device. Will be included in the advertising data. */
#endif

#define APP_ADV_INTERVAL                320                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 200 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      5                                          

#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define NO_GROUP 0

volatile unsigned char badgeGroup;
volatile unsigned short badgeID;

volatile bool isConnected;
volatile bool isAdvertising;

typedef enum ble_pauseReq_src
{
    PAUSE_REQ_FIRST=0,
    PAUSE_REQ_STORER=0,
    PAUSE_REQ_COLLECTOR=1,
    PAUSE_REQ_NONE=2
} ble_pauseReq_src;

volatile bool pauseRequest[PAUSE_REQ_NONE];

typedef enum ble_status_t
{
    BLE_INACTIVE,             // no radio activity should be occurring
    BLE_CONNECTED,            // connection active
    BLE_ADVERTISING,          // advertising active
} ble_status_t;



#define CUSTOM_DATA_LEN 9   // bytes in custom data struct to be sent in advertising payload.
                            //   Note that this differs from sizeof(custom_adv_data_t), which includes padding at end of struct
 
typedef struct
{
    float battery;
    unsigned char synced;
    unsigned char collecting;
    unsigned short ID;
    unsigned char group;
} custom_adv_data_t;

volatile bool needAdvDataUpdate;



/**
 * Callback function for asserts in the SoftDevice; called in case of SoftDevice assert.
 * Parameters are file name and line number where assert occurred.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name);


/**
 * Function for reporting BLE errors.
 * Parameters are error code and line number where error occurs
 * Use macro below to report proper line number
 */
static void ble_error_handler(uint32_t error_code, uint32_t line_num);

/**
 * Macro for checking error codes.  Defined as a macro so we can report the line number.
 * Calls ble_error_handler if ERR_CODE is not NRF_SUCCESS (i.e. 0)
 */
#define BLE_ERROR_CHECK(ERR_CODE)                           \
    do                                                      \
    {                                                       \
        const uint32_t LOCAL_ERR_CODE = (ERR_CODE);         \
        if (LOCAL_ERR_CODE != NRF_SUCCESS)                  \
        {                                                   \
            ble_error_handler(LOCAL_ERR_CODE,__LINE__);     \
        }                                                   \
    } while (0)

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void);

/**@brief Function for initializing battery and UART services
 */
static void services_init(void);

/**@brief Function for initializing security parameters.
 */
static void sec_params_init(void);

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
//static void on_adv_evt(ble_adv_evt_t ble_adv_evt);


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt);


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called after a BLE stack event has been received.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt);


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in] sys_evt  System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt);


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void);


/**@brief Function for initializing the Advertising functionality.
 */
void advertising_init(void);


/**
 * Function to allow other modules to tell BLE module that advertising data should be updated.
 *   Will be set after next disconnect/advertising timeout
 */
void updateAdvData();


/**
 * Refreshes advertising data.
 */
void setAdvData();


/**
 * Initialize BLE structures
 */
void BLE_init();

/**
 * Initialize BLE structures
 */
void BLEstartAdvertising();

/**
 * Stop softdevice
 */
void BLEdisable();

/**
 * Request advertising pause.  Must specify source of pause request.
 *   Returns true if advertising is inactive.
 */
bool BLEpause(ble_pauseReq_src source);

/**
 * Resume advertising (after pausing it).  Must specify source of resume request.
 */
void BLEresume(ble_pauseReq_src source);

/**
 * Disconnect from server forcefully.
 */
void BLEforceDisconnect();

/**
 * Get BLE status.
 *   See ble_status_t enum above
 */
ble_status_t BLEgetStatus();

bool BLEpauseReqPending();

/**
 * Functions called on connection or disconnection events
 */
void BLEonConnect();
void BLEonDisconnect();

/** 
 * Function for handling incoming data from the BLE UART service
 */
void BLEonReceive(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length);

/**
 * Function for handling BLE advertising reports (from scan)
 * XXXXXXXXXXXXXXXXXXXXXXXXXParameters are 6-byte array of BLE address, and RSSI signal strength
 */
void BLEonAdvReport(ble_gap_evt_adv_report_t* advReportPtr);

/**
 * Function called on timeout of scan, to finalize and store scan results
 */
void BLEonScanTimeout();

/**
 * Function to return whether the client has yet enabled notifications
 */
bool notificationEnabled();

/** Function to wrap ble_nus_string_send, for convenience
 */
bool BLEwrite(uint8_t* data, uint16_t len);

/** Function to wrap ble_nus_string_send for sending one char, for convenience
 */
bool BLEwriteChar(uint8_t dataChar);




#endif //BLE_SETUP_H