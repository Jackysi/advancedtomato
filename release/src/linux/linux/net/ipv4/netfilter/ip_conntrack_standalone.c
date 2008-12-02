/* This file contains all the functions required for the standalone
   ip_conntrack module.

   These are not required by the compatibility layer.
*/

/* (c) 1999 Paul `Rusty' Russell.  Licenced under the GNU General
   Public Licence. */

#include <linux/types.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/brlock.h>
#include <net/checksum.h>

#define ASSERT_READ_LOCK(x) MUST_BE_READ_LOCKED(&ip_conntrack_lock)
#define ASSERT_WRITE_LOCK(x) MUST_BE_WRITE_LOCKED(&ip_conntrack_lock)

#include <linux/netfilter_ipv4/ip_conntrack.h>
#include <linux/netfilter_ipv4/ip_conntrack_protocol.h>
#include <linux/netfilter_ipv4/ip_conntrack_core.h>
#include <linux/netfilter_ipv4/ip_conntrack_helper.h>
#include <linux/netfilter_ipv4/listhelp.h>

#define DEBUGP(format, args...)

struct module *ip_conntrack_module = THIS_MODULE;
MODULE_LICENSE("GPL");

#define CLEAR_IP_CONNTRACK
#define DEL_IP_CONNTRACK_ENTRY 1
#ifdef DEL_IP_CONNTRACK_ENTRY
/*
  *
  *This part of code add for delete an entry in ip_conntrack table.
  *
  */


#define DEL_LIST_PATH "/tmp/.del_ip_conntrack"
#define printkerrline() printk("del_ip_conntrack error : %s %s %d\n", __FILE__, __func__, __LINE__)

struct del_list
{
	unsigned short proto;
	unsigned int begin_port;
	unsigned int end_port;
	unsigned int ip;
	struct del_list *next;
};

void free_del_list(struct del_list *head);
void print_del_list(struct del_list *head);
static struct del_list * malloc_new_node(const char *buf, struct del_list * head);
struct del_list * init_del_list(const char *buf, size_t size);
static int read_del_file(char * buf, unsigned int size, char *path);
static int del_match_method(const struct ip_conntrack_tuple_hash *pConn, const struct del_list * pList);
static int del_conntrack_check(const struct ip_conntrack_tuple_hash *pConn, const struct del_list * head);
void pf_del_ip_conntrack(void);
static int proc_read_del_ip_conntrack(char *page, char **start, off_t off, int count, int *eof, void *context);
static int proc_write_del_ip_conntrack(struct file *file, const char *buffer, unsigned long count, void *data);
static int end_proc_read(const char *p, char *page, off_t off, int count, char **start, int *eof);

void pf_del_ip_conntrack(void)
{
#define MAX_BUF_SIZE 1024*2
	int i;
	char buf[MAX_BUF_SIZE];
	struct del_list * del_head = NULL;
	struct list_head *head, *temp_head;
	struct ip_conntrack_tuple_hash *tuple_hash;
	
	//printk("pf_del_ip_conntrack---------------------------------------1\n");
	memset(buf, 0, MAX_BUF_SIZE);
	
	if(read_del_file(buf, MAX_BUF_SIZE, DEL_LIST_PATH) == -1)
	{
		goto final_return;
	}
	
	buf[MAX_BUF_SIZE - 1] = '\0';
	del_head = init_del_list(buf, MAX_BUF_SIZE - 1);
	//print_del_list(del_head);
	READ_LOCK(&ip_conntrack_lock);
	for (i = 0; i < ip_conntrack_htable_size; i++) 
	{
		head = &ip_conntrack_hash[i];
		temp_head = head;
		while(1) 
		{	
			temp_head = temp_head->next;				
			if(temp_head == head) 
			{			
				head = NULL;			
				temp_head = NULL;
				break;			
			}
			tuple_hash = (struct ip_conntrack_tuple_hash *)temp_head;
			if(del_conntrack_check(tuple_hash, del_head) == 1)
			{
				del_selected_conntrack(tuple_hash);
			}
		}					
	}
	READ_UNLOCK(&ip_conntrack_lock);
	free_del_list(del_head);

final_return:
	
	//printk("pf_del_ip_conntrack---------------------------------------2\n");
	return;
#undef MAX_BUF_SIZE
}

