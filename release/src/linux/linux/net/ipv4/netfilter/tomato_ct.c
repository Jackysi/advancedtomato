/*

	tomato_ct.c
	Copyright (C) 2006 Jonathan Zarate

	Licensed under GNU GPL v2.

*/
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/netfilter_ipv4/ip_conntrack.h>
#include <linux/netfilter_ipv4/ip_conntrack_core.h>

//	#define TEST_HASHDIST


#ifdef TEST_HASHDIST
static int hashdist_read(char *buffer, char **start, off_t offset, int length, int *eof, void *data)
{
	struct list_head *h;
	struct list_head *e;
	int i;
	int n;
	int count;
	char *buf;
	int max;
	
	// do this the easy way...
	max = ip_conntrack_htable_size * sizeof("12345\t12345\n");
	buf = kmalloc(max + 1, GFP_KERNEL);
	if (buf == NULL) return 0;

	n = 0;
	max -= sizeof("12345\t12345\n");
	
	READ_LOCK(&ip_conntrack_lock);

	for (i = 0; i < ip_conntrack_htable_size; ++i) {
		count = 0;
		h = &ip_conntrack_hash[i];
		if (h) {
			e = h;
			while (e->next != h) {
				++count;
				e = e->next;
			}
		}
		
		n += sprintf(buf + n, "%d\t%d\n", i, count);
		if (n > max) {
			printk("hashdist: %d > %d\n", n, max);
			break;
		}
	}
	
	READ_UNLOCK(&ip_conntrack_lock);
	
	if (offset < n) {
		n = n - offset;
		if (n > length) {
			n = length;
			*eof = 0;
		}			
		else {
			*eof = 1;
		}
		memcpy(buffer, buf + offset, n);
		*start = buffer;
	}
	else {
		n = 0;
		*eof = 1;
	}
	
	kfree(buf);
	return n;
}
#endif


static void iterate_all(void (*func)(struct ip_conntrack *, unsigned long), unsigned long data)
{
	int i;
	struct list_head *h;
	struct list_head *e;
	
	WRITE_LOCK(&ip_conntrack_lock);
	for (i = 0; i < ip_conntrack_htable_size; ++i) {
		h = &ip_conntrack_hash[i];
		if (h) {
			e = h;
			while (e->next != h) {
				e = e->next;
				func(((struct ip_conntrack_tuple_hash *)e)->ctrack, data);
			}
		}
	}
	WRITE_UNLOCK(&ip_conntrack_lock);
}

static void expireearly(struct ip_conntrack *ct, unsigned long data)
{
	if (ct->timeout.expires > data) {
		if (del_timer(&ct->timeout)) {
			ct->timeout.expires = data;
			add_timer(&ct->timeout);
		}
	}
}

static int expireearly_write(struct file *file, const char *buffer, unsigned long length, void *data)
{
	char s[8];
	unsigned long n;
	
	if ((length > 0) && (length < 6)) {
		memcpy(s, buffer, length);
		s[length] = 0;
		n = simple_strtoul(s, NULL, 10);
		if (n < 10) n = 10;
			else if (n > 86400) n = 86400;
		
		iterate_all(expireearly, jiffies + (n * HZ));
	}

/*	
	if ((length > 0) && (buffer[0] == '1')) {
		iterate_all(expireearly, jiffies + (20 * HZ));
	}
*/
	
	return length;
}


static void clearmarks(struct ip_conntrack *ct, unsigned long data)
{
	ct->mark = 0;
}

static int clearmarks_write(struct file *file, const char *buffer, unsigned long length, void *data)
{
	if ((length > 0) && (buffer[0] == '1')) {
		iterate_all(clearmarks, 0);
	}
	return length;
}

/* From ip_conntrack_core.c */
extern int ip_conntrack_clear;

static int conntrack_clear_write(struct file *file, const char *buffer, unsigned long length, void *data)
{
	if ((length > 0) && (buffer[0] == '1')) {
		ip_conntrack_clear = 1;
	}
	return length;
}

static int __init init(void)
{
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *p;

	printk(__FILE__ " [" __DATE__ " " __TIME__ "]\n");

#ifdef TEST_HASHDIST
	p = create_proc_entry("hash_dist", 0400, proc_net);
	if (p) {
		p->owner = THIS_MODULE;
		wmb();
		p->read_proc = hashdist_read;
	}
#endif

	p = create_proc_entry("expire_early", 0200, proc_net);
	if (p) {
		p->owner = THIS_MODULE;
		wmb();
		p->write_proc = expireearly_write;
	}

	p = create_proc_entry("clear_marks", 0200, proc_net);
	if (p) {
		p->owner = THIS_MODULE;
		wmb();
		p->write_proc = clearmarks_write;
	}

	p = create_proc_entry("conntrack_clear", 0200, proc_net);
	if (p) {
		p->owner = THIS_MODULE;
		wmb();
		p->write_proc = conntrack_clear_write;
	}
#endif /* CONFIG_PROC_FS */
	
	return 0;
}

static void __exit fini(void)
{
#ifdef CONFIG_PROC_FS
#ifdef TEST_HASHDIST
	remove_proc_entry("hash_dist", proc_net);
#endif
	remove_proc_entry("expire_early", proc_net);
	remove_proc_entry("clear_marks", proc_net);
	remove_proc_entry("conntrack_clear", proc_net);
#endif /* CONFIG_PROC_FS */
}

module_init(init);
module_exit(fini);

MODULE_LICENSE("GPL");
