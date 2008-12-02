/*

	NVRAM Utility
	Copyright (C) 2006-2008 Jonathan Zarate

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>

#include <bcmdevs.h>
#include <bcmnvram.h>
#include <utils.h>
#include <shutils.h>
#include <shared.h>

#include "nvram_convert.h"
#include "defaults.h"


__attribute__ ((noreturn))
static void help(void)
{
	printf(
		"NVRAM Utility\n"
		"Copyright (C) 2006-2008 Jonathan Zarate\n\n"	
		"Usage: nvram set <key=value> | get <key> | unset <key> | "
		"ren <key> <key> | commit | show [--nosort|--nostat] | "
		"find <text> | defaults <--yes|--initcheck> | backup <filename> | "
		"restore <filename> [--test] [--force] [--nocommit] | "
		"export <--c|--dump|--dump0|--set|--tab>"
//		"test"
		"\n");
	exit(1);
}

static void getall(char *buffer)
{
	if (nvram_getall(buffer, NVRAM_SPACE) != 0) {
		fprintf(stderr, "Error reading NVRAM\n");
		exit(1);
	}
}

static int set_main(int argc, char **argv)
{
	char *b, *p;

	if ((b = strdup(argv[1])) == NULL) {
		fprintf(stderr, "Not enough memory");
		return 1;
	}
	if ((p = strchr(b, '=')) != NULL) {
		*p = 0;
		nvram_set(b, p + 1);
		return 0;
	}
	help();
}

static int get_main(int argc, char **argv)
{
	char *p;

	if ((p = nvram_get(argv[1])) != NULL) {
		puts(p);
		return 0;
	}
	return 1;
}

static int unset_main(int argc, char **argv)
{
	nvram_unset(argv[1]);
	return 0;
}

static int ren_main(int argc, char **argv)
{
	char *p;

	if ((p = nvram_get(argv[1])) == NULL) {
		fprintf(stderr, "Unable to find %s\n", argv[1]);
		return 1;
	}
	if (strcmp(argv[1], argv[2]) != 0) {
		nvram_set(argv[2], p);
		nvram_unset(argv[1]);
	}
	return 0;
}

static int show_main(int argc, char **argv)
{
	char *p, *q;
	char buffer[NVRAM_SPACE];
	int n;
	int count;
	int show = 1;
	int stat = 1;
	int sort = 1;

	for (n = 1; n < argc; ++n) {
        if (strcmp(argv[n], "--nostat") == 0) stat = 0;
			else if (strcmp(argv[n], "--nosort") == 0) sort = 0;
				else help();
	}

	if (sort) {
		system("nvram show --nostat --nosort | sort");	// smallest and easiest way :)
		show = 0;
	}

	getall(buffer);
	count = 0;
	for (p = buffer; *p; p += strlen(p) + 1) {
		q = p;
		while (*q) {
			if (!isprint(*q)) *q = ' ';
			++q;
		}
		if (show) puts(p);
		++count;
	}
	if (stat) {
		n = sizeof(struct nvram_header) + (p - buffer);
		printf("---\n%d entries, %d bytes used, %d bytes free.\n", count, n, NVRAM_SPACE - n);
	}
	return 0;
}

static int find_main(int argc, char **argv)
{
	char cmd[512];
	int r;

	snprintf(cmd, sizeof(cmd), "nvram show --nostat --nosort | sort | grep \"%s\"", argv[1]);
	r = system(cmd);
	return (r == -1) ? 1 : WEXITSTATUS(r);
}

static int defaults_main(int argc, char **argv)
{
	const defaults_t *t;
	char *p;
	char s[256];
	int i;
	int force = 0;
	int commit = 0;

	if (strcmp(argv[1], "--yes") == 0) {
		force = 1;
	}
	else if (strcmp(argv[1], "--initcheck") != 0) {
		help();
	}

	if ((!nvram_match("restore_defaults", "0")) || (!nvram_match("os_name", "linux"))) {
		force = 1;
	}

#if 0	// --need to test--
	// prevent lockout if upgrading from DD-WRT v23 SP2+ w/ encrypted password
	if (nvram_match("nvram_ver", "2")) {
		nvram_unset("nvram_ver");
		
		// If username is "", root or admin, then nvram_ver was probably a left-over
		// from an old DD-WRT installation (ex: DD-WRT -> Linksys (reset) ->
		// Tomato) and we don't need to do anything. Otherwise, reset.
		if ((p = nvram_get("httpd_username")) != NULL) {
			if ((*p != 0) && (strcmp(p, "root") != 0) && (strcmp(p, "admin") != 0)) {
				nvram_unset("httpd_passwd");
				nvram_unset("httpd_username");	// not used here, but dd will try to re-encrypt this
				// filled below
			}
		}
	}
#else
	if (force) nvram_unset("nvram_ver");	// prep to prevent problems later
#endif


	for (t = defaults; t->key; t++) {
		if (((p = nvram_get(t->key)) == NULL) || (force)) {
			nvram_set(t->key, t->value);
			commit = 1;
//			if (!force) cprintf("SET %s=%s\n", t->key, t->value);
		}
		else if (strncmp(t->key, "wl_", 3) == 0) {
			// sync wl_ and wl0_
			strcpy(s, "wl0_");
			strcat(s, t->key + 3);
			if (nvram_get(s) == NULL) nvram_set(s, nvram_safe_get(t->key));
		}
	}

	// todo: moveme
	if ((strtoul(nvram_safe_get("boardflags"), NULL, 0) & BFL_ENETVLAN) ||
		(check_hw_type() == HW_BCM4712)) t = if_vlan;
			else t = if_generic;
	for (; t->key; t++) {
		if (((p = nvram_get(t->key)) == NULL) || (*p == 0) || (force)) {
			nvram_set(t->key, t->value);
			commit = 1;
//			if (!force) cprintf("SET %s=%s\n", t->key, t->value);
		}
	}

	if (force) {
		for (i = 0; i < 20; i++) {
			sprintf(s, "wl0_wds%d", i);
			nvram_unset(s);
		}
		for (i = 0; i < LED_COUNT; ++i) {
			sprintf(s, "led_%s", led_names[i]);
			nvram_unset(s);
		}
		
		// 0 = example
		for (i = 1; i < 50; i++) {
			sprintf(s, "rrule%d", i);
			nvram_unset(s);
		}
	}

	if (!nvram_match("boot_wait", "on")) {
		nvram_set("boot_wait", "on");
		commit = 1;
	}

	nvram_set("os_name", "linux");
	nvram_set("os_version", tomato_version);
	nvram_set("os_date", tomato_buildtime);

/*	if (nvram_match("dirty", "1")) {
		nvram_unset("dirty");
		commit = 1;
	}*/

	if ((commit) || (force)) {
		printf("Saving...\n");
		nvram_commit();
	}
	else {
		printf("No change was necessary.\n");
	}
	return 0;
}