static int del_conntrack_check(const struct ip_conntrack_tuple_hash *pConn, const struct del_list * head)
{
	int ret;
	const struct del_list * p;

	ret = 0;

	if(pConn == NULL || head == NULL)
	{
		ret = -1;
		goto final_return;
	}

	for(p = head; p; p = p->next)
	{
		if(del_match_method(pConn, p) == 1)
		{
			//Match,jump out
			ret = 1;
			break;
		}		
	}

final_return:
	return ret;
} 

static int del_match_method(const struct ip_conntrack_tuple_hash *pConn, const struct del_list * pList)
{
	int ret;
	typedef enum
	{
		TCP_PROTO = 0x06,
		UDP_PROTO = 0x11,
	}proto_type;
	proto_type pt[2] = {TCP_PROTO, UDP_PROTO};

	ret = 0;
	//Check tcp and udp only
	if(pConn->tuple.dst.protonum == TCP_PROTO || pConn->tuple.dst.protonum == UDP_PROTO)
	{
		//Check proto match
		if((pList->proto == 3) || 
			((pList->proto == 0 || pList->proto == 1) && (pConn->tuple.dst.protonum == pt[pList->proto])))
		{
			//Chcek ip address match
			if(pConn->ctrack->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip == pList->ip)
			{
				//Check port match
				unsigned int tport;
				if(pConn->tuple.dst.protonum == TCP_PROTO)
				{
					//TCP
					tport = pConn->ctrack->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.tcp.port;
				}
				else
				{
					//UDP
					tport = pConn->ctrack->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.udp.port;
				}
				tport = htons(tport);
				if(tport >= pList->begin_port && tport <= pList->end_port)
				{
					ret = 1;
				}
			}
		}
	}
	return ret;
}

static int read_del_file(char * buf, unsigned int size, char *path)
{
	int retval, orgfsuid, orgfsgid;
	mm_segment_t orgfs;
	struct file *srcf;
	
	// Save uid and gid used for filesystem access.
	// Set user and group to 0 (root)       
	orgfsuid = current->fsuid;
	orgfsgid = current->fsgid;
	current->fsuid=current->fsgid = 0;
	orgfs = get_fs();
	set_fs(KERNEL_DS);

	if(path && *path)
	{
		srcf = filp_open(path, O_RDONLY, 0);
		if(IS_ERR(srcf))
		{
			printkerrline();
			retval = -1;
			goto final_return;
		}
		else
		{
			if(srcf->f_op && srcf->f_op->read)
			{
				memset(buf, 0x00, size);
				retval=srcf->f_op->read(srcf, buf, size, &srcf->f_pos);
				if(retval < 0)
				{
					printkerrline();
					retval = -1;
					goto final_return;
				}
				else
				{
					//Success,go!
					retval = 0;
					goto final_return;
				}
			}
			else
			{
				printkerrline();
				retval = -1;
				goto final_return;
			}
		}
	}
	else
	{
		printkerrline();
		retval = -1;
		goto final_return;
	}

final_return:
	if(!IS_ERR(srcf))
	{
		retval=filp_close(srcf,NULL);
		if(retval)
		{
			printkerrline();
			retval = -1;
		}
	}
	set_fs(orgfs);
	current->fsuid = orgfsuid;
	current->fsgid = orgfsgid;
	
	return retval;
}

struct del_list * init_del_list(const char *buf, size_t size)
{
#define LINE_FEED "\n"
#define TMP_BUF_SIZE 100
	const char *begin, *end;
	char tmpbuf[TMP_BUF_SIZE];
	struct del_list * head = NULL, *tmp_p;

	if(buf == NULL || size <= 0 || buf[size] != '\0')
	{
		head = NULL;
		goto final_return;
	}
	
