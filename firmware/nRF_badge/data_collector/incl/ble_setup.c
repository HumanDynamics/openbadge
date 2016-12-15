#include <app_fifo.h>

#include "app_fifo_util.h"
#include "ble_setup.h"

#include "battery.h"

// Size of BLE FIFO, must be power of two.
#define BLE_FIFO_SIZE 512

#define BLE_ADV_PAUSE_CALLBACK_QUEUE_SIZE 3

static ble_gap_sec_params_t             m_sec_params;                               /**< Security requirements for this application. */
static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */

//static ble_bas_t                        m_bas;      //Struct for Battery Service module
static ble_nus_t                        m_nus;      //Struct for Nordic UART Service module


// Buffer used to allow for sending of long messages (up to 512 bytes) via ble_write_buffered().
static app_fifo_t m_ble_fifo;
static uint8_t m_ble_fifo_buffer[BLE_FIFO_SIZE];

static adv_paused_callback_t m_adv_pause_callback_queue[BLE_ADV_PAUSE_CALLBACK_QUEUE_SIZE] = {0};

volatile bool isConnected = false;
volatile bool isAdvertising = false;
//volatile bool isScanning = false;

//ble_uuid_t m_adv_uuids[] = {{BLE_UUID_BATTERY_SERVICE, BLE_UUID_TYPE_BLE},        // Universally unique service identifiers.
//                            {BLE_UUID_NUS_SERVICE,     BLE_UUID_TYPE_BLE}};  
ble_uuid_t m_adv_uuids[] = {{BLE_UUID_NUS_SERVICE,     BLE_UUID_TYPE_BLE}};      
// ^^^ With both service UUIDs, ID, and group #, adv payload is too big.  Quick fix above to make it fit.
//    may also be fixable by specifying size of custom data manually, instead of sizeof (which includes padding)


void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    debug_log("ERR: SoftDevice assert, line %d in file %s .  Halting...\r\n",(int)line_num,p_file_name);
    while(1);
}


static void ble_error_handler(uint32_t error_code, uint32_t line_num)
{
    debug_log("ERR: BLE, error code %d, line %d.\r\n",(int)error_code,(int)line_num);
    while(1);
}


void ble_queue_adv_paused_callback(adv_paused_callback_t callback) {
    for (int i = 0; i < BLE_ADV_PAUSE_CALLBACK_QUEUE_SIZE; i++) {
        if (m_adv_pause_callback_queue[i] == NULL || m_adv_pause_callback_queue[i] == callback) {
            m_adv_pause_callback_queue[i] = callback;
            return;
        }
    }
    // No room for this callback.
    APP_ERROR_CHECK(NRF_ERROR_NO_MEM);
}

static void ble_adv_paused_call_callbacks(void) {
    for (int i = 0; i < BLE_ADV_PAUSE_CALLBACK_QUEUE_SIZE; i++) {
        if (m_adv_pause_callback_queue[i] != NULL) {
            m_adv_pause_callback_queue[i]();
            m_adv_pause_callback_queue[i] = NULL;
        }
    }
}

static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);  //no security needed

    //set BLE name
    err_code = sd_ble_gap_device_name_set(&sec_mode,(const uint8_t *)DEVICE_NAME,strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    //set BLE appearance
    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_TAG);
    APP_ERROR_CHECK(err_code);
}


static void services_init(void)
{
    uint32_t err_code;
    ble_nus_init_t nus_init;           //Nordic UART Service - for emulating a UART over BLE
    memset(&nus_init,0,sizeof(nus_init));
    
    nus_init.data_handler = BLEonReceive;
    
    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
    
    
    /*
    ble_bas_init_t bas_init;           //Battery service  (part of BLE standard)
    memset(&bas_init,0,sizeof(bas_init));

    bas_init.evt_handler          = NULL;
    bas_init.support_notification = false;
    bas_init.p_report_ref         = NULL;
    bas_init.initial_batt_level   = 100;

    err_code = ble_bas_init(&m_bas, &bas_init);
    BLE_ERROR_CHECK(err_code);
    */
    
}


