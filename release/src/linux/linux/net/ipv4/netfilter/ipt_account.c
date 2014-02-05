/* Copyright (c) 2004-2006 Piotr 'QuakeR' Gasidlo <quaker@barbara.eu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/moduleparam.h>
// #include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#include <asm/uaccess.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#define IPT_ACCOUNT_VERSION "0.1.20"

// #define DEBUG_IPT_ACCOUNT

MODULE_AUTHOR("Piotr Gasidlo <quaker@barbara.eu.org>");
MODULE_DESCRIPTION("Traffic accounting module");
MODULE_LICENSE("GPL");

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_account.h>

/* defaults, can be overriden */
static unsigned int netmask = 16; /* Safe netmask, if you try to create table
																		 for larger netblock you will get error. 
																		 Increase by command line only when you
																		 known what are you doing. */

#ifdef DEBUG_IPT_ACCOUNT
static int debug = 0;
#endif

MODULE_PARM(netmask, "i");
MODULE_PARM_DESC(netmask,"maximum *save* netmask");
#ifdef DEBUG_IPT_ACCOUNT
MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug,"enable debugging output");
#endif

/* structure with statistics counter, used when table is created without --ashort switch */
struct t_ipt_account_stat_long {
	u_int64_t b_all, b_tcp, b_udp, b_icmp, b_other;
	u_int64_t p_all, p_tcp, p_udp, p_icmp, p_other;
};

/* same as above, for tables created with --ashort switch */
struct t_ipt_account_stat_short {
	u_int64_t b_all;
	u_int64_t p_all;
};

/* structure holding to/from statistics for single ip when table is created without --ashort switch */
struct t_ipt_account_stats_long {
	struct t_ipt_account_stat_long src, dst;
	struct timespec time; /* time, when statistics was last modified */
};

/* same as above, for tables created with --ashort switch */
struct t_ipt_account_stats_short {
	struct t_ipt_account_stat_short src, dst;
	struct timespec time;
};

/* structure describing single table */
struct t_ipt_account_table {
	struct list_head list;
	atomic_t use; /* use counter, the number of rules which points to this table */

	char name[IPT_ACCOUNT_NAME_LEN + 1]; /* table name ( = filename in /proc/net/ipt_account/) */
	u_int32_t network, netmask, count; /* network/netmask/hosts count coverted by table */

	int shortlisting:1; /* gather only total statistics (set for tables created with --ashort switch) */
	int timesrc:1; /* update time when accounting outgoing traffic */
	int timedst:1; /* update time when accounting incomming traffic */
	
	union { /* statistics for each ip in network/netmask */
		struct t_ipt_account_stats_long *l;
		struct t_ipt_account_stats_short *s;
	} stats;
	rwlock_t stats_lock; /* lock, to assure that above union can be safely modified */

	struct proc_dir_entry *pde; /* handle to proc entry */
};

static LIST_HEAD(ipt_account_tables);
static rwlock_t ipt_account_lock = RW_LOCK_UNLOCKED; /* lock, to assure that table list can be safely modified */
static DECLARE_MUTEX(ipt_account_mutex); /* additional checkentry protection */

static struct file_operations ipt_account_proc_fops;
static struct proc_dir_entry *ipt_account_procdir;

/*
 * Function creates new table and inserts it into linked list.
 */
