/*
* Copyright 2005, Broadcom Corporation
* All Rights Reserved.
* 
* THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
* KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
* SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
*
* $Id: mapmgr.c,v 1.10 2005/03/07 08:35:32 kanki Exp $
*/

#include "upnp_dbg.h"
#include "upnp_osl.h"
#include "upnp.h"
#include "igd.h"
#include "wanipc.h"
#include "mapmgr.h"
#include "igd.h"

#include <netconf.h>
#include <bcmnvram.h>
#include <utils.h>

#if	FD_SETSIZE < 200
#error "FD_SETSIZE is too small.  Must be >= 200"
#endif

typedef struct map_set {
	fd_set map;
	int count;
} map_set_t;


static int get_nat_list(netconf_nat_t **plist, int *nrules);
static void add_nat_entry(netconf_nat_t *nat);
static void delete_nat_entry(netconf_nat_t *pnat);
static bool find_matching_entry(const netconf_nat_t *e, netconf_nat_t *result);
static bool nat_equal(const netconf_nat_t *e1, const netconf_nat_t *e2);

static map_set_t ports;
static map_set_t ranges;


/* 
* Persistent (static) port forwards are described by a netconf_nat_t
* structure. On Linux, a netconf_filter_t that matches the target
* parameters of the netconf_nat_t should also be added to the INPUT
* and FORWARD tables to ACCEPT the forwarded connection.
*/
static bool get_forward_port(int which, netconf_nat_t *nat);
static bool set_forward_port(int which, const netconf_nat_t *nat);
static bool del_forward_port(int which);


void mapmgr_update()
{
	int i;
	netconf_nat_t m;

	memset(&ports, 0, sizeof(ports));
	memset(&ranges, 0, sizeof(ranges));

	for (i = 0; i < NFDBITS; i++) {
		if (get_forward_port(i, &m)) {
			if (m.match.dst.ports[0] == m.match.dst.ports[1]) {
				FD_SET(i, &ports.map);
				ports.count++;
			}
			else {
				FD_SET(i, &ranges.map);
				ranges.count++;
			}
		}
	}
}


/* Get the n'th port mapping entry from nvram.  This routine maintains
the illusion that port mappings are stored in a compact, sequential
list, no matter what the interlying storage really is.  */
static bool mapmgr_get_map(map_set_t *pset, int n, mapping_t *m)
{
	bool foundit = FALSE;
	int i;

	for (i = 0; i < NFDBITS; i++) {
		if (FD_ISSET(i, &pset->map)) {
			if (n-- == 0) {
				foundit = get_forward_port(i, (netconf_nat_t*)m);
				break;
			}
		} 
	}
	
	return foundit;
}

/* Write the n'th port mapping entry from nvram.  This routine maintains
the illusion that port mappings are stored in a compact, sequential
list, no matter what the interlying storage really is.  */
static bool mapmgr_add_map(map_set_t *pset, mapping_t *m)
{
	bool foundit = FALSE;
	int i;

	UPNP_TRACE(("%s\n", __FUNCTION__));

	foundit = FALSE;
	for (i = 0; i < NFDBITS; i++) {
		if (!FD_ISSET(i, &ports.map) && !FD_ISSET(i, &ranges.map) ) {
			foundit = set_forward_port(i, (netconf_nat_t*)m);
			FD_SET(i, &pset->map);
			pset->count++;
			break;
		}
	}

	return foundit;
}


/* Deletes the i'th forward_port%d entry from nvram and moves all subsequent entries up. */
static bool mapmgr_delete_map(map_set_t *pset, int n)
{
	bool foundit = FALSE;
	int i;

	UPNP_TRACE(("%s\n", __FUNCTION__));

	for (i = 0; i < NFDBITS; i++) {
		if (FD_ISSET(i, &pset->map)) {	
			if (n-- == 0) {
				char name[32];
				FD_CLR(i, &pset->map);
				pset->count--;
				snprintf(name, sizeof(name), "forward_port%d", i);
				foundit = del_forward_port(i);
				break;
			}
		}
	}

	return foundit;
}



