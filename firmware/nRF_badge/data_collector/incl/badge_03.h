/**
 * Hardware configuration for Badge 03 (square blue badges)
 * Badges implement external SPI-connected flash
 *   and a Sparkfun MEMS mic module, on separate 1.8V regulated supply (fed to AREF01)
 *   and 2 LEDs
 */


#ifndef BADGE_03_H
#define BADGE_03_H

// LEDs definitions for badge_03
#define LEDS_NUMBER    2

#define LED_START      10
#define LED_1          10
#define LED_2          11
#define LED_STOP       11

#define LEDS_LIST { LED_1, LED_2}

#define GREEN_LED LED_1
#define RED_LED LED_2



#define BUTTONS_NUMBER 0



#define RX_PIN_NUMBER  17
#define TX_PIN_NUMBER  18
#define CTS_PIN_NUMBER 0
#define RTS_PIN_NUMBER 0
#define HWFC           false



#define SPI_MASTER_0_ENABLE 1
#define SPIM0_MISO_PIN  3    // SPI MISO signal. 
#define SPIM0_SS_PIN   4    // SPI slave select signal. 
#define SPIM0_MOSI_PIN  1    // SPI MOSI signal. 
#define SPIM0_SCK_PIN   2    // SPI clock signal. 


#endif // BADGE_03_H