static void sec_params_init(void)
{
    m_sec_params.bond         = SEC_PARAM_BOND;
    m_sec_params.mitm         = SEC_PARAM_MITM;
    m_sec_params.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    m_sec_params.oob          = SEC_PARAM_OOB;
    m_sec_params.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    m_sec_params.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
}

// Sends all buffered bytes that can currently be sent.
static void send_buffered_bytes(void) {
    bool tx_buffers_available = true;
    while (app_fifo_len(&m_ble_fifo) > 0 && tx_buffers_available) {
        uint8_t nus_packet[BLE_NUS_MAX_DATA_LEN];
        uint16_t packet_len = app_fifo_get_bytes(&m_ble_fifo, nus_packet, BLE_NUS_MAX_DATA_LEN);
        uint32_t err_code = ble_nus_string_send(&m_nus, nus_packet, packet_len);

        switch (err_code) {
            case NRF_SUCCESS:
                // Yay! We could queue the packet with the SoftDevice.
                break;
            case BLE_ERROR_NO_TX_BUFFERS:
                // Expected failure. The SoftDevice is out of buffers to take our packet with.
                // Recover by rewinding our buffer packet_len bytes so we send it next time.
                tx_buffers_available = false;
                app_fifo_rewind(&m_ble_fifo, packet_len);
                break;
            default:
                // Some unexpected error occurred. Reset!
                APP_ERROR_CHECK(err_code);
        }
    }
}

static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                         err_code;
    
    // security-related
    static ble_gap_evt_auth_status_t m_auth_status;
    bool                             master_id_matches;
    ble_gap_sec_kdist_t *            p_distributed_keys;
    ble_gap_enc_info_t *             p_enc_info;
    ble_gap_irk_t *                  p_id_info;
    ble_gap_sign_info_t *            p_sign_info;
    static ble_gap_enc_key_t         m_enc_key;           /**< Encryption Key (Encryption Info and Master ID). */
    static ble_gap_id_key_t          m_id_key;            /**< Identity Key (IRK and address). */
    static ble_gap_sign_info_t       m_sign_key;          /**< Signing Key (Connection Signature Resolving Key). */
    static ble_gap_sec_keyset_t      m_keys = {.keys_periph = {&m_enc_key, &m_id_key, &m_sign_key}};


    switch (p_ble_evt->header.evt_id)
    {
        case BLE_EVT_TX_COMPLETE:
            send_buffered_bytes();
            break;
        case BLE_GAP_EVT_CONNECTED:  //on BLE connect event
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            BLEonConnect();
            break;
        case BLE_GAP_EVT_DISCONNECTED:  //on BLE disconnect event
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            //debug_log("Disconnect reason: %d\r\n",(int)p_ble_evt->evt.gap_evt.params.disconnected.reason);
            BLEonDisconnect();
            break;
        case BLE_GAP_EVT_ADV_REPORT:  //On receipt of a response to an advertising request (during a scan)
            BLEonAdvReport(&(p_ble_evt->evt.gap_evt.params.adv_report));
            break;
        
        case BLE_GAP_EVT_TIMEOUT:
            if(p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN)  {
                //debug_log("Scan ended\r\n");
                BLEonScanTimeout();
            }
            /*else  {
                debug_log("Timeout.  src=%d\r\n", p_ble_evt->evt.gap_evt.params.timeout.src);
            }*/
            break;
            
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
                                                   BLE_GAP_SEC_STATUS_SUCCESS,
                                                   &m_sec_params,
                                                   &m_keys);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle,
                                                 NULL,
                                                 0,
                                                 BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_GAP_EVT_AUTH_STATUS:
            m_auth_status = p_ble_evt->evt.gap_evt.params.auth_status;
            break;
        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            master_id_matches  = memcmp(&p_ble_evt->evt.gap_evt.params.sec_info_request.master_id,
                                        &m_enc_key.master_id,
                                        sizeof(ble_gap_master_id_t)) == 0;
            p_distributed_keys = &m_auth_status.kdist_periph;

            p_enc_info  = (p_distributed_keys->enc  && master_id_matches) ? &m_enc_key.enc_info : NULL;
            p_id_info   = (p_distributed_keys->id   && master_id_matches) ? &m_id_key.id_info   : NULL;
            p_sign_info = (p_distributed_keys->sign && master_id_matches) ? &m_sign_key         : NULL;

            err_code = sd_ble_gap_sec_info_reply(m_conn_handle, p_enc_info, p_id_info, p_sign_info);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}


