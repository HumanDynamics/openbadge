#include "battery_lib.h"

#include "adc_lib.h"
#include "systick_lib.h" // Needed for battery_selftest()
#include "app_timer.h"
#include "app_scheduler.h"
#include "advertiser_lib.h"
#include "app_util_platform.h"
#include "debug_lib.h"

#define BATTERY_PERIOD_MS							10000
#define BATTERY_REFERENCE_VOLTAGE					1.2f 	/**< The internal reference voltage (NRF_ADC_CONFIG_REF_VBG) */
#define BATTERY_SAMPLES_PER_AVERAGE					5
#define BATTERY_SELFTEST_NUM_VOLTAGE_MEASUREMENTS	10		/**< Number of measurements for the selftest */

static adc_instance_t adc_instance;
static volatile float average_voltage = 0;

APP_TIMER_DEF(internal_battery_timer);
static void internal_battery_callback(void* p_context);
static void update_advertiser_voltage(void * p_event_data, uint16_t event_size);

ret_code_t battery_init(void) {
	adc_instance.adc_peripheral = 0;
	adc_instance.nrf_adc_config.resolution 	= NRF_ADC_CONFIG_RES_10BIT;
	adc_instance.nrf_adc_config.scaling		= NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD;
	adc_instance.nrf_adc_config.reference	= NRF_ADC_CONFIG_REF_VBG;
	adc_instance.nrf_adc_config_input		= NRF_ADC_CONFIG_INPUT_DISABLED;
	
	ret_code_t ret = adc_init(&adc_instance, 0);
	if(ret != NRF_SUCCESS) return ret;
	
	// create a timer for periodic battery measurement
	ret = app_timer_create(&internal_battery_timer, APP_TIMER_MODE_REPEATED, internal_battery_callback);
	if(ret != NRF_SUCCESS) return ret;
	
	// Now start the timer
	ret = app_timer_start(internal_battery_timer, APP_TIMER_TICKS(BATTERY_PERIOD_MS, 0), NULL);
	if(ret != NRF_SUCCESS) return ret;	
	
	return NRF_SUCCESS;
}

float battery_get_voltage(void) {
	float tmp = 0;
	CRITICAL_REGION_ENTER();
	tmp = average_voltage;
	CRITICAL_REGION_EXIT();
	return tmp;
}

/**@brief Function to read the current supply voltage in Volts. Internally the voltage is averaged.
 *
 * @param[out]	voltage		Read supply voltage in Volts.
 *
 * @retval 	NRF_SUCCESS		On success.
 * @retval	NRF_ERROR_BUSY	If the ADC-interface is busy.
 */
static ret_code_t battery_read_voltage(float* voltage) {
	ret_code_t ret = adc_read_voltage(&adc_instance, voltage, BATTERY_REFERENCE_VOLTAGE);
	if(ret != NRF_SUCCESS) return ret;
	
	
	static uint8_t first_read = 1;
	if(first_read) {
		first_read = 0;
		CRITICAL_REGION_ENTER();
		average_voltage = *voltage;
		CRITICAL_REGION_EXIT();
		return NRF_SUCCESS;
	}
	
	
	CRITICAL_REGION_ENTER();
	average_voltage -= average_voltage * (1.f / (float) BATTERY_SAMPLES_PER_AVERAGE);
	average_voltage += (*voltage) * (1.f / (float) BATTERY_SAMPLES_PER_AVERAGE);
	*voltage = average_voltage;
	CRITICAL_REGION_EXIT();
	
	debug_log("BATTERY: Average battery voltage: %f\n", average_voltage);
	
	
	return NRF_SUCCESS;
}


static void internal_battery_callback(void* p_context) {
	float voltage = 0;
	ret_code_t ret = battery_read_voltage(&voltage);
	if(ret != NRF_SUCCESS) return;
	
	// Here we want to update the advertising data, but this should be done in main-context not in timer context.
	app_sched_event_put(NULL, 0, update_advertiser_voltage);
}

static void update_advertiser_voltage(void * p_event_data, uint16_t event_size) {
	float tmp = 0;
	CRITICAL_REGION_ENTER();
	tmp = average_voltage;
	CRITICAL_REGION_EXIT();
	// Update the advertising data battery-voltage
	advertiser_set_battery_voltage(tmp);
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