/* Get the n'th port mapping entry from nvram.  This routine maintains
the illusion that port mappings are stored in a compact, sequential
list, no matter what the interlying storage really is.  */
bool mapmgr_get_port_map(int n, mapping_t *m)
{
	return mapmgr_get_map(&ports, n, m);
}

/* Write the n'th port mapping entry from nvram.  This routine maintains
the illusion that port mappings are stored in a compact, sequential
list, no matter what the interlying storage really is.  */
bool mapmgr_add_port_map(mapping_t *m)
{
	bool success;

	if ((success = mapmgr_add_map(&ports, m)) != FALSE  && !(m->match.flags & NETCONF_DISABLED)) {
		add_nat_entry((netconf_nat_t *) m);
		bump_generation();
	}

	return success;
}


/* Deletes the i'th forward_port%d entry from nvram. */
bool mapmgr_delete_port_map(int n)
{
	bool success = FALSE;
	mapping_t m;
	mapping_t result;


	if (mapmgr_get_map(&ports, n, &m)) {
		if (find_matching_entry(&m, &result)) {
			delete_nat_entry((netconf_nat_t *) &result);
		}
		success = mapmgr_delete_map(&ports, n);
		bump_generation();
	}
	return success;
}




/* Get the n'th port mapping entry from nvram.  This routine maintains
the illusion that port mappings are stored in a compact, sequential
list, no matter what the interlying storage really is.  */
bool mapmgr_get_range_map(int n, mapping_t *m)
{
	return mapmgr_get_map(&ranges, n, m);
}

/* Write the n'th port mapping entry from nvram.  This routine maintains
the illusion that port mappings are stored in a compact, sequential
list, no matter what the interlying storage really is.  */
bool mapmgr_add_range_map(mapping_t *m)
{
	return mapmgr_add_map(&ranges, m);
}


/* Deletes the i'th forward_port%d entry from nvram and moves all subsequent entries up. */
bool mapmgr_delete_range_map(int n)
{
	return mapmgr_delete_map(&ranges, n);
}



int mapmgr_port_map_count()
{
	return ports.count;
}

/*
static int mapmgr_range_map_count()
{
	return ranges.count;
}
*/

/* get the current list of static NAT mappings. 
Return 0 on success, non-zero on failure.
*/
static int get_nat_list(netconf_nat_t **plist, int *nrules)
{
	int needlen = 0, listlen;
	netconf_nat_t *nat_list = 0;

	netconf_get_nat(NULL, &needlen);
	if (needlen > 0) {
		nat_list = (netconf_nat_t *) malloc(needlen);
		if (nat_list) {
			memset(nat_list, 0, needlen);
			listlen = needlen;
			if (netconf_get_nat(nat_list, &listlen) == 0 && needlen == listlen) {
				*nrules = needlen/sizeof(netconf_nat_t);
				*plist = nat_list;
				return 0;
			}
			free(nat_list);
		}
		return 1;
	} else {
		*nrules = 0;
		*plist = NULL;
		return 0;
	}
}


/* Add port forward and a matching ACCEPT rule to the FORWARD table */
static void add_nat_entry(netconf_nat_t *entry)
{
	UPNP_TRACE(("%s\n", __FUNCTION__));

	int dir = NETCONF_FORWARD;
	int target = (atoi(nvram_safe_get("log_in")) & 2) ? NETCONF_LOG_ACCEPT : NETCONF_ACCEPT;
	netconf_filter_t filter;
	struct in_addr netmask = { 0xffffffff };
	netconf_nat_t nat = *entry;
	
	if (entry->ipaddr.s_addr == 0xffffffff) {
		inet_aton(nvram_safe_get("lan_ipaddr"), &nat.ipaddr);
		inet_aton(nvram_safe_get("lan_netmask"), &netmask);
		nat.ipaddr.s_addr &= netmask.s_addr;
		nat.ipaddr.s_addr |= (0xffffffff & ~netmask.s_addr);
	}

	/* We want to match destination ip address */
	inet_aton(nvram_safe_get("wan_ipaddr"), &nat.match.dst.ipaddr);     // by honor
	nat.match.dst.netmask.s_addr = htonl(0xffffffff);

	/* Set up LAN side match */
	memset(&filter, 0, sizeof(filter));
	filter.match.ipproto = nat.match.ipproto;
    filter.match.src.ipaddr.s_addr = nat.match.src.ipaddr.s_addr;	// 4309
	filter.match.src.ports[1] = nat.match.src.ports[1];
	filter.match.dst.ipaddr.s_addr = nat.ipaddr.s_addr;
	filter.match.dst.netmask.s_addr = netmask.s_addr;
	filter.match.dst.ports[0] = nat.ports[0];
	filter.match.dst.ports[1] = nat.ports[1];
	strncpy(filter.match.in.name, nat.match.in.name, IFNAMSIZ);

	/* Accept connection */
	filter.target = target;
	filter.dir = dir;

	/* Do it */
	netconf_add_nat(&nat);
	netconf_add_filter(&filter);
}



