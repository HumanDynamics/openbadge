#include "battery_lib.h"

#include "adc_lib.h"
#include "systick_lib.h" // Needed for battery_selftest()

#define BATTERY_REFERENCE_VOLTAGE					1.2f 	/**< The internal reference voltage (NRF_ADC_CONFIG_REF_VBG) */
#define BATTERY_SAMPLES_PER_AVERAGE					5
#define BATTERY_SELFTEST_NUM_VOLTAGE_MEASUREMENTS	10		/**< Number of measurements for the selftest */

static adc_instance_t adc_instance;
static float average_voltage = 0;

void battery_init(void) {
	adc_instance.adc_peripheral = 0;
	adc_instance.nrf_adc_config.resolution 	= NRF_ADC_CONFIG_RES_10BIT;
	adc_instance.nrf_adc_config.scaling		= NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD;
	adc_instance.nrf_adc_config.reference	= NRF_ADC_CONFIG_REF_VBG;
	adc_instance.nrf_adc_config_input		= NRF_ADC_CONFIG_INPUT_DISABLED;
	
	adc_init(&adc_instance);
}

ret_code_t battery_read_voltage(float* voltage) {
	ret_code_t ret = adc_read_voltage(&adc_instance, voltage, BATTERY_REFERENCE_VOLTAGE);
	if(ret != NRF_SUCCESS) return ret;
	
	static uint8_t first_read = 1;
	if(first_read) {
		first_read = 0;
		average_voltage = *voltage;
		return NRF_SUCCESS;
	}
	
	average_voltage -= average_voltage * (1.f / (float) BATTERY_SAMPLES_PER_AVERAGE);
	average_voltage += (*voltage) * (1.f / (float) BATTERY_SAMPLES_PER_AVERAGE);
	*voltage = average_voltage;
	return NRF_SUCCESS;
}



bool battery_selftest(void) {
	
	ret_code_t ret;
	for(uint32_t i = 0; i < BATTERY_SELFTEST_NUM_VOLTAGE_MEASUREMENTS; i++) {
		float voltage;
		ret = battery_read_voltage(&voltage);
		if(ret != NRF_SUCCESS)
			return 0;
		if(voltage > 4.0 || voltage < 1.0)
			return 0;
		systick_delay_millis(10);
	}
	return 1;
}