	for(begin = end = buf; begin && (begin - buf < size); begin = end + strlen(LINE_FEED))
	{
		end = strstr(begin, LINE_FEED);
		if(end)
		{
			if((end - begin) > (TMP_BUF_SIZE - 1))
			{
				//Too large,go on
				continue;
			}
			else
			{
				memcpy(tmpbuf, begin, end - begin);
				tmpbuf[end - begin] = '\0';
				//printk("obtain string : %s\n", tmpbuf);
				if((tmp_p = malloc_new_node(tmpbuf, head)) == NULL)
				{
					//Invalid format or malloc fail,go on
					continue;
				}
				else
				{
					head = tmp_p;
				}			
			}
		}
		else
		{
			//printk("Last string : %s\n", begin);
			if((tmp_p = malloc_new_node(begin, head)) == NULL)
			{
				//Invalid format or malloc fail,jump out
				break;
			}
			else
			{
				head = tmp_p;
			}
		}
	}

final_return:
	return head;

#undef TMP_BUF_SIZE
#undef LINE_FEED
}

static struct del_list * malloc_new_node(const char *buf, struct del_list * head)
{
#define SSCANF_MATCH_NUM 7
	int i, j, k, c1, c2, c3, c4;
	struct del_list *p = NULL;

	if(sscanf(buf, "%d %d.%d.%d.%d %d-%d", &i, &c4, &c3, &c2, &c1, &j, &k) != SSCANF_MATCH_NUM)
	{
		p = NULL;
		goto final_return;
	}
	else
	{
		if(p = (struct del_list *)kmalloc(sizeof(struct del_list), GFP_ATOMIC))
		{
			p->proto = i;
			#if 0
			//Big endian
			((char *)&(p->ip))[0] = (char)c1;
			((char *)&(p->ip))[1] = (char)c2;
			((char *)&(p->ip))[2] = (char)c3;
			((char *)&(p->ip))[3] = (char)c4;
			#else
			//Little endian
			((char *)&(p->ip))[3] = (char)c1;
			((char *)&(p->ip))[2] = (char)c2;
			((char *)&(p->ip))[1] = (char)c3;
			((char *)&(p->ip))[0] = (char)c4;
			#endif
			p->begin_port = j;
			p->end_port = k;
			p->next = head;
		}
		else
		{
			p = NULL;
			goto final_return;	
		}
	}

final_return:
	return p;
#undef SSCANF_MATCH_NUM
}

void print_del_list(struct del_list *head)
{
	int i;
	struct del_list *tmp_p;

	for(i = 1, tmp_p = head; tmp_p; tmp_p = tmp_p->next, i++)
	{
		printk("Node(%d): proto=%d | ip=%0x | port=[%d-%d]\n", i, tmp_p->proto, tmp_p->ip, tmp_p->begin_port, tmp_p->end_port);
	}
}

void free_del_list(struct del_list *head)
{
	int i;
	struct del_list *tmp_p;
	
	if(head == NULL)
	{
		goto final_return;
	}
	for(i = 1, tmp_p = head; head; head = tmp_p, i++)
	{
		tmp_p = head->next;
		//printk("Free@Node(%d):proto=%d | ip=%0x | port=[%d-%d]\n", i, head->proto, head->ip, head->begin_port, head->end_port);
		kfree(head);
	}

final_return:
	return;
}

static int proc_read_del_ip_conntrack(char *page, char **start, off_t off, int count, int *eof, void *context)
{
        char *p;
 
        p = page;
        p += sprintf(page, "%s\n", "use echo \"1(0)\" to enable or disbable");
        return end_proc_read(p, page, off, count, start, eof);
}
 
static int proc_write_del_ip_conntrack(struct file *file, const char *buffer, unsigned long count, void *data)
{
        unsigned char tmp[2];
 
        if(buffer)
        {
                memset(tmp, 0, sizeof(tmp));
                copy_from_user(tmp, buffer, count);
                tmp[1] = 0x00;
                switch(*tmp)
                {
                        case '0':
                                //Do something here
                                break;
 
                        case '1':
                                pf_del_ip_conntrack();
                                break;
 
                        default:
                                printk("<1>invalid args\n");
                }
                return count;
        }
        return 0;
}