/* Combination PREROUTING DNAT and FORWARD ACCEPT */
static void 
delete_nat_entry(netconf_nat_t *entry)
{
	UPNP_TRACE(("%s\n", __FUNCTION__));

	int dir = NETCONF_FORWARD;
	int target = (atoi(nvram_safe_get("log_out")) & 2) ? NETCONF_LOG_ACCEPT : NETCONF_ACCEPT;
	netconf_filter_t filter;
	struct in_addr netmask = { 0xffffffff };
	netconf_nat_t nat = *entry;
	
	if (entry->ipaddr.s_addr == 0xffffffff) {
		inet_aton(nvram_safe_get("lan_ipaddr"), &nat.ipaddr);
		inet_aton(nvram_safe_get("lan_netmask"), &netmask);
		nat.ipaddr.s_addr &= netmask.s_addr;
		nat.ipaddr.s_addr |= (0xffffffff & ~netmask.s_addr);
	}

	/* Set up LAN side match */
	memset(&filter, 0, sizeof(filter));
	filter.match.ipproto = nat.match.ipproto;
    filter.match.src.ipaddr.s_addr = nat.match.src.ipaddr.s_addr;	// 4309
	filter.match.src.ports[1] = nat.match.src.ports[1];
	filter.match.dst.ipaddr.s_addr = nat.ipaddr.s_addr;
	filter.match.dst.netmask.s_addr = netmask.s_addr;
	filter.match.dst.ports[0] = nat.ports[0];
	filter.match.dst.ports[1] = nat.ports[1];
	strncpy(filter.match.in.name, nat.match.in.name, IFNAMSIZ);

	/* Accept connection */
	filter.target = target;
	filter.dir = dir;

	/* Do it */
	errno = netconf_del_nat(&nat);
	if (errno)
		UPNP_ERROR(("netconf_del_nat returned error %d\n", errno));

	errno = netconf_del_filter(&filter);
	if (errno)
		UPNP_ERROR(("netconf_del_filter returned error %d\n", errno));
}

static bool find_matching_entry(const netconf_nat_t *e, netconf_nat_t *result)
{
	bool foundit = FALSE;
	netconf_nat_t *rule = NULL;
	int nrules, rulenum;
	netconf_nat_t *nat_list = NULL;

	if (!get_nat_list(&nat_list, &nrules)) {
		for ( rulenum = 0; rulenum < nrules; rulenum++, rule = NULL ) {
			rule = &nat_list[rulenum];
			// Only match DNAT entries 
			// (there may be a MASQUERADE entry that we don't want.)
			if (rule->target != NETCONF_DNAT) {
				continue;
			}

			// does the source information match?
			if (nat_equal(e, rule)) {

				// copy the matched entry into our results.
				*result = *rule;

				// initialize the next and prev pointers so 
				// this entry looks like a single element list.
				netconf_list_init((netconf_fw_t *) result);

				// indicate that we found a matching entry.
				foundit = TRUE;
				break;
			} 
		}
		free(nat_list);
	}
	
	return foundit;
}

