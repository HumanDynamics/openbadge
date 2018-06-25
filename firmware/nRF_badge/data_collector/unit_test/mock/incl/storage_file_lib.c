#include "storage_file_lib.h"


#include "stdio.h"
#include "string.h"


/** @brief Function to convert a byte into hex string.
 *
 * @param[out]	hex		Pointer to memory where to store the hex string (at least 2 Bytes)
 * @param[in]	data	The byte to convert.
 */
static void convert_to_hex(char* hex, uint8_t data) {
	
	for(uint8_t i = 0; i < 2; i++) {
		uint8_t tmp = (data >> (i*4)) & 0x0F;
		if(tmp < 10) {
			hex[1-i] = tmp + 48;
		} else {
			hex[1-i] = tmp - 10 + 65;
		}
	}
}


/** @brief Function to convert a word into decimal string.
 *
 * @param[out]	dec			Pointer to memory where to store the decimal string (at least 10 Bytes)
 * @param[in]	data		The word to convert.
 */
static void convert_to_dec(char* dec, uint32_t data) {
	
	char tmp[11];
	
	sprintf(tmp, "%u", data);
	uint32_t len = strlen(tmp);
	uint32_t offset = 10 - len;
	for(uint8_t i = 0; i < 10; i++) {
		if(i < offset) {
			dec[i] = '0';
		} else {
			dec[i] = tmp[i - offset];
		}
	}
}



void storage_file_write_to_file(const char* filename, uint8_t* bytes, uint32_t len) {
	
	FILE* pfile = fopen(filename, "wb");
	
	
	
	
	//char header [] = "Address (hex)\tAddress(dec)\t0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F\n";
	char header [] = "Address (hex)\tAddress(dec)\t 0  | 1  | 2  | 3  | 4  | 5  | 6  | 7  | 8  | 9  | A  | B  | C  | D  | E  | F\n";
	fwrite(header, strlen(header), 1, pfile);
	
	
	
	for(uint32_t i = 0; i < len/16; i++) {
		
		{
		char tmp[] = {'0', 'x', '0', '0', '0', '0', '0', '0', '0', '0'};
		uint32_t address = i*16;
		convert_to_hex(&tmp[2], (address >> 24) & 0xFF);
		convert_to_hex(&tmp[4], (address >> 16) & 0xFF);
		convert_to_hex(&tmp[6], (address >> 8) & 0xFF);
		convert_to_hex(&tmp[8], (address >> 0) & 0xFF);
		fwrite(tmp, 10, 1, pfile);
		}
		fwrite("\t\t", 2, 1, pfile);
		{
		char tmp[] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
		uint32_t address = i*16;
		convert_to_dec(&tmp[0], address);
		fwrite(tmp, 10, 1, pfile);
		}
		fwrite("\t\t", 2, 1, pfile);
	
		
		for(uint32_t j = 0; j < 16; j++) {
			char tmp[] = {'0', 'x', '0', '0', ' '};
			convert_to_hex(&tmp[2], bytes[i*16 + j]);
			fwrite(tmp, 5, 1, pfile);
		}
		fwrite("\t", 1, 1, pfile);
		
		
		for(uint32_t j = 0; j < 16; j++) {
			char tmp[5];
			uint8_t len = 0;
			if(bytes[i*16 + j] == 0x0A) { // \n	
				sprintf(tmp, "\\n");
				len = strlen(tmp);
			} else if(bytes[i*16 + j] == 0x0D) { // \r
				sprintf(tmp, "\\r");
				len = strlen(tmp);
			} else {
				tmp[0]= bytes[i*16 + j];
				len = 1;
			}
			fwrite(tmp, len, 1, pfile);
		}	
		fwrite("\n", 1, 1, pfile);
		
	}
	
	fclose(pfile);
}

