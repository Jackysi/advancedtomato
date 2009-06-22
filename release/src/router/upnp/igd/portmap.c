/*

	portmap.c
	Alternate UPnP Port Mapping Functions
	Copyright (C) 2006-2007 Jonathan Zarate

*/

#include "upnp_dbg.h"
#include "upnp_osl.h"
#include "upnp.h"
#include "igd.h"

#include <signal.h>
#include <libiptc/libiptc.h>
#include <linux/netfilter_ipv4/ip_nat_rule.h>
#include <bcmnvram.h>
#include <shared.h>


#define MAX_PMAP_ENTRIES	100

typedef struct {
	char enabled;
	char proto;
	unsigned short ext_port;
	unsigned short int_port;
	struct in_addr int_addr;
	char desc[32];
} pmap_entry_t;

pmap_entry_t pmap_entries[MAX_PMAP_ENTRIES];
int pmap_entries_count = 0;


// -----------------------------------------------------------------------------


static struct ipt_entry *uipt_create_entry(in_addr_t dest, unsigned short proto,
	unsigned short dport, in_addr_t nat_dest, unsigned short nat_dport)
{
	unsigned char buffer[1024];
	struct ipt_entry *e;

	memset(buffer, 0, sizeof(buffer));

	e = (struct ipt_entry *)buffer;
	e->target_offset = sizeof(struct ipt_entry);
	e->next_offset = sizeof(struct ipt_entry);
	e->nfcache = NFC_UNKNOWN;

	// -d x.x.x.x/32
	e->ip.dst.s_addr = dest;
	e->ip.dmsk.s_addr = 0xFFFFFFFF;

	struct ipt_entry_match *m = (struct ipt_entry_match *)&e->elems;
	if (proto == IPPROTO_TCP) {
		// -p tcp --dport x

		strcpy(m->u.user.name, "tcp");
		m->u.match_size = IPT_ALIGN(sizeof(struct ipt_entry_match)) + IPT_ALIGN(sizeof(struct ipt_tcp));

		struct ipt_tcp *tcp = (struct ipt_tcp *)&m->data;
		tcp->dpts[0] = tcp->dpts[1] = dport;
		tcp->spts[0] = 0;
		tcp->spts[1] = 0xFFFF;

		e->ip.proto = IPPROTO_TCP;
		e->target_offset += m->u.match_size;
	}
	else if (proto == IPPROTO_UDP) {
		// -p udp --dport x

		strcpy(m->u.user.name, "udp");
		m->u.match_size = IPT_ALIGN(sizeof(struct ipt_entry_match)) + IPT_ALIGN(sizeof(struct ipt_udp));

		struct ipt_udp *udp = (struct ipt_udp *)&m->data;
		udp->dpts[0] = udp->dpts[1] = dport;
		udp->spts[0] = 0;
		udp->spts[1] = 0xFFFF;

		e->ip.proto = IPPROTO_UDP;
		e->target_offset += m->u.match_size;
	}
	else {
		return NULL;
	}


	if (nat_dest != 0) {
		// -j DNAT --to-destination x:y

		struct ipt_entry_target *t = (struct ipt_entry_target *)((char *)e + e->target_offset);
		strcpy(t->u.user.name, "DNAT");
		t->u.target_size = IPT_ALIGN(sizeof(struct ipt_entry_target) + sizeof(struct ip_nat_multi_range));

		struct ip_nat_multi_range *nmr = (struct ip_nat_multi_range *)&t->data;
		nmr->rangesize = 1;

		nmr->range[0].flags = IP_NAT_RANGE_MAP_IPS;
		nmr->range[0].min_ip = nmr->range[0].max_ip = nat_dest;

		if (nat_dport) {
			if (nat_dport != dport) {
				nmr->range[0].flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
				nat_dport = htons(nat_dport);
				if (proto == IPPROTO_TCP) {
					nmr->range[0].min.tcp.port = nmr->range[0].max.tcp.port = nat_dport;
				}
				else {
					nmr->range[0].min.udp.port = nmr->range[0].max.udp.port = nat_dport;
				}
			}
		}

		e->next_offset = e->target_offset + t->u.target_size;
	}
	else {
		// -j ACCEPT
		struct ipt_standard_target *st = (struct ipt_standard_target *)((char *)e + e->target_offset);
		strcpy(st->target.u.user.name, "ACCEPT");	// fixme: logaccept
		st->target.u.target_size = IPT_ALIGN(sizeof(struct ipt_standard_target));
		e->next_offset = e->target_offset + st->target.u.target_size;
	}


//	printf("create_entry\n");
//	printf("next_offset=%d target_offset=%d\n", e->next_offset, e->target_offset);

	struct ipt_entry *ep;

	if ((ep = (struct ipt_entry *)malloc(e->next_offset)) != NULL) memcpy(ep, buffer, e->next_offset);
	return ep;
}

