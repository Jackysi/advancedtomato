/* multimon - CGI program to monitor several UPSes from one page

   Copyright (C) 1998  Russell Kroll <rkroll@exploits.org>

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
#include <sys/param.h>

#include "upsfetch.h"
#include "cgiconfig.h"
#include "cgilib.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN 256
#endif

#ifndef DEFAULT_REFRESH
#define DEFAULT_REFRESH 30
#endif

static void do_model (const char *monhost, const char *suffix);
static void do_system (const char *monhost, const char *suffix);
static void do_fulldata (const char *monhost, const char *suffix);
static void do_status (const char *monhost, const char *suffix);
static void do_upstemp (const char *monhost, const char *suffix);
static void do_upstempc (const char *monhost, const char *suffix);
static void do_upstempf (const char *monhost, const char *suffix);
static void do_humidity (const char *monhost, const char *suffix);
static void do_ambtemp (const char *monhost, const char *suffix);
static void do_ambtempc (const char *monhost, const char *suffix);
static void do_ambtempf (const char *monhost, const char *suffix);
static void do_utility (const char *monhost, const char *suffix);

static const struct {
        const char *name;
        void    (*func)(const char *monhost, const char *suffix);
}       fields[] =
{
        { "MODEL",      do_model                },
        { "SYSTEM",     do_system               },
        { "STATUS",     do_status               },
        { "DATA",       do_fulldata             },
        { "UPSTEMP",    do_upstemp              },
        { "UPSTEMPC",   do_upstempc             },
        { "UPSTEMPF",   do_upstempf             },
        { "HUMIDITY",   do_humidity             },
        { "AMBTEMP",    do_ambtemp              },
        { "AMBTEMPC",   do_ambtempc             },
        { "AMBTEMPF",   do_ambtempf             },
        { "UTILITY",    do_utility              },
        { NULL,         (void(*)(const char *, const char *))(NULL)       }
};

static const struct {
        const char    *name;
        const char    *desc;
        const int     severity;
}       stattab[] =
{
        { "OFF",        "OFF",                  1       },
        { "OL",         "ONLINE",               0       },
        { "OB",         "ON BATTERY",           2       },
        { "LB",         "LOW BATTERY",          2       },
        { "RB",         "REPLACE BATTERY",      2       },
        { "NB",         "NO BATTERY",           2       },
        { "OVER",       "OVERLOAD",             2       },
        { "TRIM",       "VOLTAGE TRIM",         1       },
        { "BOOST",      "VOLTAGE BOOST",        1       },
        { "CAL",        "CALIBRATION",          1       },
        { "CL",         "COMM LOST",            2       },
        { "SD",         "SHUTTING DOWN",        2       },
        { "SLAVE",      "SLAVE",                0       },
        { NULL,         NULL,                   0       }
};

struct ftype_t;

struct ftype_t {
        const char      *var;
        const char      *name;
        const char      *suffix;
        ftype_t         *next;
};        

static  ftype_t *firstfield = NULL;
static  int     numfields = 0;
static  int     use_celsius;
static  char    *desc;
static  int     refresh = DEFAULT_REFRESH;

void parsearg(const char *var, const char *value)
{
    if (strcmp(var, "refresh") == 0) {
        refresh = atoi(value);
        if (refresh < 0) {
            refresh = DEFAULT_REFRESH;
        }
    }
}

static void report_error(const char *str) 
{
    printf("<p>%s</p>\n", str);
    html_finish();
    exit(EXIT_FAILURE);
}

/* handler for "MODEL" */
static void do_model (const char *monhost, const char *suffix)
{
    char    model[256];

    if (getupsvar(monhost, "model", model, sizeof(model)) < 0) {
        (void) puts ("<td class=\"Empty\">-</td>");

    } else {
        fputs ("<td class=\"Label\">", stdout);
        html_puts(model);
        (void) puts("</td>");
    }
}

/* handler for "SYSTEM" */
static void do_system(const char *monhost, const char *suffix)
{
    /* provide system name and link to upsstats -> link removed, R. Morris (Sep 18/11) */
    printf ("<td class=\"Label\">%s</td>\n", desc);
/*    printf ("<td class=\"Label\">");
    printf ("<a href=\"upsstats.cgi?host=%s&amp;temp=%c\">%s</a></td>\n",
        monhost, use_celsius ? 'C' : 'F', desc);*/
}

