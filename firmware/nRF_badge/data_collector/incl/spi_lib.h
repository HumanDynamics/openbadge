#ifndef __SPI_LIB_H
#define __SPI_LIB_H



/** @file
 *
 * @brief SPI abstraction library.
 *
 * @details It enables to call the SPI peripherals from different contexts. 
 *			If the selected SPI peripheral is currently in use, it will inform the other context by returning NRF_ERROR_BUSY.
 *			Each spi peripheral is specified by the values of nrf_drv_spi_config except the ss_pin. The ss_pin could be set independently for each spi instance.
 *			So it is important that every spi instance that uses the same spi peripheral has the same configuration except the ss_pin.
 *
 *			There are actually two main capabilities: transmit and transmit_receive. The second can be used to receive data from the slave while transmitting.
 */
 

#include "sdk_common.h"	// Needed for the definition of ret_code_t and the error-codes
#include "nrf_drv_spi.h"



/**@brief The different SPI operations. These operations will be used to set the peripheral busy or not. */
typedef enum {
	SPI_NO_OPERATION 				= 0,			/**< Currently no spi operation ongoing. */
	SPI_TRANSMIT_OPERATION 			= (1 << 0),		/**< Currently there is a spi transmit operation ongoing. */
	SPI_TRANSMIT_RECEIVE_OPERATION 	= (1 << 1),		/**< Currently there is a spi transmit receive operation ongoing. */
} spi_operation_t;


/**@brief  SPI driver event types, passed to the handler routine provided by the bkgnd-functions. */
typedef enum
{
    SPI_TRANSFER_DONE, /**< Transfer done */
} spi_evt_type_t;

typedef struct
{
    spi_evt_type_t  type;      /**< Event type */
} spi_evt_t;

/**
 * @brief SPI event handler type.
 */
typedef void (*spi_handler_t)(spi_evt_t const * p_event);



/**@example	Example of spi_instance_t 
 *
 *
 * 			spi_instance_t spi_instance;														// Create an spi_instance-struct.
 *
 *			spi_instance.spi_peripheral 					= 0;								// Set the spi peripheral to 0 (the selected peripheral has to be enabled in sdk_config.h).		
 *			spi_instance.nrf_drv_spi_config.frequency 		= NRF_DRV_SPI_FREQ_8M; 				// Set the spi frequency. NRF_DRV_SPI_FREQ_8M from nrf_drv_spi.h.
 *			spi_instance.nrf_drv_spi_config.bit_order 		= NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;	// Set the spi bit order. NRF_DRV_SPI_BIT_ORDER_MSB_FIRST from nrf_drv_spi.h.
 *			spi_instance.nrf_drv_spi_config.mode			= NRF_DRV_SPI_MODE_3;				// Set the spi mode. NRF_DRV_SPI_MODE_3 (means SCK active low, sample on trailing edge of clock) from nrf_drv_spi.h.
 *			spi_instance.nrf_drv_spi_config.orc				= 0;								// Seth the spi overrun-character.
 *			spi_instance.nrf_drv_spi_config.irq_priority	= APP_IRQ_PRIORITY_LOW;				// The interrupt priotity of the spi peripheral. APP_IRQ_PRIORITY_LOW from app_util_platform.h.
 *			spi_instance.nrf_drv_spi_config.ss_pin 			= 2;								// Set the slave-select pin.
 *			spi_instance.nrf_drv_spi_config.miso_pin		= 1;								// Set the miso pin.
 *			spi_instance.nrf_drv_spi_config.mosi_pin		= 4;								// Set the mosi pin.
 *			spi_instance.nrf_drv_spi_config.sck_pin			= 3;								// Set the sck pin.
 *
 *			spi_init(&spi_instance);															// Initialize the spi_instance.
 */
 
 
/**@brief SPI instance type. */
typedef struct {	
	uint8_t			 		spi_peripheral;			/**< Set to the desired spi peripheral. The Peripheral has to be enabled in the sdk_config.h file */
	nrf_drv_spi_config_t	nrf_drv_spi_config;		/**< Set the SPI configuration (possible parameters in nrf_drv_spi.h)  */	
	uint32_t 				spi_instance_id;		/**< Instance index: Setted by the init-function (do not set!) */
	nrf_drv_spi_t			nrf_drv_spi_instance;	/**< The initialized low level spi instance: Setted by the init-function (do not set!) */
} spi_instance_t;


/**@brief   Function for initializing an instance for the spi peripheral.
 *
 * @details This functions actually checks if the specified spi_peripheral exists and sets the spi_instance_id of spi_instance.
 *			The application must set the spi_peripheral to a peripheral index that is activated in sdk_config.h.
 *			Furthermore, this function handles the slave-select pin initialization manually (because the nrf_drv_spi-library can't handle multiple slave select pins on the same peripheral).
 *			 
 *
 * @param[in,out]   spi_instance		Pointer to an preconfigured spi_instance.
 *
 * @retval  NRF_SUCCESS    				If the adc_instance was successfully initialized.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified peripheral or the configuration is not correct.
 */
