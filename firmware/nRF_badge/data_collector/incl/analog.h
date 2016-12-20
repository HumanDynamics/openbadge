//sdfsdf

#ifndef ANALOG_H
#define ANALOG_H

#include "nrf_drv_config.h"
#include "nrf_soc.h"
//#include "nrf_drv_rtc.h"
//#include "app_error.h"
#include "nrf_adc.h"

#include "ble_setup.h"
#include "debug_log.h"
#include "rtc_timing.h"

// Should be defined in custom board files.  (default values here in case, e.g., programming NRFDK board)
#if !(defined(MIC_PIN) && defined(MIC_AREF))
    #define MIC_PIN ADC_CONFIG_PSEL_AnalogInput6  //GPIO P05
    #define MIC_AREF NRF_ADC_CONFIG_REF_EXT_REF1  //GPIO P06
#endif


//default: 10bit res, full scale, external VCC_MIC reference
#define ANALOG_CONFIG_MIC   { NRF_ADC_CONFIG_RES_8BIT,                      \
                                NRF_ADC_CONFIG_SCALING_INPUT_FULL_SCALE,     \
                                MIC_AREF }
                               
// for measuring supply voltage:
#define ANALOG_CONFIG_VBAT  {NRF_ADC_CONFIG_RES_10BIT,                      \
                                NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD,    \
                                NRF_ADC_CONFIG_REF_VBG }

//Configure ADC to read analog input pins, with MIC_VCC AREF
void adc_config(void);

/**
 * Read an analog input
 * Alters ADC configuration, reads battery voltage, then returns to default (analog input pin) configuration
 */
__attribute__((long_call, section(".data"))) int analogRead(nrf_adc_config_input_t input);

/**
 * Reads the battery voltage (VDD) using the ADC.
 * This requires a special ADC configuration so it cannot be done with analogRead().
 * Note for most usages, BatteryMonitor_getBatteryVoltage is perfered, as that number is a running average and is
 *   much more stable.
 *
 * @return the battery voltage (VDD) as a float
 */
float readVDD(void);

#endif //TIMING_H