#include "adc_lib.h"


// TODO: 
// - document
// - input checking
// - error codes
// - do the parameterstuff (NRF_ADC_CONFIG_INPUT_DISABLED, ) as own definition?!

// - remove
extern void pprintf(const char* format, ...);

#define ADC_PERIPHERAL_NUMBER 		ADC_COUNT

static volatile adc_operation_t  	adc_operations[ADC_PERIPHERAL_NUMBER] 	= {0};
static adc_instance_t				adc_instances[ADC_PERIPHERAL_NUMBER] 	= {{0}};
static uint32_t 					adc_instance_number = 1; 	// Starts at 1 because the above init of the arrays are always to 0



void adc_init(adc_instance_t* adc_instance) {	
	adc_instance->adc_instance_id 		= adc_instance_number;
	adc_instance_number++;
}



void adc_read_raw(const adc_instance_t* adc_instance, int32_t* raw){
	
	if(adc_operations[adc_instance->adc_peripheral] != ADC_NO_OPERATION){
		pprintf("ADC Busy!\n");
		// TODO: return error code BUSY!
		return;
	}
	
	adc_operations[adc_instance->adc_peripheral] = ADC_READING_OPERATION;
	
	// Check if it is my instance that was working before?
	if(adc_instances[adc_instance->adc_peripheral].adc_instance_id != adc_instance->adc_instance_id){
		pprintf("Switch/Set ADC Instance: %d\n", adc_instance->adc_instance_id);
		// Set the instance to my own instance!
		adc_instances[adc_instance->adc_peripheral] = *adc_instance;
		
		// If not, we have to reconfigure the instance/adc_config!
		nrf_adc_configure((nrf_adc_config_t *)  &(adc_instance->nrf_adc_config)); 
		
		
		// TODO: Only configure the input if we have to, or each time the function is called, but this one activates the ADC (so probably for sleeping it would be good?)
		nrf_adc_input_select(adc_instance->nrf_adc_config_input);
	}
	
	// TODO: Only configure the input if we have to, or each time the function is called, but this one activates the ADC (so probably for sleeping it would be good?)
	//nrf_adc_input_select(adc_instance->nrf_adc_config_input);
	
	
	nrf_adc_start();
	while(!nrf_adc_conversion_finished());
	*raw = nrf_adc_result_get();
	nrf_adc_conversion_event_clean();
    nrf_adc_stop();
	
	adc_operations[adc_instance->adc_peripheral] = ADC_NO_OPERATION;
	
	// TODO: return SUCCESS
	
}

void adc_read_voltage(const adc_instance_t* adc_instance, float* voltage, float ref_voltage) {
	int32_t raw = 0;
	// TODO: check error code
	adc_read_raw(adc_instance, &raw);
	
	nrf_adc_config_scaling_t		scaling		= (adc_instance->nrf_adc_config).scaling;
	nrf_adc_config_resolution_t		resolution 	= (adc_instance->nrf_adc_config).resolution;
	
	uint32_t resolution_as_number = (resolution == NRF_ADC_CONFIG_RES_8BIT) ? 256 : ((resolution == NRF_ADC_CONFIG_RES_9BIT)? 512 : 1024 );
    float scaling_as_number = (scaling == NRF_ADC_CONFIG_SCALING_INPUT_TWO_THIRDS || scaling == NRF_ADC_CONFIG_SCALING_SUPPLY_TWO_THIRDS) ? 0.66666666 : ((scaling == NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD|| scaling == NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD) ? 0.33333333 : 1);
    
	
	*voltage = (raw * (ref_voltage / ((float) resolution_as_number)))/scaling_as_number;
	
	// TODO: return SUCCESS
}


/**
* You can not switch of only one instance, but the whole peripheral, that is responsible for this instance.
*
*/
void adc_power_off(uint8_t adc_peripheral){
	nrf_adc_input_select(NRF_ADC_CONFIG_INPUT_DISABLED);
}


void adc_power_on(uint8_t adc_peripheral){
	// Check if we have the adc_instance is valid?
	if(adc_instances[adc_peripheral].adc_instance_id > 0){
		nrf_adc_configure((nrf_adc_config_t *)  &(adc_instances[adc_peripheral].nrf_adc_config)); 
		nrf_adc_input_select(adc_instances[adc_peripheral].nrf_adc_config_input);
	}
}





