#ifndef __ADC_LIB_H
#define __ADC_LIB_H


/** @file
 *
 * @brief ADC abstraction library.
 *
 * @details It enables to read from the ADC from different contexts. 
 *			If the selected ADC peripheral (currently only only adc_peripheral = 0 is supported)
 *			is currently in use, it will inform the other context by returning NRF_ERROR_BUSY.
 *		
 */
 

#include "sdk_common.h"	// Needed for the definition of ret_code_t and the error-codes
#include "nrf_adc.h"


/**@example	Example of adc_instance_t 
 *
 *
 * 			adc_instance_t adc_instance;											// Create an adc_instance-struct.
 *
 * 			adc_instance.adc_peripheral = 0;										// Set the adc_peripheral index to 0 (has to be enabled in sdk_config.h).
 * 			adc_instance.nrf_adc_config.resolution 	= NRF_ADC_CONFIG_RES_8BIT;		// Set the adc resolution. NRF_ADC_CONFIG_RES_8BIT from nrf_adc.h.
 *			adc_instance.nrf_adc_config.reference	= NRF_ADC_CONFIG_REF_EXT_REF1;	// Set the adc reference. NRF_ADC_CONFIG_REF_EXT_REF1 from nrf_adc.h.
 *			adc_instance.nrf_adc_config_input		= NRF_ADC_CONFIG_INPUT_6;		// Set the adc conversion input. NRF_ADC_CONFIG_INPUT_6 from nrf_adc.h.
 *
 *			adc_init(&adc_instance);												// Initialize the adc_instance.
 *
 */

 
 /**@brief ADC instance type. */
typedef struct {
	uint8_t			 		adc_peripheral;			/**< Set to the desired adc peripheral (should be 0 in case of ADC). The Peripheral has to be enabled in the sdk_config.h file */
	nrf_adc_config_t 		nrf_adc_config;			/**< Set the adc configuration: resolution, scaling, reference (possible parameters in nrf_adc.h) */
	nrf_adc_config_input_t 	nrf_adc_config_input;	/**< Set the adc input (possible parameters in nrf_adc.h) */
	int32_t 				adc_instance_id;		/**< Instance index: Setted by the init-function (do not set!) */
} adc_instance_t;


/**@brief   Function for initializing an instance for the adc peripheral.
 *
 * @details This functions actually only checks the specified adc_peripheral and sets the adc_instance_id of adc_instance.
 *			The application must set adc_peripheral = 0 of the adc_instance (because currently only one adc peripheral is available).
 *			Furthermore, the application has to set nrf_adc_config, nrf_adc_config_input of adc_instance before calling this function.
 *
 *
 * @param[in,out]   adc_instance		Pointer to an preconfigured adc_instance.
 * @param[in]   	default_instance	Flag if it is the default instance or not (1 or 0).
 *
 * @details	If it is the default instance, every time the adc_read_raw (or adc_read_voltage)
 *			function is called, it reconfigures the adc-configuration to the default one.
 *			This is done to achieve very fast readings through the function adc_read_raw_default,
 *			where no configuration has to be done.
 *
 * @retval  NRF_SUCCESS    				If the adc_instance was successfully initialized.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified peripheral in the adc_instance is not correct.
 */
ret_code_t adc_init(adc_instance_t* adc_instance, uint8_t default_instance);


/**@brief   Function for reading a raw ADC value on the input of the specified adc_instance in blocking mode.
 *
 * @details This function reads out the raw ADC value of the specified adc_instance in blocking mode.
 *			It tries to minimize the configuration and input-selection calls to be more efficiently.
 *
 * @param[in]   adc_instance		Pointer to an initialized adc_instance.
 * @param[out]  raw					Pointer to memory, where the sampled ADC value, depending on the configuration of the adc_instance and the input voltage, is stored.
 *
 * @retval  NRF_SUCCESS    			If the ADC read was successful.
 * @retval  NRF_ERROR_BUSY  		If the selected ADC peripheral is currently in use.
 */
ret_code_t adc_read_raw(const adc_instance_t* adc_instance, int32_t* raw);



/**@brief   Function for reading a raw ADC value on the input of the specified adc_instance in blocking mode (The specified adc_instance has to be the default one!).
 *
 * @details This function reads out the raw ADC value of the specified adc_instance in blocking mode.
 *			It tries to minimize the configuration and input-selection calls to be more efficiently.
 *
 * @param[in]   adc_instance		Pointer to an initialized adc_instance.
 * @param[out]  raw					Pointer to memory, where the sampled ADC value, depending on the configuration of the adc_instance and the input voltage, is stored.
 *
 * @retval  NRF_SUCCESS    			If the ADC read was successful.
 * @retval  NRF_ERROR_BUSY  		If the selected ADC peripheral is currently in use.
 * @retval  NRF_ERROR_INVALID_PARAM	If the adc_instance is not the default one.
 */
ret_code_t adc_read_raw_default(const adc_instance_t* adc_instance, int32_t* raw);

/**@brief   Function for reading the voltage on the input of specified adc_instance in blocking mode.
 *
 * @details This function reads out the voltage by calling the adc_read_raw() and 
 *			converts the raw value to a voltage in respect to the reference voltage. 
 *
 * @param[in]   adc_instance		Pointer to an initialized adc_instance.
 * @param[out]  voltage				Pointer to memory, where the voltage in mV value is stored.
 * @param[in]   ref_voltage			The reference voltage in mV of the adc_instance.
 *
 * @retval  NRF_SUCCESS    			If the ADC read was successful.
 * @retval  NRF_ERROR_BUSY  		If the selected ADC peripheral is currently in use.
 */
ret_code_t adc_read_voltage(const adc_instance_t* adc_instance, float* voltage, float ref_voltage);


#endif
