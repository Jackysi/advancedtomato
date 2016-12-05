/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"


//#define DEBUG 1

/*
	<% ident(arg, "arg", 'arg'); %>

	Syntax checking is very relaxed and all arguments are considered a
	string. Example, the following are the same:

		<% ident(foo); %>
		<% ident('foo'); %>

*/
int parse_asp(const char *path)
{
	char *buffer;
	char *cp;
	char *a, *b, *c;
	char x;
	int argc;
	char *argv[32];
	char *ident;
	const aspapi_t *api;

	if (f_read_alloc_string(path, &buffer, 128 * 1024) < 0) {
		free(buffer);
		if (!header_sent) send_error(500, NULL, "Requested file could not be read!");
		return 0;
	}

	if (!header_sent) send_header(200, NULL, mime_html, 0);

	// <% id(arg, arg); %>
	cp = buffer;
	while (*cp) {
		if ((b = strstr(cp, "%>")) == NULL) {
			web_puts(cp);
			break;
		}
		*b = 0;

		//xx <% <% %>
		//xx %>

		a = cp;
		while ((c = strstr(a, "<%")) != NULL) {
			a = c + 2;
		}

		if (a == cp) {
			*b = '%';
			b += 2;
			web_write(cp, b - cp);
			cp = b;
			continue;
		}

		web_write(cp, (a - cp) - 2);

		cp = b + 2;

		while (*a == ' ') ++a;
		ident = a;
		while (((*a >= 'a') && (*a <= 'z')) || ((*a >= 'A') && (*a <= 'Z')) || ((*a >= '0') && (*a <= '9')) || (*a == '_')) {
			++a;
		}
		if (ident == a) {
#ifdef DEBUG
			syslog(LOG_WARNING, "Identifier not found in %s @%u", path, a - buffer);
#endif
			continue;
		}
		b = a;
		while (*a == ' ') ++a;
		if (*a++ != '(') {
#ifdef DEBUG
			syslog(LOG_WARNING, "Expecting ( in %s @%u", path, a - buffer);
#endif
			continue;
		}
		*b = 0;

		// <% foo(123, "arg"); %>
		// a -----^            ^--- null

//		printf("\n[[['%s'\n", ident);

		argc = 0;
		while (*a) {
			while (*a == ' ') ++a;
			if (*a == ')') {
FINAL:
				++a;
				while ((*a == ' ') || (*a == ';')) ++a;
				if (*a != 0) break;

				for (api = aspapi; api->name; ++api) {
					if (strcmp(api->name, ident) == 0) {
						api->exec(argc, argv);
						break;
					}
				}

				a = NULL;
/*
				int z;
				for (z = 0; z < argc; ++z) {
					printf(" %d '%s'\n", z, argv[z]);
				}
*/
				break;
			}

			if (argc >= 32) {
#ifdef DEBUG
				syslog(LOG_WARNING, "Error while parsing arguments in %s @%u", path, a - buffer);
#endif
				break;
			}

			if ((*a == '"') || (*a == '\'')) {
				x = *a;
				argv[argc++] = a + 1;
				while ((*++a != x) && (*a != 0)) {
					if (*a == '\\') {
						if (*++a == 0) break;
						*(a - 1) = *a;
					}
				}
				if (*a == 0) break;
				*a++ = 0;
			}
			else {
				argv[argc++] = a;
				while ((*a != ',') && (*a != ')') && (*a != ' ') && (*a != 0)) ++a;
			}
			while (*a == ' ') ++a;
			if (*a == ')') {
				*a = 0;
				goto FINAL;
			}
			if (*a != ',') break;
			*a++ = 0;
		}

#ifdef DEBUG
		if (a != NULL) syslog(LOG_WARNING, "Error while parsing arguments in %s @%u", path, a - buffer);
#endif

//		printf("argc=%d]]]\n", argc);
	}


	free(buffer);
	return 1;
}

void wo_asp(char *path)
{
	parse_asp(path);
}