ret_code_t spi_init(spi_instance_t* spi_instance);


/**@brief   Function for transmitting data in asynchronous/non-blocking/background mode.
 *
 * @details This is a non-blocking function. If there is already an ongoing spi operation this function returns NRF_ERROR_BUSY.
 * 			If the operation was started successfully and terminates, the provided spi_handler is called with event: SPI_TRANSFER_DONE.
 *	
 * @warning The transmit data must be kept in memory until the operation has terminated.
 *
 * @param[in]   spi_instance	Pointer to an initialized spi instance. 	   	
 * @param[in]   spi_handler		Handler function that should be called if the operation is done, with event: SPI_TRANSFER_DONE. Could also be NULL if no handler should be called.
 * @param[in]   tx_data         Pointer to the data to transmit.
 * @param[in]   tx_data_len    	Length of the data to transmit.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If there is already an ongoing operation (transmit or transmit receive).
 * @retval 	NRF_ERROR_INVALID_ADDR    	If the provided buffers are not placed in the Data RAM region.
 */
ret_code_t spi_transmit_bkgnd(const spi_instance_t* spi_instance, spi_handler_t spi_handler, const uint8_t* tx_data, uint32_t tx_data_len);


/**@brief   Function for transmitting data in blocking mode.
 *
 * @details Function uses internally spi_transmit_bkgnd() for transmitting the data
 *			and the spi_get_operation() to wait until the operation has terminated.
 *
 * @param[in]   spi_instance	Pointer to an initialized spi instance.
 * @param[in]   tx_data         Pointer to the data to transmit.
 * @param[in]   tx_data_len    	Length of the data to transmit.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If there is already an ongoing operation (transmit or transmit receive).
 * @retval 	NRF_ERROR_INVALID_ADDR    	If the provided buffers are not placed in the Data RAM region.
 */
ret_code_t spi_transmit(const spi_instance_t* spi_instance, const uint8_t* tx_data, uint32_t tx_data_len);


/**@brief   Function for transmitting and receiving data in asynchronous/non-blocking/background mode.
 *
 * @details This is a non-blocking function. If there is already an ongoing spi operation this function returns NRF_ERROR_BUSY.
 * 			If the operation was started successfully and terminates, the provided spi_handler is called with event: SPI_TRANSFER_DONE.
 *	
 * @warning The transmit and receive data must be kept in memory until the operation has terminated.
 *
 * @param[in]   spi_instance	Pointer to an initialized spi instance. 	   	
 * @param[in]   spi_handler		Handler function that should be called if the operation is done, with event: SPI_TRANSFER_DONE. Could also be NULL if no handler should be called.
 * @param[in]   tx_data         Pointer to the data to transmit.
 * @param[in]   tx_data_len    	Length of the data to transmit.
 * @param[in]   rx_data         Pointer to the receive buffer.
 * @param[in]   rx_data_len    	Length of the receive buffer.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If there is already an ongoing operation (transmit or transmit receive).
 * @retval 	NRF_ERROR_INVALID_ADDR    	If the provided buffers are not placed in the Data RAM region.
 */
ret_code_t spi_transmit_receive_bkgnd(const spi_instance_t* spi_instance, spi_handler_t spi_handler, const uint8_t* tx_data, uint32_t tx_data_len, uint8_t* rx_data, uint32_t rx_data_len);



/**@brief   Function for transmitting and receiving data in blocking mode.
 *
 * @details Function uses internally spi_transmit_receive_bkgnd() for transmitting the data
 *			and the spi_get_operation() to wait until the operation has terminated.
 *	
 * @warning The transmit and receive data must be kept in memory until the operation has terminated.
 *
 * @param[in]   spi_instance	Pointer to an initialized spi instance.
 * @param[in]   tx_data         Pointer to the data to transmit.
 * @param[in]   tx_data_len    	Length of the data to transmit.
 * @param[in]   rx_data         Pointer to the receive buffer.
 * @param[in]   rx_data_len    	Length of the receive buffer.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If there is already an ongoing operation (transmit or transmit receive).
 * @retval 	NRF_ERROR_INVALID_ADDR    	If the provided buffers are not placed in the Data RAM region.
 */
ret_code_t spi_transmit_receive(const spi_instance_t* spi_instance, const uint8_t* tx_data, uint32_t tx_data_len, uint8_t* rx_data, uint32_t rx_data_len);


/**@brief   Function for retrieving the current spi status/operation.
 *
 * @details This function returns the current spi_operation_t. 
 *			The application can check the status through this function, 
 *			to decide whether the spi operation is done.
 *
 * @retval SPI_NO_OPERATION 				If there is currently no spi operation in process for the peripheral in spi_instance.
 * @retval SPI_TRANSMIT_OPERATION			If there is currently a spi transmit operation in process for the peripheral in spi_instance.
 * @retval SPI_TRANSMIT_RECEIVE_OPERATION	If there is currently a spi transmit receive operation in process for the peripheral in spi_instance.		    	
 */
spi_operation_t spi_get_operation(const spi_instance_t* spi_instance);


#endif
