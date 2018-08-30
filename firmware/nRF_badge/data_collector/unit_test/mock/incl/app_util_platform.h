/** @file
 * @brief Simulaton of various types and definitions available to all applications.
 */

#ifndef APP_UTIL_PLATFORM_H__
#define APP_UTIL_PLATFORM_H__

#include <stdint.h>
#include <stdbool.h>
#include "timer_lib.h"		// Needed for the Enter/Exit Critical Section-function


#ifdef __cplusplus
extern "C" {
#endif

// This is extracted from nrf.h and nrf5X.h (But we need it for the priority declaration)
#if defined (NRF51)
	#define __CORTEX_M                (0x00U)                                      /*!< Cortex-M Core */
#elif defined (NRF52840_XXAA)
	#define __CORTEX_M                (0x04U)                                      /*!< Cortex-M Core */
#elif defined (NRF52832_XXAA)
	#define __CORTEX_M                (0x04U)                                      /*!< Cortex-M Core */
#else
	#error "Device must be defined. See nrf.h."
#endif /* NRF51, NRF52832_XXAA, NRF52840_XXAA */



#if __CORTEX_M == (0x00U)
#define _PRIO_SD_HIGH       0
#define _PRIO_APP_HIGH      1
#define _PRIO_APP_MID       1
#define _PRIO_SD_LOW        2
#define _PRIO_APP_LOW       3
#define _PRIO_APP_LOWEST    3
#define _PRIO_THREAD        4
#elif __CORTEX_M == (0x04U)
#define _PRIO_SD_HIGH       0
#define _PRIO_SD_MID        1
#define _PRIO_APP_HIGH      2
#define _PRIO_APP_MID       3
#define _PRIO_SD_LOW        4
#define _PRIO_SD_LOWEST     5
#define _PRIO_APP_LOW       6
#define _PRIO_APP_LOWEST    7
#define _PRIO_THREAD        15
#else
    #error "No platform defined"
#endif


//lint -save -e113 -e452
/**@brief The interrupt priorities available to the application while the SoftDevice is active. */
typedef enum
{
#ifndef SOFTDEVICE_PRESENT
    APP_IRQ_PRIORITY_HIGHEST = _PRIO_SD_HIGH,
#else
    APP_IRQ_PRIORITY_HIGHEST = _PRIO_APP_HIGH,
#endif
    APP_IRQ_PRIORITY_HIGH    = _PRIO_APP_HIGH,
#ifndef SOFTDEVICE_PRESENT
    APP_IRQ_PRIORITY_MID     = _PRIO_SD_LOW,
#else
    APP_IRQ_PRIORITY_MID     = _PRIO_APP_MID,
#endif
    APP_IRQ_PRIORITY_LOW     = _PRIO_APP_LOW,
    APP_IRQ_PRIORITY_LOWEST  = _PRIO_APP_LOWEST,
    APP_IRQ_PRIORITY_THREAD  = _PRIO_THREAD     /**< "Interrupt level" when running in Thread Mode. */
} app_irq_priority_t;


/**@brief Macro for entering a critical region.
 *
 * @note Due to implementation details, there must exist one and only one call to
 *       CRITICAL_REGION_EXIT() for each call to CRITICAL_REGION_ENTER(), and they must be located
 *       in the same scope.
 */
#ifdef SOFTDEVICE_PRESENT
#define CRITICAL_REGION_ENTER()                                                             \
    {                                                                                       \
        timer_enter_critical_section();
#else
#define CRITICAL_REGION_ENTER() timer_enter_critical_section()
#endif

/**@brief Macro for leaving a critical region.
 *
 * @note Due to implementation details, there must exist one and only one call to
 *       CRITICAL_REGION_EXIT() for each call to CRITICAL_REGION_ENTER(), and they must be located
 *       in the same scope.
 */
#ifdef SOFTDEVICE_PRESENT
#define CRITICAL_REGION_EXIT()                                                              \
        timer_exit_critical_section();                                         				\
    }
#else
#define CRITICAL_REGION_EXIT() timer_exit_critical_section()
#endif


#ifdef __cplusplus
}
#endif

#endif
