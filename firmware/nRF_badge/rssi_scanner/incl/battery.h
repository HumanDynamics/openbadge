//
// Created by Andrew Bartow on 12/14/16.
//

#ifndef OPENBADGE_BATTERY_H
#define OPENBADGE_BATTERY_H

/*
 * Initializes the battery monitoring service.
 *
 * Depends on ADC already being initialized.
 */
void BatteryMonitor_init(void);

/*
 * Returns the current supply voltage of the battery. Should be used in place of getBatteryVoltage in analog.c
 *
 * Note: this value is a running average across several battery readings.
 */
float BatteryMonitor_getBatteryVoltage(void);

#endif //OPENBADGE_BATTERY_H
