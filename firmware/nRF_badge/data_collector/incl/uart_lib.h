#ifndef __UART_LIB_H
#define __UART_LIB_H



/** @file
 *
 * @brief UART abstraction library.
 *
 * @details It enables to call the UART peripheral from different contexts. 
 *			If the selected UART peripheral is currently in use, it will inform the other context by returning NRF_ERROR_BUSY.
 *			It is possible to transmit and receive/receive-buffer data parallel.
 *			There are two different receive-modes: normal receive and receive-buffer.
 *			In normal receive operation a predefined amount of data is received.
 *			In receive-buffer operation a variable amount of data can be received, 
 *			the received data are buffered in the internal circular rx-buffer.
 *			To get the bytes from the circular rx-buffer the uart_receive_buffer_get()-function can be used.
 *			Furthermore, there is the capability to use a printf-styled function, to transmit formatted data via UART.
 *			For the printf-functions the tx-buffer in uart_buffer must be initialized.
 *			If float/double should be supported, the linker has to be configured accordingly: LDFLAGS += -u _printf_float.
 */

#include "sdk_common.h"	// Needed for the definition of ret_code_t and the error-codes
#include "nrf_drv_uart.h"



/**@brief The different UART operations. These operations will be used to set the peripheral busy or not. */
typedef enum {
	UART_NO_OPERATION 					= 0,			/**< Currently no uart operation ongoing. */
	UART_TRANSMIT_OPERATION 			= (1 << 0),		/**< Currently an uart transmit operation ongoing. */
	UART_RECEIVE_OPERATION 				= (1 << 1),		/**< Currently an uart receive operation ongoing. */
	UART_RECEIVE_BUFFER_OPERATION		= (1 << 2),		/**< Currently an uart receive to buffer operation ongoing. */
} uart_operation_t;



/**@brief The uart buffer type. Needed for the printf and receive-buffer operations. */
typedef struct 
{
	uint8_t * rx_buf;     					/**< Pointer to the circular RX buffer. */
    uint32_t  rx_buf_size; 					/**< Size of the circular RX buffer. */
	volatile uint32_t rx_buf_read_index;	/**< The read index of the circular RX buffer (uart_receive_buffer_get() reads from this index). */
	volatile uint32_t rx_buf_write_index;	/**< The write index of the circular RX buffer (the internal receive-callback() writes to this index when receiving a byte) */
    uint8_t * tx_buf;      					/**< Pointer to the TX buffer. */
    uint32_t  tx_buf_size; 					/**< Size of the TX buffer. */
} uart_buffer_t;


/**@brief  UART driver event types, passed to the handler routine provided by the bkgnd-functions. */
typedef enum
{
    UART_TRANSMIT_DONE, 	/**< Transmit done */
	UART_RECEIVE_DONE, 		/**< Receive done */
	UART_DATA_AVAILABLE, 	/**< Data available */
	UART_ERROR,				/**< Communication error */
} uart_evt_type_t;

typedef struct
{
	uart_evt_type_t  type;	/**< Event type */
} uart_evt_t;





/**
 * @brief UART event handler type. 
 */
typedef void (*uart_handler_t)(uart_evt_t const * p_event);

/**@example	Example of uart_instance_t 
 *
 *
 * 			uart_instance_t uart_instance;															// Create an uart_instance-struct.
 *
 *			uart_instance.uart_peripheral 							= 0;							// Set the uart peripheral to 0 (the selected peripheral has to be enabled in sdk_config.h).	
 *			uart_instance.nrf_drv_uart_config.baudrate				= NRF_UART_BAUDRATE_115200;		// Set the uart baudrate. NRF_UART_BAUDRATE_115200 from nrf_uart.h.
 *			uart_instance.nrf_drv_uart_config.hwfc					= NRF_UART_HWFC_DISABLED;		// Set the uart hardware flow control. NRF_UART_HWFC_DISABLED from nrf_uart.h.
 *			uart_instance.nrf_drv_uart_config.interrupt_priority	= APP_IRQ_PRIORITY_LOW;			// The interrupt priotity of the spi peripheral. APP_IRQ_PRIORITY_LOW from app_util_platform.h.
 *			uart_instance.nrf_drv_uart_config.parity				= NRF_UART_PARITY_EXCLUDED;		// Set the uart parity. NRF_UART_PARITY_EXCLUDED from nrf_uart.h.
 *			uart_instance.nrf_drv_uart_config.pselcts				= 0;							// Set the uart CTS-pin.
 *			uart_instance.nrf_drv_uart_config.pselrts				= 0;							// Set the uart RTS-pin.
 *			uart_instance.nrf_drv_uart_config.pselrxd				= 11;							// Set the uart RXD-pin.
 *			uart_instance.nrf_drv_uart_config.pseltxd				= 10;							// Set the uart TXD-pin.
 *
 *			ret_code_t ret;
 *			UART_BUFFER_INIT(&uart_instance, &ret);													// Initialize the uart_instance.
 *
 */

