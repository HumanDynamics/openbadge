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





#endif
