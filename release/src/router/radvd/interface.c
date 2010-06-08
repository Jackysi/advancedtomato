/*
 *   $Id: interface.c,v 1.7 2004/10/26 05:30:34 psavola Exp $
 *
 *   Authors:
 *    Lars Fenneberg		<lf@elemental.net>	 
 *
 *   This software is Copyright 1996,1997 by the above mentioned author(s), 
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <lutchann@litech.org>.
 *
 */

#include <config.h>
#include <includes.h>
#include <radvd.h>
#include <defaults.h>

void
iface_init_defaults(struct Interface *iface)
{
	memset(iface, 0, sizeof(struct Interface));

	iface->IgnoreIfMissing	  = DFLT_IgnoreIfMissing;
	iface->AdvSendAdvert	  = DFLT_AdvSendAdv;
	iface->MaxRtrAdvInterval  = DFLT_MaxRtrAdvInterval;
	iface->AdvSourceLLAddress = DFLT_AdvSourceLLAddress;
	iface->AdvReachableTime	  = DFLT_AdvReachableTime;
	iface->AdvRetransTimer    = DFLT_AdvRetransTimer;
	iface->AdvLinkMTU	  = DFLT_AdvLinkMTU;
	iface->AdvCurHopLimit	  = DFLT_AdvCurHopLimit;
	iface->AdvIntervalOpt	  = DFLT_AdvIntervalOpt;
	iface->AdvHomeAgentInfo	  = DFLT_AdvHomeAgentInfo;
	iface->AdvHomeAgentFlag	  = DFLT_AdvHomeAgentFlag;
	iface->HomeAgentPreference = DFLT_HomeAgentPreference;
	iface->MinDelayBetweenRAs   = DFLT_MinDelayBetweenRAs;

	iface->MinRtrAdvInterval  = -1;
	iface->AdvDefaultLifetime = -1;
	iface->AdvDefaultPreference = DFLT_AdvDefaultPreference;
	iface->HomeAgentLifetime  = 0;
}

void
prefix_init_defaults(struct AdvPrefix *prefix)
{
	memset(prefix, 0, sizeof(struct AdvPrefix));
		
	prefix->AdvOnLinkFlag = DFLT_AdvOnLinkFlag;
	prefix->AdvAutonomousFlag = DFLT_AdvAutonomousFlag;
	prefix->AdvRouterAddr = DFLT_AdvRouterAddr;
	prefix->AdvValidLifetime = DFLT_AdvValidLifetime;
	prefix->AdvPreferredLifetime = DFLT_AdvPreferredLifetime;
	prefix->if6to4[0] = 0;
	prefix->enabled = 1;
}

void
route_init_defaults(struct AdvRoute *route, struct Interface *iface)
{
	memset(route, 0, sizeof(struct AdvRoute));
		
	route->AdvRouteLifetime = DFLT_AdvRouteLifetime(iface);
	route->AdvRoutePreference = DFLT_AdvRoutePreference;
}

