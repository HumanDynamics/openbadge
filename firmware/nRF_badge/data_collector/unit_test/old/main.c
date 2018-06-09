
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "eeprom_lib.h"
#include "debug_lib.h"
#include "flash_lib.h"


int main(void)
{
	eeprom_init();
	
	debug_log("Debug Log Test. EEPROM-size: %d\n", eeprom_get_size());
	
	char* s = "Halllooo!";
	uint8_t buf[100];
	uint32_t len = strlen(s);
	memcpy(buf, (uint8_t*) s, len);
	
	debug_log("EEPROM-Store ret: %d\n", eeprom_store(5, buf, len));
	
	eeprom_read(0, buf, len + 10);
	debug_log_dump(buf, len + 10);
	
	flash_init();
	
	debug_log("Flash erase ret: %d\n", flash_erase(0, 1));
	
	uint32_t words[3] = {0x1234ABCD, 0x11111111, 0xDEADBEEF};
	
	debug_log("Flash store ret: %d\n", flash_store(10*2, words, 3));
	
	
	for(uint32_t i = 0; i < 3; i++) {
		uint32_t w;
		flash_read(10*2 + i, &w, 1);
		if(w != words[i])
			debug_log("Flash read failed at %d: 0x%X, 0x%X\n", i, w, words[i]);
	}
	
	
}