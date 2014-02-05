/*
 * Client test program for apcnisd to be used
 * as a base for the new master/slave code.
 *
 * Build it with: cc newslave.c ../lib/libapc.a -o newclient
 *
 * Execute: ./newslave [host[:port]]
 *
 * The two commands currently (Apr 2001) accepted by the
 * server are "status" and "events".
 *
 */

#include "apc.h"

#ifdef HAVE_NISLIB

/* Default values, can be changed on command line */
#define SERV_TCP_PORT 3551
#define SERV_HOST_ADDR "127.0.0.1"

#define BIGBUF 4096
char statbuf[BIGBUF];
int statlen = BIGBUF;

/* List of variables that can be read by getupsvar()
 * First field is that name given to getupsvar(),
 * Second field is our internal name as produced by the STATUS
 *   output from apcupsd.
 * Third field, if 0 returns everything to the end of the
 *    line, and if 1 returns only to first space (e.g. integers,
 *    and floating point values.
 */
static struct {
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
   {NULL, NULL}
};

int fetch_data(char *host, int port);
int getupsvar(char *host, int port, const char *request, char *answer, int anslen);
int fill_buffer(int sockfd);

extern int net_errno;

void error_abort(char *msg)
{
   fprintf(stderr, msg);
   exit(1);
}

int main(int argc, char *argv[])
{
   int port;
   char host[200];
   char msg[200], *p;
   char hostname[100];
   char release[100];
   char upsname[100];
   char status[100];

   strcpy(host, SERV_HOST_ADDR);
   port = SERV_TCP_PORT;

   if (argc > 1) {
      strcpy(host, argv[1]); /* get host from command line */
      p = strchr(host, ':');
      if (p) {
	 *p++ = 0;
	 port = atoi(p);
      }
   }

   if (getupsvar(host, port, "hostname", msg, sizeof(msg)) <= 0) {
       printf("Error getting variable\n");
       exit(1);
   }
   strcpy(hostname, msg);

   if (getupsvar(host, port, "version", msg, sizeof(msg)) <= 0) {
       printf("Error getting variable\n");
       exit(1);
   }
   strcpy(release, msg);

   if (getupsvar(host, port, "upsname", msg, sizeof(msg)) <= 0) {
       printf("Error getting variable\n");
       exit(1);
   }
   strcpy(upsname, msg);

   if (getupsvar(host, port, "status", msg, sizeof(msg)) <= 0) {
       printf("Error getting variable\n");
       exit(1);
   }
   strcpy(status, msg);

   printf("For host=%s ups=%s apcupsd version=%s, the Status=%s\n",
       hostname, upsname, release, status);

   exit(0);
}


/*
 * Read data into memory buffer to be used by getupsvar()
 * Returns 0 on error
 * Returns 1 if data fetched
 */
int fetch_data(char *host, int port)
{
   int sockfd;
   int stat;

   if ((sockfd = net_open(host, NULL, port)) < 0) {
      printf("fetch_data: tcp_open failed for %s port %d", host, port);
      return 0;
   }

   stat = fill_buffer(sockfd);		     /* fill statbuf */
   net_close(sockfd);
   return stat;

}

/*
 *
 * Returns 1 if var found
 *   answer has var
 * Returns 0 if variable name not found
 *   answer has "Not found" is variable name not found
 *   answer may have "N/A" if the UPS does not support this
 *	 feature
 * Returns -1 if network problem
 *   answer has "N/A" if host is not available or network error
 */
int getupsvar(char *host, int port, const char *request, char *answer, int anslen)
{
    int i;
    const char *stat_match = NULL;
    char *find;
    int nfields = 0;

    if (!fetch_data(host, port)) {
        strcpy(answer, "N/A");
	return -1;
    }

    for (i=0; cmdtrans[i].request; i++)
	if (!(strcmp(cmdtrans[i].request, request))) {
	     stat_match = cmdtrans[i].upskeyword;
	     nfields = cmdtrans[i].nfields;
	}

    if (stat_match != NULL) {
	if ((find=strstr(statbuf, stat_match)) != NULL) {
	     if (nfields == 1)	/* get one field */
                 sscanf (find, "%*s %*s %s", answer);
	     else {		/* get everything to eol */
		 i = 0;
		 find += 11;  /* skip label */
                 while (*find != '\n')
		     answer[i++] = *find++;
		 answer[i] = 0;
	     }
             if (strcmp(answer, "N/A") == 0)
		 return 0;
	     return 1;
	}
    }

    strcpy(answer, "Not found");
    return 0;
}

#define MAXLINE 512

/* Fill buffer with data from UPS network daemon
 * Returns 0 on error
 * Returns 1 if OK
 */
int fill_buffer(int sockfd)
{
   int n, stat = 1;
   char buf[1000];

   statbuf[0] = 0;
   statlen = 0;
   if (net_send(sockfd, "status", 6) != 6) {
      printf("fill_buffer: write error on socket\n");
      return 0;
   }

   while ((n = net_recv(sockfd, buf, sizeof(buf)-1)) > 0) {
      buf[n] = 0;
      strcat(statbuf, buf);
   }
   if (n < 0)
      stat = 0;

   statlen = strlen(statbuf);
   return stat;

}

#else /* HAVE_NISLIB */

int main(int argc, char *argv[]) {
    printf("Sorry, NIS code is not compiled in apcupsd.\n");
    return 1;
}

#endif /* HAVE_NISLIB */
