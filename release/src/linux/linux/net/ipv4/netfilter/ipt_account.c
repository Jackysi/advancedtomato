/* 
 * accounting match (ipt_account.c)
 * (C) 2003,2004 by Piotr Gasidlo (quaker@barbara.eu.org)
 *
 * Version: 0.1.7
 *
 * This software is distributed under the terms of GNU GPL
 */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/ctype.h>

#include <linux/seq_file.h>

#include <asm/uaccess.h>

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_account.h>

#if defined(CONFIG_IP_NF_MATCH_ACCOUNT_DEBUG)
	#define dprintk(format,args...) printk(format,##args)
#else
        #define dprintk(format,args...)
#endif

static char version[] =
KERN_INFO IPT_ACCOUNT_NAME " " IPT_ACCOUNT_VERSION " : Piotr Gasid³o <quaker@barbara.eu.org>, http://www.barbara.eu.org/~quaker/ipt_account/\n";

/* rights for files created in /proc/net/ipt_account/ */
static int permissions = 0644;
/* maximal netmask for single table */
static int netmask = 16;

/* module information */
MODULE_AUTHOR("Piotr Gasidlo <quaker@barbara.eu.org>");
MODULE_DESCRIPTION("Traffic accounting modules");
MODULE_LICENSE("GPL");
MODULE_PARM(permissions,"i");
MODULE_PARM_DESC(permissions,"permissions on /proc/net/ipt_account/* files");
MODULE_PARM(netmask, "i");
MODULE_PARM_DESC(netmask, "maximum *save* size of one list (netmask)");

/* structure with statistics counters */
struct t_ipt_account_stat {
	u_int64_t b_all, b_tcp, b_udp, b_icmp, b_other;		/* byte counters for all/tcp/udp/icmp/other traffic  */
	u_int64_t p_all, p_tcp, p_udp, p_icmp, p_other;		/* packet counters for all/tcp/udp/icmp/other traffic */
};

/* stucture with statistics counters, used when table is created with --ashort switch */
struct t_ipt_account_stat_short {
	u_int64_t b_all;					/* byte counters for all traffic */
	u_int64_t p_all;					/* packet counters for all traffic */
};
 
/* structure holding to/from statistics for single ip */
struct t_ipt_account_ip_list {
	struct t_ipt_account_stat src;
	struct t_ipt_account_stat dest;
	unsigned long time;					/* time when this record was last updated */	
	
};

/* same as above, for tables with --ashort switch */
struct t_ipt_account_ip_list_short {
	struct t_ipt_account_stat_short src;
	struct t_ipt_account_stat_short dest;
	unsigned long time;
};

/* structure describing single table */
struct t_ipt_account_table {
	char name[IPT_ACCOUNT_NAME_LEN];	/* table name ( = filename in /proc/net/ipt_account/) */
	union {					/* table with statistics for each ip in network/netmask */
		struct t_ipt_account_ip_list *l;
		struct t_ipt_account_ip_list_short *s;
	} ip_list;
	u_int32_t network;			/* network/netmask covered by table*/
	u_int32_t netmask;					
	u_int32_t count;
	int shortlisting:1;			/* show only total columns of counters */	
	int use_count;				/* rules counter - counting number of rules using this table */
	struct t_ipt_account_table *next;
	spinlock_t ip_list_lock;
	struct proc_dir_entry *status_file;
};

/* we must use spinlocks to avoid parallel modifications of table list */
static spinlock_t account_lock = SPIN_LOCK_UNLOCKED;

static struct proc_dir_entry *proc_net_ipt_account = NULL;

/* root pointer holding list of the tables */
static struct t_ipt_account_table *account_tables = NULL;