static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    on_ble_evt(p_ble_evt);
    
    // Intercept advertising timeout events to implement infinite advertising, and disconnect events to let radio-conflicting 
    //   operations to execute after a connection closes
    if( (p_ble_evt->header.evt_id == BLE_GAP_EVT_TIMEOUT                                        // if timeout event
            && p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISING)    //   from advertising
        || p_ble_evt->header.evt_id == BLE_GAP_EVT_DISCONNECTED)                                // OR if disconnect event
    {
        // Count number of pending pause requests
        int pauseReqSrc = PAUSE_REQ_NONE;
        for(int src=0; src<PAUSE_REQ_NONE; src++)  // look through all pause request sources
        {
            if(pauseRequest[src]) 
            {
                pauseReqSrc = src;
                break;  // check for any pending pause requests
            }
        }
        
        // If there are any pending pause requests, don't restart advertising yet.
        if(pauseReqSrc != PAUSE_REQ_NONE)
        {
            isAdvertising = false;
            debug_log("ADV: advertising paused, src %d.\r\n",pauseReqSrc);

            ble_adv_paused_call_callbacks();
        }
        
        // Otherwise restart advertising, for infinite advertising.
        else
        {
            isAdvertising = true;
            if(needAdvDataUpdate)
            {
                setAdvData();
                /*ble_advdata_t advdata;
                constructAdvData(&advdata);
                uint32_t result = ble_advdata_set(&advdata, NULL);  // set advertising data, don't set scan response data.
                if(result != NRF_SUCCESS)
                {
                    debug_log("ERR: error setting advertising data, #%d.\r\n",(int)result);
                }
                else
                {
                    debug_log("Updated adv. data.\r\n");
                }*/
                needAdvDataUpdate = false;
            }
                
            //debug_log("ADV: advertising Restarted\r\n");
            uint32_t err_code = ble_advertising_start(BLE_ADV_MODE_FAST);  // restart advertising
            APP_ERROR_CHECK(err_code);
        }
    }
    else
    {
        ble_advertising_on_ble_evt(p_ble_evt);
    }
    
    //ble_bas_on_ble_evt(&m_bas, p_ble_evt);
    ble_nus_on_ble_evt(&m_nus, p_ble_evt);
}


static void sys_evt_dispatch(uint32_t sys_evt)
{
    ble_advertising_on_sys_evt(sys_evt);
    storer_on_sys_evt(sys_evt);
}


static void ble_stack_init(void)
{
    uint32_t err_code;
    
    //Initiate softdevice, with external low-freq crystal
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);

    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
    
    ble_enable_params.gatts_enable_params.attr_tab_size   = BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);
    
    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
    
    // Register with the SoftDevice handler module for system events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}

void advertising_init(void)
{
    uint32_t      err_code;
    
    // Build advertising data struct to pass into @ref ble_advertising_init.
    ble_advdata_t advdata;    
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = false;
    advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = m_adv_uuids;

    ble_adv_modes_config_t options = {0};
    options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;
    options.ble_adv_slow_enabled = BLE_ADV_SLOW_DISABLED;
    options.ble_adv_directed_enabled = BLE_ADV_DIRECTED_DISABLED;
    options.ble_adv_whitelist_enabled = BLE_ADV_WHITELIST_DISABLED;

    err_code = ble_advertising_init(&advdata, NULL, &options, NULL /*on_adv_evt*/, NULL);
    APP_ERROR_CHECK(err_code);
    setAdvData();
}

