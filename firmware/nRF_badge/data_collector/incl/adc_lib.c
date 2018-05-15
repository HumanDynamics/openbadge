#include "adc_lib.h"


// TODO: 
// - document
// - input checking
// - do the parameterstuff (NRF_ADC_CONFIG_INPUT_DISABLED, ) as own definition?!

// - remove
extern void pprintf(const char* format, ...);




#define ADC_PERIPHERAL_NUMBER 		ADC_COUNT

static volatile adc_operation_t  	adc_operations[ADC_PERIPHERAL_NUMBER] 	= {0};
static adc_instance_t *				adc_instances[ADC_PERIPHERAL_NUMBER] 	= {0};
static uint32_t 					adc_instance_number = 1; 	// Starts at 1 because the above init of the arrays are always to 0 (otherwise the check of which instance is currently working would fail)



ret_code_t adc_init(adc_instance_t* adc_instance) {	

	// Check if the peripheral selected peripheral exists!
	if(adc_instance->adc_peripheral == 0) {
		#if ADC_ENABLED
		#else		
		return NRF_ERROR_INVALID_PARAM;
		#endif
	} else {
		return NRF_ERROR_INVALID_PARAM;
	}

	adc_instance->adc_instance_id 		= adc_instance_number;
	adc_instance_number++;
	
	return NRF_SUCCESS;
}



ret_code_t adc_read_raw(adc_instance_t* adc_instance, int32_t* raw){
	
	// Check if there is an operation ongoing on this peripheral
	if(adc_operations[adc_instance->adc_peripheral] != ADC_NO_OPERATION){
		return NRF_ERROR_BUSY;
	}
	
	// Set the operation
	adc_operations[adc_instance->adc_peripheral] = ADC_READING_OPERATION;
	
	// Check if it is my instance that was working before?
	if((*adc_instances[adc_instance->adc_peripheral]).adc_instance_id != adc_instance->adc_instance_id){
		
		// Set the instance to the current instance!
		adc_instances[adc_instance->adc_peripheral] = adc_instance;
		
		// If not, we have to reconfigure the instance/adc_config!
		nrf_adc_configure((nrf_adc_config_t *)  &(adc_instance->nrf_adc_config)); 
		
		
		// Only configure the input, if we have to
		nrf_adc_input_select(adc_instance->nrf_adc_config_input);
	}
		

	
	nrf_adc_start();
	while(!nrf_adc_conversion_finished());
	*raw = nrf_adc_result_get();
	nrf_adc_conversion_event_clean();
    nrf_adc_stop();
	
	adc_operations[adc_instance->adc_peripheral] = ADC_NO_OPERATION;
	
	return NRF_SUCCESS;
	
}

ret_code_t adc_read_voltage(adc_instance_t* adc_instance, float* voltage, float ref_voltage) {
	int32_t raw = 0;
	
	ret_code_t ret = adc_read_raw(adc_instance, &raw);
	
	if(ret != NRF_SUCCESS) {
		return ret; 
	}
	
	nrf_adc_config_scaling_t		scaling		= (adc_instance->nrf_adc_config).scaling;
	nrf_adc_config_resolution_t		resolution 	= (adc_instance->nrf_adc_config).resolution;
	
	// Compute the resolution and scaling based on the ADC-Channel setting
	uint32_t resolution_as_number = (resolution == NRF_ADC_CONFIG_RES_8BIT) ? 256 : ((resolution == NRF_ADC_CONFIG_RES_9BIT)? 512 : 1024 );
    float scaling_as_number = (scaling == NRF_ADC_CONFIG_SCALING_INPUT_TWO_THIRDS || scaling == NRF_ADC_CONFIG_SCALING_SUPPLY_TWO_THIRDS) ? 0.66666666 : ((scaling == NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD|| scaling == NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD) ? 0.33333333 : 1);
    
	
	*voltage = (raw * (ref_voltage / ((float) resolution_as_number)))/scaling_as_number;
	
	return NRF_SUCCESS;
}


/**
* You can not switch of only one instance, but the whole peripheral, that is responsible for this instance.
*
*/
void adc_power_off(uint8_t adc_peripheral){
	nrf_adc_input_select(NRF_ADC_CONFIG_INPUT_DISABLED);
}


void adc_power_on(uint8_t adc_peripheral){
	// Check if the adc_instance is valid?
	if((*adc_instances[adc_peripheral]).adc_instance_id > 0){
		nrf_adc_configure((nrf_adc_config_t *)  &((*adc_instances[adc_peripheral]).nrf_adc_config)); 
		nrf_adc_input_select((*adc_instances[adc_peripheral]).nrf_adc_config_input);
	}
}





