#ifndef __ACCEL_LIB_H
#define __ACCEL_LIB_H


#include <stdbool.h>
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes

#ifndef UNIT_TEST
#include "custom_board.h"
#else
#define ACCELEROMETER_PRESENT 1
#endif

#ifndef ACCELEROMETER_PRESENT
#define ACCELEROMETER_PRESENT 0
#endif

/**@brief Accelerometer datarate type. 
 * 
 * @details	For more information see: Table 4: Data rate configuration at p.9/59 in AN5005
 */
typedef enum {
	ACCEL_DATARATE_1_HZ				= 0b0001,
	ACCEL_DATARATE_10_HZ			= 0b0010,
	ACCEL_DATARATE_25_HZ			= 0b0011,
	ACCEL_DATARATE_50_HZ			= 0b0100,
	ACCEL_DATARATE_100_HZ			= 0b0101,
	ACCEL_DATARATE_200_HZ			= 0b0110,
	ACCEL_DATARATE_400_HZ			= 0b0111,
	ACCEL_DATARATE_1600_HZ			= 0b1000,	/**< Only available in low-power mode */
	ACCEL_DATARATE_1344_HZ_5376_HZ	= 0b1001,	/**< In normal-mode/high-resolution-mode -> 1344Hz and in low-power mode -> 5376Hz */
} accel_datarate_t;


/**@brief Accelerometer operating mode type. */
typedef enum {
	ACCEL_POWER_DOWN_MODE = 0,
	ACCEL_LOW_POWER_MODE = 1,
	ACCEL_NORMAL_MODE = 2,
	ACCEL_HIGH_RESOLUTION_MODE = 3,
} accel_operating_mode_t;


/**@brief Accelerometer axis type. */
typedef enum {
	ACCEL_X_AXIS_ENABLE		= (1 << 0),
	ACCEL_Y_AXIS_ENABLE 	= (1 << 1),
	ACCEL_Z_AXIS_ENABLE 	= (1 << 2),
	ACCEL_X_AXIS_DISABLE	= (1 << 3),
	ACCEL_Y_AXIS_DISABLE	= (1 << 4),
	ACCEL_Z_AXIS_DISABLE	= (1 << 5),
} accel_axis_t;


/**@brief Accelerometer full scale type. */
typedef enum {
	ACCEL_FULL_SCALE_2G 	= 0b00,
	ACCEL_FULL_SCALE_4G 	= 0b01,
	ACCEL_FULL_SCALE_8G 	= 0b10,
	ACCEL_FULL_SCALE_16G 	= 0b11,
} accel_full_scale_t;


/**@brief Accelerometer FIFO type/mode. */
typedef enum {
	ACCEL_FIFO_DISABLE = 0,			/**< FIFO disabled == Bypass mode */
	ACCEL_FIFO_STREAM_ENABLE = 1,	/**< FIFO stream mode */
} accel_fifo_t;


/**@brief Accelerometer high-pass filter type. */
typedef enum {
	ACCEL_HP_FILTER_DISABLE = 0,
	ACCEL_HP_FILTER_ENABLE 	= 1,
} accel_HP_filter_t;


/**@brief Accelerometer interrupt event type. */
typedef enum {
	ACCEL_NO_INTERRUPT 			= 0,		/**< No interrupt will be generated */
	ACCEL_MOTION_INTERRUPT 		= 1,		/**< Wake up interrupt on motion */
	ACCEL_SINGLE_TAP_INTERRUPT 	= 2,		/**< Tap interrupt on single tap */
	ACCEL_DOUBLE_TAP_INTERRUPT 	= 3,		/**< Tap interrupt on double tap */
} accel_interrupt_event_t;


/**@brief Accelerometer event handler type. */
typedef void (*accel_interrupt_handler_t) (accel_interrupt_event_t const * p_event);