static int end_proc_read(const char *p, char *page, off_t off, int count, char **start, int *eof)
{
	int len = p - page;
	
	if(len < off + count)
	{
		*eof = 1;
	}
	
	*start = page + off;
	len -= off;
	if(len > count)
	{
		len = count;
	}
	
	if(len < 0)
	{
		len = 0;
	}
	
	return len;
}

#endif

#ifdef CLEAR_IP_CONNTRACK
void clear_ip_conntrack(void)
{
	int i;
	struct list_head *head, *temp_head;
	struct ip_conntrack_tuple_hash *tuple_hash;

	printk("warning : %s %d\n", __func__, __LINE__);
	
	READ_LOCK(&ip_conntrack_lock);
	for (i = 0; i < ip_conntrack_htable_size; i++) 
	{
		head = &ip_conntrack_hash[i];
		temp_head = head;
		while(1) 
		{	
			temp_head = temp_head->next;				
			if(temp_head == head) 
			{			
				head = NULL;			
				temp_head = NULL;
				break;			
			}
			tuple_hash = (struct ip_conntrack_tuple_hash *)temp_head;
			del_selected_conntrack(tuple_hash);
		}					
	}
	READ_UNLOCK(&ip_conntrack_lock);
}

static int proc_read_clear_ip_conntrack(char *page, char **start, off_t off, int count, int *eof, void *context)
{
        char *p;
 
        p = page;
        p += sprintf(page, "%s\n", "use echo \"1(0)\" to enable or disbable");
        return end_proc_read(p, page, off, count, start, eof);
}
 
static int proc_write_clear_ip_conntrack(struct file *file, const char *buffer, unsigned long count, void *data)
{
        unsigned char tmp[2];
 
        if(buffer)
        {
                memset(tmp, 0, sizeof(tmp));
                copy_from_user(tmp, buffer, count);
                tmp[1] = 0x00;
                switch(*tmp)
                {
                        case '0':
                                //Do something here
                                break;
 
                        case '1':
				    clear_ip_conntrack();
                                break;
 
                        default:
                                printk("<1>invalid args\n");
                }
                return count;
        }
        return 0;
}
#endif

static int kill_proto(const struct ip_conntrack *i, void *data)
{
	return (i->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum == 
			*((u_int8_t *) data));
}

static unsigned int
print_tuple(char *buffer, const struct ip_conntrack_tuple *tuple,
	    struct ip_conntrack_protocol *proto)
{
	int len;

	len = sprintf(buffer, "src=%u.%u.%u.%u dst=%u.%u.%u.%u ",
		      NIPQUAD(tuple->src.ip), NIPQUAD(tuple->dst.ip));

	len += proto->print_tuple(buffer + len, tuple);

	return len;
}

static unsigned int
print_expect(char *buffer, const struct ip_conntrack_expect *expect)
{
	unsigned int len;

	if (!expect  || !expect->expectant || !expect->expectant->helper) {
		DEBUGP("expect  %x expect->expectant %x expect->expectant->helper %x\n", 
			expect, expect->expectant, expect->expectant->helper);
		return 0;
	}

	if (expect->expectant->helper->timeout)
		len = sprintf(buffer, "EXPECTING: %lu ",
			      timer_pending(&expect->timeout)
			      ? (expect->timeout.expires - jiffies)/HZ : 0);
	else
		len = sprintf(buffer, "EXPECTING: - ");
	len += sprintf(buffer + len, "use=%u proto=%u ",
		      atomic_read(&expect->use), expect->tuple.dst.protonum);
	len += print_tuple(buffer + len, &expect->tuple,
			   __ip_ct_find_proto(expect->tuple.dst.protonum));
	len += sprintf(buffer + len, "\n");
	return len;
}

