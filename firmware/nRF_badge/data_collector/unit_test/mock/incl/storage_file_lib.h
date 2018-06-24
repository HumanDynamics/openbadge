#ifndef __STORAGE_FILE_LIB_H
#define __STORAGE_FILE_LIB_H

#include "stdint.h"

void storage_file_write_to_file(const char* filename, uint8_t* bytes, uint32_t len);

#endif