typedef struct ipt_entry * (*uipt_entryfunc_t)(const pmap_entry_t *);

static struct ipt_entry *uipt_nat(const pmap_entry_t *p)
{
	// -t nat -A upnp -d <wanip> -p tcp --dport <ext_dport> -j DNAT --to-destination <int_addr>:<int_port>
	return uipt_create_entry(inet_addr(get_wanip()), p->proto, p->ext_port, p->int_addr.s_addr, p->int_port);
}

static struct ipt_entry *uipt_filter(const pmap_entry_t *p)
{
	// -t filter -A upnp -D <int_addr> --dport <int_port> -j ACCEPT
	return uipt_create_entry(p->int_addr.s_addr, p->proto, p->int_port, 0, 0);
}

static int _uipt_delete(const char *table, uipt_entryfunc_t func, pmap_entry_t *p)
{
	iptc_handle_t h;
	struct ipt_entry *e;
	int ok;
	unsigned char *mask;

	ok = 0;
	e = NULL;
	if ((h = iptc_init(table)) != NULL) {
		if ((e = func(p)) != NULL) {
			if ((mask = malloc(e->next_offset)) != NULL) {
				memset(mask, 0xFF, e->next_offset);
				if (iptc_delete_entry("upnp", e, mask, &h)) {
					ok = iptc_commit(&h);
				}
				free(mask);
			}
			free(e);
		}
		if (h) iptc_free(&h);
	}
	if (!ok) {
		UPNP_TRACE(("%s table=%s find=%p error=%d %s\n", __FUNCTION__, table, e, errno, iptc_strerror(errno)));
	}
	return ok;
}

static void uipt_delete(pmap_entry_t *p)
{
	_uipt_delete("nat", uipt_nat, p);
	_uipt_delete("filter", uipt_filter, p);
}

static int _uipt_add(const char *table, uipt_entryfunc_t func, pmap_entry_t *p)
{
	iptc_handle_t h;
	struct ipt_entry *e;
	int ok;

	ok = 0;
	e = NULL;
	if ((h = iptc_init(table)) != NULL) {
		if (((e = func(p)) != NULL) && (iptc_append_entry("upnp", e, &h)))
			ok = iptc_commit(&h);
		if (h) iptc_free(&h);
		free(e);
	}
	if (!ok) {
		UPNP_TRACE(("%s table=%s e=%p error=%d %s\n", __FUNCTION__, table, e, errno, iptc_strerror(errno)));
	}
	return ok;
}

static int uipt_add(pmap_entry_t *p)
{
	if ((_uipt_add("nat", uipt_nat, p)) && (_uipt_add("filter", uipt_filter, p))) return 1;
	uipt_delete(p);
	return 0;
}

static void uipt_flush(const char *table)
{
	iptc_handle_t h;

	if ((h = iptc_init(table)) != NULL) {
		if (iptc_flush_entries("upnp", &h)) iptc_commit(&h);
		if (h) iptc_free(&h);
	}
}

// -----------------------------------------------------------------------------

static int pmap_delete(unsigned short proto, unsigned short ext_port, int doipt);