/* handler for "STATUS" */
static void do_status(const char *monhost, const char *suffix)
{
    char    status[64], *stat, stattxt[128], temp[128];
    const char *class_type;
    int     i, severity;
    long    ups_status;

    if (getupsvar (monhost, "status", status, sizeof(status)) <= 0) {
       printf ("<td class=\"Fault\">Unavailable</td>\n");
       return;
    }

    stattxt[0] = '\0';
    severity = 0;
    ups_status = strtol(status, 0, 16);
    status[0] = '\0';

    if (ups_status & UPS_calibration) 
       strcat(status, "CAL ");
    if (ups_status & UPS_trim)
       strcat(status, "TRIM ");
    if (ups_status & UPS_boost)
       strcat(status, "BOOST ");
    if (ups_status & UPS_online)
       strcat(status, "OL ");
    if (ups_status & UPS_onbatt) 
       strcat(status, "OB ");
    if (ups_status & UPS_overload)
       strcat(status, "OVER ");
    if (ups_status & UPS_battlow) 
       strcat(status, "LB ");
    if (ups_status & UPS_replacebatt)
       strcat(status, "RB ");
    if (!(ups_status & UPS_battpresent))
       strcat(status, "NB ");
    if (ups_status & UPS_commlost)
       strcat(status, "CL ");
    if (ups_status & UPS_shutdown)
       strcat(status, "SD ");
    if (ups_status & UPS_slave)
       strcat(status, "SLAVE ");

    stat = strtok (status, " ");
    while (stat != NULL) {  
        for (i = 0; stattab[i].name != NULL; i++) {
                if (strcmp(stattab[i].name, stat) == 0) {
                        snprintf (temp, sizeof(temp), "%s %s<br />",
                                  stattxt, stattab[i].desc);
                        snprintf (stattxt, sizeof(stattxt), "%s",
                                  temp);
                        if (stattab[i].severity > severity)
                                severity = stattab[i].severity;
                }
        }
        stat = strtok (NULL, " ");
    }

    switch (severity) {
        case 0:
            class_type = "Normal";
            break;

        case 1:
            class_type = "Warning";
            break;

        case 2:
        default:
            class_type = "Fault";
            break;
    }
    printf ("<td class=\"%s\">%s</td>\n", class_type, stattxt);
}

/* handler for "DATA" */
static void do_fulldata (const char *monhost, const char *suffix)
{
    printf ("<td class=\"Label\">");
    printf ("<a href=\"upsfstats.cgi?host=%s\">%s</a></td>\n", monhost, suffix);
}

/* handler for "UPSTEMPC" */
static void do_upstempc(const char *monhost, const char *suffix)
{
    char    upstemp[64];

    if (getupsvar(monhost, "upstemp", upstemp, sizeof(upstemp)) > 0) {
        printf ("<td class=\"Normal\">%s&deg; C</td>\n", upstemp); 

    } else {
        (void) puts ("<td class=\"Empty\">-</td>");
    }
}

/* handler for "UPSTEMPF" */
static void do_upstempf(const char *monhost, const char *suffix)
{
    char    upstemp[64];
    float   tempf;

    if (getupsvar(monhost, "upstemp", upstemp, sizeof(upstemp)) > 0) {
        tempf = (strtod (upstemp, 0) * 1.8) + 32;
        printf ("<td class=\"Normal\">%.1f&deg; F</td>\n", tempf); 

    } else {
        (void) puts ("<td class=\"Empty\">-</td>");
    }
}

/* handler for "UPSTEMP" */
static void do_upstemp(const char *monhost, const char *suffix)
{
    if (use_celsius) {
        do_upstempc(monhost, suffix);

    } else {
        do_upstempf(monhost, suffix);
    }
}

/* handler for "HUMIDITY" */
static void do_humidity(const char *monhost, const char *suffix)
{
    char        humidity[64];
    float       ambhum;

    if (getupsvar (monhost, "humidity", humidity, sizeof(humidity)) > 0) {
        ambhum = strtod (humidity, 0);
        printf ("<td class=\"Normal\">%.1f %%</td>\n", ambhum);

    } else {
         (void) puts ("<td class=\"Empty\">-</td>");
    }
}

/* handler for "AMBTEMPC" */
static void do_ambtempc(const char *monhost, const char *suffix)
{
        char    ambtemp[64];

    if (getupsvar (monhost, "ambtemp", ambtemp, sizeof(ambtemp)) > 0) {
        printf ("<td class=\"Normal\">%s&deg; C</td>\n", ambtemp); 

    } else {
        (void) puts ("<td class=\"Empty\">-</td>");
    }
}

