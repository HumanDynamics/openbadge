#ifndef __SPI_LIB_H
#define __SPI_LIB_H

#include "sdk_config.h"
#include "nrf_drv_spi.h"
#include "nrf_gpio.h"

// Could be retrieved from ADC_COUNT of nrf51822_peripherals..

/*
// Has to be an increasing number < ADC_PERIPHERAL, because used as array-indizes
typedef enum {
	SPI0 = 0,
	SPI1 = 1,
} spi_peripheral_t;
*/


typedef enum {
	SPI_NO_OPERATION 			= 0,
	SPI_SEND_OPERATION 			= (1 << 0),	
	SPI_SEND_RECEIVE_OPERATION 	= (1 << 1),	
} spi_operation_t;



typedef enum
{
    SPI_TRANSFER_DONE, ///< Transfer done.
} spi_evt_type_t;

typedef struct
{
    spi_evt_type_t  type;      ///< Event type.
} spi_evt_t;


/**
 * @brief SPI master driver event handler type.
 */
typedef void (*spi_handler_t)(spi_evt_t const * p_event);




typedef struct {
	int32_t 				spi_instance_id;		// Setted by the Init-function!
	nrf_drv_spi_t			nrf_drv_spi_instance;	// Setted by the Init-function!
	
	
	uint8_t			 		spi_peripheral;			// Needed to check whether the same peripheral is already in use!
	nrf_drv_spi_config_t	nrf_drv_spi_config;	
} spi_instance_t;


ret_code_t spi_init(spi_instance_t* spi_instance);

ret_code_t spi_send_IT(const spi_instance_t* spi_instance, spi_handler_t spi_handler, const uint8_t* tx_data, uint32_t tx_data_len);


ret_code_t spi_send(const spi_instance_t* spi_instance, uint8_t* tx_data, uint32_t tx_data_len);



#endif
