#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

int main(int argc, char const *argv[])
{
	unsigned char data[] = {0x35,	// 0011 0101 << 4 = 0101 0000 (0x50)
							0x30};
	unsigned int value;
	unsigned int val;

	value = (data[0]<<4 & 0xFF);
	memcpy(&val, &data[0], 1);

	printf("%.2x\n", value);
	printf("hex: %.2x\n", val);
	printf("int: %d\n", val);
	printf("char: %c\n", val);

	return 0;
}