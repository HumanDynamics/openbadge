#ifndef ACCEL_H
#define ACCEL_H

#define SPIM0_SS_PIN_2              2    // accelerometer
#define LIS2DH_WHO_AM_I_REGISTER 		0x0F
#define LIS2DH_WHO_AM_I_VALUE       0x33
#define CLICKTHRESHHOLD             20   //Threshhold for tap accel
#define LIS2DH_REG_CLICKSRC         0x39
#define LIS2DH_REG_CTRL1            0x20
#define LIS2DH_REG_CTRL2            0x21
#define LIS2DH_REG_CTRL3            0x22
#define LIS2DH_REG_CTRL4            0x23
#define LIS2DH_REG_CTRL5            0x24
#define LIS2DH_REG_TEMPCFG          0x1F
#define LIS2DH_REG_CLICKCFG         0x38
#define LIS2DH_REG_CLICKTHS         0x3A
#define LIS2DH_REG_TIMELIMIT        0x3B
#define LIS2DH_REG_TIMELATENCY      0x3C
#define LIS2DH_REG_TIMEWINDOW       0x3D
#define LIS2DH_REG_ACTTHS           0x3E
#define LIS2DH_REG_ACTDUR           0x3F
#define INT1_THS                    0x32
#define INT1_DURATION               0x00
#define INT1_CFG                    0x30

void accel_set_int_motion();
void acc_self_test();
void setDataRate();
void accel_test();
void tap_accel();
void motion_interruput();
void accel_spi_init();
void accel_init();
void accel_set_int_tap();

#endif //ACCEL_H