static struct t_ipt_account_table *
ipt_account_table_init(struct t_ipt_account_info *info)
{	
	struct t_ipt_account_table *table;
	
#ifdef IPT_ACCOUNT_DEBUG
	if (debug) printk(KERN_DEBUG "ipt_account [ipt_account_table_init]: name = %s\n", info->name);
#endif

	/*
	 * Allocate memory for table.
	 */
	table = vmalloc(sizeof(struct t_ipt_account_table));
	if (!table) {
		printk(KERN_ERR "ipt_account [ipt_account_table_init]: table = vmalloc(sizeof(struct t_ipt_account_table)) failed.\n");
		goto cleanup_none;
	}
	memset(table, 0, sizeof(struct t_ipt_account_table));

	/*
	 * Table attributes.
	 */
	strncpy(table->name, info->name, IPT_ACCOUNT_NAME_LEN);	
	table->name[IPT_ACCOUNT_NAME_LEN] = '\0';

	table->network = info->network;
	table->netmask = info->netmask;
	table->count = (0xffffffff ^ table->netmask) + 1;
	
	/*
	 * Table properties.
	 */
	table->shortlisting = info->shortlisting;
	table->timesrc = 1;
	table->timedst = 1;
	
	/*
	 * Initialize use counter.
	 */
	atomic_set(&table->use, 1);

	/*
	 * Allocate memory for statistic counters.
	 */
	if (table->shortlisting) {
		table->stats.s = vmalloc(sizeof(struct t_ipt_account_stats_short) * table->count);
		if (!table->stats.s) {
			printk(KERN_ERR "ipt_account [ipt_account_table_init]: table->stats.s = vmalloc(sizeof(struct t_ipt_account_stats_short) * table->count) failed.\n");
			goto cleanup_table;
		}
		memset(table->stats.s, 0, sizeof(struct t_ipt_account_stats_short) * table->count);
	} else {
		table->stats.l = vmalloc(sizeof(struct t_ipt_account_stats_long) * table->count);
		if (!table->stats.l) {
			printk(KERN_ERR "ipt_account [ipt_account_table_init]: table->stats.l = vmalloc(sizeof(struct t_ipt_account_stats_long) * table->count) failed.\n");
			goto cleanup_table;
		}
		memset(table->stats.l, 0, sizeof(struct t_ipt_account_stats_long) * table->count);
	}
	
	/*
	 * Reset locks.
	 */
	table->stats_lock = RW_LOCK_UNLOCKED;
	
	/*
	 * Create /proc/ipt_account/name entry.
	 */
	table->pde = create_proc_entry(table->name, S_IWUSR | S_IRUSR, ipt_account_procdir);
	if (!table->pde) {
		goto cleanup_stats;
	}
	table->pde->proc_fops = &ipt_account_proc_fops;
	table->pde->data = table;
	
	/*
	 * Insert table into list.
	 */
	write_lock_bh(&ipt_account_lock);
	list_add(&table->list, &ipt_account_tables);
	write_unlock_bh(&ipt_account_lock);
	
	return table;

	/*
	 * If something goes wrong we end here.
	 */
cleanup_stats:
	if (table->shortlisting)
		vfree(table->stats.s);
	else
		vfree(table->stats.l);
	
cleanup_table:
	vfree(table);
cleanup_none:
	return NULL;
	
}

/*
 * Function destroys table. Table *must* be already unlinked.
 */
static void
ipt_account_table_destroy(struct t_ipt_account_table *table)
{
#ifdef IPT_ACCOUNT_DEBUG
	if (debug) printk(KERN_DEBUG "ipt_account [ipt_account_table_destory]: name = %s\n", table->name);
#endif
	remove_proc_entry(table->pde->name, table->pde->parent);
	if (table->shortlisting)
		vfree(table->stats.s);
	else
		vfree(table->stats.l);
	vfree(table);
}

/*
 * Function increments use counter for table.
 */
static inline void
ipt_account_table_get(struct t_ipt_account_table *table) 
{
#ifdef IPT_ACCOUNT_DEBUG
	if (debug) printk(KERN_DEBUG "ipt_account [ipt_account_table_get]: name = %s\n", table->name);
#endif
	atomic_inc(&table->use);
}

/*
 * Function decrements use counter for table. If use counter drops to zero,
 * table is removed from linked list and destroyed.
 */
static inline void
ipt_account_table_put(struct t_ipt_account_table *table) 
{
#ifdef IPT_ACCOUNT_DEBUG
	if (debug) printk(KERN_DEBUG "ipt_account [ipt_account_table_put]: name = %s\n", table->name);
#endif
	if (atomic_dec_and_test(&table->use)) {
		write_lock_bh(&ipt_account_lock);
		list_del(&table->list);
		write_unlock_bh(&ipt_account_lock);
		ipt_account_table_destroy(table);
	}
}

