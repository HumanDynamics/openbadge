#ifndef __UART_LIB_H
#define __UART_LIB_H


#include <stdargs.h>
#include "nrf_drv_uart.h"



#ifndef MAX_UART_INSTANCE_NUMBER
	#define MAX_UART_INSTANCE_NUMBER	2
#endif






typedef struct
{
	
} uart_config_t;

typedef struct 
{
	uint8_t * 	rx_buf;      /**< Pointer to the RX buffer. */
    uint32_t  	rx_buf_size; /**< Size of the RX buffer. */
	uint32_t 	rx_buf_read_index;
	uint32_t 	rx_buf_write_index;
    uint8_t * 	tx_buf;      /**< Pointer to the TX buffer. */
    uint32_t  	tx_buf_size; /**< Size of the TX buffer. */
} uart_buffer_t;



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
#define UART_BUFFER_INIT(P_UART_BUFFER, RX_BUF_SIZE, TX_BUF_SIZE) \
    do                                                                                             \
    {                                                                                              \
        static uint8_t     rx_buf[RX_BUF_SIZE];                                                    \
        static uint8_t     tx_buf[TX_BUF_SIZE];                                                    \
                                                                                                   \
		(P_UART_BUFFER)->rx_buf      = rx_buf;                                                     \
        (P_UART_BUFFER)->rx_buf_size = sizeof (rx_buf);                                            \
		(P_UART_BUFFER)->rx_buf_read_index = 0;                                            		   \
		(P_UART_BUFFER)->rx_buf_write_index = 0;                                            	   \
        (P_UART_BUFFER)->tx_buf      = tx_buf;                                                     \
        (P_UART_BUFFER)->tx_buf_size = sizeof (tx_buf);                                            \
    } while (0)

#define UART_INIT(P_UART_INSTANCE_ID, P_UART_CONFIG, RX_BUF_SIZE, TX_BUF_SIZE) \
    do                                                                                             \
    {                                                                                              \
        static uart_buffer_t uart_buffer;                                         			       \
        UART_BUFFER_INIT(&uart_buffer, RX_BUF_SIZE, TX_BUF_SIZE);                                  \
                                                                                                   \
		uart_init(P_UART_INSTANCE_ID, P_UART_CONFIG, &uart_buffer);                                \
    } while (0)

		
		





#endif
