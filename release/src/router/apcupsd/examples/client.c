/*
 * Client test program for apcnetd
 * This program reads from standard input and passes the
 * commands to the apcupsd network information server.
 * It then prints to stdout the responses from the server.
 *
 * Build it with: cc -I../include client.c ../src/lib/libapc.a -o client
 *
 * Execute: ./client [host[:port]] [command]
 *   reads commands from STDIN if command is not present
 *   if command is present, it is sent to the daemon,
 *   the output is retrieved, then the program exits.
 *
 * The two commands currently (Apr 2001) accepted by the
 * server are "status" and "events".
 *
 * For additional examples of code, see cgi/upsfetch.c
 */

#include "apc.h"

#ifdef HAVE_NISLIB

/* Default values, can be changed on command line */
#define SERV_TCP_PORT 3551
#define SERV_HOST_ADDR "127.0.0.1"

void handle_client(FILE *fp, int sockfd, char *cmd);

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

   if ((sockfd = net_open(host, NULL, port)) < 0) {
      sprintf(msg, "client: tcp_open for host %s on %d failed\n", host, port);
      error_abort(msg);
   }

   handle_client(stdin, sockfd, cmd);	    /* do it all */
   net_close(sockfd);
   exit(0);
}

/*
 * Read the contents of the FILE *fp, write each line to the
 * stream socket (to the server process), then read a line back from
 * the socket and write it to the standard output.
 *
 * Return to the caller when an EOF is encountered on the input file.
 */

#define MAXLINE 5000

void handle_client(FILE *fp, int sockfd, char *cmd)
{
   int n;
   char sendline[MAXLINE];
   char recvline[MAXLINE+1];
   int quit = 0;

   while (!quit) {
      if (cmd) {
	 strcpy(sendline, cmd);       /* one shot command */
	 quit = 1;
      } else if (fgets(sendline, MAXLINE, fp) == NULL) {
	 break;
      }
      n = strlen(sendline);
      if (net_send(sockfd, sendline, n) != n)
         error_abort("handle_client: write error on socket");

      while ((n = net_recv(sockfd, recvline, sizeof(recvline))) > 0) {
	  recvline[n] = 0;
	  fputs(recvline, stdout);
      }
      if (n < 0) {
	 char msg[200];
         sprintf(msg, "handle_client: net_recv error: %s\n", strerror(-n));
	 error_abort(msg);
     }
   }
}

#else /* HAVE_NISLIB */

int main(int argc, char *argv[]) {
    printf("Sorry, NIS code is not compiled in apcupsd.\n");
    return 1;
}

#endif /* HAVE_NISLIB */