/* convert ascii to ip */
int atoip(char *buffer, u_int32_t *ip) {

	char *bufferptr = buffer;
	int part, shift;
	
	/* zero ip */
	*ip = 0;

	/* first must be a digit */
	if (!isdigit(*bufferptr))
		return 0;

	/* parse first 3 octets (III.III.III.iii) */
	for (part = 0, shift = 24; *bufferptr && shift; bufferptr++) {
		if (isdigit(*bufferptr)) {
			part = part * 10 + (*bufferptr - '0');
			continue;
		}
		if (*bufferptr == '.') {
			if (part > 255)
				return 0;
			*ip |= part << shift;
			shift -= 8;
			part = 0;
			continue;
		}
		return 0;
	}
	
	/* we expect more digts */
	if (!*bufferptr)
		return 0;
	/* parse last octet (iii.iii.iii.III) */
	for (; *bufferptr; bufferptr++) {
		if (isdigit(*bufferptr)) {
			part = part * 10 + (*bufferptr - '0');			
			continue;
		} else {
			if (part > 255)
				return 0;
			*ip |= part;
			break;
		}
	}
	return (bufferptr - buffer);
}

/* convert ascii to 64bit integer */
int atoi64(char *buffer, u_int64_t *i) {	
	char *bufferptr = buffer;

	/* zero integer */
	*i = 0;
	
	while (isdigit(*bufferptr)) {
		*i = *i * 10 + (*bufferptr - '0');
		bufferptr++;
	}
	return (bufferptr - buffer);
}

static void *account_seq_start(struct seq_file *s, loff_t *pos)
{
	struct proc_dir_entry *pde = s->private;
	struct t_ipt_account_table *table = pde->data;

	unsigned int *bucket;
	
	spin_lock_bh(&table->ip_list_lock);
	if (*pos >= table->count)
		return NULL;

	bucket = kmalloc(sizeof(unsigned int), GFP_KERNEL);
	if (!bucket)
		return ERR_PTR(-ENOMEM);
	*bucket = *pos;
	return bucket;
}

static void *account_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct proc_dir_entry *pde = s->private;
	struct t_ipt_account_table *table = pde->data;
	
	unsigned int *bucket = (unsigned int *)v;
	
	*pos = ++(*bucket);
	if (*pos >= table->count) {
		kfree(v);
		return NULL;
	}
	return bucket;
}

static void account_seq_stop(struct seq_file *s, void *v)
{
	struct proc_dir_entry *pde = s->private;
	struct t_ipt_account_table *table = pde->data;
	unsigned int *bucket = (unsigned int *)v;
	kfree(bucket);
	spin_unlock_bh(&table->ip_list_lock);
}

