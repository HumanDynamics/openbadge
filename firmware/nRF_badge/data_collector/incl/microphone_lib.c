#include "microphone_lib.h"

#include "custom_board.h"		// Needed for the Pin configuration
#include "adc_lib.h"
#include "debug_lib.h"
#include "systick_lib.h" // Needed for microphone_selftest()


#define ABS(x) (((x) >= 0)? (x) : -(x))
#define MICROPHONE_ZERO_OFFSET								125		/**< The zero noise mmicrophone offset to compensate */
#define MICROPHONE_SELFTEST_TIME_FOR_NOISE_GENERATION_MS	10000	/**< The time to wait for noise generation, for selftest */
#define MICROPHONE_SELFTEST_THRESHOLD						30		/**< The threshold that has to be exceeded to pass the selftest */

static adc_instance_t adc_instance;

void microphone_init(void) {
	
	adc_instance.adc_peripheral = 0;
	adc_instance.nrf_adc_config.resolution 	= NRF_ADC_CONFIG_RES_8BIT;
	adc_instance.nrf_adc_config.scaling		= NRF_ADC_CONFIG_SCALING_INPUT_FULL_SCALE;
	adc_instance.nrf_adc_config.reference	= MIC_AREF; // NRF_ADC_CONFIG_REF_EXT_REF1;
	adc_instance.nrf_adc_config_input		= MIC_PIN;	//NRF_ADC_CONFIG_INPUT_6; //ADC_CONFIG_PSEL_AnalogInput6;
	
	
	adc_init(&adc_instance);
}

ret_code_t microphone_read(uint8_t* value) {
	int32_t raw;
	ret_code_t ret = adc_read_raw(&adc_instance, &raw);
	*value = (uint8_t) ABS((raw - MICROPHONE_ZERO_OFFSET));
	return ret;
}

/*
bool microphone_selftest(void) {
	debug_log("Waiting for noise for: %u ms.\n", MICROPHONE_SELFTEST_TIME_FOR_NOISE_GENERATION_MS);
	uint32_t end_ms = systick_get_continuous_millis() + MICROPHONE_SELFTEST_TIME_FOR_NOISE_GENERATION_MS;
	int16_t min = 0xFF;
	int16_t max = 0;
	int16_t diff = 0;
	
	
	while(systick_get_continuous_millis() < end_ms) {
		systick_delay_millis(2);	
		uint8_t value = 0;		
		ret_code_t ret = microphone_read(&value);
		if(ret != NRF_SUCCESS)
			return 0;
			
		if(value < min)
			min = value;
		if(value > max)
			max = value;
		
		diff = ABS(max-min);
		if(diff > MICROPHONE_SELFTEST_THRESHOLD)
			break;
	}
	debug_log("Selftest microphone data diff: %u\n", diff);
	if(diff > MICROPHONE_SELFTEST_THRESHOLD)
		return 1;
	
	return 0;
	
}*/

/**@brief Function that returns the average micorphone value over ~50ms
 *
 * @retval The average microphone value.
 */
 
static uint8_t get_avg_value(void) {
	uint32_t avg_value = 0;
	uint32_t avg_counter = 0;
	for(uint8_t i = 0; i < 25; i++) {
		systick_delay_millis(2);	
		uint8_t value = 0;		
		ret_code_t ret = microphone_read(&value);
		if(ret != NRF_SUCCESS)
			return 0;
		avg_value += value;	
		avg_counter++;
	}
	avg_value /= avg_counter;
	avg_value = avg_value > 255 ? 255 : avg_value;
	return (uint8_t) avg_value;
}

bool microphone_selftest(void) {
	debug_log("Waiting for noise pattern for: %u ms.\n", MICROPHONE_SELFTEST_TIME_FOR_NOISE_GENERATION_MS);
	uint32_t end_ms = systick_get_continuous_millis() + MICROPHONE_SELFTEST_TIME_FOR_NOISE_GENERATION_MS;
	
	// We need to search for a pattern: Noise, no noise, noise. Or no noise, noise, no noise
	
	
	int16_t cur_reference_value = (int16_t) get_avg_value();	// First read a reference value
	int8_t search_status = 0; // 0: nothing found yet, 1: found no noise -> noise transition, -1: found noise -> no noise transition
	debug_log("Microphone ref value: %u\n", cur_reference_value);
	
	while(systick_get_continuous_millis() < end_ms) {
		int16_t cur_value = (int16_t) get_avg_value();
		
		if(cur_value > cur_reference_value + MICROPHONE_SELFTEST_THRESHOLD) {	// no noise -> noise transition
			if(search_status == 0) {
				cur_reference_value = cur_value;
				search_status = 1;
				debug_log("Found noise\n");
			} else if(search_status == -1) {	// If we have already found a noise -> no noise transition, we can return
				debug_log("Found noise -> no noise -> noise transition\n");
				return 1;
			}
			
		} else if(cur_value + MICROPHONE_SELFTEST_THRESHOLD < cur_reference_value) {	// noise -> no noise transition
			if(search_status == 0) {
				cur_reference_value = cur_value;
				search_status = -1;
				debug_log("Found no noise\n");
			} else if(search_status == 1) {	// If we have already found a no noise -> noise transition, we can return
				debug_log("Found no noise -> noise -> no noise transition\n");
				return 1;
			}
		}		
	}
	
	return 0;
	
}
