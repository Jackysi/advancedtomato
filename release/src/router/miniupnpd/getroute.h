/* $Id: getroute.h,v 1.2 2012/09/27 16:00:10 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2012 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef  GETROUTE_H_INCLUDED
#define  GETROUTE_H_INCLUDED

int
get_src_for_route_to(const struct sockaddr * dst,
                     void * src, size_t * src_len);

#endif