/* handler for "AMBTEMPF" */
static void do_ambtempf (const char *monhost, const char *suffix)
{
        char    ambtemp[64];
        float   tempf;

    if (getupsvar (monhost, "ambtemp", ambtemp, sizeof(ambtemp)) > 0) {
        tempf = (strtod (ambtemp, 0) * 1.8) + 32;
        printf ("<td class=\"Normal\">%.1f&deg; F</td>\n", tempf); 

    } else {
        (void) puts ("<td class=\"Empty\">-</td>");
    }
}

/* handler for "AMBTEMP" */
static void do_ambtemp(const char *monhost, const char *suffix)
{
    if (use_celsius) {
        do_ambtempc(monhost, suffix);

    } else {
        do_ambtempf(monhost, suffix);
    }
}

/* handler for "UTILITY" */
static void do_utility(const char *monhost, const char *suffix)
{
    char utility[64], lowxfer[64], highxfer[64];
    int  lowx, highx, util;

    if (getupsvar (monhost, "utility", utility, sizeof(utility)) > 0) {
        /* try to get low and high transfer points for color codes */

        lowx = highx = 0;

        if (getupsvar (monhost, "lowxfer", lowxfer, sizeof(lowxfer)) > 0)
            lowx = atoi(lowxfer);
        if (getupsvar (monhost, "highxfer", highxfer, sizeof(highxfer)) > 0)
            highx = atoi(highxfer);

        printf ("<td class=\"");

        /* only do this if we got both values */
        if ((lowx != 0) && (highx != 0)) {
            util = atoi(utility);

            if ((util < lowx) || (util > highx))
                printf ("Fault");
            else
                printf ("Normal");
        }
        else
            printf ("Normal");

        printf ("\">%s VAC</td>\n", utility);

    } else {
        (void) puts ("<td class=\"Empty\">-</td>");
    }
}

/* Get and print information for requested host */
static void getinfo(char *monhost)
{
    ftype_t   *tmp;
    char    tmpbuf[256];
    int     i, found;

//    (void) puts ("<tr align=\"center\">");

    /* grab a dummy variable to see if the host is up */
    if (getupsvar(monhost, "date", tmpbuf, sizeof(tmpbuf)) <= 0) {
        printf ("<td class=\"Fault\">%s</td>\n", desc);
        printf ("<td colspan=\"%d\" class=\"Fault\">Not available: %s</td></tr>\n",
                 numfields - 1, errmsg);
        return;
    }

    /* process each field one by one */
    for (tmp = firstfield; tmp != NULL; tmp = tmp->next) {
        found = 0;

        /* search for a recognized special field name */
        for (i = 0; fields[i].name != NULL; i++) {
                if (strcmp(fields[i].name, tmp->var) == 0) {
                        fields[i].func(monhost, tmp->suffix);
                        found = 1;
                }
        }

        if (found)
            continue;

        if (getupsvar(monhost, tmp->var, tmpbuf, sizeof(tmpbuf)) > 0) {
            if (tmp->suffix == NULL) {
                printf ("<td class=\"Normal\">%s</td>\n", tmpbuf);
            } else {
                printf ("<td class=\"Normal\">%s %s</td>\n", 
                                tmpbuf, tmp->suffix);
            }
        } else {
           (void) puts ("<td class=\"Empty\">-</td>");
        }
    }
    (void) puts ("</tr>");
}

/* add a field to the linked list */
static void addfield(const char *var, const char *name, const char *suffix)
{
    ftype_t   *tmp, *last;

    tmp = last = firstfield;

    while (tmp != NULL) {
        last = tmp;
        tmp = tmp->next;
    }

    tmp = (ftype_t *)malloc (sizeof (ftype_t));
    tmp->var = var;
    tmp->name = name;
    tmp->suffix = suffix;
    tmp->next = NULL;

    if (last == NULL) {
        firstfield = tmp;
    } else {
        last->next = tmp;
    }
    numfields++;
}

