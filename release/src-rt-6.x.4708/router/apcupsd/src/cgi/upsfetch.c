/*
 * upsfetch - utility program to retrieve data from apcupsd.status
 *
 * Original Author: Russell Kroll <rkroll@exploits.org>
 *
 * This source creates a handy object file that can be linked into other
 * programs for easy retrieval of common UPS parameters from the apcupsd
 * status file.
 *
 * Modified: Jonathan Benson <jbenson@technologist.com>
 *         19/6/98 to suit apcupsd
 *         23/6/98 added more graphs and menu options
 *
 * Modified: Kern Sibbald <kern@sibbald.com>
 *         2 Nov 99 to work with apcupsd named pipes
 *         5 Nov 99 added more graphs         
 *        11 Nov 99 to work with apcnetd networking
 *                  also modified it to be a bit closer 
 *                  to the original version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cgiconfig.h"
#include "config.h"
#include "nis.h"

static int fill_buffer(int sockfd);

char statbuf[4096];
size_t  statlen = 0;

static char last_host[256] = "";
char errmsg[200] = "";

/* List of variables that can be read by getupsvar()   
 * First field is that name given to getupsvar(),
 * Second field is our internal name as produced by the STATUS 
 *   output from apcupsd.
 * Third field, if 0 returns everything to the end of the
 *    line, and if 1 returns only to first space (e.g. integers,
 *    and floating point values.
 */
static const struct {   
   const char *request;
   const char *upskeyword;
   int nfields;
} cmdtrans[] = {
   {"model",      "MODEL",    0},
   {"upsmodel",   "UPSMODEL", 0},
   {"date",       "DATE",     0},
   {"battcap",    "BCHARGE",  1},
   {"mbattchg",   "MBATTCHG", 1},
   {"battvolt",   "BATTV",    1},
   {"nombattv",   "NOMBATTV", 1},
   {"utility",    "LINEV",    1},
   {"upsload",    "LOADPCT",  1},
   {"loadpct",    "LOADPCT",  1},
   {"outputv",    "OUTPUTV",  1},
   {"status",     "STATFLAG", 1},
   {"linemin",    "MINLINEV", 1},
   {"linemax",    "MAXLINEV", 1},
   {"upstemp",    "ITEMP",    1},
   {"humidity",   "HUMIDITY", 1}, 
   {"ambtemp",    "AMBTEMP",  1},
   {"outputfreq", "LINEFREQ", 1},
   {"translo",    "LOTRANS",  1},
   {"transhi",    "HITRANS",  1},
   {"runtime",    "TIMELEFT", 1},
   {"mintimel",   "MINTIMEL", 1},
   {"retpct",     "RETPCT",   1},          /* min batt to turn on UPS */
   {"sense",      "SENSE",    1},
   {"hostname",   "HOSTNAME", 1},
   {"battdate",   "BATTDATE", 1},
   {"serialno",   "SERIALNO", 1},
   {"lastxfer",   "LASTXFER", 0},          /* reason for last xfer to batteries */
   {"selftest",   "SELFTEST", 1},          /* results of last self test */
   {"laststest",  "LASTSTEST", 0},
   {"version",    "VERSION",  1},
   {"upsname",    "UPSNAME",  1},
   {"lowbatt",    "DLOWBATT", 1},          /* low battery power off delay */
   {"battpct",    "BCHARGE",  1},
   {"highxfer",   "HITRANS",  1},
   {"lowxfer",    "LOTRANS",  1},
   {"cable",      "CABLE",    0},
   {"firmware",   "FIRMWARE", 0},
   {NULL, NULL, 0}
};

/*
 * Read data into memory buffer to be used by getupsvar()
 * Returns 0 on error
 * Returns 1 if data fetched
 */
static int fetch_data(const char *host)
{
   int nis_port = NISPORT;
   int sockfd;
   int stat;
   char *p;
   char lhost[200];

   if (statlen != 0 && (strcmp(last_host, host) == 0))
       return 1;                      /* alread have data this host */
   strncpy(last_host, host, sizeof(last_host)); 
   last_host[sizeof(last_host) - 1] = '\0';
   statlen = 0;
   strncpy(lhost, host, sizeof(lhost)-1);
   lhost[sizeof(lhost)-1] = '\0';
   p = strchr(lhost, ':');
   if (p) {
      *p++ = '\0';
      nis_port = atoi(p);
   }
   if ((sockfd = net_open(lhost, NULL, nis_port)) < 0) {
      (void) snprintf(errmsg, sizeof (errmsg),
         "upsfetch: tcp_open failed for %s port %d", lhost, nis_port);
      return 0;
   }

   stat = fill_buffer(sockfd);               /* fill statbuf */
   if (stat == 0) {
      *last_host = '\0';
      statlen = 0;
   }
   net_close(sockfd);
   return stat;
} 

