
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>


static void test(uint8_t a) {
	if(a == 0 || a == 1) {
		printf("Oh yes!\n");
	} else {
		printf("Come on...\n");
	}
}




int main(void)
{
	printf("Helloo!!\n");
	
	uint8_t a = 0;
	
	test(a);
	
	//a++;

	
	test(a);
	
	//a++;
	//test(a);
	
}