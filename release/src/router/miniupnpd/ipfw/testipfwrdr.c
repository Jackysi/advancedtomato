/* $Id: testipfwrdr.c,v 1.4 2011/05/28 09:29:08 nanard Exp $ */
/*
 * MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2009-2011 Jardel Weyrich, Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution
 */

#include <stdio.h>
#include <syslog.h>
#include <time.h>
#include <netinet/in.h>
#include "ipfwrdr.h"

// test program for ipfwrdr.c

int main(int argc, char * * argv) {
	unsigned int timestamp;
	char desc[64];
	char addr[16];
	unsigned short iport = 0;

	desc[0] = '\0';
	addr[0] = '\0';
	openlog("testipfwrdrd", LOG_CONS | LOG_PERROR, LOG_USER);
	init_redirect();
	delete_redirect_rule("lo", 2222, IPPROTO_TCP);
	add_redirect_rule2("lo", 2222, "10.1.1.16", 4444, IPPROTO_TCP, "miniupnpd", time(NULL) + 60);
	get_redirect_rule("lo", 2222, IPPROTO_TCP, addr, sizeof(addr), &iport,
	                  desc, sizeof(desc), &timestamp, NULL, NULL);
	printf("%s:%hu '%s' %u\n", addr, iport, desc, timestamp);
	shutdown_redirect();
	return 0;
}

