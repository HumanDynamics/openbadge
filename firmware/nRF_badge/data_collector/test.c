
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

//#define TEST

#ifdef TEST
static void test(void) {
	printf("aaaa\n");
}
#endif

#ifndef TEST
static void test(void) {
	printf("bbbb\n");
}
#endif

int main(void)
{
	printf("HALLO!!\n");
	
	uint8_t a = 0;
	
	if(a == 0 || a == 1)
		test();
	
}