static int account_seq_write(struct file *file, const char *ubuffer, 
		size_t ulength, loff_t *pos)
{
	struct proc_dir_entry *pde = ((struct seq_file *)file->private_data)->private;
	struct t_ipt_account_table *table = pde->data;
	char buffer[1024], *bufferptr;
	int length;

	u_int32_t ip;
	int len, i;
	struct t_ipt_account_ip_list l;
	struct t_ipt_account_ip_list_short s;
	u_int64_t *p, dummy;
	
	
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() entered.\n");
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() ulength = %zi.\n", ulength);
	
	length = ulength;
	if (ulength > 1024)
		length = 1024;
	if (copy_from_user(buffer, ubuffer, length))
		return -EFAULT;
	buffer[length - 1] = 0;
	bufferptr = buffer;

	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() buffer = \'%s\' length = %i.\n", buffer, length);
	
	/* reset table counters */
	if (!memcmp(buffer, "reset", 5)) {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got \"reset\".\n");
		if (!table->shortlisting) {
			spin_lock_bh(&table->ip_list_lock);
			memset(table->ip_list.l, 0, sizeof(struct t_ipt_account_ip_list) * table->count);
			spin_unlock_bh(&table->ip_list_lock);
		} else {
			spin_lock_bh(&table->ip_list_lock);
			memset(table->ip_list.s, 0, sizeof(struct t_ipt_account_ip_list_short) * table->count);
			spin_unlock_bh(&table->ip_list_lock);
		}
		return length;
	}

	if (!memcmp(buffer, "ip", 2)) {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got \"ip\".\n");
		bufferptr += 2;
		if (!isspace(*bufferptr)) {
			dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
			return length; /* expected space */
		}
		bufferptr += 1;
		if (*bufferptr != '=') {
			dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected equal (%ti).\n", bufferptr - buffer);
			return length; /* expected equal */
		}
		bufferptr += 1;
		if (!isspace(*bufferptr)) {
			dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
			return length; /* expected space */
		}
		bufferptr += 1;
		if (!(len = atoip(bufferptr, &ip))) {
			dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected ip (%ti).\n", bufferptr - buffer);
			return length; /* expected ip */
		}
		bufferptr += len;
		if ((ip & table->netmask) != table->network) {
			dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected ip [%u.%u.%u.%u] from table's network/netmask [%u.%u.%u.%u/%u.%u.%u.%u].\n", HIPQUAD(ip), HIPQUAD(table->network), HIPQUAD(table->netmask));
			return length; /* expected ip from table's network/netmask */
		}
		if (!table->shortlisting) {
			memset(&l, 0, sizeof(struct t_ipt_account_ip_list));
			while(*bufferptr) {
				if (!isspace(*bufferptr)) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
					return length; /* expected space */
				}
				bufferptr += 1;
				if (!memcmp(bufferptr, "bytes_src", 9)) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got bytes_src (%ti).\n", bufferptr - buffer);
					p = &l.src.b_all;
					bufferptr += 9;
				} else if (!memcmp(bufferptr, "bytes_dest", 10)) {					
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got bytes_dest (%ti).\n", bufferptr - buffer);
					p = &l.dest.b_all;
					bufferptr += 10;
				} else if (!memcmp(bufferptr, "packets_src", 11)) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got packets_src (%ti).\n", bufferptr - buffer);
					p = &l.src.p_all;
					bufferptr += 11;
				} else if (!memcmp(bufferptr, "packets_dest", 12)) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got packets_dest (%ti).\n", bufferptr - buffer);
					p = &l.dest.p_all;
					bufferptr += 12;
				} else if (!memcmp(bufferptr, "time", 4)) {
					/* time hack, ignore time tokens */
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got time (%ti).\n", bufferptr - buffer);
					bufferptr += 4;
					if (!isspace(*bufferptr)) {
						dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
						return length; /* expected space */
					}
					bufferptr += 1;
					if (*bufferptr != '=') {
						dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected equal (%ti).\n", bufferptr - buffer);
						return length; /* expected equal */
					}
					bufferptr += 1;
					if (!isspace(*bufferptr)) {
						dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
						return length; /* expected space */
					}
					bufferptr += 1;
					if (!(len = atoi64(bufferptr, &dummy))) {
						dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected int64 (%ti).\n", bufferptr - buffer);
						return length; /* expected int64 */
					}
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got %llu (%ti).\n", dummy, bufferptr - buffer);
					bufferptr += len;
					continue; /* skip time token */
				} else
					return length;	/* expected token */
				if (!isspace(*bufferptr)) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
					return length; /* expected space */
				}
				bufferptr += 1;
				if (*bufferptr != '=') {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected equal (%ti).\n", bufferptr - buffer);
					return length; /* expected equal */
				}
				bufferptr += 1;
				for (i = 0; i < 5; i++) {
					if (!isspace(*bufferptr)) {
						dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
						return length; /* expected space */
					}
					bufferptr += 1;
					if (!(len = atoi64(bufferptr, p))) {
						dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected int64 (%ti).\n", bufferptr - buffer);
						return length; /* expected int64 */
					}
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got %llu (%ti).\n", *p, bufferptr - buffer);
					bufferptr += len;
					p++;
				}
			}
			dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() updating row.\n");
			spin_lock_bh(&table->ip_list_lock);
			/* update counters, do not overwrite time field */
			memcpy(&table->ip_list.l[ip - table->network], &l, sizeof(struct t_ipt_account_ip_list) - sizeof(unsigned long));
			spin_unlock_bh(&table->ip_list_lock);
		} else {
			memset(&s, 0, sizeof(struct t_ipt_account_ip_list_short));
			while(*bufferptr) {
				if (!isspace(*bufferptr)) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
					return length; /* expected space */
				}
				bufferptr += 1;
				if (!memcmp(bufferptr, "bytes_src", 9)) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got bytes_src (%ti).\n", bufferptr - buffer);
					p = &s.src.b_all;
					bufferptr += 9;
				} else if (!memcmp(bufferptr, "bytes_dest", 10)) {					
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got bytes_dest (%ti).\n", bufferptr - buffer);
					p = &s.dest.b_all;
					bufferptr += 10;
				} else if (!memcmp(bufferptr, "packets_src", 11)) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got packets_src (%ti).\n", bufferptr - buffer);
					p = &s.src.p_all;
					bufferptr += 11;
				} else if (!memcmp(bufferptr, "packets_dest", 12)) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got packets_dest (%ti).\n", bufferptr - buffer);
					p = &s.dest.p_all;
					bufferptr += 12;
				} else if (!memcmp(bufferptr, "time", 4)) {
					/* time hack, ignore time tokens */
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got time (%ti).\n", bufferptr - buffer);
					bufferptr += 4;
					if (!isspace(*bufferptr)) {
						dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
						return length; /* expected space */
					}
					bufferptr += 1;
					if (*bufferptr != '=') {
						dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected equal (%ti).\n", bufferptr - buffer);
						return length; /* expected equal */
					}
					bufferptr += 1;
					if (!isspace(*bufferptr)) {
						dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
						return length; /* expected space */
					}
					bufferptr += 1;
					if (!(len = atoi64(bufferptr, &dummy))) {
						dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected int64 (%ti).\n", bufferptr - buffer);
						return length; /* expected int64 */
					}
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got %llu (%ti).\n", dummy, bufferptr - buffer);
					bufferptr += len;
					continue; /* skip time token */
				} else {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected token (%ti).\n", bufferptr - buffer);
					return length;	/* expected token */
				}
				if (!isspace(*bufferptr)) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
					return length; /* expected space */
				}
				bufferptr += 1;
				if (*bufferptr != '=') {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected equal (%ti).\n", bufferptr - buffer);
					return length; /* expected equal */
				}
				bufferptr += 1;
				if (!isspace(*bufferptr)) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected space (%ti).\n", bufferptr - buffer);
					return length; /* expected space */
				}
				bufferptr += 1;
				if (!(len = atoi64(bufferptr, p))) {
					dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() expected int64 (%ti).\n", bufferptr - buffer);
					return length; /* expected int64 */
				}
				dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() got %llu (%ti).\n", *p, bufferptr - buffer);
				bufferptr += len;
			}
			dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() updating row.\n");
			spin_lock_bh(&table->ip_list_lock);
			/* update counters, do not overwrite time field */
			memcpy(&table->ip_list.s[ip - table->network], &s, sizeof(struct t_ipt_account_ip_list_short) - sizeof(unsigned long));
			spin_unlock_bh(&table->ip_list_lock);
		}
	}
	
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": account_seq_write() left.\n");
	return length;
}