static bool nat_equal(const netconf_nat_t *e1, const netconf_nat_t *e2)
{
	bool matched = FALSE;

	do {
		if (e1->match.ipproto != e2->match.ipproto) {
			continue;
		}

		if (e1->match.dst.ipaddr.s_addr && 
				( e1->match.dst.netmask.s_addr != e2->match.dst.netmask.s_addr 
					|| e1->match.dst.ipaddr.s_addr != e2->match.dst.ipaddr.s_addr) ) {
			continue;
		}
		
		if (e1->match.dst.ports[0] != 0 
				&& e1->match.dst.ports[0] != e2->match.dst.ports[0]) {
			continue;
		}
		if (e1->match.dst.ports[1] 
				&& (e1->match.dst.ports[1] != e2->match.dst.ports[1])) {
			continue;
		}
		matched = TRUE;
	} while (0);

	return matched;
}

static bool valid_forward_port(const netconf_nat_t *nat)
{
	UPNP_TRACE(("%s\n", __FUNCTION__));

	/* Check WAN destination port range */
	if (ntohs(nat->match.dst.ports[0]) > ntohs(nat->match.dst.ports[1]))
		return FALSE;

	/* Check protocol */
	if (nat->match.ipproto != IPPROTO_TCP && nat->match.ipproto != IPPROTO_UDP)
		return FALSE;

	/* Check LAN IP address */
	if (nat->ipaddr.s_addr == htonl(0))
		return FALSE;

	/* Check LAN destination port range */
	if (ntohs(nat->ports[0]) > ntohs(nat->ports[1]))
		return FALSE;

	/* Check port range size */
	if ((ntohs(nat->match.dst.ports[1]) - ntohs(nat->match.dst.ports[0])) !=
			(ntohs(nat->ports[1]) - ntohs(nat->ports[0])))
		return FALSE;

	return TRUE;
}

static bool get_forward_port(int which, netconf_nat_t *nat)
{
	UPNP_TRACE(("%s\n", __FUNCTION__));

	char name[32];
	char value[1000];
	char *wan_port0, *wan_port1, *lan_ipaddr, *lan_port0, *lan_port1, *proto;
	char *enable, *desc;

	memset(nat, 0, sizeof(netconf_nat_t));

	/* Parse wan_port0-wan_port1>lan_ipaddr:lan_port0-lan_port1[:,]proto[:,]enable[:,]desc */
	sprintf(name, "forward_port%d", which);
	if (!nvram_invmatch(name, "")) return FALSE;
	strncpy(value, nvram_get(name), sizeof(value));

	/* Check for LAN IP address specification */
	lan_ipaddr = value;
	wan_port0 = strsep(&lan_ipaddr, ">");
	if (!lan_ipaddr)
		return FALSE;

	/* Check for LAN destination port specification */
	lan_port0 = lan_ipaddr;
	lan_ipaddr = strsep(&lan_port0, ":");
	if (!lan_port0)
		return FALSE;

	/* Check for protocol specification */
	proto = lan_port0;
	lan_port0 = strsep(&proto, ":,");
	if (!proto)
		return FALSE;

	/* Check for enable specification */
	enable = proto;
	proto = strsep(&enable, ":,");
	if (!enable)
		return FALSE;

	/* Check for description specification (optional) */
	desc = enable;
	enable = strsep(&desc, ":,");

	/* Check for WAN destination port range (optional) */
	wan_port1 = wan_port0;
	wan_port0 = strsep(&wan_port1, "-");
	if (!wan_port1)
		wan_port1 = wan_port0;

	/* Check for LAN destination port range (optional) */
	lan_port1 = lan_port0;
	lan_port0 = strsep(&lan_port1, "-");
	if (!lan_port1)
		lan_port1 = lan_port0;

	/* Parse WAN destination port range */
	nat->match.dst.ports[0] = htons(atoi(wan_port0));
	nat->match.dst.ports[1] = htons(atoi(wan_port1));

	/* Parse LAN IP address */
	/* Check IP, add by honor */
	if(get_single_ip(lan_ipaddr, 0) != get_single_ip(nvram_safe_get("lan_ipaddr"), 0) ||
			get_single_ip(lan_ipaddr, 1) != get_single_ip(nvram_safe_get("lan_ipaddr"), 1) ||
			get_single_ip(lan_ipaddr, 2) != get_single_ip(nvram_safe_get("lan_ipaddr"), 2)){
		/* Lan IP Address have been changed, so we must to adjust IP */
		char ip3[5];
		char *ip;
		char buf[254];
		snprintf(ip3, sizeof(ip3), "%d", get_single_ip(lan_ipaddr, 3));
		ip = get_complete_lan_ip(ip3);	
		(void) inet_aton(ip, &nat->ipaddr);
		snprintf(buf, sizeof(buf), "%s-%s>%s:%s-%s,%s,%s,%s", wan_port0, wan_port1, ip, lan_port0, lan_port1, proto, enable, desc);
		nvram_set(name, buf);
	}
	else
		(void) inet_aton(lan_ipaddr, &nat->ipaddr);

	/* Parse LAN destination port range */
	nat->ports[0] = htons(atoi(lan_port0));
	nat->ports[1] = htons(atoi(lan_port1));

	/* Parse protocol */
	if (!strncasecmp(proto, "tcp", 3))
		nat->match.ipproto = IPPROTO_TCP;
	else if (!strncasecmp(proto, "udp", 3))
		nat->match.ipproto = IPPROTO_UDP;
	else
		return FALSE;

	/* Parse enable */
	if (!strncasecmp(enable, "off", 3))
		nat->match.flags = NETCONF_DISABLED;

	/* Parse description */
	if (desc)
		strncpy(nat->desc, desc, sizeof(nat->desc));
	/* Set WAN source port range (match packets from any source port) */
	nat->match.src.ports[1] = htons(0xffff);
	

	/* Set target (DNAT) */
	nat->target = NETCONF_DNAT;

	return valid_forward_port(nat);
}	

