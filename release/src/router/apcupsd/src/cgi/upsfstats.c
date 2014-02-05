/*
 *  Program to print the full status output from apcupsd
 *
 *   Kern Sibbald, 17 November 1999
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "cgiconfig.h"
#include "cgilib.h"
#include "upsfetch.h"

#ifndef DEFAULT_REFRESH
#define DEFAULT_REFRESH 30
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

static char   monhost[MAXHOSTNAMELEN] = "127.0.0.1";
static int    refresh = DEFAULT_REFRESH;

void parsearg(const char *var, const char *value) 
{
    if (strcmp(var, "host") == 0) {
        strncpy (monhost, value, sizeof(monhost));
        monhost[sizeof(monhost) - 1] = '\0';

    } else if (strcmp(var, "refresh") == 0) {
        refresh = atoi(value);
        if (refresh < 0) {
            refresh = DEFAULT_REFRESH;
        }
    }
}

int main(int argc, char **argv) 
{
    char   answer[256];

    (void) extractcgiargs();

    html_begin("APCUPSD Full Status Page", refresh);

    if (!checkhost(monhost)) {
        fputs ("<p><strong>Access to host ", stdout);
	html_puts(monhost);
        puts (" is not authorized.</strong></p>");

	html_finish();
	exit (EXIT_FAILURE);
    }

    /* check if host is available */
    if (getupsvar (monhost, "date", answer, sizeof(answer)) < 0)  {
        fputs ("<p><strong>Unable to communicate with the UPS on ", stdout);
	html_puts(monhost);
        puts (".</strong></p>");

	html_finish();
	exit (EXIT_FAILURE);
    }

    fputs ("<blockquote><pre>", stdout);
    html_puts (statbuf);
    puts ("</pre></blockquote>");

    html_finish();
    exit(EXIT_SUCCESS);
}
