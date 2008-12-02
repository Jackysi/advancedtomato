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

#define VERSION "1.10"

#define FTABSIZ 20 /* max number of outstanding requests */
#define CACHESIZ 150 /* default cache size */
#define MAXTOK 50 /* token in DHCP leases */
#define SMALLDNAME 63 /* most domain names are smaller than this */
#define CONFFILE "/etc/dnsmasq.conf"
#define HOSTSFILE "/etc/hosts"
#define RESOLVFILE "/etc/resolv.conf"
#define RUNFILE "/var/run/dnsmasq.pid"
#define CHUSER "nobody"
#define IP6INTERFACES "/proc/net/if_inet6"

/* Follows system specific switches. If you run on a 
   new system, you may want to edit these. 
   May replace this with Autoconf one day. 

HAVE_IPV6
   define this to include IPv6 support. There's very little to be gained
   by leaving IPv6 support out, but this flag should enable compilation
   against a libc which doesn't support IPv6

HAVE_LINUX_IPV6_PROC
   define this to do IPv6 interface discovery using
   proc/net/if_inet6 ala LINUX. 

HAVE_GETOPT_LONG
   define this if you have GNU libc or GNU getopt. 

HAVE_BROKEN_SOCKADDR_IN6
   we provide our own declaration of sockaddr_in6,
   since old versions of glibc are broken. 

HAVE_ARC4RANDOM
   define this if you have arc4random() to get better security from DNS spoofs
   by using really random ids (OpenBSD) 

HAVE_RANDOM
   define this if you have the 4.2BSD random() function (and its
   associated srandom() function), which is at least as good as (if not
   better than) the rand() function.

HAVE_DEV_RANDOM
   define this if you have the /dev/random device, which gives truly
   random numbers but may run out of random numbers.

HAVE_DEV_URANDOM
   define this if you have the /dev/urandom device, which gives
   semi-random numbers when it runs out of truly random numbers.

HAVE_SOCKADDR_SA_LEN
   define this if struct sockaddr has sa_len field (*BSD) 

HAVE_FORK
  define this if unless you don't want dnsmasq to fork while 
  backgrounding itself. Undefing it is only useful on uCLinux.

NOTES:
   For Linux you should define 
      HAVE_IPV6
      HAVE_LINUX_IPV6_PROC 
      HAVE_GETOPT_LONG
      HAVE_RANDOM
      HAVE_DEV_RANDOM
      HAVE_DEV_URANDOM
   you should NOT define 
      HAVE_ARC4RANDOM
      HAVE_SOCKADDR_SA_LEN
   and you MAY have to define 
     HAVE_BROKEN_SOCKADDR_IN6 - if you have an old libc6.

   For *BSD systems you should define 
     HAVE_IPV6
     HAVE_SOCKADDR_SA_LEN
     HAVE_RANDOM
   you should NOT define  
     HAVE_LINUX_IPV6_PROC 
     HAVE_BROKEN_SOCKADDR_IN6
   and you MAY define  
     HAVE_ARC4RANDOM - OpenBSD and FreeBSD 
     HAVE_DEV_URANDOM - OpenBSD and FreeBSD
     HAVE_DEV_RANDOM - FreeBSD (OpenBSD with hardware random number generator)
     HAVE_GETOPT_LONG - only if you link GNU getopt. 

   For all *nix systems _other_ than uClinux you should define
     HAVE_FORK
*/

/* Must preceed __linux__ dince uClinux defines __linux__ too. */
#if defined(__uClinux__)
#undef HAVE_IPV6
#undef HAVE_LINUX_IPV6_PROC
#define HAVE_GETOPT_LONG
#undef HAVE_ARC4RANDOM
#define HAVE_RANDOM
#define HAVE_DEV_URANDOM
#define HAVE_DEV_RANDOM
#undef HAVE_SOCKADDR_SA_LEN
#undef HAVE_BROKEN_SOCKADDR_IN6
#undef HAVE_FORK
#define HAVE_FILE_SYSTEM

#undef RESOLVFILE
#define RESOLVFILE "/etc/config/resolv.conf"

#elif defined(__linux__)
#define HAVE_IPV6
#define HAVE_LINUX_IPV6_PROC
#define HAVE_GETOPT_LONG
#undef HAVE_ARC4RANDOM
#define HAVE_RANDOM
#define HAVE_DEV_URANDOM
#define HAVE_DEV_RANDOM
#undef HAVE_SOCKADDR_SA_LEN
#define HAVE_BROKEN_SOCKADDR_IN6
#define HAVE_FORK
#define HAVE_FILE_SYSTEM

#elif defined(__FreeBSD__) || defined(__OpenBSD__)
#define HAVE_IPV6
#undef HAVE_LINUX_IPV6_PROC
#undef HAVE_GETOPT_LONG
#define HAVE_ARC4RANDOM
#define HAVE_RANDOM
#define HAVE_DEV_URANDOM
#define HAVE_SOCKADDR_SA_LEN
#undef HAVE_BROKEN_SOCKADDR_IN6
#define HAVE_FORK
#define HAVE_FILE_SYSTEM

#elif defined(__NetBSD__)
#define HAVE_IPV6
#undef HAVE_LINUX_IPV6_PROC
#undef HAVE_GETOPT_LONG
#undef HAVE_ARC4RANDOM
#define HAVE_RANDOM
#undef HAVE_DEV_URANDOM
#undef HAVE_DEV_RANDOM
#define HAVE_SOCKADDR_SA_LEN
#undef HAVE_BROKEN_SOCKADDR_IN6
#define HAVE_FORK
#define HAVE_FILE_SYSTEM
 
/* env "LIBS=-lsocket -lnsl" make */
#elif defined(__sun) || defined(__sun__)
#define HAVE_IPV6
#undef HAVE_LINUX_IPV6_PROC
#undef HAVE_GETOPT_LONG
#undef HAVE_ARC4RANDOM
#define HAVE_RANDOM
#undef HAVE_DEV_URANDOM
#undef HAVE_DEV_RANDOM
#undef HAVE_SOCKADDR_SA_LEN
#undef HAVE_BROKEN_SOCKADDR_IN6
#define HAVE_FORK
#define HAVE_FILE_SYSTEM

#endif