static int account_seq_show(struct seq_file *s, void *v)
{
	struct proc_dir_entry *pde = s->private;
	struct t_ipt_account_table *table = pde->data;
	unsigned int *bucket = (unsigned int *)v;

	u_int32_t address = table->network + *bucket;
	struct timespec last;

	if (!table->shortlisting) {
		jiffies_to_timespec(jiffies - table->ip_list.l[*bucket].time, &last);
		seq_printf(s,
				"ip = %u.%u.%u.%u bytes_src = %llu %llu %llu %llu %llu packets_src = %llu %llu %llu %llu %llu bytes_dest = %llu %llu %llu %llu %llu packets_dest = %llu %llu %llu %llu %llu time = %lu\n",
				HIPQUAD(address),
				table->ip_list.l[*bucket].src.b_all,
				table->ip_list.l[*bucket].src.b_tcp,
				table->ip_list.l[*bucket].src.b_udp,
				table->ip_list.l[*bucket].src.b_icmp,
				table->ip_list.l[*bucket].src.b_other,
				table->ip_list.l[*bucket].src.p_all,
				table->ip_list.l[*bucket].src.p_tcp,
				table->ip_list.l[*bucket].src.p_udp,
				table->ip_list.l[*bucket].src.p_icmp,
				table->ip_list.l[*bucket].src.p_other,
				table->ip_list.l[*bucket].dest.b_all,
				table->ip_list.l[*bucket].dest.b_tcp,
				table->ip_list.l[*bucket].dest.b_udp,
				table->ip_list.l[*bucket].dest.b_icmp,
				table->ip_list.l[*bucket].dest.b_other,				
				table->ip_list.l[*bucket].dest.p_all,
				table->ip_list.l[*bucket].dest.p_tcp,
				table->ip_list.l[*bucket].dest.p_udp,
				table->ip_list.l[*bucket].dest.p_icmp,
				table->ip_list.l[*bucket].dest.p_other,
				last.tv_sec
			);
	} else {
		jiffies_to_timespec(jiffies - table->ip_list.s[*bucket].time, &last);
		seq_printf(s,
				"ip = %u.%u.%u.%u bytes_src = %llu packets_src = %llu bytes_dest = %llu packets_dest = %llu time = %lu\n",
				HIPQUAD(address),
				table->ip_list.s[*bucket].src.b_all,
				table->ip_list.s[*bucket].src.p_all,
				table->ip_list.s[*bucket].dest.b_all,
				table->ip_list.s[*bucket].dest.p_all,
				last.tv_sec
			  );
	}
	return 0;
}

