#ifndef	__OSDEP_H__
#define __OSDEP_H__

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#if	defined(__linux__) || defined(__FreeBSD__) || defined(__MIPSEB__)
#define	ETHERBOOT32
#define ntohl(x) (x)
#define htonl(x) (x)
#define ntohs(x) (x)
#define htons(x) (x)

#endif




/* ANSI prototyping macro */
#ifdef	__STDC__
#define	P(x)	x
#else
#define	P(x)	()
#endif

#endif

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */
