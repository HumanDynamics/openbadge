/**
 * Hardware configuration for Badge 03v2 (tall narrow blue badges), Dynastream module populated
 * Badges implement:
 *   external SPI-connected flash
 *   a MEMS mic plus amplifier circuit, on separate 1.8V regulated supply (fed to AREF01)
 *   2 LEDs
 *   tact switch
 */


#ifndef BADGE_03V2_DYNASTREAM_H
#define BADGE_03V2_DYNASTREAM_H

// LEDs definitions for badge_03
#define LEDS_NUMBER    2

#define LED_1          2
#define LED_2          31

#define GREEN_LED LED_1
#define RED_LED LED_2



#define BUTTONS_NUMBER 1

#define BUTTON_1 12



#define RX_PIN_NUMBER  24
#define TX_PIN_NUMBER  30
#define CTS_PIN_NUMBER 0
#define RTS_PIN_NUMBER 0
#define HWFC           false


#define SPI_MASTER_0_ENABLE 1
#define SPIM0_MISO_PIN  8    // SPI MISO signal. 
#define SPIM0_SS_PIN   9    // SPI CSN signal. 
#define SPIM0_MOSI_PIN  3    // SPI MOSI signal. 
#define SPIM0_SCK_PIN   5    // SPI SCK signal. 


#define MIC_PIN ADC_CONFIG_PSEL_AnalogInput2  //GPIO P01
#define MIC_AREF NRF_ADC_CONFIG_REF_EXT_REF0  //GPIO P00


#endif // BADGE_03V2_DYNASTREAM_H
