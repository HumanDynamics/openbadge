#ifndef __SYSTEM_EVENT_LIB_H
#define __SYSTEM_EVENT_LIB_H

#include "sdk_common.h"






#define MAX_NUMBER_SYSTEM_EVENT_HANDLER	4



typedef void (*system_event_handler_t)(uint32_t sys_evt);


void system_event_init(void);


ret_code_t system_event_register_handler(system_event_handler_t event_handler);





#endif