void updateAdvData()
{
    needAdvDataUpdate = true;
}

void setAdvData()
{

    int scaledBatteryLevel = (int)(100.0*BatteryMonitor_getBatteryVoltage()) - 100;
    customAdvData.battery = (scaledBatteryLevel <= 255) ? scaledBatteryLevel : 255;  // clip scaled level
    customAdvData.statusFlags = 0;
    customAdvData.statusFlags |= (dateReceived) ? 0x01 : 0x00;  // set sync status bit
    customAdvData.statusFlags |= (isCollecting) ? 0x02 : 0x00;  // set collector status bit
    customAdvData.statusFlags |= (scanner_enable) ? 0x04 : 0x00;  // set collector status bit
    customAdvData.group = badgeAssignment.group;
    customAdvData.ID = badgeAssignment.ID;
    // customAdvData.MAC is already set
    
    //debug_log("custom data size: %d\r\n",sizeof(custom_data_array));
    
    ble_advdata_manuf_data_t custom_manuf_data;
    custom_manuf_data.company_identifier = 0xFF00;  // unofficial manufacturer code
    custom_manuf_data.data.p_data = (uint8_t*)(&customAdvData);
    custom_manuf_data.data.size = CUSTOM_ADV_DATA_LEN;
    
    // Build advertising data struct to pass into @ref ble_advdata_set.
    ble_advdata_t advdata;
    memset(&advdata, 0, sizeof(ble_advdata_t));
    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = false;
    advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = m_adv_uuids;
    
    advdata.p_manuf_specific_data = &custom_manuf_data;
    
    uint32_t result = ble_advdata_set(&advdata, NULL);  // set advertising data, don't set scan response data.
    if(result != NRF_SUCCESS)  {
        debug_log("ERR: error setting advertising data, #%d.\r\n",(int)result);
    }
    else  {
        debug_log("  Updated adv. data.\r\n");
        needAdvDataUpdate = false;
    }
    
}


void BLEsetBadgeAssignment(badge_assignment_t assignment)
{
    if (assignment.ID == RESET_ID)  {
        assignment.ID = defaultID;
    }
    if (assignment.group == RESET_GROUP)  {
        assignment.group = NO_GROUP;
    }
    badgeAssignment = assignment;
    debug_log("BLE: new assignment ID:0x%hX group:%d.\r\n",   badgeAssignment.ID, (int)badgeAssignment.group);
    updateAdvData();
}


void BLE_init()
{
    ble_stack_init();
    gap_params_init();
    services_init();
    sec_params_init();
    
    ble_gap_addr_t MAC;
    sd_ble_gap_address_get(&MAC);
    debug_log("MAC address: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X", MAC.addr[5],MAC.addr[4],MAC.addr[3],
                                                                MAC.addr[2],MAC.addr[1],MAC.addr[0]);
    
    defaultID = crc16_compute(MAC.addr,6,NULL);  // compute default badge ID from MAC address
    debug_log("-->default ID: 0x%hX\r\n",defaultID);
    badgeAssignment.ID = defaultID;
    badgeAssignment.group = NO_GROUP;
    
    // Copy MAC address into custom advertising data struct.
    for(int i = 0; i <= 5; i++)  { 
        customAdvData.MAC[i] = MAC.addr[i];
    }

    app_fifo_init(&m_ble_fifo, m_ble_fifo_buffer, sizeof(m_ble_fifo_buffer));
    
    //advertising_init();
    //uint32_t err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    //BLE_ERROR_CHECK(err_code);
}

