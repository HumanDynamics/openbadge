/** @file
 * @brief Simulation of the common application error handler and macros for utilizing a common error handler.
 */
#ifndef APP_ERROR_H__
#define APP_ERROR_H__

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "sdk_errors.h"


#define APP_ERROR_CHECK(ERR_CODE)                           \
    do                                                      \
    {                                                       \
        (void) ERR_CODE;                                    \
    } while (0)

#endif
