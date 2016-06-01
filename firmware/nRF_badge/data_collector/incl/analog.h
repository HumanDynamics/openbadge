//sdfsdf

#ifndef ANALOG_H
#define ANALOG_H

#include "nrf_drv_config.h"
#include "nrf_soc.h"
//#include "nrf_drv_rtc.h"
//#include "app_error.h"
#include "nrf_adc.h"

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

float currentBatteryVoltage;
unsigned long lastBatteryUpdate;
#define MIN_BATTERY_READ_INTERVAL 100000UL  // minimum time between supply analogReads.  We don't need to do this often.
#define MAX_BATTERY_READ_INTERVAL 200000UL  // time after lastBatteryUpdate to consider currentBatteryVoltage invalid



//Configure ADC to read analog input pins, with MIC_VCC AREF
void adc_config(void);

/**
 * Read an analog input
 * Alters ADC configuration, reads battery voltage, then returns to default (analog input pin) configuration
 */
int analogRead(nrf_adc_config_input_t input);

// Read battery voltage
//float readBattery();

/**
 * Get buffered battery voltage.  If buffered value is outdated, do an analogRead of battery.
 *   updateBatteryVoltage must be called periodically to keep this result current.  (ideally when radio is inactive)
 */
float getBatteryVoltage();

/**
 * If enough time has passed since the last battery voltage read, do another reading.
 */
void updateBatteryVoltage();

/**
 * Get actual battery voltage (performs analogRead)
 *   Use with caution - may return inaccurate results if performed during a spike in power usage
 */
float getBatteryVoltage();



#endif //TIMING_H