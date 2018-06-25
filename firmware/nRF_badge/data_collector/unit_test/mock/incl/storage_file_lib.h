#ifndef __STORAGE_FILE_LIB_H
#define __STORAGE_FILE_LIB_H

#include "stdint.h"


/** @brief Function to write some bytes as formatted storage layout/visualization to a file (text-file).
 *
 * @details	The bytes are represented in their hexadecimal-representation.
 *			16 Bytes are represented in one line. At the beginning of a line the address (in hex and dec) is displayed.
 * 			At the end of a line the 16 Bytes are dumped (written in plain). 
 *
 * @param[in]	filename	The filename where to store the storage visualization.
 * @param[in]	bytes		Pointer to the bytes to visualize.
 * @param[in]	len			The number of bytes to visualize.
 */
void storage_file_write_to_file(const char* filename, uint8_t* bytes, uint32_t len);

#endif
