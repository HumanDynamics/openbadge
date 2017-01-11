#ifndef _NRF_DELAY_H
#define _NRF_DELAY_H

#include "nrf.h"

void nrf_delay_us(uint32_t volatile number_of_us);

void nrf_delay_ms(uint32_t volatile number_of_ms);

#endif