static int commit_main(int argc, char **argv)
{
	int r;

	printf("Commit... ");
	fflush(stdout);
	r = nvram_commit();
	printf("done.\n");
	return r ? 1 : 0;
}

#define X_C			0
#define X_SET		1
#define X_TAB		2

static int export_main(int argc, char **argv)
{
	char *p;
	char buffer[NVRAM_SPACE];
	int eq;
	int mode;

	getall(buffer);
	p = buffer;

	if (strcmp(argv[1], "--dump") == 0) {
		for (p = buffer; *p; p += strlen(p) + 1) {
			puts(p);
		}
		return 0;
	}
	if (strcmp(argv[1], "--dump0") == 0) {
		for (p = buffer; *p; p += strlen(p) + 1) { }
		fwrite(buffer, p - buffer, 1, stdout);
		return 0;
	}

	if (strcmp(argv[1], "--c") == 0) mode = X_C;
	else if (strcmp(argv[1], "--set") == 0) mode = X_SET;
	else if (strcmp(argv[1], "--tab") == 0) mode = X_TAB;
	else help();

	while (*p) {
		eq = 0;
		if (mode != X_TAB) printf((mode == X_SET) ? "nvram set \"" : "{ \"");
		do {
			switch (*p) {
			case 9:
				printf("\\t");
				break;
			case 13:
				printf("\\r");
				break;
			case 10:
				printf("\\n");
				break;
			case '"':
			case '\\':
				printf("\\%c", *p);
				break;
			case '=':
				if ((eq == 0) && (mode != X_SET)) {
					printf((mode == X_C) ? "\", \"" : "\t");
					break;
				}
				eq = 1;
			default:
				if (!isprint(*p)) printf("\\x%02x", *p);
					else putchar(*p);
				break;
			}
			++p;
		} while (*p);
		if (mode != X_TAB) printf((mode == X_SET) ? "\"" : "\" },");
		putchar('\n');
		++p;
	}
	return 0;
}