/*
 * Read data into memory buffer to be used by getupsvar()
 * Returns 0 on error
 * Returns 1 if data fetched
 */
int fetch_events(const char *host)
{
   int nis_port = NISPORT;
   char buf[500];
   int sockfd;
   int n, stat = 1;
   char *p;
   char lhost[200];
   size_t len;

   statlen = 0;
   statbuf[0] = '\0';
   strncpy(lhost, host, sizeof(lhost)-1);
   lhost[sizeof(lhost)-1] = '\0';
   p = strchr(lhost, ':');
   if (p) {
      *p++ = '\0';
      nis_port = atoi(p);
   }
   if ((sockfd = net_open(lhost, NULL, nis_port)) < 0) {
      snprintf(errmsg, sizeof(errmsg),
          "upsfetch: tcp_open failed for %s port %d", lhost, nis_port);
      fputs(errmsg, stdout);
      return 0;
   }

   if (net_send(sockfd, "events", 6) != 6) {
      snprintf(errmsg, sizeof(errmsg), "fill_buffer: write error on socket\n");
      fputs(errmsg, stdout);
      return 0;
   }
   /*
    * Now read the events and invert them for the list box,
    * with most recend event at the beginning.  
    * by dg2fer.  
    */
   while ((n = net_recv(sockfd, buf, sizeof(buf)-1)) > 0) {
      /* terminate string for strlen()-calls in next lines */
      if (n >= (int)sizeof(buf)) {
         n = (int)sizeof(buf)-1;
      }
      buf[n] = '\0';                     /* ensure string terminated */
      len = strlen(buf);
      /* if message is bigger than the buffer, truncate it */
      if (len < sizeof(statbuf)) {
         /* move previous messages to the end of the buffer */
         memmove(statbuf+len, statbuf, sizeof(statbuf)-len);
         /* copy new message */
         memcpy(statbuf, buf, len);
      } else {
         strncpy(statbuf, buf, sizeof(statbuf)-1);
      }
      statbuf[sizeof(statbuf)-1] = '\0';
   }

   if (n < 0) {
      stat = 0;
   }
   *last_host = '\0';
   net_close(sockfd);
   return stat;

} 


/* In our version, we have prefetched all the data, so the
 * host argument is ignored here.
 * Returns 1 if var found
 *   answer has var
 * Returns 0 if variable name not found
 *   answer has "Not found" is variable name not found
 *   answer may have "N/A" if the UPS does not support this
 *       feature
 * Returns -1 if network problem
 *   answer has "N/A" if host is not available or network error
 */
int getupsvar(const char *host, const char *request,
    char *answer, size_t anslen) 
{
    int i;
    const char *stat_match = NULL;
    char *find;
    int nfields = 0;
     
    if (fetch_data(host) == 0) {
        strncpy(answer, "N/A", anslen);
        answer[anslen - 1] = '\0';
        return -1;
    }

    for (i=0; cmdtrans[i].request; i++) 
        if (strcmp(cmdtrans[i].request, request) == 0) {
             stat_match = cmdtrans[i].upskeyword;
             nfields = cmdtrans[i].nfields;
        }

    if (stat_match != NULL) {
        if ((find=strstr(statbuf, stat_match)) != NULL) {
             if (nfields == 1)  /* get one field */
                 sscanf (find, "%*s %*s %s", answer);
             else {             /* get everything to eol */
                 i = 0;
                 find += 11;  /* skip label */
                 while (*find != '\n')
                     answer[i++] = *find++;
                 answer[i] = '\0';
             }
             if (strcmp(answer, "N/A") == 0)
                 return 0;
             return 1;
        }
    }

    strncpy(answer, "Not found", anslen);
    answer[anslen - 1] = '\0';
    return 0;
}

/* Fill buffer with data from UPS network daemon   
 * Returns 0 on error
 * Returns 1 if OK
 */
static int fill_buffer(int sockfd)
{
   int n, stat = 1; 
   char buf[1000];

   statbuf[0] = '\0';
   statlen = 0;
   if (net_send(sockfd, "status", 6) != 6) {
      snprintf(errmsg, sizeof(errmsg), "fill_buffer: write error on socket\n");
      return 0;
   }

   while ((n = net_recv(sockfd, buf, sizeof(buf)-1)) > 0) {
      buf[n] = '\0';
      strncat(statbuf, buf, sizeof(statbuf)-statlen-1);
      statlen = strlen(statbuf);
   }
   if (n < 0)
      stat = 0;

   return stat;
}