/**@brief   Function for initializing the acceleromenter module.
 *
 * @details This functions initializes the underlying spi-module.
 *			The spi peripheral has to be enabled in the config-file: sdk_config.h.
 *			Furthermore, it reads out the WHO_AM_I-register to check whether the communication works.
 *			
 *
 * @retval  NRF_SUCCESS    		If the module was successfully initialized.
 * @retval 	NRF_ERROR_BUSY		If the register-read operation failed (because of an ongoing spi operation).
 * @retval  NRF_ERROR_INTERNAL  If there was an error while initializing the spi-module (e.g. bad configuration), the WHO_AM_I-register mismatch or there was antother internal problem.
 */
ret_code_t accel_init(void);


/**@brief Function for setting the datarate of the accelerometer.
 *
 * @param[in]   accel_datarate			The datarate that should be set.
 *
 * @retval  NRF_SUCCESS    				If the operation was successful.
 * @retval 	NRF_ERROR_BUSY				If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL			If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
ret_code_t accel_set_datarate(accel_datarate_t accel_datarate);


/**@brief Function for setting the operating mode of the accelerometer.
 *
 * @param[in]   accel_operating_mode	The operating-mode that should be set.
 *
 * @retval  NRF_SUCCESS    				If the operation was successful.
 * @retval 	NRF_ERROR_BUSY				If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL			If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
ret_code_t accel_set_operating_mode(accel_operating_mode_t accel_operating_mode);



/**@brief Function for enabling/disabling one or more axis of the accelerometer.
 *
 * @param[in]   accel_axis				A logical or concatenation of different axis (e.g. (ACCEL_X_AXIS_ENABLE | ACCEL_Y_AXIS_ENABLE | ACCEL_Z_AXIS_ENABLE))
 *
 * @retval  NRF_SUCCESS    				If the operation was successful.
 * @retval 	NRF_ERROR_BUSY				If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL			If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
ret_code_t accel_set_axis(accel_axis_t accel_axis);


/**@brief Function for setting the full-scale value of the accelerometer.
 *
 * @param[in]   accel_full_scale		The full-scale value that should be set.
 *
 * @retval  NRF_SUCCESS    				If the operation was successful.
 * @retval 	NRF_ERROR_BUSY				If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL			If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
ret_code_t accel_set_full_scale(accel_full_scale_t accel_full_scale);


/**@brief Function for enabling/disabling the high-pass filter.
 *
 * @param[in]   accel_HP_filter			ACCEL_HP_FILTER_DISABLE or ACCEL_HP_FILTER_ENABLE.
 *
 * @retval  NRF_SUCCESS    				If the operation was successful.
 * @retval 	NRF_ERROR_BUSY				If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL			If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
ret_code_t accel_set_HP_filter(accel_HP_filter_t accel_HP_filter);


/**@brief Function for setting the interrupt-handler that is called when an interrupt is generated.
 *
 * @param[in]   accel_interrupt_handler		The handler that is called when an interrupt is generated (called with the event-source).
 *
 */
void accel_set_interrupt_handler(accel_interrupt_handler_t accel_interrupt_handler);



