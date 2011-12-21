/*
 * smtp.c
 *
 * A simple SMTP mail client for apcupsd.
 *
 * Derived from a SMTPclient:
 *
 *     SMTPclient -- simple SMTP client
 *     Copyright (C) 1997 Ralf S. Engelschall, All Rights Reserved.
 *     rse@engelschall.com
 *     www.engelschall.com
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald and John Walker
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#define APCUPSD

#ifdef APCUPSD

# include "apc.h"
# undef main
# define my_name_is(x, y, z)
# define bstrdup(x) strdup(x)
UPSINFO myUPS;
UPSINFO *core_ups = &myUPS;

# define Pmsg2 Dmsg2
# define Pmsg1 Dmsg1
# define Pmsg0 Dmsg0
# define MY_NAME "smtp"

#else

# include "bacula.h"
# include "jcr.h"
# define MY_NAME "bsmtp"

#endif


#ifndef MAXSTRING
# define MAXSTRING 254
#endif

static int s;

static char *from_addr = NULL;
static char *cc_addr = NULL;
static char *subject = NULL;
static char *err_addr = NULL;
static const char *mailhost = NULL;
static char *reply_addr = NULL;
static int mailport = 25;
static char my_hostname[MAXSTRING];

/* Needed by lib/apcconfig.c */
char argvalue[MAXSTRING];

/* like fgets(3), but for a socket */
static char *sockgets(char *buf, int len, int s)
{
   int i=0;

   /* Inefficient, but simple */
   while (i < len-1) {
      if (recv(s, buf+i, 1, 0) != 1)
         break;
      i++;
      if (buf[i-1] == '\n')
         break;
   }
   buf[i] = '\0';
   return i==0 ? NULL : buf;
}

/* examine message from server */
static void get_response(void)
{
   char buf[MAXSTRING];

   Dmsg0(50, "Calling sockgets on read socket.\n");

   while (sockgets(buf, sizeof(buf), s)) {
      buf[strlen(buf) - 1] = 0;
      Dmsg2(10, "%s --> %s\n", mailhost, buf);

      if (!isdigit((int)buf[0]) || buf[0] > '3') {
         Pmsg2(0, "Fatal malformed reply from %s: %s\n", mailhost, buf);
         exit(1);
      }

      if (buf[3] != '-')
         break;
   }
}

/* like vfprintf(3), but for a socket */
static void vsockwrite(int s, const char *fmt, va_list ap)
{
   static char temp[MAXSTRING];

   avsnprintf(temp, sizeof(temp), fmt, ap);
   send(s, temp, strlen(temp), 0);
}

/* like fprintf(3), but for a socket */
static void sockwrite(int s, const char *fmt, ...)
{
   va_list ap;

   va_start(ap, fmt);
   vsockwrite(s, fmt, ap);
   va_end(ap);
}

/* say something to server and check the response */
static void chat(const char *fmt, ...)
{
   va_list ap;

   va_start(ap, fmt);
   vsockwrite(s, fmt, ap);
   if (debug_level >= 10) {
      fprintf(stdout, "%s --> ", my_hostname);
      vfprintf(stdout, fmt, ap);
   }
   va_end(ap);

   if (debug_level >= 10)
      fflush(stdout);

   get_response();
}


static void usage()
{
   fprintf(stderr,
      "\n"
      "Usage: %s [-f from] [-h mailhost] [-s subject] [-c copy] [recepient ...]\n"
      "       -c          set the Cc: field\n"
      "       -dnn        set debug level to nn\n"
      "       -f          set the From: field\n"
      "       -h          use mailhost:port as the SMTP server\n"
      "       -s          set the Subject: field\n"
      "       -?          print this message.\n" "\n", MY_NAME);

   exit(1);
}