#define V1	0x31464354L

typedef struct {
	unsigned long sig;
	unsigned long hwid;
	char buffer[NVRAM_SPACE];
} backup_t;

static int backup_main(int argc, char **argv)
{
	backup_t data;
	unsigned int size;
	char *p;
	char s[512];
	char tmp[128];
	int r;

	getall(data.buffer);

	data.sig = V1;
	data.hwid = check_hw_type();

	p = data.buffer;
	while (*p != 0) p += strlen(p) + 1;

	size = (sizeof(data) - sizeof(data.buffer)) + (p - data.buffer) + 1;

	strcpy(tmp, "/tmp/nvramXXXXXX");
	mktemp(tmp);
	if (f_write(tmp, &data, size, 0, 0) != size) {
		printf("Error saving file.\n");
		return 1;
	}
	sprintf(s, "gzip < %s > %s", tmp, argv[1]);
	r = system(s);
	unlink(tmp);

	if (r != 0) {
		unlink(argv[1]);
		printf("Error compressing file.\n");
		return 1;
	}

	printf("Saved.\n");
	return 0;
}

static int restore_main(int argc, char **argv)
{
	char *name;
	int test;
	int force;
	int commit;
	backup_t data;
	unsigned int size;
	char s[512];
	char tmp[128];
	unsigned long hw;
	char current[NVRAM_SPACE];
	char *b, *bk, *bv;
	char *c, *ck, *cv;
	int nset;
	int nunset;
	int nsame;
	int cmp;
	int i;

	test = 0;
	force = 0;
	commit = 1;
	name = NULL;
	for (i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			if (strcmp(argv[i], "--test") == 0) {
				test = 1;
			}
			else if (strcmp(argv[i], "--force") == 0) {
				force = 1;
			}
			else if (strcmp(argv[i], "--nocommit") == 0) {
				commit = 0;
			}
			else {
				help();
			}
		}
		else {
			name = argv[i];
		}
	}
	if (!name) help();

	strcpy(tmp, "/tmp/nvramXXXXXX");
	mktemp(tmp);
	sprintf(s, "gzip -d < %s > %s", name, tmp);
	if (system(s) != 0) {
		unlink(tmp);
		printf("Error decompressing file.\n");
		return 1;
	}

	size = f_size(tmp);
	if ((size <= (sizeof(data) - sizeof(data.buffer))) || (size > sizeof(data)) ||
		(f_read(tmp, &data, sizeof(data)) != size)) {
		unlink(tmp);
		printf("Invalid data size or read error.\n");
		return 1;
	}

	unlink(tmp);

	if (data.sig != V1) {
		printf("Invalid signature: %08lX / %08lX\n", data.sig, V1);
		return 1;
	}

	hw = check_hw_type();
	if ((data.hwid != hw) && (!force)) {
		printf("Invalid hardware type: %08lX / %08lX\n", data.hwid, hw);
		return 1;
	}

	// 1 - check data

	size -= sizeof(data) - sizeof(data.buffer);
	if ((data.buffer[size - 1] != 0) || (data.buffer[size - 2] != 0)) {
CORRUPT:
		printf("Corrupted data area.\n");
		return 1;
	}

	b = data.buffer;
	while (*b) {
		bk = b;
		b += strlen(b) + 1;
		if ((bv = strchr(bk, '=')) == NULL) {
			goto CORRUPT;
		}
		*bv = 0;
		if (strcmp(bk, "et0macaddr") == 0) {
			if (!nvram_match(bk, bv + 1)) {
				if (!force) {
					printf("Cannot restore on a different router.\n");
					return 1;
				}
			}
		}
		*bv = '=';
	}
	if (((b - data.buffer) + 1) != size) {
		printf("Extra data found at the end.\n");
		return 1;
	}


	// 2 - set

	if (!test) {
		if (!wait_action_idle(10)) {
			printf("System busy.\n");
			return 1;
		}
		set_action(ACT_SW_RESTORE);
		led(LED_DIAG, 1);
	}

	nset = nunset = nsame = 0;

	b = data.buffer;
	while (*b) {
		bk = b;
		b += strlen(b) + 1;
		bv = strchr(bk, '=');
		*bv++ = 0;

		if (!nvram_match(bk, bv)) {
			if (test) printf("nvram set \"%s=%s\"\n", bk, bv);
				else nvram_set(bk, bv);
			++nset;
		}
		else {
			++nsame;
		}

		*(bv - 1) = '=';
	}


	// 3 - unset

	getall(current);
	c = current;
	while (*c) {
		ck = c;
		c += strlen(c) + 1;
		if ((cv = strchr(ck, '=')) == NULL) {
			printf("Invalid data in NVRAM: %s.", ck);
			continue;
		}
		*cv++ = 0;

		cmp = 1;
		b = data.buffer;
		while (*b) {
			bk = b;
			b += strlen(b) + 1;
			bv = strchr(bk, '=');
			*bv++ = 0;
			cmp = strcmp(bk, ck);
			*(bv - 1) = '=';
			if (cmp == 0) break;
		}
		if (cmp != 0) {
			++nunset;
			if (test) printf("nvram unset \"%s\"\n", ck);
				else nvram_unset(ck);
		}
	}

	if ((nset == 0) && (nunset == 0)) commit = 0;
	printf("\nPerformed %d set and %d unset operations. %d required no changes.\n%s\n",
		nset, nunset, nsame, commit ? "Committing..." : "Not commiting.");
	fflush(stdout);

	if (!test) {
		set_action(ACT_IDLE);
		if (commit) nvram_commit();
	}
	return 0;
}

