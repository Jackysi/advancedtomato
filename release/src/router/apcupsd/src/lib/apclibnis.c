/*
 * apclibnis.c
 *
 * Network utility routines.
 *
 * Part of this code is derived from the Prentice Hall book
 * "Unix Network Programming" by W. Richard Stevens
 *
 * Developers, please note: do not include apcupsd headers
 * or other apcupsd internal information in this file
 * as it is used by independent client programs such as the cgi
 * programs.
 */

/*
 * Copyright (C) 1999-2006 Kern Sibbald
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

#include "apc.h"

#ifdef HAVE_NISLIB

/* Some Win32 specific screwery */
#ifdef HAVE_MINGW

#define close(fd)             closesocket(fd)
#define ioctl(s,p,v)          ioctlsocket((s),(p),(u_long*)(v))
#define getsockopt(s,l,o,d,z) getsockopt((s),(l),(o),(char*)(d),(z))
#define EINPROGRESS           WSAEWOULDBLOCK

int dummy = WSA_Init();

#undef errno
#define errno   WSAGetLastError()

#undef h_errno
#define h_errno WSAGetLastError()

#endif // HAVE_MINGW

/*
 * Read nbytes from the network.
 * It is possible that the total bytes require in several
 * read requests
 */

static int read_nbytes(int fd, char *ptr, int nbytes)
{
   int nleft, nread = 0;
   struct timeval timeout;
   int rc;
   fd_set fds;

   nleft = nbytes;

   while (nleft > 0) {
      do {
         /* Expect data from the server within 15 seconds */
         timeout.tv_sec = 15;
         timeout.tv_usec = 0;

         FD_ZERO(&fds);
         FD_SET(fd, &fds);

         rc = select(fd + 1, &fds, NULL, NULL, &timeout);

         switch (rc) {
         case -1:
            if (errno == EINTR || errno == EAGAIN)
               continue;
            return -errno;       /* error */
         case 0:
            return -ETIMEDOUT;   /* timeout */
         }

         nread = recv(fd, ptr, nleft, 0);
      } while (nread == -1 && (errno == EINTR || errno == EAGAIN));

      if (nread == 0)
         return 0;               /* EOF */
      if (nread < 0)
         return -errno;          /* error */

      nleft -= nread;
      ptr += nread;
   }

   return nbytes - nleft;        /* return >= 0 */
}

/*
 * Write nbytes to the network.
 * It may require several writes.
 */
static int write_nbytes(int fd, const char *ptr, int nbytes)
{
   int nleft, nwritten;

   nleft = nbytes;
   while (nleft > 0) {
#if defined HAVE_OPENBSD_OS || defined HAVE_FREEBSD_OS
      /*       
       * Work around a bug in OpenBSD & FreeBSD userspace pthreads
       * implementations.
       *
       * The pthreads implementation under the hood sets O_NONBLOCK
       * implicitly on all fds. This setting is not visible to the user
       * application but is relied upon by the pthreads library to prevent
       * blocking syscalls in one thread from halting all threads in the
       * process. When a process exit()s or exec()s, the implicit
       * O_NONBLOCK flags are removed from all fds, EVEN THOSE IT INHERITED.
       * If another process is still using the inherited fds, there will
       * soon be trouble.
       *
       * apcupsd is bitten by this issue after fork()ing a child process to
       * run apccontrol.
       *
       * This seemingly-pointless fcntl() call causes the pthreads
       * library to reapply the O_NONBLOCK flag appropriately.
       */
      fcntl(fd, F_SETFL, fcntl(fd, F_GETFL));
#endif
      nwritten = send(fd, ptr, nleft, 0);

      switch (nwritten) {
      case -1:
         if (errno == EINTR || errno == EAGAIN)
            continue;
         return -errno;           /* error */
      case 0:
         return nbytes - nleft;   /* EOF */
      }

      nleft -= nwritten;
      ptr += nwritten;
   }

   return nbytes - nleft;
}

/* 
 * Receive a message from the other end. Each message consists of
 * two packets. The first is a header that contains the size
 * of the data that follows in the second packet.
 * Returns number of bytes read
 * Returns 0 on end of file
 * Returns -1 on hard end of file (i.e. network connection close)
 * Returns -2 on error
 */
int net_recv(int sockfd, char *buff, int maxlen)
{
   int nbytes;
   short pktsiz;

   /* get data size -- in short */
   if ((nbytes = read_nbytes(sockfd, (char *)&pktsiz, sizeof(short))) <= 0) {
      /* probably pipe broken because client died */
      return nbytes;               /* assume hard EOF received */
   }
   if (nbytes != sizeof(short))
      return -EINVAL;

   pktsiz = ntohs(pktsiz);         /* decode no. of bytes that follow */
   if (pktsiz > maxlen)
      return -EINVAL;
   if (pktsiz == 0)
      return 0;                    /* soft EOF */

   /* now read the actual data */
   if ((nbytes = read_nbytes(sockfd, buff, pktsiz)) <= 0)
      return nbytes;
   if (nbytes != pktsiz)
      return -EINVAL;

   return nbytes;                /* return actual length of message */
}

/*
 * Send a message over the network. The send consists of
 * two network packets. The first is sends a short containing
 * the length of the data packet which follows.
 * Returns number of bytes sent
 * Returns -1 on error
 */
int net_send(int sockfd, const char *buff, int len)
{
   int rc;
   short pktsiz;

   /* send short containing size of data packet */
   pktsiz = htons((short)len);
   rc = write_nbytes(sockfd, (char *)&pktsiz, sizeof(short));
   if (rc <= 0)
      return rc;
   if (rc != sizeof(short))
      return -EINVAL;

   /* send data packet */
   rc = write_nbytes(sockfd, buff, len);
   if (rc <= 0)
      return rc;
   if (rc != len)
      return -EINVAL;

   return rc;
}