/* Program to send email */
int main(int argc, char *argv[])
{
   char buf[MAXSTRING];
   struct sockaddr_in sin;
   struct hostent *hp;
   int i, ch;
   struct passwd *pwd;
   char *cp, *p;
   time_t now = time(NULL);
   struct tm tm;

   my_name_is(argc, argv, "bsmtp");

   while ((ch = getopt(argc, argv, "c:d:f:h:r:s:?")) != -1) {
      switch (ch) {
      case 'c':
         Dmsg1(20, "cc=%s\n", optarg);
         cc_addr = optarg;
         break;

      case 'd':    /* set debug level */
         debug_level = atoi(optarg);
         if (debug_level <= 0)
            debug_level = 1;
         Dmsg1(20, "Debug level = %d\n", debug_level);
         break;

      case 'f':    /* from */
         from_addr = optarg;
         break;

      case 'h':    /* smtp host */
         Dmsg1(20, "host=%s\n", optarg);
         p = strchr(optarg, ':');
         if (p) {
            *p++ = 0;
            mailport = atoi(p);
         }
         mailhost = optarg;
         break;

      case 's':    /* subject */
         Dmsg1(20, "subject=%s\n", optarg);
         subject = optarg;
         break;

      case 'r':    /* reply address */
         reply_addr = optarg;
         break;

      case '?':
      default:
         usage();

      }
   }

   argc -= optind;
   argv += optind;

   if (argc < 1) {
      Pmsg0(0, "Fatal error: no recipient given.\n");
      usage();
      exit(1);
   }

   /* Determine SMTP server */
   if (mailhost == NULL) {
      if ((cp = getenv("SMTPSERVER")) != NULL)
         mailhost = cp;
      else
         mailhost = "localhost";
   }

#ifdef HAVE_WIN32
   WSA_Init();
#endif

   /*
    *  Find out my own host name for HELO; 
    *  if possible, get the fully qualified domain name
    */
   if (gethostname(my_hostname, sizeof(my_hostname) - 1) == -1) {
      Pmsg2(0, "Fatal gethostname error: %s ERR=%s\n", my_hostname, strerror(errno));
      exit(1);
   }

   if ((hp = gethostbyname(my_hostname)) == NULL) {
      Pmsg2(0, "Fatal gethostbyname for myself failed \"%s\": ERR=%s\n",
         my_hostname, strerror(errno));
      exit(1);
   }

   astrncpy(my_hostname, hp->h_name, sizeof(my_hostname));
   Dmsg1(20, "My hostname is: %s\n", my_hostname);

   /* Determine from address. */
   if (from_addr == NULL) {
      if ((pwd = getpwuid(getuid())) == 0)
         asnprintf(buf, sizeof(buf), "userid-%d@%s", (int)getuid(), my_hostname);
      else
         asnprintf(buf, sizeof(buf), "%s@%s", pwd->pw_name, my_hostname);

      from_addr = bstrdup(buf);
   }

   Dmsg1(20, "From addr=%s\n", from_addr);

   /* Connect to smtp daemon on mailhost. */
hp:
   if ((hp = gethostbyname(mailhost)) == NULL) {
      Pmsg2(0, "Error unknown mail host \"%s\": ERR=%s\n", mailhost,
         strerror(errno));
      if (strcasecmp(mailhost, "localhost") != 0) {
         Pmsg0(0, "Retrying connection using \"localhost\".\n");
         mailhost = "localhost";
         goto hp;
      }
      exit(1);
   }

   if (hp->h_addrtype != AF_INET) {
      Pmsg1(0, "Fatal error: Unknown address family for smtp host: %d\n",
         hp->h_addrtype);
      exit(1);
   }

   memset((char *)&sin, 0, sizeof(sin));
   memcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
   sin.sin_family = hp->h_addrtype;
   sin.sin_port = htons(mailport);

   if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      Pmsg1(0, "Fatal socket error: ERR=%s\n", strerror(errno));
      exit(1);
   }

   if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
      Pmsg2(0, "Fatal connect error to %s: ERR=%s\n", mailhost, strerror(errno));
      exit(1);
   }

   Dmsg0(20, "Connected\n");

   /* Send SMTP headers */
   get_response();                 /* banner */
   chat("helo %s\r\n", my_hostname);
   chat("mail from:<%s>\r\n", from_addr);

   for (i = 0; i < argc; i++) {
      Dmsg1(20, "rcpt to: %s\n", argv[i]);
      chat("rcpt to:<%s>\r\n", argv[i]);
   }

   if (cc_addr)
      chat("rcpt to:<%s>\r\n", cc_addr);

   Dmsg0(20, "Data\n");
   chat("data\r\n");

   /* Send message header */
   sockwrite(s, "From: %s\r\n", from_addr);
   if (subject)
      sockwrite(s, "Subject: %s\r\n", subject);

   if (reply_addr)
      sockwrite(s, "Reply-To: %s\r\n", reply_addr);

   if (err_addr)
      sockwrite(s, "Errors-To: %s\r\n", err_addr);

   if ((pwd = getpwuid(getuid())) == 0)
      sockwrite(s, "Sender: userid-%d@%s\r\n", (int)getuid(), my_hostname);
   else
      sockwrite(s, "Sender: %s@%s\r\n", pwd->pw_name, my_hostname);

   sockwrite(s, "To: %s", argv[0]);
   for (i = 1; i < argc; i++)
      sockwrite(s, ",%s", argv[i]);
   sockwrite(s, "\r\n");

   if (cc_addr)
      sockwrite(s, "Cc: %s\r\n", cc_addr);

   /* Add RFC822 date */
   localtime_r(&now, &tm);

#ifdef HAVE_WIN32
   // Annoyingly, Windows does not properly implement %z (it always spells
   // out the timezone name) so we need to emulate it manually.
   i = strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S ", &tm);
   tzset();
   unsigned int offset = abs(_timezone) / 60;
   snprintf(buf+i, sizeof(buf)-i, "%c%02u%02u", 
      _timezone < 0 ? '+' : '-', // _timezone is UTC-local
      offset/60, offset%60);
#else
   strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %z", &tm);
#endif

   sockwrite(s, "Date: %s\r\n", buf);

   sockwrite(s, "\r\n");

   /* Send message body */
   while (fgets(buf, sizeof(buf), stdin)) {
      buf[strlen(buf) - 1] = 0;
      if (strcmp(buf, ".") == 0)   /* quote lone dots */
         sockwrite(s, "..\r\n");
      else                         /* pass body through unchanged */
         sockwrite(s, "%s\r\n", buf);
   }

   /* Send SMTP quit command */
   chat(".\r\n");
   chat("quit\r\n");

   /* Go away gracefully */
   exit(0);
}
