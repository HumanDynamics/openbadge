
#include "analog.h"

static float readBattery()
{
    // Configure ADC for measuring supply voltage
    nrf_adc_config_t nrf_adc_config_bat = ANALOG_CONFIG_VBAT;
    nrf_adc_configure( (nrf_adc_config_t *)&nrf_adc_config_bat);

    nrf_adc_start();  //start conversion
    while(!nrf_adc_conversion_finished());  //wait till conversion complete.
    int reading = nrf_adc_result_get();  //get reading
    //reset adc
    nrf_adc_conversion_event_clean();
    nrf_adc_stop();


    nrf_adc_config_t nrf_adc_config_mic = ANALOG_CONFIG_MIC;
    nrf_adc_configure( (nrf_adc_config_t *)&nrf_adc_config_mic);

    return reading * (3.6 / 1023.0); // convert value to voltage
}


// ADC initialization.
void adc_config(void)
{
    //default: 10bit res, 1/3 prescalar, ext VCC_MIC reference
    const nrf_adc_config_t nrf_adc_config = ANALOG_CONFIG_MIC;

    // Initialize and configure ADC
    nrf_adc_configure( (nrf_adc_config_t *)&nrf_adc_config);
    nrf_adc_input_select(NRF_ADC_CONFIG_INPUT_DISABLED);
    
    currentBatteryVoltage = readBattery();
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

float getBatteryVoltage()
{
    return currentBatteryVoltage;
}

float getRealBatteryVoltage()
{
    return readBattery();
}

void updateBatteryVoltage() 
{
    if(millis() - lastBatteryUpdate >= MIN_BATTERY_READ_INTERVAL)
    {
        currentBatteryVoltage = readBattery();
        debug_log("Read battery: %d.\r\n",(int)(1000.0*currentBatteryVoltage));
        lastBatteryUpdate = millis();
    }
}

