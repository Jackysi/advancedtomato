/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/

#include "tomato.h"


//	<% nvram("x,y,z"); %>	-> nvram = {'x': '1','y': '2','z': '3'};
void asp_nvram(int argc, char **argv)
{
	char *list;
	char *p, *k;
	const char *v;

	if ((argc != 1) || ((list = strdup(argv[0])) == NULL)) return;
	web_puts("\nnvram = {\n");
	p = list;
	while ((k = strsep(&p, ",")) != NULL) {
		if (*k == 0) continue;
		v = nvram_get(k);
		if (!v) {
			v = "";
		}
		web_printf("\t%s: '", k);
		web_putj(v);
//		web_puts((p == NULL) ? "'\n" : "',\n");
		web_puts("',\n");
	}
	free(list);
	web_puts("\thttp_id: '");
	web_putj(nvram_safe_get("http_id"));
	web_puts("'};\n");
//	web_puts("};\n");
}

// <% nvramseq('foo', 'bar%d', 5, 8); %>	-> foo = ['a','b','c'];
void asp_nvramseq(int argc, char **argv)
{
	int i, e;
	char s[256];

	if (argc != 4) return;

	web_printf("\n%s = [\n", argv[0]);
	e = atoi(argv[3]);
	for (i = atoi(argv[2]); i <= e; ++i) {
		snprintf(s, sizeof(s), argv[1], i);
		web_puts("'");
		web_putj(nvram_safe_get(s));
		web_puts((i == e) ? "'" : "',");
	}
	web_puts("];\n");
}

void asp_nv(int argc, char **argv)
{
	if (argc == 1) {
		web_puts(nvram_safe_get(argv[0]));
	}
}
