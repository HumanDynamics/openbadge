/**
 * Hardware configuration for Badge 03v6 (tall narrow badges, with lanyard clip hole)
 * Badges implement:
 *   external SPI-connected flash
 *   a MEMS mic plus amplifier circuit, on separate 1.8V regulated supply (fed to AREF01)
 *   2 LEDs
 *   i2c RTC module
 *   Accel LIS3DH
 *   Note that SS and MISO  pin have shuffled (in 03v2 they were pins 1 and 2 respectively)
 */

#ifndef BADGE_03V6
#define BADGE_03V6

#define GREEN_LED LED_1
#define RED_LED LED_2

#define LEDS_NUMBER                 2

#define LED_1                       8
#define LED_2                       9

#define RX_PIN_NUMBER               11
#define TX_PIN_NUMBER               10
#define CTS_PIN_NUMBER              0
#define RTS_PIN_NUMBER              0
#define HWFC                        false

#define SPI_MASTER_0_ENABLE         1
#define SPIM0_MISO_PIN              1    // SPI MISO signal.
#define SPIM0_SS_PIN                0    // SPI CSN signal.
#define SPIM0_MOSI_PIN              4    // SPI MOSI signal.
#define SPIM0_SCK_PIN               3    // SPI SCK signal.
#define INT_PIN_INPUT               25    // SPI SCK signal.

#define MIC_PIN ADC_CONFIG_PSEL_AnalogInput6  //GPIO P05
#define MIC_AREF NRF_ADC_CONFIG_REF_EXT_REF1  //GPIO P06

#define EXIST_ACCL                        true    // board has accelerator?

#endif // BADGE_03V6
