#include "adc_lib.h"


#define ADC_PERIPHERAL_NUMBER 		ADC_COUNT			 /**< Number of activated adc peripherals in sdk_config.h. */

/**@brief The different ADC operations. These operations will be used to set the peripheral busy or not. */
typedef enum {
	ADC_NO_OPERATION 		= 0,			/**< Currently no adc operation ongoing. */
	ADC_READING_OPERATION 	= (1 << 0),		/**< Currently there is an adc operation ongoing. */
} adc_operation_t;


static volatile adc_operation_t  	adc_operations[ADC_PERIPHERAL_NUMBER] 	= {0};		/**< Array to save the current adc_operations (needed to check if there is an ongoing adc operation) */
static const adc_instance_t *		adc_default_instances[ADC_PERIPHERAL_NUMBER] 	= {NULL};	/**< Array of pointers to the current adc_instances (needed to check whether the configuration and input-selection has to be done again) */
static uint32_t 					adc_instance_number = 1; 							/**< adc_instance_number starts at 1 not 0 because all entries in the spi_instances-arrays are 0. So the check for the adc_instance_id-element may not work correctly. */ 



ret_code_t adc_init(adc_instance_t* adc_instance, uint8_t default_instance) {	

	// Check if the selected peripheral exists
	if(adc_instance->adc_peripheral == 0) {
		
		#if ADC_ENABLED
		#else		
		return NRF_ERROR_INVALID_PARAM;
		#endif
	} else {
		return NRF_ERROR_INVALID_PARAM;
	}

	adc_instance->adc_instance_id 		= adc_instance_number;
	if(default_instance) {
		adc_default_instances[adc_instance->adc_peripheral] = adc_instance;
		nrf_adc_configure((nrf_adc_config_t *)  &(adc_default_instances[adc_instance->adc_peripheral]->nrf_adc_config)); 
		nrf_adc_input_select(adc_default_instances[adc_instance->adc_peripheral]->nrf_adc_config_input);
	}
	adc_instance_number++;
	
	
	return NRF_SUCCESS;
}

ret_code_t adc_read_raw(const adc_instance_t* adc_instance, int32_t* raw) {
	uint8_t peripheral_index = adc_instance->adc_peripheral;
	
	// Check if there is an operation ongoing on this peripheral
	if(adc_operations[peripheral_index] != ADC_NO_OPERATION){
		return NRF_ERROR_BUSY;
	}
	
	// Set the operation
	adc_operations[peripheral_index] = ADC_READING_OPERATION;
	

	nrf_adc_configure((nrf_adc_config_t *)  &(adc_instance->nrf_adc_config)); 
	
	// Only configure the input, if we have to
	nrf_adc_input_select(adc_instance->nrf_adc_config_input);

	// Do the ADC conversion on the configured channel
	nrf_adc_start();
	while(!nrf_adc_conversion_finished());
	*raw = nrf_adc_result_get();
	nrf_adc_conversion_event_clean();
    nrf_adc_stop();
	
	
	// Check if we have an default instance. If yes, configure it again.
	if(adc_default_instances[peripheral_index] != NULL) {
		nrf_adc_configure((nrf_adc_config_t *)  &(adc_default_instances[peripheral_index]->nrf_adc_config)); 
		nrf_adc_input_select(adc_default_instances[peripheral_index]->nrf_adc_config_input);
	}
	
	
	// Reset the ADC Operation
	adc_operations[peripheral_index] = ADC_NO_OPERATION;
	
	
	
	return NRF_SUCCESS;
}

ret_code_t adc_read_raw_default(const adc_instance_t* adc_instance, int32_t* raw) {
	
	
	uint8_t peripheral_index = adc_instance->adc_peripheral;
	
	if(adc_instance->adc_instance_id != adc_default_instances[peripheral_index]->adc_instance_id)
		return NRF_ERROR_INVALID_PARAM;
	
	// Check if there is an operation ongoing on this peripheral
	if(adc_operations[peripheral_index] != ADC_NO_OPERATION){
		return NRF_ERROR_BUSY;
	}
	
	// Set the operation
	adc_operations[peripheral_index] = ADC_READING_OPERATION;

	// Do the ADC conversion on the configured channel
	nrf_adc_start();
	while(!nrf_adc_conversion_finished());
	*raw = nrf_adc_result_get();
	nrf_adc_conversion_event_clean();
    nrf_adc_stop();
	
	
	// Reset the ADC Operation
	adc_operations[peripheral_index] = ADC_NO_OPERATION;
	
	return NRF_SUCCESS;
	
}

ret_code_t adc_read_voltage(const adc_instance_t* adc_instance, float* voltage, float ref_voltage) {
	int32_t raw = 0;
	
	ret_code_t ret = adc_read_raw(adc_instance, &raw);
	
	if(ret != NRF_SUCCESS) {
		return ret; 
	}
	
	nrf_adc_config_scaling_t		scaling		= (adc_instance->nrf_adc_config).scaling;
	nrf_adc_config_resolution_t		resolution 	= (adc_instance->nrf_adc_config).resolution;
	
	// Compute the actual value of resolution and scaling based on the ADC-Channel setting
	uint32_t resolution_as_number = (resolution == NRF_ADC_CONFIG_RES_8BIT) ? 256 : ((resolution == NRF_ADC_CONFIG_RES_9BIT)? 512 : 1024 );
    float scaling_as_number = (scaling == NRF_ADC_CONFIG_SCALING_INPUT_TWO_THIRDS || scaling == NRF_ADC_CONFIG_SCALING_SUPPLY_TWO_THIRDS) ? 0.66666666 : ((scaling == NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD|| scaling == NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD) ? 0.33333333 : 1);
    
	// Compute the voltage in mV
	*voltage = (raw * (ref_voltage / ((float) resolution_as_number)))/scaling_as_number;
	
	return NRF_SUCCESS;
}





