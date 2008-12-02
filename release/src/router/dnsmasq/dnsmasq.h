/* dnsmasq is Copyright (c) 2000 Simon Kelley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

/* Author's email: simon@thekelleys.org.uk */

#include "config.h"

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#if defined(__sun) || defined(__sun__)
#include <sys/sockio.h>
#endif
#include <sys/time.h>
#include <net/if.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <syslog.h>
#ifdef HAVE_GETOPT_LONG
#  include <getopt.h>
#endif
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#define OPT_BOGUSPRIV     1
#define OPT_FILTER        2
#define OPT_LOG           4
#define OPT_SELFMX        8
#define OPT_NO_HOSTS      16
#define OPT_NO_POLL       32
#define OPT_DEBUG         64
#define OPT_ORDER         128

struct all_addr {
  union {
    struct in_addr addr4;
#ifdef HAVE_IPV6
    struct in6_addr addr6;
#endif
  } addr;
};

union bigname {
  char name[MAXDNAME];
  union bigname *next; /* freelist */
};

struct crec { 
  struct crec *next, *prev;
  time_t ttd; /* time to die */
  int flags;
  struct all_addr addr;
  union {
    char sname[SMALLDNAME];
    union bigname *bname;
  } name;
  char query_name[SMALLDNAME];
};

#define F_IMMORTAL  1
#define F_NEW       2
#define F_REVERSE   4
#define F_FORWARD   8
#define F_DHCP      16 
#define F_NEG       32       
#define F_HOSTS     64
#define F_IPV4      128
#define F_IPV6      256
#define F_BIGNAME   512
#define F_UPSTREAM  1024
#define F_SERVER    2048

/* struct sockaddr is not large enough to hold any address,
   and specifically not big enough to hold and IPv6 address.
   Blech. Roll our own. */
union mysockaddr {
  struct sockaddr sa;
  struct sockaddr_in in;
#ifdef HAVE_IPV6
#ifdef HAVE_BROKEN_SOCKADDR_IN6
  /* early versions of glibc don't include sin6_scope_id in sockaddr_in6
     but latest kernels _require_ it to be set. The choice is to have
     dnsmasq fail to compile on back-level libc or fail to run
     on latest kernels with IPv6. Or to do this: sorry that it's so gross. */
  struct my_sockaddr_in6 {
    sa_family_t     sin6_family;    /* AF_INET6 */
    uint16_t        sin6_port;      /* transport layer port # */
    uint32_t        sin6_flowinfo;  /* IPv6 traffic class & flow info */
    struct in6_addr sin6_addr;      /* IPv6 address */
    uint32_t        sin6_scope_id;  /* set of interfaces for a scope */
  } in6;
#else
  struct sockaddr_in6 in6;
#endif
#endif
};

struct server {
  union mysockaddr addr;
  char *domain; /* set if this server only handles a domain. */ 
  int from_resolv; /* 1 for servers from resolv, 0 for command line. */
  struct server *next; /* circle */
};

/* linked list of all the interfaces in the system and 
   the sockets we have bound to each one. */
struct irec {
  union mysockaddr addr;
  int fd;
  struct irec *next;
};

struct iname {
  char *name;
  union mysockaddr addr;
  struct iname *next;
  int found;
};

/* ******* zg porting DWG814I Source code on 2006.11.06 ******* */
/* ******* To fixed cdrouterv3.3 item 333(dna_45) item 334(dns_45) failed bug ******* */
#define DNS_MSG_BUF_NUM FTABSIZ
#define DNSPACKETSZ             PACKETSZ+MAXDNAME+RRFIXEDSZ
struct DNS_MSG_BUF
{
        int udpfd;
        union mysockaddr udpaddr;
        struct server *servers;
        struct server *firstsentto;
        struct server *nextsentto;
        int gotname;
        int plen;
        char header[DNSPACKETSZ];
        char usedflag;
};
/* ******* end by zg porting DWG814I Source code on 2006.11.06 ******* */

struct frec {
  union mysockaddr source;
  struct server *sentto;
  unsigned short orig_id, new_id;
  int fd;
  time_t time;
  /* ******* zg porting DWG814I Source code on 2006.11.06 ******* */
  /* ******* To fixed cdrouterv3.3 item 333(dna_45) item 334(dns_45) failed bug ******* */ 
  struct DNS_MSG_BUF * dnsMsgBufPtr;
};

/* ******* zg porting DWG814I Source code on 2006.11.06 ******* */
/* ******* To fixed cdrouterv3.3 item 333(dna_45) item 334(dns_45) failed bug ******* */
void init_dns_msg_buf(void);
struct DNS_MSG_BUF * get_dns_msg_buf(void);
void free_dns_msg_buf(struct DNS_MSG_BUF * dnsMsgBufPtr);


/* cache.c */
void cache_init(int cachesize, int log);
void log_query(int flags, char *name, struct all_addr *addr);
struct crec *cache_find_by_addr(struct crec *crecp,
				struct all_addr *addr, time_t now, int prot);
struct crec *cache_find_by_name(struct crec *crecp, 
				char *name, time_t now, int prot);
void cache_start_insert(void);
void cache_end_insert(int fail);
void cache_link(struct crec *crecp);
void cache_insert(char *name, struct all_addr *addr, time_t now, 
		  unsigned long ttl, int flags, int *fail);
void cache_reload(int no_hosts, char *buff);
struct crec *cache_clear_dhcp(void);
void dump_cache(int debug, int size);
char *cache_get_name(struct crec *crecp);

/* rfc1035.c */
int extract_request(HEADER *header,unsigned int qlen, char *name);
void extract_addresses(HEADER *header, unsigned int qlen, char *namebuff, unsigned long *timetolive);
int answer_request(HEADER *header, char *limit, unsigned int qlen, char *mxname, 
		   char *mxtarget, unsigned int options, char *namebuff);

/* dhcp.c */
void load_dhcp(char *file, char *suffix, time_t now, char *hostnamebuff);

/* util.c */
unsigned short rand16(void);
void canonicalise(char *s);
void die(char *message, char *arg1);
void *safe_malloc(int size);
char *safe_string_alloc(char *cp);
int sa_len(union mysockaddr *addr);
int sockaddr_isequal(union mysockaddr *s1, union mysockaddr *s2);
void safe_free(void *mem);

/* option.c */
unsigned int read_opts(int argc, char **argv, char *buff, char **resolv_file, 
		       char **mxname, char **mxtarget, char **lease_file, 
		       char **username, char **domain_suffix, char **runfile, 
		       struct iname **if_names, struct iname **if_addrs,
		       struct server **serv_addrs, int *cachesize, int *port, unsigned long *timetolive);

/* forward.c */
void forward_init(int first);
struct server *forward_query(int udpfd, int peerfd, int peerfd6,
			     union mysockaddr *udpaddr, HEADER *header, 
			     int plen, int strict_order, char *dnamebuff, 
			     struct server *servers, struct server *last_server);

/* ******* zg porting DWG814I Source code on 2006.11.06 ******* */
/* ******* To fixed cdrouterv3.3 item 333(dna_45) item 334(dns_45) failed bug ******* */
int reforward_query(struct frec *forward, int peerfd, int peerfd6, int strict_order, char *dnamebuff);

struct server *reply_query(int fd, char *packet, char *dnamebuff, struct server *last_server, unsigned long *timetolive);


/* network.c */
struct server *reload_servers(char *fname, char *buff, struct server *servers);
struct server *check_servers(struct server *new, struct irec *interfaces,
			     int peerfd, int peerfd6); 
struct irec *find_all_interfaces(struct iname *names, struct iname *addrs, int port);
