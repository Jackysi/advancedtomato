/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET  is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions for the Forwarding Information Base.
 *
 * Authors:	A.N.Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */

#ifndef _NET_IP_FIB_H
#define _NET_IP_FIB_H

#include <net/flow.h>
#include <linux/seq_file.h>
#include <net/fib_rules.h>

struct fib_config {
	u8			fc_dst_len;
	u8			fc_tos;
	u8			fc_protocol;
	u8			fc_scope;
	u8			fc_type;
	/* 3 bytes unused */
	u32			fc_table;
	__be32			fc_dst;
	__be32			fc_gw;
	int			fc_oif;
	u32			fc_flags;
	u32			fc_priority;
	__be32			fc_prefsrc;
	struct nlattr		*fc_mx;
	struct rtnexthop	*fc_mp;
	int			fc_mx_len;
	int			fc_mp_len;
	u32			fc_flow;
	u32			fc_mp_alg;
	u32			fc_nlflags;
	struct nl_info		fc_nlinfo;
 };

struct fib_info;

struct fib_nh {
	struct net_device	*nh_dev;
	struct hlist_node	nh_hash;
	struct fib_info		*nh_parent;
	unsigned		nh_flags;
	unsigned char		nh_scope;
#ifdef CONFIG_IP_ROUTE_MULTIPATH
	int			nh_weight;
	int			nh_power;
#endif
#ifdef CONFIG_NET_CLS_ROUTE
	__u32			nh_tclassid;
#endif
	int			nh_oif;
	__be32			nh_gw;
};

/*
 * This structure contains data shared by many of routes.
 */

struct fib_info {
	struct hlist_node	fib_hash;
	struct hlist_node	fib_lhash;
	int			fib_treeref;
	atomic_t		fib_clntref;
	int			fib_dead;
	unsigned		fib_flags;
	int			fib_protocol;
	__be32			fib_prefsrc;
	u32			fib_priority;
	u32			fib_metrics[RTAX_MAX];
#define fib_mtu fib_metrics[RTAX_MTU-1]
#define fib_window fib_metrics[RTAX_WINDOW-1]
#define fib_rtt fib_metrics[RTAX_RTT-1]
#define fib_advmss fib_metrics[RTAX_ADVMSS-1]
	int			fib_nhs;
#ifdef CONFIG_IP_ROUTE_MULTIPATH
	int			fib_power;
#endif
#ifdef CONFIG_IP_ROUTE_MULTIPATH_CACHED
	u32			fib_mp_alg;
#endif
	struct fib_nh		fib_nh[0];
#define fib_dev		fib_nh[0].nh_dev
};


#ifdef CONFIG_IP_MULTIPLE_TABLES
struct fib_rule;
#endif

struct fib_result {
	unsigned char	prefixlen;
	unsigned char	nh_sel;
	unsigned char	type;
	unsigned char	scope;
#ifdef CONFIG_IP_ROUTE_MULTIPATH_CACHED
	__be32          network;
	__be32          netmask;
#endif
	struct fib_info *fi;
#ifdef CONFIG_IP_MULTIPLE_TABLES
	struct fib_rule	*r;
#endif
};

struct fib_result_nl {
	__be32		fl_addr;   /* To be looked up*/
	u32		fl_mark;
	unsigned char	fl_tos;
	unsigned char   fl_scope;
	unsigned char   tb_id_in;

	unsigned char   tb_id;      /* Results */
	unsigned char	prefixlen;
	unsigned char	nh_sel;
	unsigned char	type;
	unsigned char	scope;
	int             err;      
};

#ifdef CONFIG_IP_ROUTE_MULTIPATH

#define FIB_RES_NH(res)		((res).fi->fib_nh[(res).nh_sel])
#define FIB_RES_RESET(res)	((res).nh_sel = 0)

#else /* CONFIG_IP_ROUTE_MULTIPATH */

#define FIB_RES_NH(res)		((res).fi->fib_nh[0])
#define FIB_RES_RESET(res)

#endif /* CONFIG_IP_ROUTE_MULTIPATH */

#define FIB_RES_PREFSRC(res)		((res).fi->fib_prefsrc ? : __fib_res_prefsrc(&res))
#define FIB_RES_GW(res)			(FIB_RES_NH(res).nh_gw)
#define FIB_RES_DEV(res)		(FIB_RES_NH(res).nh_dev)
#define FIB_RES_OIF(res)		(FIB_RES_NH(res).nh_oif)

#ifdef CONFIG_IP_ROUTE_MULTIPATH_CACHED
#define FIB_RES_NETWORK(res)		((res).network)
#define FIB_RES_NETMASK(res)	        ((res).netmask)
#else /* CONFIG_IP_ROUTE_MULTIPATH_CACHED */
#define FIB_RES_NETWORK(res)		(0)
#define FIB_RES_NETMASK(res)	        (0)
#endif /* CONFIG_IP_ROUTE_MULTIPATH_WRANDOM */

