
#include "analog.h"

#define V_REF_BATTERY             3.6f
#define BATTERY_READ_RESOLUTION   1024 //Set to 2^10 because ANALOG_CONFIG_VBAT configures ADC to 10-bit resolution.

float readVDD(void)
{
    // Configure ADC for measuring supply voltage
    nrf_adc_config_t nrf_adc_config_bat = ANALOG_CONFIG_VBAT;
    nrf_adc_configure(&nrf_adc_config_bat);

    // INPUT_DISABLED as we configure the CONFIG.INPSEL mux to use VDD in our struct above, so we don't need
    //   an input for the CONFIG.PSEL mux here. (See Figure 71 in NRF51 Reference Guide, Page 165)
    int32_t reading = analogRead(NRF_ADC_CONFIG_INPUT_DISABLED);

    nrf_adc_config_t nrf_adc_config_mic = ANALOG_CONFIG_MIC;
    nrf_adc_configure(&nrf_adc_config_mic);

    return reading * (V_REF_BATTERY / ((float) BATTERY_READ_RESOLUTION));
}

// ADC initialization.
void adc_config(void)  
{
    //default: 10bit res, 1/3 prescalar, ext VCC_MIC reference
    const nrf_adc_config_t nrf_adc_config = ANALOG_CONFIG_MIC;

    // Initialize and configure ADC
    nrf_adc_configure( (nrf_adc_config_t *)&nrf_adc_config);
    nrf_adc_input_select(NRF_ADC_CONFIG_INPUT_DISABLED);
    
}

// read an analog input
__attribute__((long_call, section(".data"))) int analogRead(nrf_adc_config_input_t input)
{
    nrf_adc_input_select(input);
    nrf_adc_start();  //start conversion
    while(!nrf_adc_conversion_finished());  //wait till conversion complete.
    int reading = nrf_adc_result_get();  //get reading
    //reset
    nrf_adc_conversion_event_clean();
    nrf_adc_stop();
    return reading;
}

void IdentifyLED_On(int time_seconds) {
    nrf_gpio_pin_write(LED_2, LED_ON);
}