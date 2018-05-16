
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

//#define TEST


static void test(uint8_t a) {
	if(a == 0 || a == 1) {
		printf("aaaa\n");
	}
}




int main(void)
{
	printf("HALLO!!\n");
	
	uint8_t a = 0;
	
	test(a);
	
	a ++;
	
	test(a);
	
	a++;
	test(a);
	
}