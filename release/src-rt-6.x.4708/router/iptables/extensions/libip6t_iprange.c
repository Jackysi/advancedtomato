/* Shared library add-on to iptables to add IP range matching support. */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <arpa/inet.h>

#include <ip6tables.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv6/ip6_tables.h>
#include <linux/netfilter/xt_iprange.h>

enum {
	F_SRCIP = 1 << 0,
	F_DSTIP = 1 << 1,
};

/* Function which prints out usage message. */
static void
help(void)
{
	printf(
"iprange match v%s options:\n"
"[!] --src-range ip6-ip6      Match source IPv6 in the specified range\n"
"[!] --dst-range ip6-ip6      Match destination IPv6 in the specified range\n"
"\n",
IPTABLES_VERSION);
}

static struct option opts[] = {
	{ "src-range", 1, 0, '1' },
	{ "dst-range", 1, 0, '2' },
	{0}
};

static struct in6_addr *numeric_to_ip6addr(const char *num)
{
	static struct in6_addr ap;
	int err;

	if ((err = inet_pton(AF_INET6, num, &ap)) == 1)
		return &ap;
	return NULL;
}

static const char *ip6addr_to_numeric(const struct in6_addr *addrp)
{
	/* 0000:0000:0000:0000:0000:000.000.000.000
	 * 0000:0000:0000:0000:0000:0000:0000:0000 */
	static char buf[50+1];
	return inet_ntop(AF_INET6, addrp, buf, sizeof(buf));
}

/* Function which parses command options; returns true if it
   ate an option */
static int
parse(int c, char **argv, int invert, unsigned int *flags,
      const struct ip6t_entry *entry,
      unsigned int *nfcache,
      struct ip6t_entry_match **match)
{
	struct xt_iprange_mtinfo *info = (void *)(*match)->data;
	const struct in6_addr *ia;
	char *end;

	switch (c) {
	case '1': /* --src-range */
		end = strchr(optarg, '-');
		if (end == NULL)
			exit_error(PARAMETER_PROBLEM, "iprange match: Bad IP range `%s'\n", optarg);
		*end = '\0';
		ia = numeric_to_ip6addr(optarg);
		if (ia == NULL)
			exit_error(PARAMETER_PROBLEM, "iprange match: Bad IP address `%s'\n", optarg);
		memcpy(&info->src_min.in, ia, sizeof(*ia));
		ia = numeric_to_ip6addr(end + 1);
		if (ia == NULL)
			exit_error(PARAMETER_PROBLEM, "iprange match: Bad IP address `%s'\n", end + 1);
		memcpy(&info->src_max.in, ia, sizeof(*ia));
		info->flags |= IPRANGE_SRC;
		if (invert)
			info->flags |= IPRANGE_SRC_INV;
		*flags |= F_SRCIP;
		return 1;

	case '2': /* --dst-range */
		end = strchr(optarg, '-');
		if (end == NULL)
			exit_error(PARAMETER_PROBLEM, "iprange match: Bad IP range `%s'\n", optarg);
		*end = '\0';
		ia = numeric_to_ip6addr(optarg);
		if (ia == NULL)
			exit_error(PARAMETER_PROBLEM, "iprange match: Bad IP address `%s'\n", optarg);
		memcpy(&info->dst_min.in, ia, sizeof(*ia));
		ia = numeric_to_ip6addr(end + 1);
		if (ia == NULL)
			exit_error(PARAMETER_PROBLEM, "iprange match: Bad IP address `%s'\n", end + 1);
		memcpy(&info->dst_max.in, ia, sizeof(*ia));
		info->flags |= IPRANGE_DST;
		if (invert)
			info->flags |= IPRANGE_DST_INV;
		*flags |= F_DSTIP;
		return 1;
	}
	return 0;
}

/* Final check; must have specified --src-range or --dst-range. */
static void
final_check(unsigned int flags)
{
	if (flags == 0)
		exit_error(PARAMETER_PROBLEM,
			   "iprange match: You must specify `--src-range' or `--dst-range'");
}

/* Prints out the info. */
static void
print(const struct ip6t_ip6 *ip,
      const struct ip6t_entry_match *match,
      int numeric)
{
	const struct xt_iprange_mtinfo *info = (const void *)match->data;

	if (info->flags & IPRANGE_SRC) {
		printf("source IP range ");
		if (info->flags & IPRANGE_SRC_INV)
			printf("! ");
		/*
		 * ipaddr_to_numeric() uses a static buffer, so cannot
		 * combine the printf() calls.
		 */
		printf("%s", ip6addr_to_numeric(&info->src_min.in6));
		printf("-%s ", ip6addr_to_numeric(&info->src_max.in6));
	}
	if (info->flags & IPRANGE_DST) {
		printf("destination IP range ");
		if (info->flags & IPRANGE_DST_INV)
			printf("! ");
		printf("%s", ip6addr_to_numeric(&info->dst_min.in6));
		printf("-%s ", ip6addr_to_numeric(&info->dst_max.in6));
	}
}

/* Saves the union ipt_info in parsable form to stdout. */
static void
save(const struct ip6t_ip6 *ip, const struct ip6t_entry_match *match)
{
#ifdef IPTABLES_SAVE
	const struct xt_iprange_mtinfo *info = (const void *)match->data;

	if (info->flags & IPRANGE_SRC) {
		if (info->flags & IPRANGE_SRC_INV)
			printf("! ");
		printf("--src-range %s", ip6addr_to_numeric(&info->src_min.in6));
		printf("-%s ", ip6addr_to_numeric(&info->src_max.in6));
	}
	if (info->flags & IPRANGE_DST) {
		if (info->flags & IPRANGE_DST_INV)
			printf("! ");
		printf("--dst-range %s", ip6addr_to_numeric(&info->dst_min.in6));
		printf("-%s ", ip6addr_to_numeric(&info->dst_max.in6));
	}
#endif
}

static struct ip6tables_match iprange = { 
	.next		= NULL,
	.name		= "iprange",
	.revision	= 1,
	.version	= IPTABLES_VERSION,
	.size		= IP6T_ALIGN(sizeof(struct xt_iprange_mtinfo)),
	.userspacesize	= IP6T_ALIGN(sizeof(struct xt_iprange_mtinfo)),
	.help		= &help,
	.parse		= &parse,
	.final_check	= &final_check,
	.print		= &print,
	.save		= &save,
	.extra_opts	= opts
};

void _init(void)
{
	register_match6(&iprange);
}