void pmap_load(void)
{
	int n;

	n = f_read("/var/lib/misc/upnp", pmap_entries, sizeof(pmap_entries));
	if ((n <= 0) || ((n % sizeof(pmap_entry_t)) != 0)) pmap_entries_count = 0;
		else pmap_entries_count = n / sizeof(pmap_entry_t);
	UPNP_TRACE(("pmap_load: n=%d each=%d pmap_entries_count=%d\n", n, sizeof(pmap_entry_t), pmap_entries_count));

	uipt_flush("nat");
	uipt_flush("filter");
	for (n = 0; n < pmap_entries_count; ++n) {
		uipt_add(&pmap_entries[n]);
	}
}

void pmap_save(void)
{
	int n;

	n = sizeof(pmap_entry_t) * pmap_entries_count;
	if (f_write("/var/lib/misc/upnp.tmp", pmap_entries, n, 0, 0) == n) {
		rename("/var/lib/misc/upnp.tmp", "/var/lib/misc/upnp");
	}
	else {
		unlink("/var/lib/misc/upnp.tmp");
	}
}

// called on SIGUSR2
void pmap_user2(void)
{
	FILE *f;
	int i;
	pmap_entry_t *p;
	unsigned short proto;
	unsigned short port;
	
	//
	if ((f = fopen("/var/spool/upnp.delete", "r")) != NULL) {
		i = fscanf(f, "%hu %hu", &proto, &port);
		fclose(f);
		unlink("/var/spool/upnp.delete");
		if (i == 2) {
			if ((proto == 0) && (port == 0)) {
				uipt_flush("nat");
				uipt_flush("filter");
				pmap_entries_count = 0;
				pmap_save();
			}
			else {
				pmap_delete(proto, port, 1);
			}
		}
	}

	//
	if ((f = fopen("/var/spool/upnp.js.tmp", "w")) == NULL) return;
	p = pmap_entries;
	for (i = pmap_entries_count; i > 0; --i) {
		fprintf(f, "\t[%d,'%s',%u,%u,'%s','%s']%s\n",
			p->enabled, (p->proto == IPPROTO_TCP) ? "TCP" : "UDP",
			p->ext_port, p->int_port, inet_ntoa(p->int_addr), p->desc,
			(i > 1) ? "," : "");
		++p;
	}
	fclose(f);
	if (!ferror(f)) rename("/var/spool/upnp.js.tmp", "/var/spool/upnp.js");
}

static int pmap_delete(unsigned short proto, unsigned short ext_port, int doipt)
{
	pmap_entry_t *p;
	int i;

	for (i = 0; i < pmap_entries_count; ++i) {
		p = &pmap_entries[i];
		if ((proto == p->proto) && (p->ext_port == ext_port)) {
			if (doipt) uipt_delete(p);

			memcpy(p, p + 1, (pmap_entries_count - i) * sizeof(*p));
			--pmap_entries_count;

			UPNP_TRACE(("%s @%d, count=%d\n", __FUNCTION__, i, pmap_entries_count));
			pmap_save();
			return 1;
		}
	}
	return 0;
}

// -----------------------------------------------------------------------------

void mapmgr_update()
{
	// nop
}

int mapmgr_port_map_count()
{
	return pmap_entries_count;
}

// -----------------------------------------------------------------------------

static char int_port[6];
static char int_addr[16];

