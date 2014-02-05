/* cgilib - common routines for CGI programs

   Copyright (C) 1999  Russell Kroll <rkroll@exploits.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cgilib.h"
#include "cgiconfig.h"

/* 
 * Attempts to extract the arguments from QUERY_STRING
 * it does a "callback" to the parsearg() routine for
 * each argument found.
 * 
 * Returns 0 on failure
 *         1 on success
 */
int extractcgiargs()
{
        char    *query, *ptr, *eq, *varname, *value, *amp;

        query = getenv ("QUERY_STRING");
        if (query == NULL)
                return 0;       /* not run as a cgi script! */

        /* varname=value&varname=value&varname=value ... */

        ptr = query;

        while (ptr) {
                varname = ptr;
                eq = strchr (varname, '=');
                if (!eq) {
                        return 0;     /* Malformed string argument */
                }
                
                *eq = '\0';
                value = eq + 1;
                amp = strchr (value, '&');
                if (amp) {
                        ptr = amp + 1;
                        *amp = '\0';
                }
                else
                        ptr = NULL;
        
                parsearg (varname, value);
        }

        return 1;
}

/* 
 * Checks if the host to be monitored is in xxx/hosts.conf
 * Returns:
 *          0 on failure
 *          1 on success (default if not hosts.conf file)
 */
int checkhost(const char *check)
{
    FILE    *hostlist;
    char    fn[256], buf[500], addr[256];

    asnprintf(fn, sizeof(fn), "%s/hosts.conf", SYSCONFDIR);
    hostlist = fopen(fn, "r");
    if (hostlist == NULL) {
        return 1;               /* default to allow */
    }

    while (fgets(buf, (size_t) sizeof(buf), hostlist)) {
        if (strncmp("MONITOR", buf, 7) == 0) {
            sscanf (buf, "%*s %s", addr);
            if (strncmp(addr, check, strlen(check)) == 0) {
                (void) fclose (hostlist);
                return 1;       /* allowed */
            }
        }
    }
    (void) fclose (hostlist);
    return 0;               /* denied */
}       

/*
 * Output a string taking care to assure that any html meta characters
 * are output properly.
 *
 * Note: XHTML added the meta character &apos;, but for backwards compatibility
 * with HTML 4.0, output it as &#39;
 */
void html_puts(const char *str)
{
    const unsigned char *p = (const unsigned char *)str;
    while (*p != '\0') {
        if (*p >= 0x7f) {
            printf ("&#%d;", (int) *p);
        } else {
            switch (*p) {
                case '\"':
                    (void) fputs("&quot;", stdout);
                    break;
                case '&':
                    (void) fputs("&amp;", stdout);
                    break;
                case '\'':
                    (void) fputs("&#39;", stdout);
                    break;
                case '<':
                    (void) fputs("&lt;", stdout);
                    break;
                case '>':
                    (void) fputs("&gt;", stdout);
                    break;
                default:
                    (void) putchar(*p);
                    break;
            }
        }
        p++;
    }
}


/*
 * Print the standard http header, html header which is common to all
 * html pages.
 */
void html_begin(const char *title, int refresh)
{
    int http_version;
    char *server_protocol;
    FILE *hostlist;
    char fn[256], buf[500];

    server_protocol = getenv("SERVER_PROTOCOL");
    if (server_protocol == NULL) {
        http_version = 10;
    } else if (strcmp(server_protocol, "HTTP/1.0") == 0) {
        http_version = 10;
    } else if (strcmp(server_protocol, "HTTP/1.1") == 0) {
        http_version = 11;
    } else {
        http_version = 11;
    }
        
    (void) puts ("Content-Type: text/html; charset=utf-8"); 
    (void) puts ("Content-Language: en");
    if (http_version > 10) {
        (void) puts ("Cache-Control: no-cache");
    } else {
        (void) puts ("Pragma: no-cache");
    }
    (void) puts ("");

    (void) puts("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>");
    (void) puts("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"");
    (void) puts("  \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">");
    (void) puts("<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">");
    (void) puts ("<head>");
    (void) fputs ("<title>", stdout);
    html_puts((char *)title);
    (void) puts ("</title>");

    if (http_version > 10) {
        (void) puts (" <meta http-equiv=\"Cache-Control\" content=\"no-cache\" />");
    }
    /* Always put the Pragma: no-cache tag in the document. Even if the request
     * was received from a HTTP 1.1 proxy, the requesting client may have
     * made the request via HTTP 1.0.
     */
    (void) puts (" <meta http-equiv=\"Pragma\" content=\"no-cache\" />");

    /*
     * Add a generator tag so that when bugs are logged, we know what version
     * of the code we are dealing with.
     */
    (void) fputs (" <meta name=\"generator\" content=\"apcupsd ", stdout);
    html_puts(VERSION);
    (void) puts(", See: http://www.apcupsd.com/\" />");

    if (refresh != 0) {
        printf (" <meta http-equiv=\"Refresh\" content=\"%d\" />\n", refresh);
    }

    asnprintf(fn, sizeof(fn), "%s/apcupsd.css", SYSCONFDIR);
    hostlist = fopen(fn, "r");
    if (hostlist != NULL) {
        (void) puts (" <style>");
        while (fgets(buf, (size_t) sizeof(buf), hostlist)) {
            (void) puts (buf);
        }
        (void) fclose (hostlist);
        (void) puts (" </style>");
    }

    (void) puts ("</head>");
    (void) puts ("<body>");
}

/*
 * Print the standard footer which appears on every html page and close out
 * the html document.
 */
void html_finish(void)
{
#ifdef VALIDATE_HTML
    (void) puts ("<hr /><div><small>");
    (void) puts ("<a href=\"http://jigsaw.w3.org/css-validator/check/referer\">");
    (void) puts ("<img style=\"float:right\" src=\"http://jigsaw.w3.org/css-validator/images/vcss\" alt=\"Valid CSS!\" height=\"31\" width=\"88\"/></a>");
    (void) puts ("<a href=\"http://validator.w3.org/check/referer\">");
    (void) puts ("<img style=\"float:right\" src=\"http://www.w3.org/Icons/valid-xhtml11\" alt=\"Valid XHTML 1.1!\" height=\"31\" width=\"88\"/></a>");
    (void) puts ("</small></div>");
#endif
    (void) puts ("</body></html>");
}