static unsigned int
print_conntrack(char *buffer, const struct ip_conntrack *conntrack)
{
	unsigned int len;
	struct ip_conntrack_protocol *proto
		= __ip_ct_find_proto(conntrack->tuplehash[IP_CT_DIR_ORIGINAL]
			       .tuple.dst.protonum);

	len = sprintf(buffer, "%-8s %u %lu ",
		      proto->name,
		      conntrack->tuplehash[IP_CT_DIR_ORIGINAL]
		      .tuple.dst.protonum,
		      timer_pending(&conntrack->timeout)
		      ? (conntrack->timeout.expires - jiffies)/HZ : 0);

	len += proto->print_conntrack(buffer + len, conntrack);
	len += print_tuple(buffer + len,
			   &conntrack->tuplehash[IP_CT_DIR_ORIGINAL].tuple,
			   proto);
	if (!(conntrack->status & IPS_SEEN_REPLY))
		len += sprintf(buffer + len, "[UNREPLIED] ");
	len += print_tuple(buffer + len,
			   &conntrack->tuplehash[IP_CT_DIR_REPLY].tuple,
			   proto);
	if (conntrack->status & IPS_ASSURED)
		len += sprintf(buffer + len, "[ASSURED] ");
	len += sprintf(buffer + len, "use=%u ",
		       atomic_read(&conntrack->ct_general.use));
	len += sprintf(buffer + len, "\n");

	return len;
}

/* Returns true when finished. */
static inline int
conntrack_iterate(const struct ip_conntrack_tuple_hash *hash,
		  char *buffer, off_t offset, off_t *upto,
		  unsigned int *len, unsigned int maxlen)
{
	unsigned int newlen;
	IP_NF_ASSERT(hash->ctrack);

	MUST_BE_READ_LOCKED(&ip_conntrack_lock);

	/* Only count originals */
	if (DIRECTION(hash))
		return 0;

	if ((*upto)++ < offset)
		return 0;

	newlen = print_conntrack(buffer + *len, hash->ctrack);
	if (*len + newlen > maxlen)
		return 1;
	else *len += newlen;

	return 0;
}

static int
list_conntracks(char *buffer, char **start, off_t offset, int length)
{
	unsigned int i;
	unsigned int len = 0;
	off_t upto = 0;
	struct list_head *e;

	READ_LOCK(&ip_conntrack_lock);
	/* Traverse hash; print originals then reply. */
	for (i = 0; i < ip_conntrack_htable_size; i++) {
		if (LIST_FIND(&ip_conntrack_hash[i], conntrack_iterate,
			      struct ip_conntrack_tuple_hash *,
			      buffer, offset, &upto, &len, length))
			goto finished;
	}

	/* Now iterate through expecteds. */
	for (e = ip_conntrack_expect_list.next; 
	     e != &ip_conntrack_expect_list; e = e->next) {
		unsigned int last_len;
		struct ip_conntrack_expect *expect
			= (struct ip_conntrack_expect *)e;
		if (upto++ < offset) continue;

		last_len = len;
		len += print_expect(buffer + len, expect);
		if (len > length) {
			len = last_len;
			goto finished;
		}
	}

 finished:
	READ_UNLOCK(&ip_conntrack_lock);

	/* `start' hack - see fs/proc/generic.c line ~165 */
	*start = (char *)((unsigned int)upto - offset);
	return len;
}

static unsigned int ip_confirm(unsigned int hooknum,
			       struct sk_buff **pskb,
			       const struct net_device *in,
			       const struct net_device *out,
			       int (*okfn)(struct sk_buff *))
{
	/* We've seen it coming out the other side: confirm it */
	return ip_conntrack_confirm(*pskb);
}

static unsigned int ip_refrag(unsigned int hooknum,
			      struct sk_buff **pskb,
			      const struct net_device *in,
			      const struct net_device *out,
			      int (*okfn)(struct sk_buff *))
{
	struct rtable *rt = (struct rtable *)(*pskb)->dst;

	/* We've seen it coming out the other side: confirm */
	if (ip_confirm(hooknum, pskb, in, out, okfn) != NF_ACCEPT)
		return NF_DROP;

	/* Local packets are never produced too large for their
	   interface.  We degfragment them at LOCAL_OUT, however,
	   so we have to refragment them here. */
	if ((*pskb)->len > rt->u.dst.pmtu) {
		/* No hook can be after us, so this should be OK. */
		ip_fragment(*pskb, okfn);
		return NF_STOLEN;
	}
	return NF_ACCEPT;
}

