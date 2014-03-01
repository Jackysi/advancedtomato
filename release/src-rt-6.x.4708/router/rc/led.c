/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "rc.h"

__attribute__ ((noreturn))
static void help(void)
{
	char s[256];
	int i;

	s[0] = 0;
	for (i = 0; i < LED_COUNT; ++i) {
		if (led(i, LED_PROBE)) {
			if (s[0]) strcat(s, "/");
			strcat(s, led_names[i]);
		}
	}
	if (s[0] == 0) {
		fprintf(stderr, "Not supported.\n");
	}
	else {
		fprintf(stderr, "led <%s> <on/off> [...]\n", s);
	}
	exit(1);
}

int led_main(int argc, char *argv[])
{

	int i;
	int j;
	char *a;

	if ((argc < 3) || ((argc % 2) != 1)) help();

	for (j = 1; j < argc; j += 2) {
		a = argv[j];
		for (i = 0; i < LED_COUNT; ++i) {
			if (strncmp(led_names[i], a, 2) == 0) break;
		}
		a = argv[j + 1];
		if ((i >= LED_COUNT) || ((strcmp(a, "on") != 0) && (strcmp(a, "off") != 0))) help();
		if (!led(i, (a[1] == 'n'))) help();
	}

	return 0;
}
