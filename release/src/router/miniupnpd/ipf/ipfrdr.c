/* $Id: ipfrdr.c,v 1.9 2008/08/24 19:54:57 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2007 Darren Reed
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
/*
 * This is a workaround for <sys/uio.h> troubles on FreeBSD, HPUX, OpenBSD.
 * Needed here because on some systems <sys/uio.h> gets included by things
 * like <sys/socket.h>
 */
#ifndef _KERNEL
# define ADD_KERNEL
# define _KERNEL
# define KERNEL
#endif
#ifdef __OpenBSD__
struct file;
#endif
#include <sys/uio.h>
#ifdef ADD_KERNEL
# undef _KERNEL
# undef KERNEL
#endif
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/ioctl.h>
#include <net/if.h>
#if __FreeBSD_version >= 300000
# include <net/if_var.h>
#endif
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#ifndef	TCP_PAWS_IDLE	/* IRIX */
# include <netinet/tcp.h>
#endif
#include <netinet/udp.h>

#include <arpa/inet.h>

#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <stdlib.h>
#include <fcntl.h>
#include <syslog.h>
#include <stddef.h>
#include <stdio.h>
#if !defined(__SVR4) && !defined(__svr4__) && defined(sun)
# include <strings.h>
#endif
#include <string.h>
#include <unistd.h>

#include "../config.h"
#include "netinet/ipl.h"
#include "netinet/ip_compat.h"
#include "netinet/ip_fil.h"
#include "netinet/ip_nat.h"
#include "netinet/ip_state.h"


#ifndef __P
# ifdef __STDC__
#  define	__P(x)	x
# else
#  define	__P(x)	()
# endif
#endif
#ifndef __STDC__
# undef		const
# define	const
#endif

#ifndef	U_32_T
# define	U_32_T	1
# if defined(__NetBSD__) || defined(__OpenBSD__) || defined(__FreeBSD__) || \
    defined(__sgi)
typedef	u_int32_t	u_32_t;
# else
#  if defined(__alpha__) || defined(__alpha) || defined(_LP64)
typedef unsigned int	u_32_t;
#  else
#   if SOLARIS2 >= 6
typedef uint32_t	u_32_t;
#   else
typedef unsigned int	u_32_t;
#   endif
#  endif
# endif /* __NetBSD__ || __OpenBSD__ || __FreeBSD__ || __sgi */
#endif /* U_32_T */


#if defined(__NetBSD__) || defined(__OpenBSD__) || \
        (_BSDI_VERSION >= 199701) || (__FreeBSD_version >= 300000) || \
	SOLARIS || defined(__sgi) || defined(__osf__) || defined(linux)
# include <stdarg.h>
typedef	int	(* ioctlfunc_t) __P((int, ioctlcmd_t, ...));
#else
typedef	int	(* ioctlfunc_t) __P((dev_t, ioctlcmd_t, void *));
#endif
typedef	void	(* addfunc_t) __P((int, ioctlfunc_t, void *));
typedef	int	(* copyfunc_t) __P((void *, void *, size_t));


/*
 * SunOS4
 */
#if defined(sun) && !defined(__SVR4) && !defined(__svr4__)
extern	int	ioctl __P((int, int, void *));
#endif

#include "../upnpglobalvars.h"

/* group name */
static const char group_name[] = "miniupnpd";

static int dev = -1;
static int dev_ipl = -1;

int init_redirect(void)
{
	dev = open(IPNAT_NAME, O_RDWR);
	if(dev<0) {
		syslog(LOG_ERR, "open(\"%s\"): %m", IPNAT_NAME);
		return -1;
	}
	dev_ipl = open(IPL_NAME, O_RDWR);
	if(dev_ipl < 0) {
		syslog(LOG_ERR, "open(\"%s\"): %m", IPL_NAME);
		return -1;
	}
	return 0;
}

void shutdown_redirect(void)
{
	if(dev >= 0) {
		close(dev);
		dev = -1;
	}
	if(dev_ipl >= 0) {
		close(dev_ipl);
		dev = -1;
	}
	return;
}