/* parse a FIELD line from the buf and call addfield */
static void parsefield(char *buf)
{
    char *ptr, *var, *name = NULL, *suffix = NULL, *tmp;
    int i = 0, in_string = 0;
    int len = strlen(buf) + 1;
    
    tmp = (char *)malloc(len);

    /* <variable> "<field name>" "<field suffix>" */

    ptr = strtok(buf, " ");
    if (ptr == NULL) {
        report_error("multimon.conf: "
                      "No separating space in FIELD line.");
    }
    var = strdup(ptr);

    ptr = buf + strlen(var) + 1;
    while (*ptr) {
        if (*ptr == '"') {
            in_string = !in_string;
            if (!in_string) {
                tmp[i] = '\0';
                i = 0;
                if (suffix) {
                        snprintf(tmp, len, "multimon.conf: "
                                "More than two strings "
                                "in field %s.", var);
                        report_error(tmp);
                }
                else if (name) {
                        suffix = strdup(tmp);
                }
                else {
                        name = strdup(tmp);
                }
            }
        } else if (in_string) {
            if (*ptr == '\\') {
                ++ptr;
                if (*ptr == '\0') {
                        snprintf(tmp, len, "multimon.conf: "
                                "Backslash at end of line "
                                "in field %s.", var);
                        report_error(tmp);
                }
            }
            tmp[i++] = *ptr;
        }
        ++ptr;
    }

    if (in_string) {
            snprintf(tmp, len, "multimon.conf: "
                    "Unbalanced quotes in field %s!", var);
            report_error(tmp);
    }

    free(tmp);

    addfield(var, name, suffix);
}       

static void readconf(void)
{
    FILE    *conf;
    char    buf[512], fn[MAXPATHLEN];

    snprintf (fn, sizeof(fn), "%s/multimon.conf", SYSCONFDIR);
    conf = fopen (fn, "r");

    /* the config file is not required */
    if (conf == NULL)
        return;

    while (fgets (buf, sizeof(buf), conf)) {
        buf[strlen(buf) - 1] = '\0';

        if (strncmp (buf, "FIELD", 5) == 0) {
             parsefield (&buf[6]);

        } else if (strncmp (buf, "TEMPC", 5) == 0) {
             use_celsius = 1;

        } else if (strncmp (buf, "TEMPF", 5) == 0) {
             use_celsius = 0;
        }
    }
    fclose (conf);
}       

/* create default field configuration */
static void defaultfields(void)
{
    addfield ("SYSTEM", "System", "");
    addfield ("MODEL", "Model", "");
    addfield ("STATUS", "Status", "");
    addfield ("battpct", "Batt Chg", "%");
    addfield ("UTILITY", "Utility", "VAC");
    addfield ("loadpct", "UPS Load", "%");
    addfield ("UPSTEMP", "UPS Temperature", "");
//    addfield ("DATA",  "Data", "All data");
}       

int main(int argc, char **argv) 
{
    FILE    *conf;
//    time_t  tod;
//    char    buf[256], fn[MAXPATHLEN], addr[256], timestr[256], *rest;
    char    buf[256], fn[MAXPATHLEN], addr[256], *rest;
    int     restofs;
    ftype_t   *tmp;

    /* set default according to compile time, but config may override */
#ifdef USE_CELSIUS
    use_celsius = 1;
#else
    use_celsius = 0;
#endif
    readconf();
    if (firstfield == NULL)         /* nothing from config file? */
            defaultfields();

    (void) extractcgiargs();
//    html_begin("Multimon: UPS Status Page", refresh);
    printf ("<table class=\"tomato-grid\">\n");

//    time (&tod);
//    strftime (timestr, sizeof(timestr), "%a %b %d %X %Z %Y", localtime(&tod));
//    printf ("<tr><th class=\"tomato\" colspan=\"%d\">\n", numfields);

    /* print column names */
//    printf ("<tr>\n");
    printf ("<tr class=\"header\">\n");
    for (tmp = firstfield; tmp != NULL; tmp = tmp->next)
        printf ("<td class=\"header\">%s</td>\n", tmp->name);
    (void) puts ("</tr>"); 

    /* ups status */
    printf ("<tr class=\"even\">\n");
    snprintf (fn, sizeof(fn), "%s/hosts.conf", SYSCONFDIR);
    conf = fopen (fn, "r");
    if (conf == NULL) {
        printf ("<td colspan=\"%d\" class=\"Fault\">Error: Cannot open hosts file (%s/hosts.conf).</td>\n",
                   numfields, SYSCONFDIR);
    } else {
        while (fgets (buf, sizeof(buf), conf)) {
            if (strncmp("MONITOR", buf, 7) == 0) {
                sscanf (buf, "%*s %s %n", addr, &restofs);
                rest = buf + restofs + 1;
                desc = strtok(rest, "\"");
                getinfo(addr);  /* get and print info for this host */
            }
        }
        fclose (conf);
    }
    (void) puts ("</tr>"); 
    (void) puts ("</table>");

//    html_finish();
    exit(EXIT_SUCCESS);
}