int
check_iface(struct Interface *iface)
{
	struct AdvPrefix *prefix;
	struct AdvRoute *route;
	int res = 0;
	int MIPv6 = 0;

	/* Check if we use Mobile IPv6 extensions */
	if (iface->AdvHomeAgentFlag || iface->AdvHomeAgentInfo ||
		iface->AdvIntervalOpt)
	{
		MIPv6 = 1;
	}

	prefix = iface->AdvPrefixList;	
	while (!MIPv6 && prefix)
	{
		if (prefix->AdvRouterAddr)
		{
			MIPv6 = 1;
		}
		prefix = prefix->next;
	}

	if (MIPv6)
	{
		flog(LOG_INFO, "using Mobile IPv6 extensions");
	}

	if (iface->MinRtrAdvInterval == -1)
		iface->MinRtrAdvInterval = DFLT_MinRtrAdvInterval(iface);

	/* Mobile IPv6 ext */
	if (MIPv6)
	{
		if ((iface->MinRtrAdvInterval < MIN_MinRtrAdvInterval_MIPv6) || 
		    (iface->MinRtrAdvInterval > MAX_MinRtrAdvInterval(iface)))
		{
			flog(LOG_ERR, 
				"MinRtrAdvInterval for %s must be between %.2f and %.2f",
				iface->Name, MIN_MinRtrAdvInterval_MIPv6,
				MAX2(MIN_MinRtrAdvInterval_MIPv6, MIN_MinRtrAdvInterval_MIPv6));
			res = -1;
		}
	}
	else
	{
		if ((iface->MinRtrAdvInterval < MIN_MinRtrAdvInterval) || 
		    (iface->MinRtrAdvInterval > MAX_MinRtrAdvInterval(iface)))
		{
			flog(LOG_ERR, 
				"MinRtrAdvInterval for %s must be between %d and %d",
				iface->Name, (int)MIN_MinRtrAdvInterval,
				(int)MAX_MinRtrAdvInterval(iface));
			res = -1;
		}
	}

	/* Mobile IPv6 ext */
	if (MIPv6)
	{
		if ((iface->MaxRtrAdvInterval < MIN_MaxRtrAdvInterval_MIPv6) 
			|| (iface->MaxRtrAdvInterval > MAX_MaxRtrAdvInterval))
		{
			flog(LOG_ERR, 
				"MaxRtrAdvInterval for %s must be between %.2f and %d",
				iface->Name, MIN_MaxRtrAdvInterval_MIPv6,
				MAX_MaxRtrAdvInterval);
			res = -1;
		}
	}
	else
	{
		if ((iface->MaxRtrAdvInterval < MIN_MaxRtrAdvInterval) 
			|| (iface->MaxRtrAdvInterval > MAX_MaxRtrAdvInterval))
		{
			flog(LOG_ERR, 
				"MaxRtrAdvInterval must be between %d and %d for %s",
				MIN_MaxRtrAdvInterval, MAX_MaxRtrAdvInterval, iface->Name);
			res = -1;
		}
	}

	/* Mobile IPv6 ext */
	if (MIPv6)
	{
		if (iface->MinDelayBetweenRAs < MIN_DELAY_BETWEEN_RAS_MIPv6) 
		{
			flog(LOG_ERR, 
				"MinDelayBetweenRAs for %s must be greater than %.2f",
				iface->Name, MIN_DELAY_BETWEEN_RAS_MIPv6);
			res = -1;
		}
	}
	else
	{
		if (iface->MinDelayBetweenRAs < MIN_DELAY_BETWEEN_RAS)
		{
			flog(LOG_ERR, 
				"MinDelayBetweenRAs for %s must be greater than %.2f",
				iface->Name, (double) MIN_DELAY_BETWEEN_RAS);
			res = -1;
		}
	}

	if (iface->if_maxmtu != -1)
	{
		if ((iface->AdvLinkMTU != 0) &&
		   ((iface->AdvLinkMTU < MIN_AdvLinkMTU) || 
		   (iface->AdvLinkMTU > iface->if_maxmtu)))
		{
			flog(LOG_ERR,  "AdvLinkMTU must be zero or between %d and %d for %s",
			MIN_AdvLinkMTU, iface->if_maxmtu, iface->Name);
			res = -1;
		}
	}
	else
	{
		if ((iface->AdvLinkMTU != 0) 
			&& (iface->AdvLinkMTU < MIN_AdvLinkMTU))
		{
			flog(LOG_ERR,  "AdvLinkMTU must be zero or greater than %d for %s",
			MIN_AdvLinkMTU, iface->Name);
			res = -1;
		}
	}

	if (iface->AdvReachableTime >  MAX_AdvReachableTime)
	{
		flog(LOG_ERR, 
			"AdvReachableTime must be less than %d for %s",
			MAX_AdvReachableTime, iface->Name);
		res = -1;
	}

	if (iface->AdvCurHopLimit > MAX_AdvCurHopLimit)
	{
		flog(LOG_ERR, "AdvCurHopLimit must not be greater than %d for %s",
			MAX_AdvCurHopLimit, iface->Name);
		res = -1;
	}
	
	if (iface->AdvDefaultLifetime == -1)
	{
		iface->AdvDefaultLifetime = DFLT_AdvDefaultLifetime(iface);
	}

	/* Mobile IPv6 ext */
	if (iface->HomeAgentLifetime == 0)
	{
		iface->HomeAgentLifetime = DFLT_HomeAgentLifetime(iface);
	}

	if ((iface->AdvDefaultLifetime != 0) &&
	   ((iface->AdvDefaultLifetime > MAX_AdvDefaultLifetime) ||
	    (iface->AdvDefaultLifetime < MIN_AdvDefaultLifetime(iface))))
	{
		flog(LOG_ERR, 
			"AdvDefaultLifetime for %s must be zero or between %.0f and %.0f",
			iface->Name, MIN_AdvDefaultLifetime(iface),
			MAX_AdvDefaultLifetime);
		res = -1;
	}

	/* Mobile IPv6 ext */
	if (iface->AdvHomeAgentInfo)
	{
		if ((iface->HomeAgentLifetime > MAX_HomeAgentLifetime) ||
			(iface->HomeAgentLifetime < MIN_HomeAgentLifetime))
		{
			flog(LOG_ERR, 
				"HomeAgentLifetime must be between %d and %d for %s",
				MIN_HomeAgentLifetime, MAX_HomeAgentLifetime,
				iface->Name);
			res = -1;
		}
	}

	/* Mobile IPv6 ext */
	if (MIPv6)
	{
		if (iface->AdvHomeAgentInfo && !(iface->AdvHomeAgentFlag))
		{
			flog(LOG_ERR, 
				"AdvHomeAgentFlag must be set with HomeAgentInfo");
			res = -1;
		}
	}

	/* XXX: need this? prefix = iface->AdvPrefixList; */

	while (prefix)
	{
		if (prefix->PrefixLen > MAX_PrefixLen)
		{
			flog(LOG_ERR, "invalid prefix length for %s", iface->Name);
			res = -1;
		}

		if (prefix->AdvPreferredLifetime > prefix->AdvValidLifetime)
		{
			flog(LOG_ERR, "AdvValidLifetime must be "
				"greater than AdvPreferredLifetime for %s", 
				iface->Name);
			res = -1;
		}

		prefix = prefix->next;
	}


	route = iface->AdvRouteList;

	while(route)
	{
		if (route->PrefixLen > MAX_PrefixLen)
		{
			flog(LOG_ERR, "invalid route prefix length for %s", iface->Name);
			res = -1;
		}

		route = route->next;
	}

	return res;
}