static struct seq_operations account_seq_ops = {
	.start = account_seq_start,
	.next  = account_seq_next,
	.stop  = account_seq_stop,
	.show  = account_seq_show
};

static int account_seq_open(struct inode *inode, struct file *file)
{
	int ret = seq_open(file, &account_seq_ops);
	
	if (!ret) {
		struct seq_file *sf = file->private_data;
		sf->private = PDE(inode);
	}
	return ret;
}

static struct file_operations account_file_ops = {
	.owner = THIS_MODULE,
	.open = account_seq_open,
	.read = seq_read,
	.write = account_seq_write,
	.llseek = seq_lseek,
	.release = seq_release
};

/* do raw accounting */
static inline void do_account(struct t_ipt_account_stat *stat, const struct sk_buff *skb) {
	
	/* update packet & bytes counters in *stat structure */
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

static inline void do_account_short(struct t_ipt_account_stat_short *stat, const struct sk_buff *skb) {

	/* update packet & bytes counters in *stat structure */
	stat->b_all += skb->len;
	stat->p_all++;
}

static int match(const struct sk_buff *skb,
	  const struct net_device *in,
	  const struct net_device *out,
	  const void *matchinfo,
	  int offset,
	  const void *hdr,
	  u_int16_t datalen,
	  int *hotdrop)
{
	
	const struct t_ipt_account_info *info = (struct t_ipt_account_info*)matchinfo;
	struct t_ipt_account_table *table;
	int ret;
	unsigned long now;

	u_int32_t address;
	
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": match() entered.\n");
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": match() match name = %s.\n", info->name);
	
	spin_lock_bh(&account_lock);
	/* find the right table */
	table = account_tables;
	while (table && strncmp(table->name, info->name, IPT_ACCOUNT_NAME_LEN) && (table = table->next));
	spin_unlock_bh(&account_lock);

	if (table == NULL) {
		/* ups, no table with that name */
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": match() table %s not found. Leaving.\n", info->name);
		return 0;
	}

	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": match() table found %s\n", table->name);

	/*  lock table while updating statistics */
	spin_lock_bh(&table->ip_list_lock);

	/* default: no match */
	ret = 0;

	/* get current time */
	now = jiffies;

	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": match() got packet src = %u.%u.%u.%u, dst = %u.%u.%u.%u, proto = %u.\n", NIPQUAD(skb->nh.iph->saddr), NIPQUAD(skb->nh.iph->daddr), skb->nh.iph->protocol);
			
	/* check whether traffic from source ip address ... */
	address = ntohl(skb->nh.iph->saddr);
	/* ... is being accounted by this table */	
	if (address && ((u_int32_t)(address & table->netmask) == (u_int32_t)table->network)) {		
		/* yes, account this packet */
		dprintk(KERN_INFO "ipt_account: match() accounting packet src = %u.%u.%u.%u, proto = %u.\n", HIPQUAD(address), skb->nh.iph->protocol);
		/* update counters this host */
		if (!table->shortlisting) {
			do_account(&table->ip_list.l[address - table->network].src, skb);
			table->ip_list.l[address - table->network].time = now;
			/* update also counters for all hosts in this table (network address) */
			if (table->netmask != INADDR_BROADCAST) {
				do_account(&table->ip_list.l[0].src, skb);
				table->ip_list.l[0].time = now;
			}
		} else {
			do_account_short(&table->ip_list.s[address - table->network].src, skb);
			table->ip_list.s[address - table->network].time = now;
			/* update also counters for all hosts in this table (network address) */
			if (table->netmask != INADDR_BROADCAST) {
				do_account_short(&table->ip_list.s[0].src, skb);
				table->ip_list.s[0].time = now;
			}
		}
		/* yes, it's a match */
		ret = 1;
	}

	/* do the same thing with destination ip address */
	address = ntohl(skb->nh.iph->daddr);
	if (address && ((u_int32_t)(address & table->netmask) == (u_int32_t)table->network)) {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": match() accounting packet dst = %u.%u.%u.%u, proto = %u.\n", HIPQUAD(address), skb->nh.iph->protocol);
		if (!table->shortlisting) {
			do_account(&table->ip_list.l[address - table->network].dest, skb);
			table->ip_list.l[address - table->network].time = now;
			if (table->netmask != INADDR_BROADCAST) {
				do_account(&table->ip_list.l[0].dest, skb);				
				table->ip_list.s[0].time = now;
			}
		} else {
			do_account_short(&table->ip_list.s[address - table->network].dest, skb);
			table->ip_list.s[address - table->network].time = now;
			if (table->netmask != INADDR_BROADCAST) {
				do_account_short(&table->ip_list.s[0].dest, skb);
				table->ip_list.s[0].time = now;
			}
		}
		ret = 1;
	}
	spin_unlock_bh(&table->ip_list_lock);
	
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": match() left.\n");	

	return ret;
}