struct fib_table {
	struct hlist_node tb_hlist;
	u32		tb_id;
	unsigned	tb_stamp;
	int		tb_default;
	int		(*tb_lookup)(struct fib_table *tb, const struct flowi *flp, struct fib_result *res);
	int		(*tb_insert)(struct fib_table *, struct fib_config *);
	int		(*tb_delete)(struct fib_table *, struct fib_config *);
	int		(*tb_dump)(struct fib_table *table, struct sk_buff *skb,
				     struct netlink_callback *cb);
	int		(*tb_flush)(struct fib_table *table);
	void		(*tb_select_default)(struct fib_table *table,
					     const struct flowi *flp, struct fib_result *res);

	unsigned char	tb_data[0];
};

#ifndef CONFIG_IP_MULTIPLE_TABLES

extern struct fib_table *ip_fib_local_table;
extern struct fib_table *ip_fib_main_table;

static inline struct fib_table *fib_get_table(u32 id)
{
	if (id != RT_TABLE_LOCAL)
		return ip_fib_main_table;
	return ip_fib_local_table;
}

static inline struct fib_table *fib_new_table(u32 id)
{
	return fib_get_table(id);
}

static inline int fib_lookup(const struct flowi *flp, struct fib_result *res)
{
	if (ip_fib_local_table->tb_lookup(ip_fib_local_table, flp, res) &&
	    ip_fib_main_table->tb_lookup(ip_fib_main_table, flp, res))
		return -ENETUNREACH;
	return 0;
}

static inline void fib_select_default(const struct flowi *flp, struct fib_result *res)
{
	if (FIB_RES_GW(*res) && FIB_RES_NH(*res).nh_scope == RT_SCOPE_LINK)
		ip_fib_main_table->tb_select_default(ip_fib_main_table, flp, res);
}

#else /* CONFIG_IP_MULTIPLE_TABLES */
#define ip_fib_local_table fib_get_table(RT_TABLE_LOCAL)
#define ip_fib_main_table fib_get_table(RT_TABLE_MAIN)

extern int fib_lookup(struct flowi *flp, struct fib_result *res);

extern struct fib_table *fib_new_table(u32 id);
extern struct fib_table *fib_get_table(u32 id);
extern void fib_select_default(const struct flowi *flp, struct fib_result *res);

#endif /* CONFIG_IP_MULTIPLE_TABLES */

/* Exported by fib_frontend.c */
extern const struct nla_policy rtm_ipv4_policy[];
extern void		ip_fib_init(void);
extern int fib_validate_source(__be32 src, __be32 dst, u8 tos, int oif,
			       struct net_device *dev, __be32 *spec_dst, u32 *itag);
extern void fib_select_multipath(const struct flowi *flp, struct fib_result *res);

struct rtentry;

/* Exported by fib_semantics.c */
extern int ip_fib_check_default(__be32 gw, struct net_device *dev);
extern int fib_sync_down(__be32 local, struct net_device *dev, int force);
extern int fib_sync_up(struct net_device *dev);
extern __be32  __fib_res_prefsrc(struct fib_result *res);

/* Exported by fib_hash.c */
extern struct fib_table *fib_hash_init(u32 id);

#ifdef CONFIG_IP_MULTIPLE_TABLES
extern void __init fib4_rules_init(void);

#ifdef CONFIG_NET_CLS_ROUTE
extern u32 fib_rules_tclass(struct fib_result *res);
#endif

#endif

static inline void fib_combine_itag(u32 *itag, struct fib_result *res)
{
#ifdef CONFIG_NET_CLS_ROUTE
#ifdef CONFIG_IP_MULTIPLE_TABLES
	u32 rtag;
#endif
	*itag = FIB_RES_NH(*res).nh_tclassid<<16;
#ifdef CONFIG_IP_MULTIPLE_TABLES
	rtag = fib_rules_tclass(res);
	if (*itag == 0)
		*itag = (rtag<<16);
	*itag |= (rtag>>16);
#endif
#endif
}

extern void free_fib_info(struct fib_info *fi);

static inline void fib_info_put(struct fib_info *fi)
{
	if (atomic_dec_and_test(&fi->fib_clntref))
		free_fib_info(fi);
}

static inline void fib_res_put(struct fib_result *res)
{
	if (res->fi)
		fib_info_put(res->fi);
#ifdef CONFIG_IP_MULTIPLE_TABLES
	if (res->r)
		fib_rule_put(res->r);
#endif
}

#ifdef CONFIG_PROC_FS
extern int  fib_proc_init(void);
extern void fib_proc_exit(void);
#endif

extern rwlock_t fib_nhflags_lock;

#endif  /* _NET_FIB_H */