/**@brief Function for setting an interrupt operation/event.
 *
 * @param[in]   threshold_mg			The threshold in mg to generate an interrupt.
 * @param[in]   minimal_duration_ms		The minimal duration in ms the acceleration has to be above the threshold to generate an interrupt.
 *
 * @retval  NRF_SUCCESS    				If the operation was successful.
 * @retval 	NRF_ERROR_BUSY				If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL			If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
ret_code_t accel_set_motion_interrupt_parameters(uint16_t threshold_mg, uint16_t minimal_duration_ms);


/**@brief Function for setting an interrupt operation/event.
 *
 * @details This function sets the interrupt according to the input parameter accel_interrupt_event.
 *			Before it sets the required configuration for the interrupt, it clears the bits of the registers 
 *			that are needed by this function. This allows the switch between different interrupt events without
 *			problems.
 *
 * @note 	The interrupt events ACCEL_SINGLE_TAP_INTERRUPT and ACCEL_DOUBLE_TAP_INTERRUPT are not implemented yet.
 *			For these two events also a function like accel_set_motion_interrupt_parameters() must be implemented, 
 *			to set the required parameters.
 *
 * @param[in]   accel_interrupt_event	The interrupt event that should be enabled. If no interrupt should be generated call with ACCEL_NO_INTERRUPT.
 *
 * @retval  NRF_SUCCESS    				If the operation was successful.
 * @retval 	NRF_ERROR_BUSY				If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL			If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
ret_code_t accel_set_interrupt(accel_interrupt_event_t accel_interrupt_event);


/**@brief Function for resetting the INT1-interrupt source of the accelerometer for latched interrupts.
 *
 * @details If the interrupt is latched (e.g. motion interrupt) this function must be called to reset the interrupt,
 * 			so that a new interrupt could be generated by the accelerometer.
 *
 * @retval  NRF_SUCCESS    			If the operation was successful.
 * @retval 	NRF_ERROR_BUSY			If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL		If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
ret_code_t accel_reset_interrupt(void);



/**@brief Function for enabling/disabling the internal FIFO of the accelerometer.
 *
 * @details Enables the stream mode of the internal 32-level FIFO.
 *
 * @param[in]   accel_fifo			ACCEL_FIFO_DISABLE or ACCEL_FIFO_ENABLE. 
 *
 * @retval  NRF_SUCCESS    			If the operation was successful.
 * @retval 	NRF_ERROR_BUSY			If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL		If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
ret_code_t accel_set_fifo(accel_fifo_t accel_fifo);




/**@brief Function for reading the current acceleration in mg.
 *
 * @details This function reads the acceleration data from the accelerometer and converts it to mg.
 *			If the FIFO is enabled this function reads everything from the FIFO and returns the number of samples
 *			in the output variable num_samples. If the FIFO is not enabled num_samples is always 1.
 *			
 * @warning The output-buffers (accel_x, accel_y, accel_z) have to be a size of at least 32 elements.
 *
 *
 * @param[out]	accel_x			Pointer to array with at least 32 entries for acceleration data in x-direction.
 * @param[out]	accel_y			Pointer to array with at least 32 entries for acceleration data in y-direction.
 * @param[out]	accel_z			Pointer to array with at least 32 entries for acceleration data in z-direction.
 * @param[out]	num_samples		Pointer to memory where to store the number of samples read from the accelerometer.
 * @param[in]	max_num_samples	Maximal number of samples to read from the accelerometer.
 *
 * @retval  NRF_SUCCESS    			If the operation was successful.
 * @retval 	NRF_ERROR_BUSY			If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL		If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
ret_code_t 	accel_read_acceleration(int16_t* accel_x, int16_t* accel_y, int16_t* accel_z, uint8_t* num_samples, uint32_t max_num_samples);



/**@brief   Function for testing the accelerometer module.
 *
 * @details	The function configures the accelerometer for a selftest.
 *			The selftest contains a wake-up interrupt test, so the user
 *			has to generate a wake-up interrupt by moving the device.
 *			The selftest can be configured by the following parameters: 
 *			- ACCEL_SELFTEST_TIME_FOR_INTERRUPT_GENERATION_MS: 	The time the wake-up event has to appear after function-call
 *			- ACCEL_SELFTEST_INTERRUPT_THRESHOLD_MG:		The threshold in mg to generate the wake-up interrupt
 *			- ACCEL_SELFTEST_INTERRUPT_MINIMAL_DURATION_MS:		Minimal duration must be above the threshold, before generating an interrupt
 *			
 *
 * @retval  0	If selftest failed.
 * @retval  1	If selftest passed.
 *
 * @note	systick_init() has to be called before.
 */
bool accel_selftest(void);



#endif