/*
 * Helper function, which returns a structure pointer to a table with
 * specified name.
 */
static struct t_ipt_account_table *
__ipt_account_table_find(char *name) 
{
	struct list_head *pos;
	list_for_each(pos, &ipt_account_tables) {
		struct t_ipt_account_table *table = list_entry(pos,
				struct t_ipt_account_table, list);
		if (!strncmp(table->name, name, IPT_ACCOUNT_NAME_LEN))
			return table;
	}
	return NULL;
}

/*
 * Function, which returns a structure pointer to a table with
 * specified name. When such table is found its use coutner
 * is incremented.
 */
static inline struct t_ipt_account_table *
ipt_account_table_find_get(char *name) 
{
	struct t_ipt_account_table *table;

#ifdef IPT_ACCOUNT_DEBUG
	if (debug) printk(KERN_DEBUG "ipt_account [ipt_account_table_find_get]: name = %s\n", name);
#endif	
	read_lock_bh(&ipt_account_lock);
	table = __ipt_account_table_find(name);
	if (!table) {
		read_unlock_bh(&ipt_account_lock);
		return NULL;
	}
	atomic_inc(&table->use);
	read_unlock_bh(&ipt_account_lock);
	return table;
}	

/*
 * Helper function, with updates statistics for specified IP. It's only
 * used for tables created without --ashort switch.
 */
static inline void
__account_long(struct t_ipt_account_stat_long *stat, const struct sk_buff *skb) 
{
	stat->b_all += skb->len;
	stat->p_all++;
	
	switch (skb->nh.iph->protocol) {
		case IPPROTO_TCP:
			stat->b_tcp += skb->len;
			stat->p_tcp++;
			break;
		case IPPROTO_UDP:
			stat->b_udp += skb->len;
			stat->p_udp++;
			break;
		case IPPROTO_ICMP:
			stat->b_icmp += skb->len;
			stat->p_icmp++;
			break;
		default:
			stat->b_other += skb->len;
			stat->p_other++;
	}
}

/*
 * Same as above, but used for tables created with --ashort switch.
 */
static inline void
__account_short(struct t_ipt_account_stat_short *stat, const struct sk_buff *skb)
{
	stat->b_all += skb->len;
	stat->p_all++;
}

/*
 * Match function. Here we do accounting stuff.
 */
static int
match(const struct sk_buff *skb,
      const struct net_device *in,
      const struct net_device *out,
      const void *matchinfo,
      int offset,
			const void *hdr,
			u_int16_t datalen,
      int *hotdrop)
{
	struct t_ipt_account_info *info = (struct t_ipt_account_info *)matchinfo;
	struct t_ipt_account_table *table = info->table;
	u_int32_t address;	
	/* Get current time. */
	struct timespec now;
	jiffies_to_timespec(jiffies, &now);
	/* Default we assume no match. */
	int ret = 0;
		
#ifdef IPT_ACCOUNT_DEBUG
	if (debug) printk(KERN_DEBUG "ipt_account [match]: name = %s\n", table->name);
#endif
	/* Check whether traffic from source ip address ... */
	address = ntohl(skb->nh.iph->saddr);
	/* ... is being accounted by this table. */	
	if (address && ((u_int32_t)(address & table->netmask) == (u_int32_t)table->network)) {		
		write_lock_bh(&table->stats_lock);
		/* Yes, account this packet. */
#ifdef IPT_ACCOUNT_DEBUG
		if (debug) printk(KERN_DEBUG "ipt_account: [match]: accounting packet src = %u.%u.%u.%u, proto = %u.\n", HIPQUAD(address), skb->nh.iph->protocol);
#endif
		/* Update counters this host. */
		if (!table->shortlisting) {
			__account_long(&table->stats.l[address - table->network].src, skb);
			if (table->timesrc)
				table->stats.l[address - table->network].time = now;
			/* Update also counters for all hosts in this table (network address) */
			if (table->count > 1) {
				__account_long(&table->stats.l[0].src, skb);
				table->stats.l[0].time = now;
			}
		} else {
			__account_short(&table->stats.s[address - table->network].src, skb);
			if (table->timedst)
				table->stats.s[address - table->network].time = now;
			if (table->count > 1) {
				__account_short(&table->stats.s[0].src, skb);
				table->stats.s[0].time = now;
			}
		}
		write_unlock_bh(&table->stats_lock);
		/* Yes, it's a match. */
		ret = 1;
	}
	
	/* Do the same thing with destination ip address. */
	address = ntohl(skb->nh.iph->daddr);
	if (address && ((u_int32_t)(address & table->netmask) == (u_int32_t)table->network)) {
		write_lock_bh(&table->stats_lock);
#ifdef IPT_ACCOUNT_DEBUG
		if (debug) printk(KERN_DEBUG "ipt_account: [match]: accounting packet dst = %u.%u.%u.%u, proto = %u.\n", HIPQUAD(address), skb->nh.iph->protocol);
#endif
		if (!table->shortlisting) {
			__account_long(&table->stats.l[address - table->network].dst, skb);
			table->stats.l[address - table->network].time = now;
			if (table->count > 1) {
				__account_long(&table->stats.l[0].dst, skb);
				table->stats.l[0].time = now;
			}
		} else {
			__account_short(&table->stats.s[address - table->network].dst, skb);
			table->stats.s[address - table->network].time = now;
			if (table->count > 1) {
				__account_short(&table->stats.s[0].dst, skb);
				table->stats.s[0].time = now;
			}
		}
		write_unlock_bh(&table->stats_lock);
		ret = 1;
	}
	
	return ret;
}

