/** @file
 *
 *
 * @brief 
 */

#ifndef CALLBACK_GENERATOR_LIB_H
#define CALLBACK_GENERATOR_LIB_H

#include "callback_generator_internal_lib.h"
#include "stdint.h"

#include "sdk_errors.h"		// Needed for the definition of ret_code_t and the error-codes

/** Accelerometer defines */
// From nrf51_bitfields.h
#define GPIOTE_CONFIG_POLARITY_LoToHi (1UL) /*!< Task mode: Set pin from OUT[n] task. Event mode: Generate IN[n] event when rising edge on pin. */
#define GPIOTE_CONFIG_POLARITY_HiToLo (2UL) /*!< Task mode: Clear pin from OUT[n] task. Event mode: Generate IN[n] event when falling edge on pin. */
#define GPIOTE_CONFIG_POLARITY_Toggle (3UL) /*!< Task mode: Toggle pin from OUT[n]. Event mode: Generate IN[n] when any change on pin. */

// From nrf_gpiote.h
typedef enum
{
  NRF_GPIOTE_POLARITY_LOTOHI = GPIOTE_CONFIG_POLARITY_LoToHi,       ///<  Low to high.
  NRF_GPIOTE_POLARITY_HITOLO = GPIOTE_CONFIG_POLARITY_HiToLo,       ///<  High to low.
  NRF_GPIOTE_POLARITY_TOGGLE = GPIOTE_CONFIG_POLARITY_Toggle        ///<  Toggle.
} nrf_gpiote_polarity_t;

// From nrf_drv_gpiote.h
typedef uint32_t nrf_drv_gpiote_pin_t;

// Declare all the needed data-types and functions:
CALLBACK_HANDLER_FUNCTION_DECLARATION(ACCEL_INT1, void, nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
CALLBACK_GENERATOR_FUNCTION_DECLARATION(ACCEL_INT1, void, nrf_drv_gpiote_pin_t* pin, nrf_gpiote_polarity_t* action);
CALLBACK_GENERATOR_DECLARATION(ACCEL_INT1);


/** BLE defines */
#include "ble_lib.h"

// Declare all the needed data-types and functions:
CALLBACK_HANDLER_FUNCTION_DECLARATION(ble_on_connect, void, void);
CALLBACK_GENERATOR_FUNCTION_DECLARATION(ble_on_connect, void, void);
CALLBACK_GENERATOR_DECLARATION(ble_on_connect);

CALLBACK_HANDLER_FUNCTION_DECLARATION(ble_on_disconnect, void, void);
CALLBACK_GENERATOR_FUNCTION_DECLARATION(ble_on_disconnect, void, void);
CALLBACK_GENERATOR_DECLARATION(ble_on_disconnect);

CALLBACK_HANDLER_FUNCTION_DECLARATION(ble_on_receive, void, uint8_t* data, uint16_t length);
CALLBACK_GENERATOR_FUNCTION_DECLARATION(ble_on_receive, void, uint8_t* data, uint16_t* length, uint16_t max_len);
CALLBACK_GENERATOR_DECLARATION(ble_on_receive);

CALLBACK_HANDLER_FUNCTION_DECLARATION(ble_on_transmit_complete, void, void);
CALLBACK_GENERATOR_FUNCTION_DECLARATION(ble_on_transmit_complete, void, void);
CALLBACK_GENERATOR_DECLARATION(ble_on_transmit_complete);

CALLBACK_HANDLER_FUNCTION_DECLARATION(ble_on_scan_report, void, ble_gap_evt_adv_report_t* scan_report);
CALLBACK_GENERATOR_FUNCTION_DECLARATION(ble_on_scan_report, void, ble_gap_evt_adv_report_t* scan_report);
CALLBACK_GENERATOR_DECLARATION(ble_on_scan_report);

CALLBACK_HANDLER_FUNCTION_DECLARATION(ble_on_scan_timeout, void, void);
CALLBACK_GENERATOR_FUNCTION_DECLARATION(ble_on_scan_timeout, void, void);
CALLBACK_GENERATOR_DECLARATION(ble_on_scan_timeout);




#endif
