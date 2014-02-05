/*
 * Check-Plugin for Nagios to check status of an APC-UPS
 * monitored by APCUPSD
 *
 *  Written by Christian Masopust, November 2005
 *
 * Build it with: cc check_apcupsd.c ../lib/libapc.a -o check_apcupsd
 *
 * Execute: ./check_apcupsd [host[:port]]
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

#define NAGIOS_OK       0
#define NAGIOS_WARNING  1
#define NAGIOS_CRITICAL 2
#define NAGIOS_UNKNOWN  3
#define S_NAGIOS_OK             "OK: "
#define S_NAGIOS_WARNING        "WARNING: "
#define S_NAGIOS_CRITICAL       "CRITICAL: "
#define S_NAGIOS_UNKNOWN        "UNKNOWN: "


/* List of variables that can be read by getupsvar()   
 * First field is that name given to getupsvar(),
 * Second field is our internal name as produced by the STATUS 
 *   output from apcupsd.
 * Third field, if 0 returns everything to the end of the
 *    line, and if 1 returns only to first space (e.g. integers,
 *    and floating point values.
 */
static struct {         
   char *request;
   char *upskeyword;
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
int getupsvar(char *host, int port, char *request, char *answer, int anslen); 
int fill_buffer(int sockfd);

extern int net_errno;

struct sockaddr_in tcp_serv_addr;

void error_abort(char *msg)
{
   fprintf(stdout, msg);
   exit(NAGIOS_CRITICAL);
}

int main(int argc, char *argv[]) 
{
   int port;
   char host[200];
   char msg[200], *p;
   char hostname[100];
   char model[100];
   char upsname[100];
   char status[1000];
   int iStatus;
   char sStatus[10];
   char loadpct[100];
   char runtime[100];
   int retVal;

   retVal = NAGIOS_UNKNOWN;
   strcpy (sStatus, S_NAGIOS_UNKNOWN);

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
       printf("%scannot get hostname from UPS-Server\n", S_NAGIOS_CRITICAL);
       exit(NAGIOS_CRITICAL);
   }
   strcpy(hostname, msg);

   if (getupsvar(host, port, "model", msg, sizeof(msg)) <= 0) {
       printf("%scannot get model from UPS-Server\n", S_NAGIOS_CRITICAL);
       exit(NAGIOS_CRITICAL);
   }
   strcpy(model, msg);

   if (getupsvar(host, port, "upsname", msg, sizeof(msg)) <= 0) {
       printf("%scannot get upsname from UPS-Server\n", S_NAGIOS_CRITICAL);
       exit(NAGIOS_CRITICAL);
   }
   strcpy(upsname, msg);

   if (getupsvar(host, port, "status", msg, sizeof(msg)) <= 0) {
       printf("%scannot get status from UPS-Server\n", S_NAGIOS_CRITICAL);
       exit(NAGIOS_CRITICAL);
   }
   iStatus = strtol(msg, 0, 16);
   status[0] = '\0';
   if (iStatus & UPS_calibration) {
      strcat(status, "CALIBRATION ");
      retVal = NAGIOS_OK;
      strcpy(sStatus, S_NAGIOS_OK);
   }
   if (iStatus & UPS_trim) {
      strcat(status, "SMART TRIM ");
      retVal = NAGIOS_OK;
      strcpy(sStatus, S_NAGIOS_OK);
   }
   if (iStatus & UPS_boost) {
      strcat(status, "SMART BOOST ");
      retVal = NAGIOS_OK;
      strcpy(sStatus, S_NAGIOS_OK);
   }
   if (iStatus & UPS_online) {
      strcat(status, "ONLINE ");
      retVal = NAGIOS_OK;
      strcpy(sStatus, S_NAGIOS_OK);
   }
   if (iStatus & UPS_onbatt) {
      strcat(status, "ON BATTERY ");
      retVal = NAGIOS_WARNING;
      strcpy(sStatus, S_NAGIOS_WARNING);
   }
   if (iStatus & UPS_overload) {
      strcat(status, "OVERLOADED ");
      retVal = NAGIOS_CRITICAL;
      strcpy(sStatus, S_NAGIOS_CRITICAL);
   }
   if (iStatus & UPS_battlow) {
      strcat(status, "BATTERY LOW ");
      retVal = NAGIOS_CRITICAL;
      strcpy(sStatus, S_NAGIOS_CRITICAL);
   }
   if (iStatus & UPS_replacebatt) {
      strcat(status, "REPLACE BATTERY ");
      retVal = NAGIOS_WARNING;
      strcpy(sStatus, S_NAGIOS_WARNING);
   }
   if (iStatus & UPS_commlost) {
      strcat(status, "COMMUNICATION LOST ");
      retVal = NAGIOS_CRITICAL;
      strcpy(sStatus, S_NAGIOS_CRITICAL);
   }
   if (iStatus & UPS_shutdown) {
      strcat(status, "SHUTDOWN ");
      retVal = NAGIOS_OK;
      strcpy(sStatus, S_NAGIOS_OK);
   }
   if (iStatus & UPS_slave) {
      strcat(status, "SLAVE ");
      retVal = NAGIOS_OK;
      strcpy(sStatus, S_NAGIOS_OK);
   }

   if (strlen(status) > 0) {
      status[strlen(status) - 1] = '\0';
   }

   if (getupsvar(host, port, "loadpct", msg, sizeof(msg)) <= 0) {
       printf("%scannot get loadpct from UPS-Server\n", S_NAGIOS_CRITICAL);
       exit(NAGIOS_CRITICAL);
   }
   strcpy(loadpct, msg);

   if (getupsvar(host, port, "runtime", msg, sizeof(msg)) <= 0) {
       printf("%scannot get runtime from UPS-Server\n", S_NAGIOS_CRITICAL);
       exit(NAGIOS_CRITICAL);
   }
   strcpy(runtime, msg);

   printf ("%sUPS: %s, Load: %s%%, Runtime: %smin, Status: %s\n", sStatus, model, loadpct, runtime, status);
   /* printf("For host=%s ups=%s model=%s, the Status=%s, loadpct=%s, runtime=%s\n",
       hostname, upsname, model, status, loadpct, runtime); */

   exit(retVal);
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

   stat = fill_buffer(sockfd);               /* fill statbuf */
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
 *       feature
 * Returns -1 if network problem
 *   answer has "N/A" if host is not available or network error
 */
int getupsvar(char *host, int port, char *request, char *answer, int anslen) 
{
    int i;
    char *stat_match = NULL;
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
             if (nfields == 1)  /* get one field */
                 sscanf (find, "%*s %*s %s", answer);
             else {             /* get everything to eol */
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
