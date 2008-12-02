/*
 * Tiny Embedded JavaScript parser
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: ej.c,v 1.9 2005/03/07 08:35:32 kanki Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "httpd.h"

static char * get_arg(char *args, char **next);
//static void call(char *func, FILE *stream);
static void call(char *func, webs_t stream);

/* Look for unquoted character within a string */
static char *
unqstrstr(char *haystack, char *needle)
{
	char *cur;
	int q;

	for (cur = haystack, q = 0;
	     cur < &haystack[strlen(haystack)] && !(!q && !strncmp(needle, cur, strlen(needle)));
	     cur++) {
		if (*cur == '"')
			q ? q-- : q++;
	}
	return (cur < &haystack[strlen(haystack)]) ? cur : NULL;
}

static char *
get_arg(char *args, char **next)
{
	char *arg, *end;

	/* Parse out arg, ... */
	if (!(end = unqstrstr(args, ","))) {
		end = args + strlen(args);
		*next = NULL;
	} else
		*next = end + 1;

	/* Skip whitespace and quotation marks on either end of arg */
	for (arg = args; isspace((int)*arg) || *arg == '"'; arg++);
	for (*end-- = '\0'; isspace((int)*end) || *end == '"'; end--)
		*end = '\0';

	return arg;
}

static void
//call(char *func, FILE *stream)
call(char *func, webs_t stream) //jimmy, https, 8/4/2003
{
	char *args, *end, *next;
	int argc;
	char * argv[16];
	struct ej_handler *handler;

	/* Parse out ( args ) */
	if (!(args = strchr(func, '(')))
		return;
	if (!(end = unqstrstr(func, ")")))
		return;
	*args++ = *end = '\0';

	/* Set up argv list */
	for (argc = 0; argc < 16 && args; argc++, args = next) {
		if (!(argv[argc] = get_arg(args, &next)))
			break;
	}

	/* Call handler */
	for (handler = &ej_handlers[0]; handler->pattern; handler++) {
		//if (strncmp(handler->pattern, func, strlen(handler->pattern)) == 0)
		if (strcmp(handler->pattern, func) == 0)
			handler->output(0, stream, argc, argv);
	}
}

void
//do_ej(char *path, FILE *stream)
do_ej(char *path, webs_t stream)	// jimmy, https, 8/4/2003
{
	FILE *fp;
	int c;
	char pattern[1000], *asp = NULL, *func = NULL, *end = NULL;
	int len = 0;

	if (!(fp = fopen(path, "r")))
		return;

	while ((c = getc(fp)) != EOF) {

		/* Add to pattern space */
		pattern[len++] = c;
		pattern[len] = '\0';
		if (len == (sizeof(pattern) - 1))
			goto release;


		/* Look for <% ... */
		if (!asp && !strncmp(pattern, "<%", len)) {
			if (len == 2)
				asp = pattern + 2;
			continue;
		}

		/* Look for ... %> */
		if (asp) {
			if (unqstrstr(asp, "%>")) {
				for (func = asp; func < &pattern[len]; func = end) {
					/* Skip initial whitespace */
					for (; isspace((int)*func); func++);
					if (!(end = unqstrstr(func, ";")))
						break;
					*end++ = '\0';

					/* Call function */
					call(func, stream);
				}
				asp = NULL;
				len = 0;
			}
			continue;
		}

	release:
		/* Release pattern space */
		//fputs(pattern, stream);
		if(wfputs(pattern, stream) == EOF) break; //jimmy, https, 8/4/2003
		len = 0;
	}

	fclose(fp);
}

int
ejArgs(int argc, char **argv, char *fmt, ...)
{
	va_list	ap;
	int arg;
	char *c;

	if (!argv)
		return 0;

	va_start(ap, fmt);
	for (arg = 0, c = fmt; c && *c && arg < argc;) {
		if (*c++ != '%')
			continue;
		switch (*c) {
		case 'd':
			*(va_arg(ap, int *)) = atoi(argv[arg]);
			break;
		case 's':
			*(va_arg(ap, char **)) = argv[arg];
			break;
		}
		arg++;
	}
	va_end(ap);

	return arg;
}
