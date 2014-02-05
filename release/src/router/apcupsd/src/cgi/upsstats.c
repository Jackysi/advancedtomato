/*
 * upsstats - cgi program to generate the main UPS info page
 *
 * Author: Russell Kroll <rkroll@exploits.org>
 *
 * To use: install the binary in a directory where CGI programs may be
 *       executed by your web server.  On many systems something like
 *       /usr/local/etc/httpd/cgi-bin will work nicely.  I recommend
 *         calling the binary "upsstats.cgi" in that directory.
 *
 *       Assuming a path like the above, the following link will suffice:
 *         <A HREF="/cgi-bin/upsstats.cgi">UPS Status</A>
 * 
 *       This program assumes that upsimage.cgi will be in the same 
 *       directory.  The install-cgi target will take care of putting
 *       things in the right place if you set the paths properly in the
 *       Makefile.
 *
 * Modified: Jonathan Benson <jbenson@technologist.com>
 *         19/6/98 to suit apcupsd
 *         23/6/98 added more graphs and menu options
 *
 * Modified: Kern Sibbald <kern@sibbald.com>
 *        Nov 1999 to work with apcupsd networking and
 *                 to include as much of the NUT code
 *                   as possible.
 *                 added runtim status
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
#define MAXHOSTNAMELEN  64
#endif

static char   monhost[MAXHOSTNAMELEN] = "127.0.0.1";
static int    img1 = 1;
static int    img2 = 6;
static int    img3 = 5;
static char   temps[16] = "C";
static int    refresh = DEFAULT_REFRESH;

void parsearg(const char *var, const char *value) 
{
    if (strcmp(var, "host") == 0) {
        strncpy (monhost, value, sizeof(monhost));
        monhost[sizeof(monhost) - 1] = '\0';

    } else if (strcmp(var, "img1") == 0) {
        img1 = atoi(value);
        if ((img1 <= 0) || (img1 > 6)) {
            img1 = 1;
        }

    } else if (strcmp(var, "img2") == 0) {
        img2 = atoi(value);
        if ((img2 <= 0) || (img2 > 6)) {
            img2 = 6;
        }

    } else if (strcmp(var, "img3") == 0) {
        img3 = atoi(value);
        if ((img3 <= 0) || (img3 > 6)) {
            img3 = 5;
        }

    } else if (strcmp(var, "temp") == 0) {
        strncpy (temps, value, sizeof(temps));
        temps[sizeof(temps) - 1] = '\0';

    } else if (strcmp(var, "refresh") == 0) {
        refresh = atoi(value);
        if (refresh < 0) {
                refresh = DEFAULT_REFRESH;
        }
    }
}

void send_image(int report, int defrpt)
{
    char   answer[256], answer2[256], answer3[256];

    if (report < 1 || report > 6)
           report = defrpt;

    fputs ("<img src=\"upsimage.cgi?display=", stdout);
    switch ( report ) {
    case 1:
        getupsvar (monhost, "battcap", answer, sizeof(answer));  
        getupsvar (monhost, "mbattchg", answer2, sizeof(answer2));
        printf ("battcap&amp;value=%s&amp;value2=%s\" alt=\"Battery Capacity %s%%\"",
            answer, answer2, answer);
        break;
    case 2: 
        getupsvar (monhost, "battvolt", answer, sizeof(answer));  
        getupsvar (monhost, "nombattv", answer2, sizeof(answer2));
        printf ("battvolt&amp;value=%s&amp;value2=%s\" alt=\"Battery Voltage %s VDC\"",
            answer, answer2, answer);
        break;
    case 3: 
        getupsvar (monhost, "utility", answer, sizeof(answer));  
        getupsvar (monhost, "lowxfer", answer2, sizeof(answer2));
        getupsvar (monhost, "highxfer", answer3, sizeof(answer3));
        printf ("utility&amp;value=%s&amp;value2=%s&amp;value3=%s\" alt=\"Utility Voltage %s VAC\"",
            answer, answer2, answer3, answer);
        break;
    case 4: 
        getupsvar (monhost, "outputv", answer, sizeof(answer));  
        printf ("outputv&amp;value=%s\" alt=\"Output Voltage %s VAC\"",
            answer, answer);
        break;
    case 5: 
        getupsvar (monhost, "upsload", answer, sizeof(answer));  
        printf ("upsload&amp;value=%s\" alt=\"UPS Load %s%%\"",
            answer, answer);
        break;
    case 6:
        getupsvar (monhost, "runtime", answer, sizeof(answer));  
        getupsvar (monhost, "mintimel", answer2, sizeof(answer2));
        printf ("runtime&amp;value=%s&amp;value2=%s\" alt=\"Run time remaining %s minutes\"",
            answer, answer2, answer);
        break;
    }
    puts (" width=\"150\" height=\"350\" />");
}

static void image_menu(int select)
{
    fputs ("               <option value=\"1\"", stdout);
    if (select == 1)
        fputs(" selected=\"selected\"", stdout);
    puts (">Battery Capacity</option>");

    fputs ("               <option value=\"2\"", stdout);
    if (select == 2)
        fputs(" selected=\"selected\"", stdout);
    puts (">Battery Voltage</option>");

    fputs ("               <option value=\"3\"", stdout);
    if (select == 3)
        fputs(" selected=\"selected\"", stdout);
    puts (">Utility Voltage</option>");

    fputs ("               <option value=\"4\"", stdout);
    if (select == 4)
        fputs(" selected=\"selected\"", stdout);
    puts (">Output Voltage</option>");

    fputs ("               <option value=\"5\"", stdout);
    if (select == 5)
        fputs(" selected=\"selected\"", stdout);
    puts (">UPS Load</option>");

    fputs ("               <option value=\"6\"", stdout);
    if (select == 6)
        fputs(" selected=\"selected\"", stdout);
    puts (">Run Time Remaining</option>");
}

int main(int argc, char **argv) 
{
    int     status;
    double  tempf;
    char *p;
    char   answer[256];

    (void) extractcgiargs();

    p = strstr(monhost, "%3");
    if (p) {
       *p++ = ':';                                     /* set colon */
       memmove(p, p+2, sizeof(monhost)-(p-monhost));   /* get rid of hex 3A */
    }

    snprintf(answer, sizeof(answer), "%s UPS Status", monhost);
    html_begin(answer, refresh);
    
    if (!checkhost(monhost)) {
        fputs ("<p><strong>Access to host ", stdout);
        html_puts(monhost);
        puts (" is not authorized.</strong></p>");
        html_finish();
        exit (EXIT_FAILURE);
    }
    
    /* check if host is available */
    if (getupsvar(monhost, "date", answer, sizeof(answer)) <= 0) {
        fputs ("<p><strong>Unable to communicate with the UPS on ", stdout);
        html_puts(monhost);
        puts (".</strong></p>");
        html_finish();
        exit (EXIT_FAILURE);
    }

     puts ("<div class=\"Center\">");
     puts ("<table border=\"2\" cellspacing=\"10\" cellpadding=\"5\">");

     getupsvar(monhost, "date", answer, sizeof(answer));
     fputs ("<tr><th>", stdout);
     html_puts(answer);
     puts ("</th>");

     puts ("<th>");
     printf ("       <form method=\"get\" action=\"upsstats.cgi\"><div>\n");
     printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\" />\n", monhost);
     printf ("       <select onchange=\"this.form.submit()\" name=\"img1\">\n");
     image_menu(img1);
     printf ("       </select>\n");
     printf ("       <input type=\"hidden\" name=\"img2\" value=\"%d\" />\n",img2);
     printf ("       <input type=\"hidden\" name=\"img3\" value=\"%d\" />\n",img3);
     printf ("       <input type=\"hidden\" name=\"temp\" value=\"%s\" />\n",temps);
     printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\" />\n",refresh);
     puts ("       </div></form>");
     puts ("</th>");

     puts ("<th>");
     printf ("       <form method=\"get\" action=\"upsstats.cgi\"><div>\n");
     printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\" />\n", monhost);
     printf ("       <input type=\"hidden\" name=\"img1\" value=\"%d\" />\n",img1);
     printf ("       <select onchange=\"this.form.submit()\" name=\"img2\">\n");
     image_menu(img2);
     printf ("       </select>\n");
     printf ("       <input type=\"hidden\" name=\"img3\" value=\"%d\" />\n",img3);
     printf ("       <input type=\"hidden\" name=\"temp\" value=\"%s\" />\n",temps);
     printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\" />\n",refresh);
     puts ("       </div></form>");
     puts ("</th>");

     puts ("<th>");
     printf ("       <form method=\"get\" action=\"upsstats.cgi\"><div>\n");
     printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\" />\n", monhost);
     printf ("       <input type=\"hidden\" name=\"img1\" value=\"%d\" />\n",img1);
     printf ("       <input type=\"hidden\" name=\"img2\" value=\"%d\" />\n",img2);
     printf ("       <select onchange=\"this.form.submit()\" name=\"img3\">\n");
     image_menu(img3);
     printf ("       </select>\n");
     printf ("       <input type=\"hidden\" name=\"temp\" value=\"%s\" />\n",temps);
     printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\" />\n",refresh);
     puts ("       </div></form>");
     puts ("</th></tr>");

     puts ("<tr><td><pre>");

     getupsvar (monhost, "hostname", answer, sizeof(answer));
     fputs ("Monitoring: ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     getupsvar (monhost, "model", answer, sizeof(answer));  
     fputs (" UPS Model: ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     getupsvar (monhost, "upsname", answer, sizeof(answer));
     fputs ("  UPS Name: ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     getupsvar (monhost, "version", answer, sizeof(answer));
     fputs ("   APCUPSD: Version ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     fputs ("    Status: ", stdout);

     if (getupsvar (monhost, "status", answer, sizeof(answer)) <= 0) {
             puts ("Not available");
     } else {
         status = strtol(answer, 0, 16);
         if (status & UPS_calibration) 
             fputs ("CALIBRATION ", stdout); 
         if (status & UPS_trim)
             fputs ("TRIM ", stdout);
         if (status & UPS_boost)
             fputs ("BOOST ", stdout);
         if (status & UPS_online)
             fputs ("ONLINE ", stdout); 
         if (status & UPS_onbatt) 
             fputs ("ON BATTERY ", stdout); 
         if (status & UPS_overload)
             fputs ("OVERLOADED ", stdout);
         if (status & UPS_battlow) 
             fputs ("BATTERY LOW ", stdout); 
         if (status & UPS_replacebatt)
             fputs ("REPLACE BATTERY ", stdout);
         if (status & UPS_commlost)
             fputs("COMM LOST ", stdout); 
         if (status & UPS_shutdown)
             fputs("SHUTDOWN ", stdout);
         if (status & UPS_slave)
             fputs("SLAVE ", stdout);
         if (!(status & UPS_battpresent))
             fputs("NOBATT ", stdout);
         fputs ("\n", stdout); 
     }

     puts ("</pre></td>");

     puts ("<td rowspan=\"3\">");
     send_image(img1, 1);
     puts ("</td>");

     puts ("<td rowspan=\"3\">");
     send_image(img2, 6);
     puts ("</td>");

     puts ("<td rowspan=\"3\">");
     send_image(img3, 5);
     puts ("</td>");
     puts ("</tr>");

     puts ("<tr><td><pre>");

     getupsvar (monhost, "selftest", answer, sizeof(answer));
     fputs ("Last UPS Self Test: ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     getupsvar(monhost, "laststest", answer, sizeof(answer));
     /* To reduce the length of the output, we drop the 
      * seconds and the trailing year.
      */
     for (p=answer; *p && *p != ':'; p++) ;
     if (*p == ':')
        p++;
     for ( ; *p && *p != ':'; p++) ;
     *p = '\0';
     fputs ("Last Test Date: ", stdout);
     html_puts (answer);
     fputs ("\n", stdout);

     puts ("</pre></td></tr>");

     puts ("<tr><td><pre>");

     getupsvar (monhost, "utility", answer, sizeof(answer));
     fputs ("Utility Voltage: ", stdout);
     html_puts (answer);
     puts (" VAC");

     getupsvar (monhost, "linemin", answer, sizeof(answer));
     fputs ("   Line Minimum: ", stdout);
     html_puts (answer);
     puts (" VAC");

     getupsvar (monhost, "linemax", answer, sizeof(answer));
     fputs ("   Line Maximum: ", stdout);
     html_puts (answer);
     puts (" VAC");

     getupsvar (monhost, "outputfreq", answer, sizeof(answer));
     fputs ("    Output Freq: ", stdout);
     html_puts (answer);
     puts (" Hz");

     if (getupsvar(monhost, "ambtemp", answer, sizeof(answer)) > 0) {
         if (strcmp(answer, "Not found" ) != 0) {
             if (strcmp(temps,"F") == 0) {
                tempf = (strtod (answer, 0) * 1.8) + 32;
                printf ("     Amb. Temp.: %.1f&deg; F\n", tempf);
             } else if (strcmp(temps,"K") == 0) {
                tempf = (strtod (answer, 0)) + 273;
                printf ("     Amb. Temp.: %.1f&deg; K\n", tempf);
             } else {
                printf ("     Amb. Temp.: %s&deg; C\n", answer);
             } 
         }
     }

     if ( getupsvar (monhost, "humidity", answer, sizeof(answer)) > 0) {
         if (strcmp(answer, "Not found") != 0) {
              fputs ("  Amb. Humidity: ", stdout);
              html_puts (answer);
              puts (" %");
         }
     }

     printf ("</pre>\n<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n<tr>\n<td colspan=\"2\">\n<pre>\n");

    if (getupsvar (monhost, "upstemp", answer, sizeof(answer)) > 0) {
         if (strcmp(temps,"F") == 0) {
              tempf = (strtod (answer, 0) * 1.8) + 32;
              printf ("       UPS Temp: %.1f \n</pre>\n</td>\n<td>\n", tempf); 
              printf ("       <form method=\"get\" action=\"upsstats.cgi\"><div>\n");
              printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\" />\n",monhost);
              printf ("       <input type=\"hidden\" name=\"img1\" value=\"%d\" />\n",img1);
              printf ("       <input type=\"hidden\" name=\"img2\" value=\"%d\" />\n",img2);
              printf ("       <input type=\"hidden\" name=\"img3\" value=\"%d\" />\n",img3);
              printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\" />\n",refresh);
              printf ("       <select onchange=\"this.form.submit();\" name=\"temp\">\n");
              printf ("          <option value=\"C\">&deg; C</option>\n");
              printf ("          <option selected=\"selected\" value=\"F\">&deg; F</option>\n");
              printf ("          <option value=\"K\">&deg; K</option>\n");

          } else if (strcmp(temps,"K") == 0) {
               tempf = (strtod (answer, 0)) + 273;
               printf ("       UPS Temp: %.1f \n</pre>\n</td>\n<td>\n", tempf); 
               printf ("       <form method=\"get\" action=\"upsstats.cgi\"><div>\n");
               printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\" />\n",monhost);
               printf ("       <input type=\"hidden\" name=\"img1\" value=\"%d\" />\n",img1);
               printf ("       <input type=\"hidden\" name=\"img2\" value=\"%d\" />\n",img2);
               printf ("       <input type=\"hidden\" name=\"img3\" value=\"%d\" />\n",img3);
               printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\" />\n",refresh);
               printf ("       <select onchange=\"this.form.submit()\" name=\"temp\">\n");
               printf ("         <option value=\"C\">&deg; C</option>\n");
               printf ("         <option value=\"F\">&deg; F</option>\n");
               printf ("         <option selected=\"selected\" value=\"K\">&deg; K</option>\n");

         } else {
               printf ("       UPS Temp: %s \n</pre>\n</td>\n<td>\n", answer);
               printf ("       <form method=\"get\" action=\"upsstats.cgi\"><div>\n");
               printf ("       <input type=\"hidden\" name=\"host\" value=\"%s\" />\n",monhost);
               printf ("       <input type=\"hidden\" name=\"img1\" value=\"%d\" />\n",img1);
               printf ("       <input type=\"hidden\" name=\"img2\" value=\"%d\" />\n",img2);
               printf ("       <input type=\"hidden\" name=\"img3\" value=\"%d\" />\n",img3);
               printf ("       <input type=\"hidden\" name=\"refresh\" value=\"%d\" />\n",refresh);
               printf ("       <select onchange=\"this.form.submit()\" name=\"temp\">\n");
               printf ("         <option selected=\"selected\" value=\"C\">&deg; C</option>\n");
               printf ("         <option value=\"F\">&deg; F</option>\n");
               printf ("         <option value=\"K\">&deg; K</option>\n");
         }
    }

     puts ("       </select>");
     puts ("       </div></form>");
     puts ("</td></tr></table>");   
     puts ("</td></tr>");

     puts ("<tr><td colspan=\"4\"><b>Recent Events</b><br />");
     puts ("<textarea rows=\"5\" cols=\"95\">");

     fetch_events(monhost);
     html_puts (statbuf);

     puts ("</textarea>");
     puts ("</td></tr>");

     puts ("</table></div>");

     html_finish();
     return 0;
}