int opts=0;
int use_inet6=0;

int
add_redirect_rule2(const char * ifname, unsigned short eport,
                   const char * iaddr, unsigned short iport, int proto,
				   const char * desc)
{
	struct ipnat ipnat;
	struct ipfobj obj;
	int r;

	if(dev<0) {
		syslog(LOG_ERR, "%s not open", IPNAT_NAME);
		return -1;
	}

	memset(&obj, 0, sizeof(obj));
	memset(&ipnat, 0, sizeof(ipnat));

	ipnat.in_redir = NAT_REDIRECT;
	ipnat.in_p = proto;
	if (proto == IPPROTO_TCP)
		ipnat.in_flags = IPN_TCP;
	if (proto == IPPROTO_UDP)
		ipnat.in_flags = IPN_UDP;
	ipnat.in_dcmp = FR_EQUAL;
	ipnat.in_pmin = htons(eport);
	ipnat.in_pmax = htons(eport);
	ipnat.in_pnext = htons(iport);
	ipnat.in_v = 4;
#ifdef USE_IFNAME_IN_RULES
	if(ifname) {
		strlcpy(ipnat.in_ifnames[0], ifname, IFNAMSIZ);
		strlcpy(ipnat.in_ifnames[1], ifname, IFNAMSIZ);
	}
#endif
	strlcpy(ipnat.in_tag.ipt_tag, group_name, IPFTAG_LEN);
	inet_pton(AF_INET, iaddr, &ipnat.in_in[0].in4);
	ipnat.in_in[1].in4.s_addr = 0xffffffff;

	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_size = sizeof(ipnat);
	obj.ipfo_ptr = &ipnat;
	obj.ipfo_type = IPFOBJ_IPNAT;

	r = ioctl(dev, SIOCADNAT, &obj);
	if (r == -1)
		syslog(LOG_ERR, "ioctl(SIOCADNAT): %m");
	return r;
}

/* get_redirect_rule()
 * return value : 0 success (found)
 * -1 = error or rule not found */
int
get_redirect_rule(const char * ifname, unsigned short eport, int proto,
                  char * iaddr, int iaddrlen, unsigned short * iport,
                  char * desc, int desclen,
                  u_int64_t * packets, u_int64_t * bytes)
{
	ipfgeniter_t iter;
	ipfobj_t obj;
	ipnat_t ipn;

	bzero((char *)&obj, sizeof(obj));
	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_type = IPFOBJ_GENITER;
	obj.ipfo_size = sizeof(iter);
	obj.ipfo_ptr = &iter;

	iter.igi_type = IPFGENITER_IPNAT;
#if IPFILTER_VERSION > 4011300
	iter.igi_nitems = 1;
#endif
	iter.igi_data = &ipn;

	if(dev<0) {
		syslog(LOG_ERR, "%s not open", IPNAT_NAME);
		return -1;
	}

	do {
		if (ioctl(dev, SIOCGENITER, &obj) == -1) {
			syslog(LOG_ERR, "ioctl(dev, SIOCGENITER): %m");
			goto error;
		}
		if ((eport == ntohs(ipn.in_pmin)) &&
		    (eport == ntohs(ipn.in_pmax)) &&
		    (ipn.in_p == proto)) {
			strlcpy(desc, "", desclen);
			if (packets != NULL)
				*packets = 0;
			if (bytes != NULL)
				*bytes = 0;
			if (iport != NULL)
				*iport = ipn.in_pnext;
			inet_ntop(AF_INET, &ipn.in_in[0].in4, iaddr, iaddrlen);
			return 0;
		}
	} while (ipn.in_next != NULL);
error:
	return -1;
}