/**@brief UART instance type. */
typedef struct {	
	uint8_t			 		uart_peripheral;		/**< Set to the desired uart peripheral. The Peripheral has to be enabled in the sdk_config.h file */
	nrf_drv_uart_config_t	nrf_drv_uart_config;	/**< Set the uart configuration (possible parameters in nrf_drv_uart.h)  */	
	uint32_t 				uart_instance_id;		/**< Instance index: Setted by the init-function (do not set!) */
	uart_buffer_t			uart_buffer;			/**< The uart buffer used for printf and receive_buffer: If you want to use the buffer functionality, use the corresponding init MACRO. Setted by the init-function (do not set!) */	
	nrf_drv_uart_t			nrf_drv_uart_instance;	/**< The initialized low level uart instance: Setted by the init-function (do not set!) */
} uart_instance_t;





/**@brief Macro for initializing the uart instance without a uart-buffer.
 *
 * @details This macro calls the uart_init()-function for initializing the uart instance.
 *			Furthermore, it sets the rx- and tx-buffer of uart_buffer to NULL. 
 *
 * @note 	It is important that all the printf- and receive_buffer-functions won't work if UART_INIT() is used for initialization.
 *
 * @param[in,out]	P_UART_INSTANCE		Pointer to a preconfigured uart instance.
 * @param[out]  	P_RET_CODE       	Pointer where the return value of uart_init() is written to.
 */
#define UART_INIT(P_UART_INSTANCE, P_RET_CODE) 	\
    do                                                                                             	\
    {                                                                                              	\
		uart_buffer_t 	uart_buffer;																\
		uart_buffer.rx_buf = NULL;																	\
		uart_buffer.tx_buf = NULL;																	\
																									\
		(P_UART_INSTANCE)->uart_buffer = uart_buffer;												\
		(*(P_RET_CODE)) = uart_init(P_UART_INSTANCE);												\
    } while (0)

		
		
		
/**@brief Macro for initializing the uart instance with uart-buffer so that the printf- and recevie_buffer-functions could work.
 *
 * @details This macro calls the uart_init()-function for initializing the uart instance.
 *			Furthermore, it initializes the uart_buffer so that the printf- and receive_buffer-functions can be used.
 *
 * @param[in,out]	P_UART_INSTANCE		Pointer to a preconfigured uart instance.
 * @param[in]		RX_BUF_SIZE			Size of the static allocated circular receive-buffer.
 * @param[in]		TX_BUF_SIZE			Size of the static allocated transmit-buffer.
 * @param[out]  	P_RET_CODE       	Pointer where the return value of uart_init() is written to.
 */
