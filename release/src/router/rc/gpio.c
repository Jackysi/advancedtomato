#include <stdio.h>
#include <stdlib.h>

int gpio_main(int argc, char **argv)
{
	unsigned int val = 0;

	if (argc >= 2) {
		val = strtoul(argv[1], NULL, 0);
		fwrite(&val, 4, 1, stdout);
	} else {
		fread(&val, 4, 1, stdin);
		printf("0x%08x\n", val);
	}

	return 0;
}