#if 0
static int test_main(int argc, char **argv)
{
/*
	static const char *extra[] = {
		"clkfreq", "pa0b0", "pa0b1", "pa0b2", "pa0itssit", "pa0maxpwr",
		"sdram_config", "sdram_init", "sdram_ncdl", "vlan0ports", NULL };
	const char **x;
*/
	char buffer[NVRAM_SPACE];
	char *k, *v, *e;
	const defaults_t *rest;
	struct nvram_convert *conv;
	
	printf("Unknown keys:\n");

	getall(buffer);
	k = buffer;
	while (*k) {
		e = k + strlen(k) + 1;
		if ((v = strchr(k, '=')) != NULL) {
			*v = 0;
			for (rest = defaults; rest->key; ++rest) {
				if (strcmp(k, rest->key) == 0) break;
			}
			if (rest->key == NULL) {
				for (conv = nvram_converts; conv->name; ++conv) {
					if ((strcmp(k, conv->name) == 0) || (strcmp(k, conv->wl0_name) == 0)) break;
				}
				if (conv->name == NULL) {
					printf("%s=%s\n", k, v + 1);
/*				
					for (x = extra; *x; ++x) {
						if (strcmp(k, *x) == 0) break;
					}
					if (*x == NULL) {
						printf("nvram unset \"%s\"\n", k);
					}
*/
				}
			}
		}
		else {
			printf("WARNING: '%s' doesn't have a '=' delimiter\n", k);
		}
		k = e;
	}

	return 0;
}
#endif


// -----------------------------------------------------------------------------

typedef struct {
	const char *name;
	int args;
	int (*main)(int argc, char *argv[]);
} applets_t;

static const applets_t applets[] = {
	{ "set",		3,	set_main		},
	{ "get",		3,	get_main		},
	{ "unset",		3,	unset_main		},
	{ "ren",		4,	ren_main		},
	{ "show",		-2,	show_main		},
	{ "commit",		2,	commit_main		},
	{ "find",		3,	find_main		},
	{ "export",		3,	export_main		},
	{ "defaults",	3,	defaults_main	},
	{ "backup",		3,	backup_main		},
	{ "restore",	-3,	restore_main	},
//	{ "test",		2,	test_main		},
	{ NULL, 		0,	NULL			}
};

int main(int argc, char **argv)
{
	const applets_t *a;

	if (argc >= 2) {
		a = applets;
		while (a->name) {
			if (strcmp(argv[1], a->name) == 0) {
				if ((argc != a->args) && ((a->args > 0) || (argc < -(a->args)))) help();
				return a->main(argc - 1, argv + 1);
			}
			++a;
		}
	}
	help();
}