/*
 * Checkentry function.
 */
static int
checkentry(const char *tablename,
	   const struct ipt_ip *ipt_ip,
	   void *matchinfo,
	   unsigned int matchsize,
	   unsigned int hook_mask)
{
	struct t_ipt_account_info *info = matchinfo;
	struct t_ipt_account_table *table;

#ifdef IPT_ACCOUNT_DEBUG
	if (debug) printk(KERN_DEBUG "ipt_account [checkentry]: name = %s\n", info->name);
#endif
	if (matchsize != IPT_ALIGN(sizeof(struct t_ipt_account_info)))
		return 0;

	/* 
	 * Sanity checks. 
	 */
	if (info->netmask < ((~0L << (32 - netmask)) & 0xffffffff)) {
		printk(KERN_ERR "ipt_account[checkentry]: too big netmask (increase module 'netmask' parameter).\n");
		return 0;
	}
	if ((info->network & info->netmask) != info->network) {
		printk(KERN_ERR "ipt_account[checkentry]: wrong network/netmask.\n");
		return 0;
	}
	if (info->name[0] == '\0') {
		printk(KERN_ERR "ipt_account[checkentry]: wrong table name.\n");
		return 0;
	}
			
	/*
	 * We got new rule. Try to find table with the same name as given in info structure.
	 * Mutex magic based on ipt_hashlimit.c.
	 */	
	down(&ipt_account_mutex);
	table = ipt_account_table_find_get(info->name);
	if (table) {
		if (info->table != NULL) {
			if (info->table != table) {
				/* 
				 * Shouldn't happen. 
				 */
				printk(KERN_ERR "ipt_account[checkentry]: reloaded rule has invalid table pointer.\n");
				up(&ipt_account_mutex);
				return 0;
			}
			up(&ipt_account_mutex);
			return 1;
		} else {
#ifdef IPT_ACCOUNT_DEBUG
			if (debug) printk(KERN_DEBUG "ipt_account [checkentry]: table found, checking.\n");
#endif
			/* 
			 * Table exists, but whether rule network/netmask/shortlisting matches 
			 * table network/netmask/shortlisting. Failure on missmatch. 
			 */		
			if (table->network != info->network || table->netmask != info->netmask || table->shortlisting != info->shortlisting) {
				printk(KERN_ERR "ipt_account [checkentry]: table found, rule network/netmask/shortlisting not match table network/netmask/shortlisting.\n");
				/*
				 * Remember to release table usage counter.
				 */
				ipt_account_table_put(table);
				up(&ipt_account_mutex);
				return 0;
			}
#ifdef IPT_ACCOUNT_DEBUG
			if (debug) printk(KERN_DEBUG "ipt_account [checkentry]: table found, reusing.\n");
#endif
			/*
			 * Link rule with table.
			 */
			info->table = table;
		}
	} else {
#ifdef IPT_ACCOUNT_DEBUG
		if (debug) printk(KERN_DEBUG "ipt_account [checkentry]: table not found, creating new one.\n");
#endif
		/*
		 * Table not exist, create new one.
		 */
		info->table = table = ipt_account_table_init(info);
		if (!table) {
			up(&ipt_account_mutex);
			return 0;
		}
	}
	up(&ipt_account_mutex);
	return 1;
}

