#include "spi_lib.h"



// Sources: Multiple slaves?! https://devzone.nordicsemi.com/f/nordic-q-a/11056/nfr51822-spi-multi_slave-conrtol
// 			Control the SS Signal/Pin outside the functions of the SPI-library!!


#include "nrf_gpio.h"
#include "sdk_config.h"

#define SPI_PERIPHERAL_NUMBER 		SPI_COUNT	/**< Number of activated spi peripherals in sdk_config.h. */






static volatile spi_operation_t  	spi_operations[SPI_PERIPHERAL_NUMBER] 	= {0};		/**< Array to save the current spi_operations (needed to check if there is an ongoing spi operation on the peripheral) */
static const spi_instance_t *		spi_instances[SPI_PERIPHERAL_NUMBER] 	= {NULL};	/**< Array of pointers to the current spi_instances (needed to deactivate the slave select pin in the interrupt handler of the peripheral after transmission) */
static uint32_t 					spi_instance_number = 1; 							/**< spi_instance_number starts at 1 not 0 because all entries in the spi_instances-arrays are 0. So the check for the spi_instance_id-element may not work correctly. */ 

static spi_handler_t				spi_handlers_transmit_bkgnd			[SPI_PERIPHERAL_NUMBER];	/**< Array to save the application handlers for the transmit operation */
static spi_handler_t				spi_handlers_transmit_receive_bkgnd	[SPI_PERIPHERAL_NUMBER];	/**< Array to save the application handlers for the transmit receive operation */




/**@brief Functions for handling the spi interrupts internally.
 *
 * @details These handlers (for each peripheral there is an own handler) deactivate the ss-pin and call the
 * 			specified application handlers (if they are not NULL). 
 */
#if SPI0_ENABLED
static void spi_0_event_handler(nrf_drv_spi_evt_t* p_event) {
	// The transfer no matter if send/receive or anything else!
	if(p_event->type == NRF_DRV_SPI_EVENT_DONE) {
		
		spi_evt_t evt;
		evt.type = SPI_TRANSFER_DONE;
		
		// The SS-Pin has to be "deacitvated" again (here we assume, that spi_instances[0] is not NULL!)
		nrf_gpio_pin_set((*spi_instances[0]).nrf_drv_spi_config.ss_pin);
		
		// We are now ready with the operation
		spi_operations[0] = SPI_NO_OPERATION;
		
		if(spi_handlers_transmit_bkgnd[0] != NULL) {
			spi_handlers_transmit_bkgnd[0](&evt);
		}
		if(spi_handlers_transmit_receive_bkgnd[0] != NULL) {
			spi_handlers_transmit_bkgnd[0](&evt);
		}
	}
}
#endif

#if SPI1_ENABLED
static void spi_1_event_handler(nrf_drv_spi_evt_t* p_event) {
	// The transfer no matter if send/receive or anything else!
	if(p_event->type == NRF_DRV_SPI_EVENT_DONE) {
		
		spi_evt_t evt;
		evt.type = SPI_TRANSFER_DONE;
		
		// The SS-Pin has to be "deacitvated" again! (here we assume, that spi_instances[0] is not NULL!)
		nrf_gpio_pin_set((*spi_instances[1]).nrf_drv_spi_config.ss_pin);
		
		spi_operations[1] = SPI_NO_OPERATION;
		
		if(spi_handlers_transmit_bkgnd[1] != NULL) {
			spi_handlers_transmit_bkgnd[1](&evt);
		}
		if(spi_handlers_transmit_receive_bkgnd[1] != NULL) {
			spi_handlers_transmit_bkgnd[1](&evt);
		}
	}
}
#endif