static unsigned int ip_conntrack_local(unsigned int hooknum,
				       struct sk_buff **pskb,
				       const struct net_device *in,
				       const struct net_device *out,
				       int (*okfn)(struct sk_buff *))
{
	/* root is playing with raw sockets. */
	if ((*pskb)->len < sizeof(struct iphdr)
	    || (*pskb)->nh.iph->ihl * 4 < sizeof(struct iphdr)) {
		if (net_ratelimit())
			printk("ipt_hook: happy cracking.\n");
		return NF_ACCEPT;
	}
	return ip_conntrack_in(hooknum, pskb, in, out, okfn);
}

/* Connection tracking may drop packets, but never alters them, so
   make it the first hook. */
static struct nf_hook_ops ip_conntrack_in_ops
= { { NULL, NULL }, ip_conntrack_in, PF_INET, NF_IP_PRE_ROUTING,
	NF_IP_PRI_CONNTRACK };
static struct nf_hook_ops ip_conntrack_local_out_ops
= { { NULL, NULL }, ip_conntrack_local, PF_INET, NF_IP_LOCAL_OUT,
	NF_IP_PRI_CONNTRACK };
/* Refragmenter; last chance. */
static struct nf_hook_ops ip_conntrack_out_ops
= { { NULL, NULL }, ip_refrag, PF_INET, NF_IP_POST_ROUTING, NF_IP_PRI_LAST };
static struct nf_hook_ops ip_conntrack_local_in_ops
= { { NULL, NULL }, ip_confirm, PF_INET, NF_IP_LOCAL_IN, NF_IP_PRI_LAST-1 };

static int init_or_cleanup(int init)
{
	struct proc_dir_entry *proc;
	int ret = 0;

	if (!init) goto cleanup;

	ret = ip_conntrack_init();
	if (ret < 0)
		goto cleanup_nothing;

#ifdef DEL_IP_CONNTRACK_ENTRY
	proc = proc_net_create("del_ip_conntrack", S_IFREG | S_IRUGO | S_IWUSR, proc_read_del_ip_conntrack);
	if(proc)
	{
		proc->write_proc = proc_write_del_ip_conntrack;
		proc->owner = THIS_MODULE;
	}
	else
	{
		//Maybe we can just let it go!
	}
#endif
#ifdef CLEAR_IP_CONNTRACK
	proc = proc_net_create("clear_ip_conntrack", S_IFREG | S_IRUGO | S_IWUSR, proc_read_clear_ip_conntrack);
	if(proc)
	{
		proc->write_proc = proc_write_clear_ip_conntrack;
		proc->owner = THIS_MODULE;
	}
	else
	{
		//Maybe we can just let it go!
	}
#endif
	proc = proc_net_create("ip_conntrack",0,list_conntracks);
	if (!proc) goto cleanup_init;
	proc->owner = THIS_MODULE;

	ret = nf_register_hook(&ip_conntrack_in_ops);
	if (ret < 0) {
		printk("ip_conntrack: can't register pre-routing hook.\n");
		goto cleanup_proc;
	}
	ret = nf_register_hook(&ip_conntrack_local_out_ops);
	if (ret < 0) {
		printk("ip_conntrack: can't register local out hook.\n");
		goto cleanup_inops;
	}
	ret = nf_register_hook(&ip_conntrack_out_ops);
	if (ret < 0) {
		printk("ip_conntrack: can't register post-routing hook.\n");
		goto cleanup_inandlocalops;
	}
	ret = nf_register_hook(&ip_conntrack_local_in_ops);
	if (ret < 0) {
		printk("ip_conntrack: can't register local in hook.\n");
		goto cleanup_inoutandlocalops;
	}

	return ret;

 cleanup:
	nf_unregister_hook(&ip_conntrack_local_in_ops);
 cleanup_inoutandlocalops:
	nf_unregister_hook(&ip_conntrack_out_ops);
 cleanup_inandlocalops:
	nf_unregister_hook(&ip_conntrack_local_out_ops);
 cleanup_inops:
	nf_unregister_hook(&ip_conntrack_in_ops);
 cleanup_proc:
	proc_net_remove("ip_conntrack");
 cleanup_init:
	ip_conntrack_cleanup();
 cleanup_nothing:
	return ret;
}