/*
 * Destroy function.
 */
static void
destroy(void *matchinfo, unsigned int matchsize)
{
	struct t_ipt_account_info *info = matchinfo;
	
#ifdef IPT_ACCOUNT_DEBUG
	if (debug) printk(KERN_DEBUG "ipt_account [destroy]: name = %s\n", info->name);
#endif
	/*
	 * Release table, by decreasing its usage counter. When
	 * counter hits zero, memory used by table structure is
	 * released and table is removed from list.
	 */
	ipt_account_table_put(info->table);
	return;
}

static struct ipt_match account_match = { 
  .name = "account", 
  .match = &match, 
  .checkentry = &checkentry, 
  .destroy = &destroy, 
  .me = THIS_MODULE
};

/*
 * Below functions (ipt_account_seq_start, ipt_account_seq_next, 
 * ipt_account_seq_stop, ipt_account_seq_show, ipt_account_proc_write) 
 * are used to implement proc stuff.
 */
static void *ipt_account_seq_start(struct seq_file *sf, loff_t *pos)
{
	struct proc_dir_entry *pde = sf->private;
	struct t_ipt_account_table *table = pde->data;
	unsigned int *i;

	read_lock_bh(&table->stats_lock);
	if (*pos >= table->count)
		return NULL;
	i = kmalloc(sizeof(unsigned int), GFP_ATOMIC);
	if (!i)
		return ERR_PTR(-ENOMEM);
	*i = *pos;
	return i;
}

static void *ipt_account_seq_next(struct seq_file *sf, void *v, loff_t *pos)
{
	struct proc_dir_entry *pde = sf->private;
	struct t_ipt_account_table *table = pde->data;
	unsigned int *i = (unsigned int *)v;

	*pos = ++(*i);
	if (*i >= table->count) {
		kfree(v);
		return NULL;
	}
	return i;
}

static void ipt_account_seq_stop(struct seq_file *sf, void *v)
{
	struct proc_dir_entry *pde = sf->private;
	struct t_ipt_account_table *table = pde->data;
	kfree(v);
	read_unlock_bh(&table->stats_lock);
}

static int ipt_account_seq_show(struct seq_file *sf, void *v)
{
	struct proc_dir_entry *pde = sf->private;
	struct t_ipt_account_table *table = pde->data;
	unsigned int *i = (unsigned int *)v;
	
	struct timespec now;
	jiffies_to_timespec(jiffies, &now);
	
	u_int32_t address = table->network + *i;

	if (!table->shortlisting) {
		struct t_ipt_account_stats_long *l = &table->stats.l[*i];
		seq_printf(sf,
				"ip = %u.%u.%u.%u bytes_src = %Lu %Lu %Lu %Lu %Lu packets_src = %Lu %Lu %Lu %Lu %Lu bytes_dst = %Lu %Lu %Lu %Lu %Lu packets_dst = %Lu %Lu %Lu %Lu %Lu time = %lu\n",
				HIPQUAD(address),
				l->src.b_all,
				l->src.b_tcp,
				l->src.b_udp,
				l->src.b_icmp,
				l->src.b_other,
				l->src.p_all,
				l->src.p_tcp,
				l->src.p_udp,
				l->src.p_icmp,
				l->src.p_other,
				l->dst.b_all,
				l->dst.b_tcp,
				l->dst.b_udp,
				l->dst.b_icmp,
				l->dst.b_other,				
				l->dst.p_all,
				l->dst.p_tcp,
				l->dst.p_udp,
				l->dst.p_icmp,
				l->dst.p_other,
				now.tv_sec - l->time.tv_sec
			);
	} else {
		struct t_ipt_account_stats_short *s = &table->stats.s[*i];
		seq_printf(sf,
				"ip = %u.%u.%u.%u bytes_src = %Lu packets_src = %Lu bytes_dst = %Lu packets_dst = %Lu time = %lu\n",
				HIPQUAD(address),
				s->src.b_all,
				s->src.p_all,
				s->dst.b_all,
				s->dst.p_all,
				now.tv_sec - s->time.tv_sec
			);
	}

	return 0;
}

