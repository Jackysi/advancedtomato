/*
 * Client test program for apcnet
 *
 * This program beats the living daylights out of your
 *  server by sending it one million requests.
 *
 * Optionally, it can send one million requests, connecting
 *  and disconnecting each time.
 *
 *
 * Build it with: cc megaclient.c ../lib/libapc.a -o megaclient
 *
 * Execute: ./megaclient [host[:port]]
 *
 * For additional examples of code, see cgi/upsfetch.c
 */


/*
 * If RECONNECT is defined, megaclient will disconnect
 *  and reconnect for every request (iteration), which is the normal
 *  way that apcupsd is currently accessed.
 *
 * If RECONNECT is not defined, a single connection
 *  is made with multiple requests.
 */
#define RECONNECT 1

#define ITERATIONS 80000

#include "apc.h"

#ifdef HAVE_NISLIB

/* Default values, can be changed on command line */
#define SERV_TCP_PORT 3551
#define SERV_HOST_ADDR "127.0.0.1"

#define MAXLINE 5000


void error_abort(const char *msg)
{
   fprintf(stderr, msg);
   exit(1);
}

int main(int argc, char *argv[])
{
   int sockfd, port;
   char host[200];
   char msg[200], *p, *cmd;
   int i, n, line;
   time_t now, done;
   char recvline[MAXLINE+1];

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

   if (argc > 2) {
      cmd = argv[2];
   } else {
      cmd = NULL;
   }

#ifdef RECONNECT

   now = time(NULL);
   for (i=0; i<ITERATIONS; i++) {
      if ((sockfd = net_open(host, NULL, port)) < 0) {
         sprintf(msg, "client: tcp_open for host %s on %d failed\n", host, port);
	 error_abort(msg);
      }

      if (net_send(sockfd, "status", 6) != 6)
         error_abort("handle_client: write error on socket");

      line = 0;
      while ((n = net_recv(sockfd, recvline, sizeof(recvline))) > 0) {
	  recvline[n] = 0;
	  line++;
/*	    fputs(recvline, stdout); */
      }
      if (n < 0) {
	 char msg[200];
         sprintf(msg, "handle_client: net_recv error: %s\n", strerror(-n));
	 error_abort(msg);
      }
      if ( (i % 100) == 0) {
         printf("%d lines=%d\n", i, line);
      }
      net_close(sockfd);
   }

#else
   /* Open once only */

   if ((sockfd = net_open(host, NULL, port)) < 0) {
      sprintf(msg, "client: tcp_open for host %s on %d failed\n", host, port);
      error_abort(msg);
   }

   now = time(NULL);
   for (i=0; i<ITERATIONS; i++) {
      if (net_send(sockfd, "status", 6) != 6)
         error_abort("handle_client: write error on socket");

      line = 0;
      while ((n = net_recv(sockfd, recvline, sizeof(recvline))) > 0) {
	  recvline[n] = 0;
	  line++;
/*	    fputs(recvline, stdout); */
      }
      if (n < 0) {
	 char msg[200];
         sprintf(msg, "handle_client: net_recv error: %s\n", strerror(-n));
	 error_abort(msg);
      }
      if ( (i % 100) == 0) {
         printf("%d lines=%d\n", i, line);
      }

   }
   net_close(sockfd);
#endif
   done = time(NULL);
   printf("Total time = %ld secs.\n", done - now);
   exit(0);
}

#else /* HAVE_NISLIB */

int main(int argc, char *argv[]) {
    printf("Sorry, NIS code is not compiled in apcupsd.\n");
    return 1;
}

#endif /* HAVE_NISLIB */