static int checkentry(const char *tablename,
	       const struct ipt_ip *ip,
	       void *matchinfo,
	       unsigned int matchinfosize,
	       unsigned int hook_mask)
{
	const struct t_ipt_account_info *info = matchinfo;
	struct t_ipt_account_table *table, *find_table, *last_table;
	int ret = 0;

	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() entered.\n");

	if (matchinfosize != IPT_ALIGN(sizeof(struct t_ipt_account_info))) return 0;
	if (!info->name || !info->name[0]) return 0;

	/* find whether table with this name already exists */
	spin_lock_bh(&account_lock);
	find_table = account_tables;
	while( (last_table = find_table) && strncmp(info->name,find_table->name,IPT_ACCOUNT_NAME_LEN) && (find_table = find_table->next) );
	if (find_table != NULL) {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() table %s found.\n", info->name);		
		/* if table exists, check whether table network/netmask equals rule network/netmask */
		if (find_table->network != info->network || find_table->netmask != info->netmask || find_table->shortlisting != info->shortlisting) {
			spin_unlock_bh(&account_lock);
			printk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() wrong parameters (not equals existing table parameters).\n");
			ret = 0;
			goto failure;
		}
		/* increment table use count */
		find_table->use_count++;
		spin_unlock_bh(&account_lock);
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() incrementing use count.\n");
		ret = 1;
		goto failure;
	}
	spin_unlock_bh(&account_lock);

	/* check netmask first, before allocating memory */
	if (info->netmask < ((1 << netmask) - 1)) {
		printk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() too big netmask.\n");
		ret = 0;
		goto failure;
	}

	/* table doesn't exist - create new */
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() allocating %zu for new table %s.\n", sizeof(struct t_ipt_account_table), info->name);
        table = vmalloc(sizeof(struct t_ipt_account_table));
	if (table == NULL) {
		printk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() failed to allocate %zu for new table %s.\n", sizeof(struct t_ipt_account_table), info->name);
		ret = 0; /* was -ENOMEM */
		goto failure;
	}
	
	/* setup table parameters */
	table->ip_list_lock = SPIN_LOCK_UNLOCKED;
	table->next = NULL;
	table->use_count = 1;
	table->network = info->network;
	table->netmask = info->netmask;
	table->shortlisting = info->shortlisting;
	table->count = (~table->netmask) + 1;
	strncpy(table->name,info->name,IPT_ACCOUNT_NAME_LEN);
	table->name[IPT_ACCOUNT_NAME_LEN - 1] = '\0';
	
	/* allocate memory for table->ip_list */
	if (!table->shortlisting) {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() allocating %zu for ip_list.\n", sizeof(struct t_ipt_account_ip_list) * table->count);
		table->ip_list.l = vmalloc(sizeof(struct t_ipt_account_ip_list) * table->count);
		if (table->ip_list.l == NULL) {
			dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() failed to allocate %zu for ip_list.\n", sizeof(struct t_ipt_account_ip_list) * table->count);
			ret = 0; /* was -ENOMEM */
			goto failure_table;
		}
		memset(table->ip_list.l, 0, sizeof(struct t_ipt_account_ip_list) * table->count);
	} else {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() allocating %zu for ip_list.\n", sizeof(struct t_ipt_account_ip_list_short) * table->count);
		table->ip_list.s = vmalloc(sizeof(struct t_ipt_account_ip_list_short) * table->count);
		if (table->ip_list.s == NULL) {
			dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() failed to allocate %zu for ip_list.\n", sizeof(struct t_ipt_account_ip_list_short) * table->count);
			ret = 0; /* was -ENOMEM */
			goto failure_table;
		}
		memset(table->ip_list.s, 0, sizeof(struct t_ipt_account_ip_list_short) * table->count);
	}
	
	/* put table into chain */
	spin_lock_bh(&account_lock);
	find_table = account_tables;
	while( (last_table = find_table) && strncmp(info->name, find_table->name, IPT_ACCOUNT_NAME_LEN) && (find_table = find_table->next) );
	if (find_table != NULL) {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() table %s found.\n", info->name);
		if (find_table->network != info->network || find_table->netmask != info->netmask) {
			spin_unlock_bh(&account_lock);
			printk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() wrong network/netmask.\n");
			ret = 0;
			goto failure_ip_list;
		}
		find_table->use_count++;
		spin_unlock_bh(&account_lock);
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() incrementing use count.\n");
		ret = 1;
		goto failure_ip_list;
	}
	if (!last_table) 
		account_tables = table; 
	else 
		last_table->next = table;
	spin_unlock_bh(&account_lock);

	/* create procfs status file */
	table->status_file = create_proc_entry(table->name, permissions, proc_net_ipt_account);
	if (table->status_file == NULL) {
		ret = 0; /* was -ENOMEM */
		goto failure_unlink;
	}
	table->status_file->owner = THIS_MODULE;
	table->status_file->data = table;	
	wmb();
//	if (!table->shortlisting)
		table->status_file->proc_fops = &account_file_ops;
//	else
//		table->status_file->proc_fops = &account_file_ops_short;

	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() left.\n");	
	/* everything went just okey */
	return 1;

	/* do cleanup in case of failure */
failure_unlink:
	/* remove table from list */
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() removing table.\n");
	spin_lock_bh(&account_lock);
	last_table = NULL;
	table = account_tables;
	if (table == NULL) {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() no table found. Leaving.\n");
		spin_unlock_bh(&account_lock);
		return 0; /* was -ENOMEM */
	}
	while (strncmp(info->name, table->name, IPT_ACCOUNT_NAME_LEN) && (last_table = table) && (table = table->next));
	if (table == NULL) {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() table already destroyed. Leaving.\n");
		spin_unlock_bh(&account_lock);
		return 0; /* was -ENOMEM */
	}
	if (last_table)
		last_table->next = table->next;
	else
		account_tables = table->next;
	spin_unlock_bh(&account_lock);
failure_ip_list:
	/* free memory allocated for statistics table */
	if (!table->shortlisting)
		vfree(table->ip_list.l);
	else
		vfree(table->ip_list.s);
failure_table:
	/* free table */
	vfree(table);
failure:
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() left. Table not created.\n");	
	/* failure return */
	return ret;
}

