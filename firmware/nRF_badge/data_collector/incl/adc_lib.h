#ifndef __ADC_LIB_H
#define __ADC_LIB_H


#include "nrf_adc.h"


#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes


typedef enum {
	ADC_NO_OPERATION 		= 0,
	ADC_READING_OPERATION 	= (1 << 0),	
} adc_operation_t;


typedef struct {
	uint8_t			 		adc_peripheral;			/**< Set to the desired adc peripheral (should be 0 in case of ADC). The Peripheral has to be enabled in the sdk_config.h file */
	nrf_adc_config_t 		nrf_adc_config;			/**< Set the adc configuration: resolution, scaling, reference (possible parameters in nrf_adc.h) */
	nrf_adc_config_input_t 	nrf_adc_config_input;	/**< Set the adc input (possible parameters in nrf_adc.h) */
	int32_t 				adc_instance_id;		/**< Instance index: Setted by the init-function (do not set!) */
} adc_instance_t;


ret_code_t adc_init(adc_instance_t* adc_instance);

ret_code_t adc_read_raw(const adc_instance_t* adc_instance, int32_t* raw);

ret_code_t adc_read_voltage(const adc_instance_t* adc_instance, float* voltage, float ref_voltage);


#endif
