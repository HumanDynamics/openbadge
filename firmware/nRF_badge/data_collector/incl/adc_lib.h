#ifndef __ADC_LIB_H
#define __ADC_LIB_H

//#include <stdio.h>
//#include <stdlib.h>
//#include <stdbool.h>
//#include <stdint.h>


#include "nrf_adc.h"



typedef enum {
	ADC_NO_OPERATION 		= 0,
	ADC_READING_OPERATION 	= (1 << 0),	
} adc_operation_t;


typedef struct {
	int32_t 				adc_instance_id;		// Setted by the Init-function!
	uint8_t			 		adc_peripheral;			// Needed to check whether the same peripheral is already in use!
	nrf_adc_config_t 		nrf_adc_config;
	nrf_adc_config_input_t 	nrf_adc_config_input;
} adc_instance_t;


void adc_init(adc_instance_t* adc_instance);

void adc_read_raw(const adc_instance_t* adc_instance, int32_t* raw);

void adc_read_voltage(const adc_instance_t* adc_instance, float* voltage, float ref_voltage);


#endif