int ip_conntrack_protocol_register(struct ip_conntrack_protocol *proto)
{
	int ret = 0;
	struct list_head *i;

	WRITE_LOCK(&ip_conntrack_lock);
	for (i = protocol_list.next; i != &protocol_list; i = i->next) {
		if (((struct ip_conntrack_protocol *)i)->proto
		    == proto->proto) {
			ret = -EBUSY;
			goto out;
		}
	}

	list_prepend(&protocol_list, proto);
	MOD_INC_USE_COUNT;

 out:
	WRITE_UNLOCK(&ip_conntrack_lock);
	return ret;
}

void ip_conntrack_protocol_unregister(struct ip_conntrack_protocol *proto)
{
	WRITE_LOCK(&ip_conntrack_lock);

	/* ip_ct_find_proto() returns proto_generic in case there is no protocol 
	 * helper. So this should be enough - HW */
	LIST_DELETE(&protocol_list, proto);
	WRITE_UNLOCK(&ip_conntrack_lock);
	
	/* Somebody could be still looking at the proto in bh. */
	br_write_lock_bh(BR_NETPROTO_LOCK);
	br_write_unlock_bh(BR_NETPROTO_LOCK);

	/* Remove all contrack entries for this protocol */
	ip_ct_selective_cleanup(kill_proto, &proto->proto);

	MOD_DEC_USE_COUNT;
}

static int __init init(void)
{
	return init_or_cleanup(1);
}

static void __exit fini(void)
{
	init_or_cleanup(0);
}

module_init(init);
module_exit(fini);

EXPORT_SYMBOL(ip_conntrack_protocol_register);
EXPORT_SYMBOL(ip_conntrack_protocol_unregister);
EXPORT_SYMBOL(invert_tuplepr);
EXPORT_SYMBOL(ip_conntrack_alter_reply);
EXPORT_SYMBOL(ip_conntrack_destroyed);
EXPORT_SYMBOL(ip_conntrack_get);
EXPORT_SYMBOL(ip_conntrack_module);
EXPORT_SYMBOL(ip_conntrack_helper_register);
EXPORT_SYMBOL(ip_conntrack_helper_unregister);
EXPORT_SYMBOL(ip_ct_selective_cleanup);
EXPORT_SYMBOL(ip_ct_refresh);
EXPORT_SYMBOL(ip_ct_find_proto);
EXPORT_SYMBOL(__ip_ct_find_proto);
EXPORT_SYMBOL(ip_ct_find_helper);
EXPORT_SYMBOL(sysctl_ip_conntrack_tcp_timeouts);
EXPORT_SYMBOL(sysctl_ip_conntrack_udp_timeouts);
EXPORT_SYMBOL(ip_conntrack_expect_related);
EXPORT_SYMBOL(ip_conntrack_change_expect);
EXPORT_SYMBOL(ip_conntrack_unexpect_related);
EXPORT_SYMBOL_GPL(ip_conntrack_expect_find_get);
EXPORT_SYMBOL_GPL(ip_conntrack_expect_put);
EXPORT_SYMBOL(ip_conntrack_tuple_taken);
EXPORT_SYMBOL(ip_ct_gather_frags);
EXPORT_SYMBOL(ip_conntrack_htable_size);
EXPORT_SYMBOL(ip_conntrack_expect_list);
EXPORT_SYMBOL(ip_conntrack_lock);
EXPORT_SYMBOL(ip_conntrack_hash);
EXPORT_SYMBOL_GPL(ip_conntrack_find_get);
EXPORT_SYMBOL_GPL(ip_conntrack_put);
