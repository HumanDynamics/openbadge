/**
 * Hardware configuration for Badge 03v4 (tall narrow badges, with lanyard clip hole)
 * Badges implement:
 *   external SPI-connected flash
 *   a MEMS mic plus amplifier circuit, on separate 1.8V regulated supply (fed to AREF01)
 *   2 LEDs
 *   tact switch
 *   i2c RTC module
 *
 *   Note that SS, MISO, and the button pin have shuffled (in 03v2 they were pins 1, 2, and 0 respectively)
 */


#ifndef BADGE_03V4
#define BADGE_03V4


#define GREEN_LED LED_1
#define RED_LED LED_2

#define LEDS_NUMBER                	2

#define LED_1                      	8
#define LED_2                       9

#define UART_RX_PIN               	11
#define UART_TX_PIN               	10
#define UART_CTS_PIN				0
#define UART_RTS_PIN				0
#define HWFC_ENABLED           		0


#define SPIM0_MISO_PIN              1    // SPI MISO signal.
#define SPIM0_MOSI_PIN              4    // SPI MOSI signal.
#define SPIM0_SCK_PIN               3    // SPI SCK signal.
#define SPIM0_ACCELEROMETER_SS_PIN  2    // SPI CSN signal for accelerometer.
#define SPIM0_EEPROM_SS_PIN  		0    // SPI CSN signal for eeprom.

#define MIC_PIN 					ADC_CONFIG_PSEL_AnalogInput6  //GPIO P05
#define MIC_AREF 					NRF_ADC_CONFIG_REF_EXT_REF1  //GPIO P06



#endif // BADGE_03V4
