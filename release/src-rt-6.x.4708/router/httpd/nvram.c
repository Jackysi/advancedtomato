/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#ifndef MAX_NVPARSE
#define MAX_NVPARSE 255
#endif


static int print_wlnv(int idx, int unit, int subunit, void *param)
{
	char *k = param;
	char *nv;

	nv = wl_nvname(k + 3, unit, subunit);
	web_printf("\t'%s': '", nv); // AB multiSSID
	web_putj(nvram_safe_get(nv));
	web_puts("',\n");

	return 1;
}

//	<% nvram("x,y,z"); %>	-> nvram = {'x': '1','y': '2','z': '3'};
void asp_nvram(int argc, char **argv)
{
	char *list;
	char *p, *k;

	if ((argc != 1) || ((list = strdup(argv[0])) == NULL)) return;
	web_puts("\nnvram = {\n");
	p = list;
	while ((k = strsep(&p, ",")) != NULL) {
		if (*k == 0) continue;
		if (strcmp(k, "wl_unit") == 0)
			continue;

		web_printf("\t'%s': '", k); // AB multiSSID
		web_putj(nvram_safe_get(k));
		web_puts("',\n");

		if (strncmp(k, "wl_", 3) == 0) {
			foreach_wif(1, k, print_wlnv);
		}
	}
	free(list);

	web_puts("\t'wl_unit': '"); // AB multiSSID
	web_putj(nvram_safe_get("wl_unit"));
	web_puts("',\n");

	web_puts("\t'http_id': '"); // AB multiSSID
	web_putj(nvram_safe_get("http_id"));
	web_puts("',\n");

	web_puts("\t'web_mx': '"); // AB multiSSID
	web_putj(nvram_safe_get("web_mx"));
	web_puts("',\n");

	web_puts("\t'web_pb': '"); // AB multiSSID
	web_putj(nvram_safe_get("web_pb"));
	web_puts("'};\n");
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

void asp_nvstat(int argc, char **argv)
{
	FILE *fp;
	struct nvram_header header;
	int part, size, used = 0;
	char s[20];

	if (mtd_getinfo("nvram", &part, &size)) {
		sprintf(s, MTD_DEV(%dro), part);

		if ((fp = fopen(s, "r"))) {
#ifdef TCONFIG_NAND
			if ((fread(&header, sizeof(header), 1, fp) == 1) &&
			    (header.magic == NVRAM_MAGIC)) {
				used = header.len;
			}
#else
			if (fseek(fp, size >= NVRAM_SPACE ? size - NVRAM_SPACE : 0, SEEK_SET) == 0) {
				if ((fread(&header, sizeof(header), 1, fp) == 1) &&
				    (header.magic == NVRAM_MAGIC)) {
					used = header.len;
				}
			}
#endif
			fclose(fp);
		}
	}

	web_printf("\nnvstat = { size: %d, free: %d };\n", NVRAM_SPACE, NVRAM_SPACE - used);
}