#define UART_BUFFER_INIT(P_UART_INSTANCE, RX_BUF_SIZE, TX_BUF_SIZE, P_RET_CODE) 	\
    do                                                                                             	\
    {                                                                                              	\
		uart_buffer_t 	uart_buffer;																\
        static uint8_t  rx_buf[RX_BUF_SIZE];                                               			\
        static uint8_t  tx_buf[TX_BUF_SIZE];                                               			\
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


/**@brief   Function for initializing an instance for the uart peripheral.
 *
 * @details Initializes the low level uart driver. It allows multiple instances on the same peripheral
 *			(it assumes that the configuration is the same for all the different instances on the same peripheral)
 *			
 * @note 	Always initialize with the Macros UART_BUFFER_INIT() or UART_INIT(). 
 *
 * @param[in,out]   uart_instance		Pointer to an preconfigured uart_instance.
 *
 * @retval  NRF_SUCCESS    				If the adc_instance was successfully initialized.
 * @retval  NRF_ERROR_INVALID_PARAM  	If the specified peripheral is not correct.
 * @retval  NRF_ERROR_INVALID_STATE  	If the peripheral was already initialized.
 */
ret_code_t uart_init(uart_instance_t* uart_instance);



/**@brief   Function for printing formatted data in asynchronous/non-blocking/background mode.
 *
 * @details This is a non-blocking function. If there is already an ongoing uart transmit operation this function returns NRF_ERROR_BUSY.
 *			Internally this function calls uart_transmit_bkgnd().
 * 			If the operation was started successfully and terminates, 
 *			the provided uart_handler is called with event: UART_TRANSMIT_DONE or UART_ERROR.
 *	
 * @note 	This function works only when the uart instance was initialized with UART_BUFFER_INIT() and the specified tx_buf-size is big enough.
 *			If the application needs to print float/double values, there has to be a linker flag: LDFLAGS += -u _printf_float.
 *
 * @param[in]   uart_instance	Pointer to an initialized uart instance. 	   	
 * @param[in]   uart_handler 	Handler function that should be called if the operation is done, with event: UART_TRANSMIT_DONE or UART_ERROR. Could also be NULL if no handler should be called.   	
 * @param[in]   format         	The format string.
 * @param[in]   ...    			Variable input parameters, needed by the format string.
 *
 * @retval  NRF_SUCCESS             If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY			If there is already an ongoing transmit/printf operation.
 * @retval  NRF_ERROR_INTERNAL		If there was an internal error in the lower level uart driver.	
 * @retval  NRF_ERROR_INVALID_ADDR	If tx_data does not point to RAM buffer (UARTE only).
 * @retval 	NRF_ERROR_NULL    		If the tx-buffer in uart_buffer is NULL.
 * @retval 	NRF_ERROR_NO_MEM    	If the formatted string is bigger than the tx-buffer size.
 */
ret_code_t uart_printf_bkgnd(uart_instance_t* uart_instance, uart_handler_t uart_handler, const char* format, ...);



/**@brief   Function for aborting a current data transmission.
 *
 * @details 	This function calls internally uart_transmit_abort_bkgnd().
 *
 * @param[in]   uart_instance			Pointer to an initialized uart instance. 	   	
 *
 * @retval  NRF_SUCCESS             	If the operation was successful.
 * @retval  NRF_ERROR_INVALID_STATE		If it is another/false uart_instance that tries to abort the current operation.
 */
ret_code_t uart_printf_abort_bkgnd(const uart_instance_t* uart_instance);



/**@brief   Function for printing formatted data in blocking mode.
 *
 * @details This function uses internally uart_transmit() to do a blocking transmit.
 *	
 * @note 	This function works only when the uart instance was initialized with UART_BUFFER_INIT() and the specified tx_buf-size is big enough.
 *			If the application needs to print float/double values, there has to be a linker flag: LDFLAGS += -u _printf_float.
 *
 * @param[in]   uart_instance	Pointer to an initialized uart instance. 	   	
 * @param[in]   format         	The format string.
 * @param[in]   ...    			Variable input paramters, needed by the format string.
 *
 * @retval  NRF_SUCCESS             If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY			If there is already an ongoing transmit/printf operation.
 * @retval  NRF_ERROR_INTERNAL		If there was an internal error in the lower level uart driver.	
 * @retval  NRF_ERROR_INVALID_ADDR	If tx_data does not point to RAM buffer (UARTE only).
 * @retval 	NRF_ERROR_NULL    		If the tx-buffer in uart_buffer is NULL.
 * @retval 	NRF_ERROR_NO_MEM    	If the formatted string is bigger than the tx-buffer size.
 */
ret_code_t uart_printf(uart_instance_t* uart_instance, const char* format, ...);



/**@brief   Function for transmitting data in asynchronous/non-blocking/background mode.
 *
 * @details This is a non-blocking function. If there is already an ongoing uart transmit operation this function returns NRF_ERROR_BUSY.
 *			If the tx_data_len is >= 255 the packet is splitted and the remaining bytes will be transmitted via the internal uart interrupt.
 *			That must be done because nrf_drv_uart-library don't accepts UART TX transfers with size >= 256. (That is very disappointing). 
 * 			If the operation was started successfully and terminates, 
 *			the provided uart_handler is called with event: UART_TRANSMIT_DONE or UART_ERROR.
 *
 * @warning The transmit data must be kept in memory until the operation has terminated. 
 *
 * @param[in]   uart_instance	Pointer to an initialized uart instance. 	   	
 * @param[in]   uart_handler 	Handler function that should be called if the operation is done, with event: UART_TRANSMIT_DONE or UART_ERROR. Could also be NULL if no handler should be called.   	
 * @param[in]   tx_data         Pointer to the transmit data.
 * @param[in]   tx_data_len  	Size of data to transmit.
 *
 * @retval  NRF_SUCCESS             If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY			If there is already an ongoing transmit/printf operation.
 * @retval  NRF_ERROR_INTERNAL		If there was an internal error in the lower level uart driver.	
 * @retval  NRF_ERROR_INVALID_ADDR	If tx_data does not point to RAM buffer (UARTE only).
 */
ret_code_t uart_transmit_bkgnd(uart_instance_t* uart_instance, uart_handler_t uart_handler, const uint8_t* tx_data, uint32_t tx_data_len);



/**@brief   Function for aborting a current data transmission.
 *
 * @param[in]   uart_instance			Pointer to an initialized uart instance. 	   	
 *
 * @retval  NRF_SUCCESS             	If the operation was successful.
 * @retval  NRF_ERROR_INVALID_STATE		If it is another/false uart_instance that tries to abort the current operation.
 */
ret_code_t uart_transmit_abort_bkgnd(const uart_instance_t* uart_instance);



/**@brief   Function for transmitting data in blocking mode.
 *
 * @details This function uses internally uart_transmit_bkgnd() for the data transmission and
 *			uart_get_operation() to wait for the termination of the transmit operation.
 *
 * @param[in]   uart_instance	Pointer to an initialized uart instance. 	   	
 * @param[in]   tx_data         Pointer to the transmit data.
 * @param[in]   tx_data_len  	Size of data to transmit.
 *
 * @retval  NRF_SUCCESS             If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY			If there is already an ongoing transmit/printf operation.
 * @retval  NRF_ERROR_INTERNAL		If there was an internal error in the lower level uart driver.	
 * @retval  NRF_ERROR_INVALID_ADDR	If tx_data does not point to RAM buffer (UARTE only).
 */
ret_code_t uart_transmit(uart_instance_t* uart_instance, const uint8_t* tx_data, uint32_t tx_data_len);



/**@brief   Function for receiving a fixed amount of data in asynchronous/non-blocking/background mode.
 *
 * @details 	This is a non-blocking function. The function returns NRF_ERROR_BUSY, 
 * 				if there is already a receive or receive-buffer operation ongoing.
 *				If the uart peripheral received the specified amount of data, 
 *				the uart_handler will be called, with event: UART_RECEIVE_DONE or UART_ERROR.
 *
 * @warning 	The receive data must be kept in memory until the operation has terminated.
 *
 * @param[in]   uart_instance	Pointer to an initialized uart instance. 	
 * @param[in]   uart_handler 	Handler function that should be called if the operation is done, with event: UART_RECEIVE_DONE or UART_ERROR. Could also be NULL if no handler should be called.   	 
 * @param[in]   rx_data         Pointer to the receive data memory.
 * @param[in]   rx_data_len  	Size of data to be received.
 *
 * @retval  NRF_SUCCESS             If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY			If there is already an ongoing receive or receive_buffer operation.
 * @retval  NRF_ERROR_INTERNAL		If there was an internal error in the lower level uart driver.	
 * @retval  NRF_ERROR_INVALID_ADDR	If rx_data does not point to RAM buffer (UARTE only).
 */
ret_code_t uart_receive_bkgnd(uart_instance_t* uart_instance, uart_handler_t uart_handler, uint8_t* rx_data, uint32_t rx_data_len);



/**@brief   Function for aborting the receiving of the fixed amount of data.
 *
 * @param[in]   uart_instance			Pointer to an initialized uart instance. 	   	
 *
 * @retval  NRF_SUCCESS             	If the operation was successful.
 * @retval  NRF_ERROR_INVALID_STATE		If it is another/false uart_instance that tries to abort the current operation.
 */
ret_code_t uart_receive_abort_bkgnd(const uart_instance_t* uart_instance);


/**@brief   Function for receiving a fixed amount of data in blocking mode.
 *
 * @details This function uses internally uart_receive_bkgnd() for the fixed amount data receive and
 *			uart_get_operation() to wait for the termination of the receive operation.
 *
 * @param[in]   uart_instance	Pointer to an initialized uart instance. 	   	
 * @param[in]   rx_data         Pointer to the receive data memory.
 * @param[in]   rx_data_len  	Size of data to be received.
 *
 * @retval  NRF_SUCCESS             If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY			If there is already an ongoing receive or receive_buffer operation.
 * @retval  NRF_ERROR_INTERNAL		If there was an internal error in the lower level uart driver.	
 * @retval  NRF_ERROR_INVALID_ADDR	If rx_data does not point to RAM buffer (UARTE only).
 */
ret_code_t uart_receive(uart_instance_t* uart_instance, uint8_t* rx_data, uint32_t rx_data_len);


/**@brief   Function for starting the receive to buffer of an variable length of data in asynchronous/non-blocking/background mode.
 *
 * @details 	This is a non-blocking function. The function returns NRF_ERROR_BUSY, 
 * 				if there is a receive or receive-buffer operation ongoing.
 *				If the uart peripheral receives a byte, it is inserted in the circular rx-buffer and
 *				the specified uart_handler will be called, with events: UART_DATA_AVAILABLE or UART_ERROR.
 *				The one byte receive operation is rescheduled internally via the interrupt handler. 
 *				The data-bytes of the circular rx-buffer could be read via the uart_receive_buffer_get()-function.
 *				It is only possible to receive so many bytes at once (without calling uart_receive_buffer_get())
 *				as specified during the initialization of the rx-buffer in uart_buffer.
 *
 * @note 		This function works only when the uart instance was initialized with UART_BUFFER_INIT() and the specified rx_buf-size is big enough.
 *				
 *
 * @param[in]   uart_instance	Pointer to an initialized uart instance. 	
 * @param[in]   uart_handler 	Handler function that should be called if the operation is done, with event: UART_DATA_AVAILABLE or UART_ERROR. Could also be NULL if no handler should be called.   	 
 *
 * @retval  NRF_SUCCESS             If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY			If there is already an ongoing receive or receive_buffer operation.
 * @retval  NRF_ERROR_INTERNAL		If there was an internal error in the lower level uart driver.	
 * @retval  NRF_ERROR_INVALID_ADDR	If rx_data does not point to RAM buffer (UARTE only).
 */
ret_code_t uart_receive_buffer_bkgnd(uart_instance_t* uart_instance,  uart_handler_t uart_handler);



/**@brief   Function for aborting the receive buffer operation.
 *
 * @param[in]   uart_instance			Pointer to an initialized uart instance. 	   	
 *
 * @retval  NRF_SUCCESS             	If the operation was successful.
 * @retval  NRF_ERROR_INVALID_STATE		If it is another/false uart_instance that tries to abort the current operation.
 */
ret_code_t uart_receive_buffer_abort_bkgnd(const uart_instance_t* uart_instance);


/**@brief   Function for aborting the receive buffer operation.
 *
 * @details		This functions pops an element from the circular rx-fifo (as long as there are elements in the circular rx-buffer).
 *
 * @param[in]   uart_instance			Pointer to an initialized uart instance. 
 * @param[out]  data_byte				Pointer to a one byte memory, where the bytes from the circular rx-buffer should be stored to.  
 *
 * @retval  NRF_SUCCESS             	If there is data available.
 * @retval  NRF_ERROR_NOT_FOUND			If no new data is available in the rx-buffer.
 */
ret_code_t uart_receive_buffer_get(uart_instance_t* uart_instance,  uint8_t* data_byte);


/**@brief   Function for retrieving the current uart operation.
 *
 * @retval  UART_NO_OPERATION             	If there is no uart operation ongoing.
 * @retval  UART_TRANSMIT_OPERATION			If there is an uart transmit operation ongoing.
 * @retval  UART_RECEIVE_OPERATION			If there is an uart receive operation ongoing.
 * @retval  UART_RECEIVE_BUFFER_OPERATION	If there is an uart receive-buffer operation ongoing.
 */
uart_operation_t uart_get_operation(const uart_instance_t* uart_instance);

#endif
