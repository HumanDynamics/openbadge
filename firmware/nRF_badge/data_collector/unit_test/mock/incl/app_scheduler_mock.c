/** @file
 * @brief Simulation of the scheduler that is used for transferring execution from the interrupt context to the main
 *        context. 
 *
 * @details	The implementation is nearly the same as in the SDK. Only the critical section stuff is changed and is 
 *			saved by a pthread-mutex.
 *			The following functions are implemented:
 *			app_sched_init, app_sched_queue_space_get, app_sched_event_put, app_sched_execute
 *			
 *
 */
#include "app_scheduler.h"
#include "sdk_common.h"
#if NRF_MODULE_ENABLED(APP_SCHEDULER)
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "pthread.h"


static pthread_mutex_t critical_section_mutex;

/**@brief Function for entering a critical section by locking the mutex.
 */
static void enter_critical_section(void) {
	pthread_mutex_lock(&critical_section_mutex);
}

/**@brief Function for exiting a critical section by locking the mutex.
 */
static void exit_critical_section(void) {
	pthread_mutex_unlock(&critical_section_mutex);
}

/**@brief Function for checking if a pointer value is aligned to a 4 byte boundary.
 *
 * @param[in]   p   Pointer value to be checked.
 *
 * @return      TRUE if pointer is aligned to a 4 byte boundary, FALSE otherwise.
 */
static bool is_word_aligned(void const* p)
{
    return (((uintptr_t)p & 0x03) == 0);
}



/**@brief Structure for holding a scheduled event header. */
typedef struct
{
    app_sched_event_handler_t handler;          /**< Pointer to event handler to receive the event. */
    uint16_t                  event_data_size;  /**< Size of event data. */
} event_header_t;



static event_header_t * m_queue_event_headers;  /**< Array for holding the queue event headers. */
static uint8_t        * m_queue_event_data;     /**< Array for holding the queue event data. */
static volatile uint8_t m_queue_start_index;    /**< Index of queue entry at the start of the queue. */
static volatile uint8_t m_queue_end_index;      /**< Index of queue entry at the end of the queue. */
static uint16_t         m_queue_event_size;     /**< Maximum event size in queue. */
static uint16_t         m_queue_size;           /**< Number of queue entries. */


/**@brief Function for incrementing a queue index, and handle wrap-around.
 *
 * @param[in]   index   Old index.
 *
 * @return      New (incremented) index.
 */
static uint8_t next_index(uint8_t index)
{
    return (index < m_queue_size) ? (index + 1) : 0;
}


static uint8_t app_sched_queue_full()
{
  uint8_t tmp = m_queue_start_index;
  return next_index(m_queue_end_index) == tmp;
}



static uint8_t app_sched_queue_empty()
{
  uint8_t tmp = m_queue_start_index;
  return m_queue_end_index == tmp;
}


uint32_t app_sched_init(uint16_t event_size, uint16_t queue_size, void * p_event_buffer)
{
	
	// Init the critical section mutex
	pthread_mutex_init (&critical_section_mutex, NULL);
	
	
    uint16_t data_start_index = (queue_size + 1) * sizeof(event_header_t);

    // Check that buffer is correctly aligned
    if (!is_word_aligned(p_event_buffer))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    // Initialize event scheduler
    m_queue_event_headers = (event_header_t*)p_event_buffer;
    m_queue_event_data    = &((uint8_t *)p_event_buffer)[data_start_index];
    m_queue_end_index     = 0;
    m_queue_start_index   = 0;
    m_queue_event_size    = event_size;
    m_queue_size          = queue_size;

    return NRF_SUCCESS;
}


uint16_t app_sched_queue_space_get()
{
    uint16_t start = m_queue_start_index;
    uint16_t end   = m_queue_end_index;
    uint16_t free_space = m_queue_size - ((end >= start) ?
                           (end - start) : (m_queue_size + 1 - start + end));
    return free_space;
}


uint32_t app_sched_event_put(void                    * p_event_data,
                             uint16_t                  event_data_size,
                             app_sched_event_handler_t handler)
{
    uint32_t err_code;

    if (event_data_size <= m_queue_event_size)
    {
        uint16_t event_index = 0xFFFF;

        enter_critical_section();

        if (!app_sched_queue_full())
        {
            event_index       = m_queue_end_index;
            m_queue_end_index = next_index(m_queue_end_index);

        }

        

        if (event_index != 0xFFFF)
        {
           
            m_queue_event_headers[event_index].handler = handler;
            if ((p_event_data != NULL) && (event_data_size > 0))
            {
                memcpy(&m_queue_event_data[event_index * m_queue_event_size],
                       p_event_data,
                       event_data_size);
                m_queue_event_headers[event_index].event_data_size = event_data_size;
            }
            else
            {
                m_queue_event_headers[event_index].event_data_size = 0;
            }

            err_code = NRF_SUCCESS;
        }
        else
        {
            err_code = NRF_ERROR_NO_MEM;
        }
		
		exit_critical_section();
    }
    else
    {
        err_code = NRF_ERROR_INVALID_LENGTH;
    }

    return err_code;
}





void app_sched_execute(void)
{
    while (!app_sched_queue_empty())
    {
        // Since this function is only called from the main loop, there is no
        // need for a critical region here, however a special care must be taken
        // regarding update of the queue start index (see the end of the loop).
        uint16_t event_index = m_queue_start_index;

        void * p_event_data;
        uint16_t event_data_size;
        app_sched_event_handler_t event_handler;

        p_event_data = &m_queue_event_data[event_index * m_queue_event_size];
        event_data_size = m_queue_event_headers[event_index].event_data_size;
        event_handler   = m_queue_event_headers[event_index].handler;

        event_handler(p_event_data, event_data_size);

        // Event processed, now it is safe to move the queue start index,
        // so the queue entry occupied by this event can be used to store
        // a next one.
        m_queue_start_index = next_index(m_queue_start_index);
    }
}
#endif //NRF_MODULE_ENABLED(APP_SCHEDULER)
