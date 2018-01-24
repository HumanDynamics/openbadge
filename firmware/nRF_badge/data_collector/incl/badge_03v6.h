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

#define SPIM0_SS_PIN_2              2    // accelerometer
#define LIS2DH_WHO_AM_I_REGISTER 		0x0F
#define LIS2DH_WHO_AM_I_VALUE       0x33
#define CLICKTHRESHHOLD             20   //Threshhold for tap accel
#define LIS3DH_REG_CLICKSRC         0x39
#define LIS3DH_REG_CTRL1            0x20
#define LIS3DH_REG_CTRL2            0x21
#define LIS3DH_REG_CTRL3            0x22
#define LIS3DH_REG_CTRL4            0x23
#define LIS3DH_REG_CTRL5            0x24
#define LIS3DH_REG_TEMPCFG          0x1F
#define LIS3DH_REG_CLICKCFG         0x38
#define LIS3DH_REG_CLICKTHS         0x3A
#define LIS3DH_REG_TIMELIMIT        0x3B
#define LIS3DH_REG_TIMELATENCY      0x3C
#define LIS3DH_REG_TIMEWINDOW       0x3D
#define LIS3DH_REG_ACTTHS           0x3E
#define LIS3DH_REG_ACTDUR           0x3F
#define INT1_THS                    0x32
#define INT1_DURATION               0x00
#define INT1_CFG                    0x30

#define MIC_PIN ADC_CONFIG_PSEL_AnalogInput6  //GPIO P05
#define MIC_AREF NRF_ADC_CONFIG_REF_EXT_REF1  //GPIO P06

#endif // BADGE_03V6