static struct seq_operations ipt_account_seq_ops = {
	.start = ipt_account_seq_start,
	.next = ipt_account_seq_next,
	.stop = ipt_account_seq_stop,
	.show = ipt_account_seq_show
};

static ssize_t ipt_account_proc_write(struct file *file, const char __user *input, size_t size, loff_t *ofs)
{
	char buffer[1024];
	struct proc_dir_entry *pde = PDE(file->f_dentry->d_inode);
	struct t_ipt_account_table *table = pde->data;

	u_int32_t o[4], ip;
	struct t_ipt_account_stats_long l;
	struct t_ipt_account_stats_short s;

#ifdef IPT_ACCOUNT_DEBUG
	if (debug) printk(KERN_DEBUG "ipt_account [ipt_account_proc_write]: name = %s.\n", table->name);
#endif
	if (copy_from_user(buffer, input, 1024))
		return -EFAULT;
	buffer[1023] = '\0';

	if (!strncmp(buffer, "reset\n", 6)) {
		/*
		 * User requested to clear all table. Ignorant, does
		 * he known how match time it took us to fill it? ;-)
		 */
		write_lock_bh(&table->stats_lock);
		if (table->shortlisting)
			memset(table->stats.s, 0, sizeof(struct t_ipt_account_stats_short) * table->count);
		else
			memset(table->stats.l, 0, sizeof(struct t_ipt_account_stats_long) * table->count);
		write_unlock_bh(&table->stats_lock);		
 	} else if (!strncmp(buffer, "time=any\n", 9)) {
 		table->timesrc = table->timedst = 1;
 	} else if (!strncmp(buffer, "time=src\n", 9)) {
 		table->timesrc = 1;
 		table->timedst = 0;
 	} else if (!strncmp(buffer, "time=dst\n", 9)) {
 		table->timesrc = 0;
 		table->timedst = 1;
	} else if (!table->shortlisting && sscanf(buffer, "ip = %u.%u.%u.%u bytes_src = %Lu %Lu %Lu %Lu %Lu packets_src = %Lu %Lu %Lu %Lu %Lu bytes_dst = %Lu %Lu %Lu %Lu %Lu packets_dst = %Lu %Lu %Lu %Lu %Lu time = %lu", 				
				&o[0], &o[1], &o[2], &o[3],
				&l.src.b_all, &l.src.b_tcp, &l.src.b_udp, &l.src.b_icmp, &l.src.b_other,
				&l.src.p_all, &l.src.p_tcp, &l.src.p_udp, &l.src.p_icmp, &l.src.p_other,
				&l.dst.b_all, &l.dst.b_tcp, &l.dst.b_udp, &l.dst.b_icmp, &l.dst.b_other,
				&l.dst.p_all, &l.dst.p_tcp, &l.dst.p_udp, &l.dst.p_icmp, &l.dst.p_other,
				&l.time.tv_sec) == 25) {
		/*
		 * We got line formated like long listing row. We have to
		 * check, if IP is accounted by table. If so, we
		 * simply replace row with user's one.
		 */
		ip = o[0] << 24 | o[1] << 16 | o[2] << 8 | o[3];
		if ((u_int32_t)(ip & table->netmask) == (u_int32_t)table->network) {
			/*
			 * Ignore user input time. Set current time.
			 */
			jiffies_to_timespec(jiffies, &l.time);
			write_lock_bh(&table->stats_lock);
			table->stats.l[ip - table->network] = l;
			write_unlock_bh(&table->stats_lock);
		}
	} else if (table->shortlisting && sscanf(buffer, "ip = %u.%u.%u.%u bytes_src = %Lu packets_src = %Lu bytes_dst = %Lu packets_dst = %Lu time = %lu\n", 
				&o[0], &o[1], &o[2], &o[3], 
				&s.src.b_all, 
				&s.src.p_all, 
				&s.dst.b_all, 
				&s.dst.p_all, 
				&s.time.tv_sec) == 9) {		
		/*
		 * We got line formated like short listing row. Do the
		 * same action like above.
		 */		
		ip = o[0] << 24 | o[1] << 16 | o[2] << 8 | o[3];		
		if ((u_int32_t)(ip & table->netmask) == (u_int32_t)table->network) {
			jiffies_to_timespec(jiffies, &s.time);
			write_lock_bh(&table->stats_lock);
			table->stats.s[ip - table->network] = s;
			write_unlock_bh(&table->stats_lock);
		}
	} else {
		/*
		 * We don't understand what user have just wrote.
		 */
		return -EIO;
	}

	return size;
}

