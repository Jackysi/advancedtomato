/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/
#include "rc.h"

int gpio_main(int argc, char *argv[])
{
	const char hex[] = "0123456789ABCDEF";
	unsigned long v;
	int bit;
	int i;
	char s[17];
	
	if (argc == 3) {
		bit = atoi(argv[2]);
		if ((bit >= 0) && (bit <= 15)) {
			bit = 1 << bit;
			if ((strncmp(argv[1], "en", 2) == 0) || (strncmp(argv[1], "di", 2) == 0)) {
				gpio_write(bit, argv[1][0] == 'e');
				return 0;
			}
		}
	}
	else if (argc == 2) {
		if (strncmp(argv[1], "po", 2) == 0) {
			while ((v = gpio_read()) != ~0) {
				for (i = 15; i >= 0; --i) {
					s[i] = (v & (1 << i)) ? hex[i] : '.';
				}
				s[16] = 0;
				printf("%08lX %s\n", v, s);
				sleep(1);
			}
			return 0;
		}
	}
	
	usage_exit(argv[0], "<enable|disable|poll> <pin>\n");
}