static bool set_forward_port(int which, const netconf_nat_t *nat)
{
	UPNP_TRACE(("%s\n", __FUNCTION__));

	char name[32];
	char value[1000];
	char *cur;
	int len;

	if (!valid_forward_port(nat))
		return FALSE;

	/* Set wan_port0-wan_port1>lan_ipaddr:lan_port0-lan_port1,proto,enable,desc */
	
	cur = value;
	len = sizeof(value);

	/* Set WAN destination port range */
	cur = safe_snprintf(cur, &len, "%d", ntohs(nat->match.dst.ports[0]));
	cur = safe_snprintf(cur, &len, "-");
	cur = safe_snprintf(cur, &len, "%d", ntohs(nat->match.dst.ports[1]));

	/* Set LAN IP address */
	cur = safe_snprintf(cur, &len, ">");
	cur = safe_snprintf(cur, &len, inet_ntoa(nat->ipaddr));

	/* Set LAN destination port range */
	cur = safe_snprintf(cur, &len, ":");
	cur = safe_snprintf(cur, &len, "%d", ntohs(nat->ports[0]));
	cur = safe_snprintf(cur, &len, "-");
	cur = safe_snprintf(cur, &len, "%d", ntohs(nat->ports[1]));

	/* Set protocol */
	cur = safe_snprintf(cur, &len, ",");
	if (nat->match.ipproto == IPPROTO_TCP)
		cur = safe_snprintf(cur, &len, "tcp");
	else if (nat->match.ipproto == IPPROTO_UDP)
		cur = safe_snprintf(cur, &len, "udp");

	/* Set enable */
	cur = safe_snprintf(cur, &len, ",");
	if (nat->match.flags & NETCONF_DISABLED)
		cur = safe_snprintf(cur, &len, "off");
	else
		cur = safe_snprintf(cur, &len, "on");

	/* Set description */
	cur = safe_snprintf(cur, &len, ",%s", nat->desc ? : "");

#if 0	
	// quickly find if this exact rule exists already	-- zzz
	for (i = 0; i < 128; ++i) {
		sprintf(name, "forward_port%d", i);
		if (((cur = nvram_safe_get(name)) != NULL) && (strcmp(cur, value) == 0))
		return FALSE;
	}	
#endif

	/* Do it */
	sprintf(name, "forward_port%d", which);
	if (nvram_set(name, value))	return FALSE;

	return TRUE;
}

static bool del_forward_port(int which)
{
	UPNP_TRACE(("%s\n", __FUNCTION__));

	char name[32];
	sprintf(name, "forward_port%d", which);
	nvram_unset(name);
	return TRUE;
}