/*     
 * Open a TCP connection to the UPS network server
 * Returns -1 on error
 * Returns socket file descriptor otherwise
 */
int net_open(const char *host, char *service, int port)
{
   int nonblock = 1;
   int block = 0;
   int sockfd, rc;
   struct sockaddr_in tcp_serv_addr;  /* socket information */

#ifndef HAVE_MINGW
   // Every platform has their own magic way to avoid getting a SIGPIPE
   // when writing to a stream socket where the remote end has closed. 
   // This method works pretty much everywhere which avoids the mess
   // of figuring out which incantation this platform supports. (Excepting
   // for win32 which doesn't support signals at all.)
   struct sigaction sa;
   sa.sa_handler = SIG_IGN;
   sigaction(SIGPIPE, &sa, NULL);
#endif

   /* 
    * Fill in the structure serv_addr with the address of
    * the server that we want to connect with.
    */
   memset((char *)&tcp_serv_addr, 0, sizeof(tcp_serv_addr));
   tcp_serv_addr.sin_family = AF_INET;
   tcp_serv_addr.sin_port = htons(port);
   tcp_serv_addr.sin_addr.s_addr = inet_addr(host);
   if (tcp_serv_addr.sin_addr.s_addr == INADDR_NONE) {
      struct hostent he;
      char *tmphstbuf = NULL;
      size_t hstbuflen = 0;
      struct hostent *hp = gethostname_re(host, &he, &tmphstbuf, &hstbuflen);
      if (!hp)
      {
         free(tmphstbuf);
         return -h_errno;
      }

      if (hp->h_length != sizeof(tcp_serv_addr.sin_addr.s_addr) || 
          hp->h_addrtype != AF_INET)
      {
         free(tmphstbuf);
         return -EINVAL;
      }

      memcpy(&tcp_serv_addr.sin_addr.s_addr, hp->h_addr, 
             sizeof(tcp_serv_addr.sin_addr.s_addr));
      free(tmphstbuf);
   }

   /* Open a TCP socket */
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      return -errno;

   /* connect to server */
#if defined HAVE_OPENBSD_OS || defined HAVE_FREEBSD_OS
   /* 
    * Work around a bug in OpenBSD & FreeBSD userspace pthreads
    * implementations. Rationale is the same as described above.
    */
   fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL));
#endif

   /* Set socket to non-blocking mode */
   if (ioctl(sockfd, FIONBIO, &nonblock) != 0) {
      close(sockfd);
      return -errno;
   }

   /* Initiate connection attempt */
   rc = connect(sockfd, (struct sockaddr *)&tcp_serv_addr, sizeof(tcp_serv_addr));
   if (rc == -1 && errno != EINPROGRESS) {
      close(sockfd);
      return -errno;
   }

   /* If connection is in progress, wait for it to complete */
   if (rc == -1) {
      struct timeval timeout;
      fd_set fds;
      int err;
      socklen_t errlen = sizeof(err);

      do {
         /* Expect connection within 5 seconds */
         timeout.tv_sec = 5;
         timeout.tv_usec = 0;
         FD_ZERO(&fds);
         FD_SET(sockfd, &fds);

         /* Wait for connection to complete */
         rc = select(sockfd + 1, NULL, &fds, NULL, &timeout);
         switch (rc) {
         case -1: /* select error */
            if (errno == EINTR || errno == EAGAIN)
               continue;
            close(sockfd);
            return -errno;
         case 0: /* timeout */
            close(sockfd);
            return -ETIMEDOUT;
         }
      }
      while (rc == -1 && (errno == EINTR || errno == EAGAIN));

      /* Connection completed? Check error status. */
      if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) {
         close(sockfd);
         return -errno;
      }
      if (errlen != sizeof(err)) {
         close(sockfd);
         return -EINVAL;
      }
      if (err) {
         close(sockfd);
         return -err;
      }
   }

   /* Connection completed successfully. Set socket back to blocking mode. */
   if (ioctl(sockfd, FIONBIO, &block) != 0) {
      close(sockfd);
      return -errno;
   }

   return sockfd;
}

/* Close the network connection */
void net_close(int sockfd)
{
   close(sockfd);
}

/*     
 * Accept a TCP connection.
 * Returns -1 on error.
 * Returns file descriptor of new connection otherwise.
 */
int net_accept(int fd, struct sockaddr_in *cli_addr)
{
#ifdef HAVE_MINGW                                       
   /* kludge because some idiot defines socklen_t as unsigned */
   int clilen = sizeof(*cli_addr);
#else
   socklen_t clilen = sizeof(*cli_addr);
#endif
   int newfd;

#if defined HAVE_OPENBSD_OS || defined HAVE_FREEBSD_OS
   int rc;
   fd_set fds;
#endif

   do {

#if defined HAVE_OPENBSD_OS || defined HAVE_FREEBSD_OS
      /*
       * Work around a bug in OpenBSD & FreeBSD userspace pthreads
       * implementations. Rationale is the same as described above.
       */
      do {
         FD_ZERO(&fds);
         FD_SET(fd, &fds);
         rc = select(fd + 1, &fds, NULL, NULL, NULL);
      } while (rc == -1 && (errno == EINTR || errno == EAGAIN));

      if (rc < 0)
         return -errno;              /* error */
#endif
      newfd = accept(fd, (struct sockaddr *)cli_addr, &clilen);
   } while (newfd == -1 && (errno == EINTR || errno == EAGAIN));

   if (newfd < 0)
      return -errno;                 /* error */

   return newfd;
}
#endif                             /* HAVE_NISLIB */