ret_code_t spi_init(spi_instance_t* spi_instance) {	
	
	
	// Small hack of handling the SS-Pin manually:
	// local copy of the ss_pin
	uint8_t ss_pin = (spi_instance->nrf_drv_spi_config).ss_pin;
	
	// Set the ss_pin to unused, so that the nrf_drv_spi-library doesn't use/set this pin!
	(spi_instance->nrf_drv_spi_config).ss_pin = NRF_DRV_SPI_PIN_NOT_USED;
	
	ret_code_t ret = NRF_SUCCESS;
	
	
	if(spi_instance->spi_peripheral == 0) {		
		
		#if SPI0_ENABLED
		spi_instance->nrf_drv_spi_instance.p_registers 	= NRF_SPI0; 
		spi_instance->nrf_drv_spi_instance.irq 			= SPI0_IRQ;
		spi_instance->nrf_drv_spi_instance.drv_inst_idx	= SPI0_INSTANCE_INDEX;
		spi_instance->nrf_drv_spi_instance.use_easy_dma	= SPI0_USE_EASY_DMA;
		
		ret = nrf_drv_spi_init(&(spi_instance->nrf_drv_spi_instance), &(spi_instance->nrf_drv_spi_config), (nrf_drv_spi_handler_t) spi_0_event_handler);
	
		#else
			
		ret = NRF_ERROR_INVALID_PARAM;
		
		#endif
		
	} else if(spi_instance->spi_peripheral == 1){
		
		#if SPI1_ENABLED
		spi_instance->nrf_drv_spi_instance.p_registers 	= NRF_SPI1; 
		spi_instance->nrf_drv_spi_instance.irq 			= SPI1_IRQ;
		spi_instance->nrf_drv_spi_instance.drv_inst_idx	= SPI1_INSTANCE_INDEX;
		spi_instance->nrf_drv_spi_instance.use_easy_dma = SPI1_USE_EASY_DMA;
		
		ret = nrf_drv_spi_init(&(spi_instance->nrf_drv_spi_instance), &(spi_instance->nrf_drv_spi_config), (nrf_drv_spi_handler_t) spi_1_event_handler);
		
		#else
		
		ret = NRF_ERROR_INVALID_PARAM;
		
		#endif
	} 	else {
		ret = NRF_ERROR_INVALID_PARAM;		
	}
	
	
	// Reset the pin again to the former value, so that the functions of this module can use it for manually set/reset the SS-Pin!
	(spi_instance->nrf_drv_spi_config).ss_pin = ss_pin;	
	
	
	// ret could be: NRF_SUCCESS, NRF_ERROR_INVALID_STATE, NRF_ERROR_BUSY and NRF_INVALID_PARAM
	// if it is NRF_ERROR_INVALID_STATE or NRF_ERROR_BUSY the specified peripheral has already been initialized, but this is ok.
	if(ret == NRF_ERROR_INVALID_PARAM) { 
		return ret;
	}
	
	// manually set the slave-select pin.
	nrf_gpio_pin_set(ss_pin);
    nrf_gpio_cfg_output(ss_pin);
	
	
	spi_instance->spi_instance_id 						= spi_instance_number;
	
	spi_instance_number++;
	
	return NRF_SUCCESS;
}



ret_code_t spi_transmit_bkgnd(const spi_instance_t* spi_instance, spi_handler_t spi_handler, const uint8_t* tx_data, uint32_t tx_data_len) {
	
	
	// Check if there is already an operation working on this peripheral, if so return (because it should not/could not do read and write parallel)
	if(spi_operations[spi_instance->spi_peripheral] != SPI_NO_OPERATION) {
		return NRF_ERROR_BUSY;
	}
	
	// Set that we are now sending!
	spi_operations[spi_instance->spi_peripheral] = SPI_TRANSMIT_OPERATION;
	
	// Set the instance-array entry to the current instance, to retrieve the ss_pin in the IRQ-handler!
	spi_instances[spi_instance->spi_peripheral] = spi_instance;
	
	// Set the send Handler for the send operation, set the other one to NULL!
	spi_handlers_transmit_bkgnd				[spi_instance->spi_peripheral] = spi_handler;
	spi_handlers_transmit_receive_bkgnd	[spi_instance->spi_peripheral] = NULL;
	
	static uint8_t tmp; // Dummy for RX 
	
	// "activate" the SS-PIN
	nrf_gpio_pin_clear((spi_instance->nrf_drv_spi_config).ss_pin);
	
	ret_code_t ret = nrf_drv_spi_transfer(&(spi_instance->nrf_drv_spi_instance), tx_data, tx_data_len, &tmp, 0);	
	
	// ret could be: NRF_SUCCESS, NRF_ERROR_BUSY and NRF_ERROR_INVALID_ADDR
	
	// If there is no success, "deactivate" the SS-PIN and clear the SPI operation 
	if(ret != NRF_SUCCESS) {
		nrf_gpio_pin_set((spi_instance->nrf_drv_spi_config).ss_pin);
		spi_operations[spi_instance->spi_peripheral] = SPI_NO_OPERATION;
	}
	
	return ret;
}


