#ifndef __UART_LIB_H
#define __UART_LIB_H


#include <stdarg.h>		// Needed for the printf-function
#include <stdio.h>		// Needed for the vsnprintf-function
#include <string.h>		// Needed for the printf-function
#include "nrf_drv_uart.h"






/**
* It is so annoying that the nrf_drv_uart-library don't accepts UART TX transfers with size > 256!!
* Therefore, a wrapper function with IRQ handling and so on has to be written!

*/



typedef enum {
	UART_NO_OPERATION 					= 0,
	UART_TRANSMIT_OPERATION 			= (1 << 0),	
	UART_RECEIVE_OPERATION 				= (1 << 1),
	UART_RECEIVE_BUFFER_OPERATION		= (1 << 2),
} uart_operation_t;






typedef struct 
{
	uint8_t * 	rx_buf;      /**< Pointer to the RX buffer. */
    uint32_t  	rx_buf_size; /**< Size of the RX buffer. */
	volatile uint32_t 	rx_buf_read_index;
	volatile uint32_t 	rx_buf_write_index;
    uint8_t * 	tx_buf;      /**< Pointer to the TX buffer. */
    uint32_t  	tx_buf_size; /**< Size of the TX buffer. */
} uart_buffer_t;



typedef enum
{
    UART_TRANSMIT_DONE, 	/**< Transmit done */
	UART_RECEIVE_DONE, 		/**< Receive done */
	UART_DATA_AVAILABLE, 	/**< Data available */
	UART_ERROR,				/**< Communication error */
} uart_evt_type_t;

typedef struct
{
	uart_evt_type_t  type;      /**< Event type */
} uart_evt_t;





/**
 * @brief UART event handler type.
 */
typedef void (*uart_handler_t)(uart_evt_t const * p_event);



typedef struct {	
	uint8_t			 		uart_peripheral;		/**< Set to the desired uart peripheral. The Peripheral has to be enabled in the sdk_config.h file */
	nrf_drv_uart_config_t	nrf_drv_uart_config;	/**< Set the uart configuration (possible parameters in nrf_drv_uart.h)  */	
	uint32_t 				uart_instance_id;		/**< Instance index: Setted by the init-function (do not set!) */
	uart_buffer_t			uart_buffer;			/**< The uart buffer used for printf and receive_buffer: If you want to use the buffer functionality, use the corresponding init MACRO. Setted by the init-function (do not set!) */	
	nrf_drv_uart_t			nrf_drv_uart_instance;	/**< The initialized low level uart instance: Setted by the init-function (do not set!) */
} uart_instance_t;






#define UART_INIT(P_UART_INSTANCE, P_RET_CODE) 	\
    do                                                                                             	\
    {                                                                                              	\
		uart_buffer_t 	uart_buffer;																\
		uart_buffer.rx_buf 				= NULL;														\
		uart_buffer.tx_buf 				= NULL;														\
																									\
		(P_UART_INSTANCE)->uart_buffer = uart_buffer;												\
		(*(P_RET_CODE)) = uart_init(P_UART_INSTANCE);												\
    } while (0)


#define UART_BUFFER_INIT(P_UART_INSTANCE, RX_BUF_SIZE, TX_BUF_SIZE, P_RET_CODE) 	\
    do                                                                                             	\
    {                                                                                              	\
		uart_buffer_t 	uart_buffer;																\
        static uint8_t     		rx_buf[RX_BUF_SIZE];                                               	\
        static uint8_t     		tx_buf[TX_BUF_SIZE];                                               	\
																									\
		uart_buffer.rx_buf 				= rx_buf;													\
		uart_buffer.rx_buf_size 		= RX_BUF_SIZE;												\
		uart_buffer.rx_buf_read_index 	= 0;														\
		uart_buffer.rx_buf_write_index 	= 0;														\
		uart_buffer.tx_buf 				= tx_buf;													\
		uart_buffer.tx_buf_size			= TX_BUF_SIZE;												\
																									\
		(P_UART_INSTANCE)->uart_buffer = uart_buffer;												\
		(*(P_RET_CODE)) = uart_init(P_UART_INSTANCE);												\
    } while (0)



/**@brief Macro for initialization of the UART buffer 
 *
 * @param[in]   P_COMM_PARAMS   Pointer to a UART communication structure: app_uart_comm_params_t
 * @param[in]   RX_BUF_SIZE     Size of desired RX buffer, must be a power of 2 or ZERO (No FIFO).
 * @param[in]   TX_BUF_SIZE     Size of desired TX buffer, must be a power of 2 or ZERO (No FIFO).
 * @param[in]   EVT_HANDLER   Event handler function to be called when an event occurs in the
 *                              UART module.
 * @param[in]   IRQ_PRIO        IRQ priority, app_irq_priority_t, for the UART module irq handler.
 * @param[out]  ERR_CODE        The return value of the UART initialization function will be
 *                              written to this parameter.
 *
 * @note Since this macro allocates a buffer and registers the module as a GPIOTE user when flow
 *       control is enabled, it must only be called once.
 */

		
		

// vllt noch init funktion uart_without_buffer?? Damit kann printf und receive_buffer_IT nicht ausgeführt werden!!
		
ret_code_t uart_init(uart_instance_t* uart_instance);


ret_code_t uart_printf_bkgnd(uart_instance_t* uart_instance, uart_handler_t uart_handler, const char* format, ...);

ret_code_t uart_printf_abort_bkgnd(uart_instance_t* uart_instance);

ret_code_t uart_printf(uart_instance_t* uart_instance, const char* format, ...);


ret_code_t uart_transmit_bkgnd(uart_instance_t* uart_instance, uart_handler_t uart_handler, const uint8_t* tx_data, uint32_t tx_data_len);

ret_code_t uart_transmit_abort_bkgnd(uart_instance_t* uart_instance);

ret_code_t uart_transmit(uart_instance_t* uart_instance, const uint8_t* tx_data, uint32_t tx_data_len);


ret_code_t uart_receive_bkgnd(uart_instance_t* uart_instance, uart_handler_t uart_handler, uint8_t* rx_data, uint32_t rx_data_len);

ret_code_t uart_receive_abort_bkgnd(uart_instance_t* uart_instance);

ret_code_t uart_receive(uart_instance_t* uart_instance, uint8_t* rx_data, uint32_t rx_data_len);


ret_code_t uart_receive_buffer_bkgnd(uart_instance_t* uart_instance,  uart_handler_t uart_handler);

ret_code_t uart_receive_buffer_abort_bkgnd(uart_instance_t* uart_instance);

ret_code_t uart_receive_buffer_get(uart_instance_t* uart_instance,  uint8_t* data_byte);







/*

ret_code_t uart_buffer_init();



ret_code_t uart_receive();

ret_code_t uart_receive_IT();

ret_code_t uart_transmit();

ret_code_t uart_transmit_IT();

ret_code_t uart_printf();

ret_code_t uart_printf_IT();

ret_code_t uart_receive_buffer_start_IT();

ret_code_t uart_receive_buffer_end_IT();

ret_code_t uart_get_buffer();

*/



#endif
