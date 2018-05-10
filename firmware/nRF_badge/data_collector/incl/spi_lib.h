#ifndef __SPI_LIB_H
#define __SPI_LIB_H

#include "sdk_config.h"
#include "nrf_drv_spi.h"
#include "nrf_gpio.h"



typedef enum {
	SPI_NO_OPERATION 			= 0,
	SPI_SEND_OPERATION 			= (1 << 0),	
	SPI_SEND_RECEIVE_OPERATION 	= (1 << 1),	
} spi_operation_t;



typedef enum
{
    SPI_TRANSFER_DONE, /**< Transfer done */
} spi_evt_type_t;

typedef struct
{
    spi_evt_type_t  type;      /**< Event type */
} spi_evt_t;


/**
 * @brief SPI master driver event handler type.
 */
typedef void (*spi_handler_t)(spi_evt_t const * p_event);




typedef struct {	
	uint8_t			 		spi_peripheral;			/**< Set to the desired spi peripheral. The Peripheral has to be enabled in the sdk_config.h file */
	nrf_drv_spi_config_t	nrf_drv_spi_config;		/**< Set the SPI configuration (possible parameters in nrf_drv_spi.h)  */	
	int32_t 				spi_instance_id;		/**< Instance index: Setted by the init-function (do not set!) */
	nrf_drv_spi_t			nrf_drv_spi_instance;	/**< The initialized low level spi instance: Setted by the init-function (do not set!) */
} spi_instance_t;


ret_code_t spi_init(spi_instance_t* spi_instance);

ret_code_t spi_send_IT(const spi_instance_t* spi_instance, spi_handler_t spi_handler, const uint8_t* tx_data, uint32_t tx_data_len);

ret_code_t spi_send(const spi_instance_t* spi_instance, const uint8_t* tx_data, uint32_t tx_data_len);

ret_code_t spi_send_receive_IT(const spi_instance_t* spi_instance, spi_handler_t spi_handler, const uint8_t* tx_data, uint32_t tx_data_len, uint8_t* rx_data, uint32_t rx_data_len);

ret_code_t spi_send_receive(const spi_instance_t* spi_instance, const uint8_t* tx_data, uint32_t tx_data_len, uint8_t* rx_data, uint32_t rx_data_len);

spi_operation_t spi_get_status(const spi_instance_t* spi_instance);

#endif
