//sdfsdf

#ifndef ANALOG_H
#define ANALOG_H

#include "nrf_drv_config.h"
#include "nrf_soc.h"
//#include "nrf_drv_rtc.h"
//#include "app_error.h"
#include "nrf_adc.h"

#include "debug_log.h"

#if defined(BOARD_BADGE_03)
  #define MIC_PIN ADC_CONFIG_PSEL_AnalogInput6  //GPIO P05
  #define MIC_AREF NRF_ADC_CONFIG_REF_EXT_REF0  //GPIO P00
  
#elif defined(BOARD_BADGE_03V2_RIGADO)
  #define MIC_PIN ADC_CONFIG_PSEL_AnalogInput6  //GPIO P05
  #define MIC_AREF NRF_ADC_CONFIG_REF_EXT_REF1  //GPIO P06
  
#elif defined(BOARD_BADGE_03V2_DYNASTREAM)
  #define MIC_PIN ADC_CONFIG_PSEL_AnalogInput2  //GPIO P01
  #define MIC_AREF NRF_ADC_CONFIG_REF_EXT_REF0  //GPIO P00
  
#else    // NRF51DK, or an unspecified board
  #define MIC_PIN ADC_CONFIG_PSEL_AnalogInput6  //GPIO P05
  #define MIC_AREF NRF_ADC_CONFIG_REF_EXT_REF0  //GPIO P00
  
#endif



//Configure ADC to read analog input pins, with MIC_VCC AREF
void adc_config(void);

/**
 * Read an analog input
 * Alters ADC configuration, reads battery voltage, then returns to default (analog input pin) configuration
 */
int analogRead(nrf_adc_config_input_t input);

// Read battery voltage
float readBattery();




#endif //TIMING_H