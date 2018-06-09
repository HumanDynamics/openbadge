#include "debug_lib.h"


#ifdef DEBUG_ENABLE

#include <stdarg.h>		// Needed for the printf-function
#include <string.h>		// Needed for the printf-function
#include <stdio.h>		// Needed for the vsnprintf-function


void debug_init(void)
{	
}

void debug_log(const char* format, ...) {
	va_list args;
	va_start(args, format);	
	vprintf(format, args);
	va_end(args);	
}

void debug_log_dump(uint8_t * p_buffer, uint32_t len)
{
    debug_log("\r\n");
    for (uint32_t index = 0; index <  len; index++)  {
        debug_log("0x%02X ", p_buffer[index]);
    }
    debug_log("\r\n");
}

#endif
