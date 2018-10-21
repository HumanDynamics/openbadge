#include "microphone_lib.h"

#include "custom_board.h"		// Needed for the Pin configuration
#include "adc_lib.h"
#include "debug_lib.h"
#include "systick_lib.h" // Needed for microphone_selftest()
#include "nrf_gpio.h"

#define ABS(x) (((x) >= 0)? (x) : -(x))
#define MICROPHONE_ZERO_OFFSET								125		/**< The zero noise mmicrophone offset to compensate */
#define MICROPHONE_SELFTEST_TIME_FOR_NOISE_GENERATION_MS	10000	/**< The time to wait for noise generation, for selftest */
#define MICROPHONE_SELFTEST_THRESHOLD						20		/**< The threshold that has to be exceeded to pass the selftest */
#define MICROPHONE_SELFTEST_SECTION_SIZE_MS					200		/**< The section size in ms */
#define MICROPHONE_SELFTEST_THRESHOLD_PERCENTAGE			60		/**<  	If this percentage of reads is above the threshold in one section, there is noise. 
																			The same for no noise. Should be > 50 percentage */

static adc_instance_t adc_instance;

void microphone_init(void) {
	
	adc_instance.adc_peripheral = 0;
	adc_instance.nrf_adc_config.resolution 	= NRF_ADC_CONFIG_RES_8BIT;
	adc_instance.nrf_adc_config.scaling		= NRF_ADC_CONFIG_SCALING_INPUT_FULL_SCALE;
	adc_instance.nrf_adc_config.reference	= MIC_AREF; // NRF_ADC_CONFIG_REF_EXT_REF1;
	adc_instance.nrf_adc_config_input		= MIC_PIN;	//NRF_ADC_CONFIG_INPUT_6; //ADC_CONFIG_PSEL_AnalogInput6;
	
	
	adc_init(&adc_instance, 1);
}

ret_code_t microphone_read(uint8_t* value) {
	int32_t raw;
	ret_code_t ret = adc_read_raw(&adc_instance, &raw);
	*value = (uint8_t) ABS((raw - MICROPHONE_ZERO_OFFSET));
	return ret;
}


/**@brief Function that returns the average micorphone value over ~50ms.
 *
 * @retval The average microphone value.
 */
 
static uint8_t get_avg_value(void) {
	uint32_t avg_value = 0;
	uint32_t avg_counter = 0;
	for(uint8_t i = 0; i < 50; i++) {
		systick_delay_millis(1);	
		uint8_t value = 0;		
		ret_code_t ret = microphone_read(&value);
		if(ret != NRF_SUCCESS)
			return 0;
		avg_value += value;	
		avg_counter++;
	}
	avg_value /= (avg_counter/2);
	avg_value = avg_value > 255 ? 255 : avg_value;
	return (uint8_t) avg_value;
}



/**@brief Function that checks the sections for the sections_pattern.
 *
 * @param[in]	sections			The array of sections.
 * @param[in]	size				Number of elements in sections array.
 * @param[in]	sections_pattern	The pattern of sections.
 * @param[in]	pattern_size		Number of elements in sections pattern array.
 *
 * @retval 1 if section pattern was found.
 * @retval 0 otherwise.
 */
static uint8_t check_sections_pattern(const int8_t* sections, uint32_t size, const int8_t* sections_pattern, uint32_t pattern_size) {
	
	if(pattern_size > size) 
		return 0;
	
	
	uint32_t pattern_index = 0;
	
	for(uint32_t i = 0; i < size; i++) {
		if(sections[i] == sections_pattern[pattern_index]) {
			pattern_index++;
			if(pattern_index >= pattern_size)
				return 1;
		}
	}
	return 0;
}

/**@brief Function that checks one section (if there is noise or not).
 *
 * @param[in]	above_threshold_counter			Number of reads above the threshold.
 * @param[in]	below_threshold_counter			Number of reads below the threshold.
 *
 * @retval 1 	section has noise.
 * @retval 0 	section has no noise.
 * @retval -1 	section is undefined.
 */
static int8_t check_section(uint32_t above_threshold_counter, uint32_t below_threshold_counter) {
	int8_t ret = -1;
	uint32_t counter_sum = above_threshold_counter + below_threshold_counter;
	if(counter_sum == 0)
		return -1;
	
	if(((above_threshold_counter * 100) / counter_sum) >= MICROPHONE_SELFTEST_THRESHOLD_PERCENTAGE) {
		ret = 1;
	} else if(((below_threshold_counter * 100) / counter_sum) >= MICROPHONE_SELFTEST_THRESHOLD_PERCENTAGE) {
		ret = 0;
	}
	return ret;
}

bool microphone_selftest(void) {
	
	// The pattern to search for.
	int8_t sections_pattern[4] = {0, 1, 0, 1};
	
	
	//systick_delay_millis(500);
	debug_log("MICROPHONE: Waiting for noise pattern for: %u ms.\n", MICROPHONE_SELFTEST_TIME_FOR_NOISE_GENERATION_MS);
	uint64_t end_ms = systick_get_continuous_millis() + MICROPHONE_SELFTEST_TIME_FOR_NOISE_GENERATION_MS;
	
	
	uint32_t number_of_sections = MICROPHONE_SELFTEST_TIME_FOR_NOISE_GENERATION_MS/MICROPHONE_SELFTEST_SECTION_SIZE_MS;
	
	uint64_t section_end_ms = systick_get_continuous_millis() + MICROPHONE_SELFTEST_SECTION_SIZE_MS;
	int8_t sections[number_of_sections];
	uint32_t section_above_threshold_counter = 0;
	uint32_t section_below_threshold_counter = 0;
	
	
	uint32_t current_section = 0;
	
	while(systick_get_continuous_millis() < end_ms) {
		// Check if we need to close the current section?
		if(systick_get_continuous_millis() >= section_end_ms) {
			
			sections[current_section] = check_section(section_above_threshold_counter, section_below_threshold_counter);
			
			// Check the sections:
			if(check_sections_pattern(sections, current_section + 1, sections_pattern, sizeof(sections_pattern))) {
				debug_log("MICROPHONE: Section check successful!\n");
				return 1;
			}

			// Increment the sections
			current_section = (current_section + 1) < number_of_sections ? (current_section + 1) : current_section;
			sections[current_section] = 0;
			section_above_threshold_counter = 0;
			section_below_threshold_counter = 0;
			
			section_end_ms = systick_get_continuous_millis() + MICROPHONE_SELFTEST_SECTION_SIZE_MS;
		} 
		
		uint8_t cur_value = get_avg_value();
		
		// Display the microphone value
		if(cur_value >= MICROPHONE_SELFTEST_THRESHOLD) {
			//nrf_gpio_pin_write(GREEN_LED, LED_ON);  
			nrf_gpio_pin_write(RED_LED, LED_OFF);  
			nrf_gpio_pin_write(GREEN_LED, LED_OFF);  
			section_above_threshold_counter++;
		} else {
			//nrf_gpio_pin_write(GREEN_LED, LED_OFF);  
			nrf_gpio_pin_write(RED_LED, LED_ON);  
			nrf_gpio_pin_write(GREEN_LED, LED_ON);  
			section_below_threshold_counter++;
		}
	
	}
	
	return 0;
	
}