/*
	0	in	NewPortMappingIndex
	1	out	NewRemoteHost
	2	out	NewExternalPort
	3	out	NewProtocol
	4	out	NewInternalPort
	5	out	NewInternalClient
	6	out	NewEnabled
	7	out	NewPortMappingDescription
	8	out	NewLeaseDuration
*/
int GetGenericPortMappingEntry(UFILE *uclient, PService psvc, PAction ac, pvar_entry_t args, int nargs)
{
	int i;
	const pmap_entry_t *e;
	static char ext_port[6];
//	static char ext_addr[16];

	UPNP_TRACE((
		"GetGenericPortMappingEntry "
		"NewPortMappingIndex=%s\n",
		ac->params[0].value));

	i = atoi(ac->params[0].value);
	if ((i < 0) || (i >= pmap_entries_count)) {
		soap_error(uclient, SOAP_SPECIFIEDARRAYINDEXINVALID);
		return FALSE;
	}

	e = &pmap_entries[i];

	sprintf(ext_port, "%u", e->ext_port);
	sprintf(int_port, "%u", e->int_port);
//	strlcpy(ext_addr, get_wanip(), sizeof(ext_addr));
	strlcpy(int_addr, inet_ntoa(e->int_addr), sizeof(int_addr));

	ac->params[1].value = "";	//	must be blank for XP
	ac->params[2].value = ext_port;
	ac->params[3].value = (e->proto == IPPROTO_TCP) ? "TCP" : "UDP";
	ac->params[4].value = int_port;
	ac->params[5].value = int_addr;
	ac->params[6].value = e->enabled ? "1" : "0";
	ac->params[7].value = (char *)e->desc;
	ac->params[8].value = "0";
	return TRUE;
}

/*
	0	in	NewRemoteHost
	1	in	NewExternalPort
	2	in	NewProtocol
	3	out	NewInternalPort
	4	out	NewInternalClient
	5	out	NewEnabled
	6	out	NewPortMappingDescription
	7	out	NewLeaseDuration
*/
int GetSpecificPortMappingEntry(UFILE *uclient, PService psvc, PAction ac, pvar_entry_t args, int nargs)
{
	UPNP_TRACE((
		"GetSpecificPortMapping "
		"NewRemoteHost=%s "
		"NewExternalPort=%s "
		"NewProtocol=%s\n",
		ac->params[0].value, ac->params[1].value, ac->params[2].value));

	if ((ac->params[0].value[0]) && (strcmp(ac->params[0].value, get_wanip()) != 0)) {
		soap_error(uclient, SOAP_NOSUCHENTRYINARRAY);
		return FALSE;
	}

	pmap_entry_t *p;
	unsigned short proto;
	unsigned short port;
	int i;

	port = strtoul(ac->params[1].value, NULL, 10);
	proto = (strcasecmp(ac->params[2].value, "TCP") == 0) ? IPPROTO_TCP : IPPROTO_UDP;

	for (i = 0; i < pmap_entries_count; ++i) {
		p = &pmap_entries[i];
		if ((proto == p->proto) && (p->ext_port == port)) {
			sprintf(int_port, "%u", p->int_port);
			strlcpy(int_addr, inet_ntoa(p->int_addr), sizeof(int_addr));
			ac->params[3].value = int_port;
			ac->params[4].value = int_addr;
			ac->params[5].value = p->enabled ? "1" : "0";
			ac->params[6].value = (char *)p->desc;
			ac->params[7].value = "0";

			UPNP_TRACE(("FOUND\n"));
			return TRUE;
		}
	}

	soap_error(uclient, SOAP_NOSUCHENTRYINARRAY);
	return FALSE;
}

