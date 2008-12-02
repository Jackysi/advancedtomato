#ifndef __LINUX_BRIDGE_EBT_IP_H
#define __LINUX_BRIDGE_EBT_IP_H

#include "linux/netfilter/x_tables.h"

#define EBT_XT_MATCH "xt"

struct ebt_xt_info
{
	/* points to the relevant module's struct containing all essential
	 * data (userspace and kernel) */
	void *xt_match_struct;
	unsigned char data[0] __attribute__ ((aligned (__alignof__(struct ebt_replace))));
};

#endif