static void destroy(void *matchinfo, 
	     unsigned int matchinfosize)
{
	const struct t_ipt_account_info *info = matchinfo;
	struct t_ipt_account_table *table, *last_table;

	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": destory() entered.\n");
	
	if (matchinfosize != IPT_ALIGN(sizeof(struct t_ipt_account_info))) return;

	/* search for table */
	spin_lock_bh(&account_lock);
	last_table = NULL;
	table = account_tables;
	if(table == NULL) {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": destory() no tables found. Leaving.\n");
		spin_unlock_bh(&account_lock);
		return;
	}
	while( strncmp(info->name,table->name,IPT_ACCOUNT_NAME_LEN) && (last_table = table) && (table = table->next) );
	if (table == NULL) {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": destory() no table %s not found. Leaving.\n", info->name);
		spin_unlock_bh(&account_lock);
		return;
	}

	/* decrement table use-count */
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": destory() decrementing use count.\n");
	table->use_count--;
	if (table->use_count) {
		dprintk(KERN_INFO IPT_ACCOUNT_NAME ": destory() table still in use. Leaving.\n");
		spin_unlock_bh(&account_lock);
		return;
	}

	/* remove table if use-count is zero */
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": destory() table %s not used. Removing.\n", table->name);

	/* unlink table */
	if(last_table) 
		last_table->next = table->next; 
	else 
		account_tables = table->next;
	spin_unlock_bh(&account_lock);

	/* wait while table is still in use */
	spin_lock_bh(&table->ip_list_lock);
	spin_unlock_bh(&table->ip_list_lock);

	/* remove proc entries */	
	remove_proc_entry(table->name, proc_net_ipt_account);

	/* remove table */
	if (!table->shortlisting)
		vfree(table->ip_list.l);
	else
		vfree(table->ip_list.s);
	vfree(table);

	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": destory() left.\n");
	return;
}