int
delete_redirect_rule(const char * ifname, unsigned short eport, int proto)
{
	ipfgeniter_t iter;
	ipfobj_t obj;
	ipnat_t ipn;
	int r;

	bzero((char *)&obj, sizeof(obj));
	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_type = IPFOBJ_GENITER;
	obj.ipfo_size = sizeof(iter);
	obj.ipfo_ptr = &iter;

	iter.igi_type = IPFGENITER_IPNAT;
#if IPFILTER_VERSION > 4011300
	iter.igi_nitems = 1;
#endif
	iter.igi_data = &ipn;

	if(dev<0) {
		syslog(LOG_ERR, "%s not open", IPNAT_NAME);
		return -1;
	}

	do {
		if (ioctl(dev, SIOCGENITER, &obj) == -1) {
			syslog(LOG_ERR, "%s:ioctl(SIOCGENITER): %m",
			       "delete_redirect_rule");
			goto error;
		}
		if ((eport == ntohs(ipn.in_pmin)) &&
		    (eport == ntohs(ipn.in_pmax)) &&
		    (ipn.in_p == proto)) {
			obj.ipfo_rev = IPFILTER_VERSION;
			obj.ipfo_size = sizeof(ipn);
			obj.ipfo_ptr = &ipn;
			obj.ipfo_type = IPFOBJ_IPNAT;
			r = ioctl(dev, SIOCRMNAT, &obj);
			if (r == -1)
				syslog(LOG_ERR, "%s:ioctl(SIOCRMNAT): %m",
				       "delete_redirect_rule");
			return r;
		}
	} while (ipn.in_next != NULL);
error:
	return -1;
}

/* thanks to Seth Mos for this function */
int
add_filter_rule2(const char * ifname, const char * iaddr,
                 unsigned short eport, unsigned short iport,
				 int proto, const char * desc)
{
	ipfobj_t obj;
	frentry_t fr;
	fripf_t ipffr;
	int r;

	if(dev_ipl<0) {
		syslog(LOG_ERR, "%s not open", IPL_NAME);
		return -1;
	}

	r = 0;

	memset(&obj, 0, sizeof(obj));
	memset(&fr, 0, sizeof(fr));
	memset(&ipffr, 0, sizeof(ipffr));

	fr.fr_flags = FR_PASS|FR_KEEPSTATE|FR_QUICK|FR_INQUE;
	if (GETFLAG(LOGPACKETSMASK))
		fr.fr_flags |= FR_LOG|FR_LOGFIRST;
	fr.fr_v = 4;

	fr.fr_type = FR_T_IPF;
	fr.fr_dun.fru_ipf = &ipffr;
	fr.fr_dsize = sizeof(ipffr);
	fr.fr_isc = (void *)-1;

	fr.fr_proto = proto;
	fr.fr_mproto = 0xff;
	fr.fr_dcmp = FR_EQUAL;
	fr.fr_dport = eport;
#ifdef USE_IFNAME_IN_RULES
	if(ifname ) {
		strlcpy(fr.fr_ifnames[0], ifname, IFNAMSIZ);
	}
#endif
	strlcpy(fr.fr_group, group_name, sizeof(fr.fr_group));

	if (proto == IPPROTO_TCP) {
		fr.fr_tcpf = TH_SYN;
		fr.fr_tcpfm = TH_SYN|TH_ACK|TH_RST|TH_FIN|TH_URG|TH_PUSH;
	}

	inet_pton(AF_INET, iaddr, &fr.fr_daddr);
	fr.fr_dmask = 0xffffffff;

	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_ptr = &fr;
	obj.ipfo_size = sizeof(fr);

	r = ioctl(dev_ipl, SIOCINAFR, &obj);
	if (r == -1) {
		if (errno == ESRCH) {
			syslog(LOG_ERR,
			       "SIOCINAFR(missing 'head %s' rule?):%m",
			       group_name);
		} else {
			syslog(LOG_ERR, "SIOCINAFR:%m");
		}
	}
	return r;
}