void BLEstartAdvertising()
{
    if((!isConnected) && (!isAdvertising))  {
        uint32_t err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
        if(err_code == NRF_SUCCESS)  {
            debug_log("ADV: advertising started\r\n");
            isAdvertising = true;
            return;
        }
        APP_ERROR_CHECK(err_code);
    }
}


void BLEdisable()
{
    uint32_t err_code = softdevice_handler_sd_disable();
    APP_ERROR_CHECK(err_code);
}


bool BLEpause(ble_pauseReq_src source)
{
    pauseRequest[source] = true;
    bool isScanning = (getScanState() == SCANNER_SCANNING);
    //debug_log("%d %d %d\r\n", isConnected, isAdvertising, isScanning);
    if(isConnected || isAdvertising || isScanning)  return false;  // return false if BLE active.
    else return true;
}

void BLEresume(ble_pauseReq_src source)
{
    pauseRequest[source] = false;
    for(int src=0; src<PAUSE_REQ_NONE; src++)  {  // look through all pause request sources
        if(pauseRequest[src]) return;  // if any pending pause requests, don't restart advertising
    }
    // If no pending pause requests, restart advertising.
    if (needAdvDataUpdate) setAdvData();
    BLEstartAdvertising();
}

void BLEforceDisconnect()
{
    sd_ble_gap_disconnect(m_conn_handle,BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
}


ble_status_t BLEgetStatus()
{
    if(isConnected)  {
        return BLE_CONNECTED;
    }
    else if(isAdvertising)  {
        return BLE_ADVERTISING;
    }
    else  {
        return BLE_INACTIVE;
    }
}

bool BLEpauseReqPending()
{
    for(int src=0; src<PAUSE_REQ_NONE; src++)  {  // look through all pause request sources
        if(pauseRequest[src]) return true;  // if any pending pause requests, don't restart advertising
    }
    return false;
}


bool notificationEnabled()  
{
    return m_nus.is_notification_enabled;
}

// Queues the len bytes at data to be sent over the BLE UART Service.
//   As much data as possible will also be immediately be queued with the SoftDevice for sending.
//   The other data will be sent as room becomes available with the SoftDevice.
// Returns the number of bytes that could be queued for transmission.
uint16_t ble_write_buffered(uint8_t * data, uint16_t len) {
    APP_ERROR_CHECK_BOOL(data != NULL);

    uint16_t num_bytes_queued = app_fifo_put_bytes(&m_ble_fifo, data, len);
    send_buffered_bytes();
    return num_bytes_queued;
}

// Queues the len bytes at data to be sent over the BLE UART Service.
// Functions similarly to ble_write_buffered, except if the entire message cannot be written, then no bytes
//  are buffered to be sent.
// Returns NRF_SUCCESS if sucessful or NRF_ERROR_NO_MEM if the message could be queued.
uint32_t ble_write_buffered_atomic(uint8_t * data, uint16_t len) {
    if (BLE_FIFO_SIZE - app_fifo_len(&m_ble_fifo) < len) {
        return NRF_ERROR_NO_MEM;
    }

    uint16_t bytesWritten = ble_write_buffered(data, len);

    APP_ERROR_CHECK_BOOL(bytesWritten == len);

    return NRF_SUCCESS;
}

bool BLEwrite(uint8_t* data, uint16_t len)  {
    uint32_t err_code = ble_nus_string_send(&m_nus, data, len);
    if (err_code != NRF_SUCCESS)  {
        //debug_log("BLEwrite error: %u\r\n",(unsigned int)err_code);
        //BLEwrite fails if we try writing before the master has received the previous packet
        //Can happen frequently while trying to send a large chunk of data
        return false;
    }
    
    //ble_timeout_set(CONNECTION_TIMEOUT_MS);  // refresh connection timeout
    
    return true;
}


bool BLEwriteChar(uint8_t dataChar)  
{
    uint8_t data = dataChar;
    return BLEwrite(&data, 1);
}