/*
	0	in	NewRemoteHost
	1	in	NewExternalPort
	2	in	NewProtocol
	3	in	NewInternalPort
	4	in	NewInternalClient
	5	in	NewEnabled
	6	in	NewPortMappingDescription
	7	in	NewLeaseDuration
*/
int AddPortMapping(UFILE *uclient, PService psvc, PAction ac, pvar_entry_t args, int nargs)
{
	if (atoi(ac->params[7].value) != 0) {
		soap_error(uclient, SOAP_ONLYPERMANENTLEASESSUPPORTED);
		return FALSE;
	}

	if (pmap_entries_count >= MAX_PMAP_ENTRIES) {
		soap_error(uclient, SOAP_ACTION_FAILED);
		return FALSE;
	}

	UPNP_TRACE((
		"AddPortMapping "
		"NewRemoteHost=%s "
		"NewExternalPort=%s "
		"NewProtocol=%s "
		"NewInternalPort=%s "
		"NewInternalClient=%s "
		"NewEnabled=%s "
		"NewPortMappingDescription=%s "
		"NewLeaseDuration=%s\n",
		ac->params[0].value, ac->params[1].value, ac->params[2].value,
		ac->params[3].value, ac->params[4].value, ac->params[5].value,
		ac->params[6].value, ac->params[7].value));

	pmap_entry_t *e;
	pmap_entry_t *p;
	int a, b;
	struct in_addr mask;
	char *c;

	// XP sends NewRemoteHost = ""

	if ((ac->params[0].value[0]) && (strcmp(ac->params[0].value, get_wanip()) != 0)) {
		soap_error(uclient, SOAP_INVALID_ARGS);
		return FALSE;
	}

	e = &pmap_entries[pmap_entries_count];
	memset(e, 0, sizeof(*e));

	e->ext_port = atoi(ac->params[1].value);
	if (e->ext_port == 0) {
		soap_error(uclient, 716);
		return FALSE;
	}

	e->int_port = atoi(ac->params[3].value);
	if (e->int_port == 0) {
		soap_error(uclient, SOAP_INVALID_ARGS);
		return FALSE;
	}

	e->int_addr.s_addr = inet_addr(ac->params[4].value);
	mask.s_addr = inet_addr(nvram_safe_get("lan_netmask"));
	if ((e->int_addr.s_addr & mask.s_addr) != (inet_addr(nvram_safe_get("lan_ipaddr")) & mask.s_addr)) {
		soap_error(uclient, SOAP_INVALID_ARGS);
		return FALSE;
	}

	e->proto = (strcasecmp(ac->params[2].value, "TCP") == 0) ? IPPROTO_TCP : IPPROTO_UDP;
	e->enabled = atoi(ac->params[5].value);
	strlcpy(e->desc, ac->params[6].value, sizeof(e->desc));
	
	c = e->desc;
	while (*c) {
		if ((!isprint(*c)) || (*c == '\'')) *c = '_';
		++c;
	}

	for (p = pmap_entries; p != e; ++p) {
		if (p->proto != e->proto) continue;
		a = (p->ext_port == e->ext_port);
		b = ((p->int_addr.s_addr == e->int_addr.s_addr) && (p->int_port == e->int_port));
		if ((a) && (b)) {
			UPNP_TRACE(("UPDATING\n"));
			
			uipt_delete(p);
			if ((e->enabled) && (!uipt_add(e))) {
				soap_error(uclient, SOAP_ACTION_FAILED);
				// todo: remove entry
				return FALSE;
			}
			
			memcpy(p, e, sizeof(*p));
			pmap_save();
			return TRUE;
		}
		if ((a) || (b)) {
			soap_error(uclient, SOAP_CONFLICTINMAPPINGENTRY);
			return FALSE;
		}
	}

	UPNP_TRACE(("ADDING @%d\n", pmap_entries_count));
	
	uipt_delete(e);
	if (!uipt_add(e)) {
		soap_error(uclient, SOAP_ACTION_FAILED);
		return FALSE;
	}

	++pmap_entries_count;
	pmap_save();
	return TRUE;
}


/*
	0	in	NewRemoteHost
	1	in	NewExternalPort
	2	in	NewProtocol
*/
int DeletePortMapping(UFILE *uclient, PService psvc, PAction ac, pvar_entry_t args, int nargs)
{
	UPNP_TRACE((
		"DeletePortMapping\n"
		"NewRemoteHost=%s\n"
		"NewExternalPort=%s\n"
		"NewProtocol=%s\n",
		ac->params[0].value, ac->params[1].value, ac->params[2].value));

	if (pmap_delete((strcasecmp(ac->params[2].value, "TCP") == 0) ? IPPROTO_TCP : IPPROTO_UDP,
		strtoul(ac->params[1].value, NULL, 10), 1)) return TRUE;

	soap_error(uclient, SOAP_NOSUCHENTRYINARRAY);
	return FALSE;
}