ret_code_t spi_transmit(const spi_instance_t* spi_instance, const uint8_t* tx_data, uint32_t tx_data_len) {

	ret_code_t ret = spi_transmit_bkgnd(spi_instance, NULL, tx_data, tx_data_len);
	if(ret != NRF_SUCCESS) {
		return ret;
	}
	
	
	// Waiting until the SPI operation has finished!
	while(spi_get_operation(spi_instance) != SPI_NO_OPERATION);
	
	return NRF_SUCCESS;
}


ret_code_t spi_transmit_receive_bkgnd(const spi_instance_t* spi_instance, spi_handler_t spi_handler, const uint8_t* tx_data, uint32_t tx_data_len, uint8_t* rx_data, uint32_t rx_data_len) {
	
	
	// Check if there is already an operation working on this peripheral, if so return (because it should not/could not do read and write parallel)
	if(spi_operations[spi_instance->spi_peripheral] != SPI_NO_OPERATION) {
		return NRF_ERROR_BUSY;
	}
	
	// Set that we are now sending!
	spi_operations[spi_instance->spi_peripheral] = SPI_TRANSMIT_RECEIVE_OPERATION;
	
	// Set the instance-array entry to the current instance, to retrieve the ss_pin in the IRQ-handler!
	spi_instances[spi_instance->spi_peripheral] = spi_instance;
	
	// Set the send Handler for the send operation!
	spi_handlers_transmit_bkgnd			[spi_instance->spi_peripheral] = NULL;
	spi_handlers_transmit_receive_bkgnd	[spi_instance->spi_peripheral] = spi_handler;
	
	
	// "activate" the SS-PIN
	nrf_gpio_pin_clear((spi_instance->nrf_drv_spi_config).ss_pin);
	
	ret_code_t ret = nrf_drv_spi_transfer(&(spi_instance->nrf_drv_spi_instance), tx_data, tx_data_len, rx_data, rx_data_len);	
	
	// ret could be: NRF_SUCCESS, NRF_ERROR_BUSY and NRF_ERROR_INVALID_ADDR
	
	// If there is no success, "deactivate" the SS-PIN and clear the SPI operation 
	if(ret != NRF_SUCCESS) {
		nrf_gpio_pin_set((spi_instance->nrf_drv_spi_config).ss_pin);
		spi_operations[spi_instance->spi_peripheral] = SPI_NO_OPERATION;
	}
	
	return ret;
}

ret_code_t spi_transmit_receive(const spi_instance_t* spi_instance, const uint8_t* tx_data, uint32_t tx_data_len, uint8_t* rx_data, uint32_t rx_data_len) {

	ret_code_t ret = spi_transmit_receive_bkgnd(spi_instance, NULL, tx_data, tx_data_len, rx_data, rx_data_len);
	if(ret != NRF_SUCCESS) {
		return ret;
	}
	
	
	// Waiting until the SPI operation has finished!
	while(spi_get_operation(spi_instance) != SPI_NO_OPERATION);
	
	return NRF_SUCCESS;
}


// Actually this could/should be the peripheral ID?!
spi_operation_t spi_get_operation(const spi_instance_t* spi_instance){
	return spi_operations[spi_instance->spi_peripheral];
}





