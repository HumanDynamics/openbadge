
#include "analog.h"


// ADC initialization.
void adc_config(void)
{
    //default: 10bit res, 1/3 prescalar, 1.2V internal reference
    const nrf_adc_config_t nrf_adc_config = {NRF_ADC_CONFIG_RES_10BIT,
                                             NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD,
                                             MIC_AREF};

    // Initialize and configure ADC
    nrf_adc_configure( (nrf_adc_config_t *)&nrf_adc_config);
    nrf_adc_input_select(NRF_ADC_CONFIG_INPUT_DISABLED);
}

// read an analog input
int analogRead(nrf_adc_config_input_t input)  
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


float readBattery() 
{
    nrf_adc_config_t nrf_adc_config = {NRF_ADC_CONFIG_RES_10BIT,
                                       NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD,
                                       NRF_ADC_CONFIG_REF_VBG};
    // Configure ADC for measuring supply voltage
    nrf_adc_configure( (nrf_adc_config_t *)&nrf_adc_config);
    
    nrf_adc_start();  //start conversion
    while(!nrf_adc_conversion_finished());  //wait till conversion complete.
    int reading = nrf_adc_result_get();  //get reading
    //reset adc
    nrf_adc_conversion_event_clean();
    nrf_adc_stop();
    

    nrf_adc_config.scaling = NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD;  //reset to regular ADC input, full scale
    nrf_adc_config.reference = MIC_AREF;            //reset to external AREF (2V mic VCC)*/
    nrf_adc_configure( (nrf_adc_config_t *)&nrf_adc_config);
    
    float batteryVoltage = reading * (3.6 / 1023.0); // convert value to voltage
    return batteryVoltage;
}