static struct ipt_match account_match = {
	.name = "account",
	.match = &match,
	.checkentry = &checkentry,
	.destroy = &destroy,
	.me = THIS_MODULE
};

static int __init init(void) 
{
	int err;
	
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": __init() entered.\n");
	printk(version);	
	/* check params */
	if (netmask > 32 || netmask < 0) {
		printk(KERN_INFO "account: Wrong netmask given by netmask parameter (%i). Valid is 32 to 0.\n", netmask);
		err = -EINVAL;
		goto doexit;
	}

	/* create /proc/net/ipt_account directory */
	proc_net_ipt_account = proc_mkdir("ipt_account", proc_net);
	if (!proc_net_ipt_account) {		
		printk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() failed to create procfs entry.\n");
		err = -ENOMEM;
		goto doexit;
	}
	proc_net_ipt_account->owner = THIS_MODULE;
	
	err = ipt_register_match(&account_match);
	if (err) {
		printk(KERN_INFO IPT_ACCOUNT_NAME ": checkentry() failed to register match.\n");
		remove_proc_entry("ipt_account", proc_net);
	}
doexit:
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": __init() left.\n");
	return err;
}

static void __exit fini(void) 
{
	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": __exit() entered.\n");
	
	ipt_unregister_match(&account_match);
	/* remove /proc/net/ipt_account/ directory */
	remove_proc_entry("ipt_account", proc_net);

	dprintk(KERN_INFO IPT_ACCOUNT_NAME ": __exit() left.\n");
}

module_init(init);
module_exit(fini);