int
delete_filter_rule(const char * ifname, unsigned short eport, int proto)
{
	ipfobj_t wobj, dobj;
	ipfruleiter_t rule;
	u_long darray[1000];
	u_long array[1000];
	friostat_t fio;
	frentry_t *fp;

	if(dev_ipl<0) {
		syslog(LOG_ERR, "%s not open", IPL_NAME);
		return -1;
	}

	wobj.ipfo_rev = IPFILTER_VERSION;
	wobj.ipfo_type = IPFOBJ_IPFSTAT;
	wobj.ipfo_size = sizeof(fio);
	wobj.ipfo_ptr = &fio;

	if (ioctl(dev, SIOCGETFS, &wobj) == -1) {
		syslog(LOG_ERR, "ioctl(SIOCGETFS): %m");
		goto error;
	}

	wobj.ipfo_rev = IPFILTER_VERSION;
	wobj.ipfo_ptr = &rule;
	wobj.ipfo_size = sizeof(rule);
	wobj.ipfo_type = IPFOBJ_IPFITER;

	fp = (frentry_t *)array;
	fp->fr_dun.fru_data = darray;
	fp->fr_dsize = sizeof(darray);

	rule.iri_inout = 0;
	rule.iri_active = fio.f_active;
#if IPFILTER_VERSION > 4011300
	rule.iri_nrules = 1;
	rule.iri_v = 4;
#endif
	rule.iri_rule = fp;

	dobj.ipfo_rev = IPFILTER_VERSION;
	dobj.ipfo_size = sizeof(*fp);
	dobj.ipfo_type = IPFOBJ_FRENTRY;

	do {

		memset(array, 0xff, sizeof(array));

		if (ioctl(dev_ipl, SIOCIPFITER, &wobj) == -1) {
			syslog(LOG_ERR, "ioctl(SIOCIPFITER): %m");
			goto error;
		}

		if (fp->fr_data != NULL)
			fp->fr_data = (char *)fp + sizeof(*fp);
		if ((fp->fr_type == FR_T_IPF) &&
		    (fp->fr_dport == eport) && (fp->fr_proto == proto)) {
			dobj.ipfo_ptr = fp;

			if (ioctl(dev_ipl, SIOCRMAFR, &dobj) == -1) {
				syslog(LOG_ERR, "ioctl(SIOCRMAFR): %m");
				goto error;
			} else {
				return 0;
			}
		}
	} while (fp->fr_next != NULL);
error:
	return -1;
}

int
get_redirect_rule_by_index(int index,
                           char * ifname, unsigned short * eport,
                           char * iaddr, int iaddrlen, unsigned short * iport,
                           int * proto, char * desc, int desclen,
                           u_int64_t * packets, u_int64_t * bytes)
{
	ipfgeniter_t iter;
	ipfobj_t obj;
	ipnat_t ipn;
	int n;

	if (index < 0)
		return -1;

	if(dev<0) {
		syslog(LOG_ERR, "%s not open", IPNAT_NAME);
		return -1;
	}

	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_ptr = &iter;
	obj.ipfo_size = sizeof(iter);
	obj.ipfo_type = IPFOBJ_GENITER;

	iter.igi_type = IPFGENITER_IPNAT;
#if IPFILTER_VERSION > 4011300
	iter.igi_nitems = 1;
#endif
	iter.igi_data = &ipn;

	for (n = 0; ; n++) {
		if (ioctl(dev, SIOCGENITER, &obj) == -1) {
			syslog(LOG_ERR, "%s:ioctl(SIOCGENITER): %m",
			       "get_redirect_rule_by_index");
			break;
		}

		if (index == n)
			break;

		if (ipn.in_next == NULL) {
			n = -1;
			break;
		}
	}

	if (index != n)
		goto error;

	*proto = ipn.in_p;
	*eport = ntohs(ipn.in_pmax);
	*iport = ntohs(ipn.in_pnext);

	if (ifname)
		strlcpy(ifname, ipn.in_ifnames[0], IFNAMSIZ);
	if (packets != NULL)
		*packets = 0;
	if (bytes != NULL)
		*bytes = 0;

	inet_ntop(AF_INET, &ipn.in_in[0].in4, iaddr, iaddrlen);

	return 0;
error:
	return -1;
}