static int ipt_account_proc_open(struct inode *inode, struct file *file)
{
	int ret = seq_open(file, &ipt_account_seq_ops);
	if (!ret) {
		struct seq_file *sf = file->private_data;
		struct proc_dir_entry *pde = PDE(inode);
		struct t_ipt_account_table *table = pde->data;
		
		sf->private = pde;

		ipt_account_table_get(table);
	}
	return ret;
}

static int ipt_account_proc_release(struct inode *inode, struct file *file)
{
	struct proc_dir_entry *pde = PDE(inode);
	struct t_ipt_account_table *table = pde->data;
	int ret;

	ret = seq_release(inode, file);

	if (!ret)
		ipt_account_table_put(table);
	
	return ret;
}

static struct file_operations ipt_account_proc_fops = {
	.owner = THIS_MODULE,
	.open = ipt_account_proc_open,
	.read = seq_read,
	.write = ipt_account_proc_write,
	.llseek = seq_lseek,
	.release = ipt_account_proc_release
};

/*
 * Module init function.
 */
static int __init init(void)
{
	int ret = 0;

	printk(KERN_INFO "ipt_account %s : Piotr Gasidlo <quaker@barbara.eu.org>, http://www.barbara.eu.org/~quaker/ipt_account/\n", IPT_ACCOUNT_VERSION);

	/* Check module parameters. */
	if (netmask > 32 || netmask < 0) {
		printk(KERN_ERR "ipt_account[__init]: Wrong netmask given as parameter (%i). Valid is 32 to 0.\n", netmask);
		ret = -EINVAL;
		goto cleanup_none;
	}
	
	/* Register match. */
	if (ipt_register_match(&account_match)) {
		ret = -EINVAL;
		goto cleanup_none;
	}

	/* Create /proc/net/ipt_account/ entry. */
	ipt_account_procdir = proc_mkdir("ipt_account", proc_net);
	if (!ipt_account_procdir) {		
		printk(KERN_ERR "ipt_account [__init]: ipt_account_procdir = proc_mkdir(\"ipt_account\", proc_net) failed.\n");
		ret = -ENOMEM;
		goto cleanup_match;
	}
	
	return ret;

	/* If something goes wrong we end here. */
cleanup_match:
	ipt_unregister_match(&account_match);
cleanup_none:
	return ret;
}

/*
 * Module exit function.
 */
static void __exit fini(void)
{
	/* Remove /proc/net/ipt_account/ */
	remove_proc_entry(ipt_account_procdir->name, ipt_account_procdir->parent);	
	ipt_unregister_match(&account_match);
}

module_init(init);
module_exit(fini);

