/* netfilter's kernel module for the geoip match
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Copyright (c) 2004 Cookinglinux
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#include <linux/netfilter_ipv4/ipt_geoip.h>
#include <linux/netfilter_ipv4/ip_tables.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samuel Jean, Nicolas Bouliane");
MODULE_DESCRIPTION("iptables/netfilter's geoip match");

struct geoip_info *head = NULL;
static spinlock_t geoip_lock = SPIN_LOCK_UNLOCKED;

static struct geoip_info *add_node(struct geoip_info *memcpy)
{
   struct geoip_info *p =
      (struct geoip_info *)kmalloc(sizeof(struct geoip_info), GFP_KERNEL);

   struct geoip_subnet *s;

   if ((p == NULL) || (copy_from_user(p, memcpy, sizeof(struct geoip_info)) != 0))
      return NULL;

   s = (struct geoip_subnet *)kmalloc(p->count * sizeof(struct geoip_subnet), GFP_KERNEL);
   if ((s == NULL) || (copy_from_user(s, p->subnets, p->count * sizeof(struct geoip_subnet)) != 0))
      return NULL;
  
   spin_lock_bh(&geoip_lock);

   p->subnets = s;
   p->ref = 1;
   p->next = head;
   p->prev = NULL;
   if (p->next) p->next->prev = p;
   head = p;

   spin_unlock_bh(&geoip_lock);
   return p;
}

static void remove_node(struct geoip_info *p)
 {
   spin_lock_bh(&geoip_lock);
   
   if (p->next) { /* Am I following a node ? */
      p->next->prev = p->prev;
      if (p->prev) p->prev->next = p->next; /* Is there a node behind me ? */
      else head = p->next; /* No? Then I was the head */
   }
   
   else 
      if (p->prev) /* Is there a node behind me ? */
         p->prev->next = NULL;
      else
         head = NULL; /* No, we're alone */

   /* So now am unlinked or the only one alive, right ?
    * What are you waiting ? Free up some memory!
    */

   kfree(p->subnets);
   kfree(p);
   
   spin_unlock_bh(&geoip_lock);   
   return;
}

static struct geoip_info *find_node(u_int16_t cc)
{
   struct geoip_info *p = head;
   spin_lock_bh(&geoip_lock);
   
   while (p) {
      if (p->cc == cc) {
         spin_unlock_bh(&geoip_lock);         
         return p;
      }
      p = p->next;
   }
   spin_unlock_bh(&geoip_lock);
   return NULL;
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
   const struct ipt_geoip_info *info = matchinfo;
   const struct geoip_info *node; /* This keeps the code sexy */
   const struct iphdr *iph = skb->nh.iph;
   u_int32_t ip, j;
   u_int8_t i;

   if (info->flags & IPT_GEOIP_SRC)
      ip = ntohl(iph->saddr);
   else
      ip = ntohl(iph->daddr);

   spin_lock_bh(&geoip_lock);
   for (i = 0; i < info->count; i++) {
      if ((node = info->mem[i]) == NULL) {
         printk(KERN_ERR "ipt_geoip: what the hell ?? '%c%c' isn't loaded into memory... skip it!\n",
               COUNTRY(info->cc[i]));
         
         continue;
      }

      for (j = 0; j < node->count; j++)
         if ((ip > node->subnets[j].begin) && (ip < node->subnets[j].end)) {
            spin_unlock_bh(&geoip_lock);
            return (info->flags & IPT_GEOIP_INV) ? 0 : 1;
         }
   }
   
   spin_unlock_bh(&geoip_lock);
   return (info->flags & IPT_GEOIP_INV) ? 1 : 0;
}

static int geoip_checkentry(const char *tablename,
                             const struct ipt_ip *ip,
                             void *matchinfo,
                             unsigned int matchsize,
                             unsigned int hook_mask)
{
   struct ipt_geoip_info *info = matchinfo;
   struct geoip_info *node;
   u_int8_t i;
   
   /* FIXME:   Call a function to free userspace allocated memory.
    *          As Martin J. said; this match might eat lot of memory
    *          if commited with iptables-restore --noflush
   void (*gfree)(struct geoip_info *oldmem);
   gfree = info->fini;
   */

   if (matchsize != IPT_ALIGN(sizeof(struct ipt_geoip_info))) {
      printk(KERN_ERR "ipt_geoip: matchsize differ, you may have forgotten to recompile me\n");
      return 0;
   }

   /* If info->refcount isn't NULL, then
    * it means that checkentry() already
    * initialized this entry. Increase a
    * refcount to prevent destroy() of
    * this entry. */
   if (info->refcount != NULL) {
      atomic_inc((atomic_t *)info->refcount);
      return 1;
   }
   
  
   for (i = 0; i < info->count; i++) {
     
      if ((node = find_node(info->cc[i])) != NULL)
            atomic_inc((atomic_t *)&node->ref);   //increase the reference
      else
         if ((node = add_node(info->mem[i])) == NULL) {
            printk(KERN_ERR
                  "ipt_geoip: unable to load '%c%c' into memory\n",
                  COUNTRY(info->cc[i]));
            return 0;
         }

      /* Free userspace allocated memory for that country.
       * FIXME:   It's a bit odd to call this function everytime
       *          we process a country.  Would be nice to call
       *          it once after all countries've been processed.
       *          - SJ
       * *not implemented for now*
      gfree(info->mem[i]);
      */

      /* Overwrite the now-useless pointer info->mem[i] with
       * a pointer to the node's kernelspace structure.
       * This avoids searching for a node in the match() and
       * destroy() functions.
       */
      info->mem[i] = node;
   }

   /* We allocate some memory and give info->refcount a pointer
    * to this memory.  This prevents checkentry() from increasing a refcount
    * different from the one used by destroy().
    * For explanation, see http://www.mail-archive.com/netfilter-devel@lists.samba.org/msg00625.html
    */
   info->refcount = kmalloc(sizeof(u_int8_t), GFP_KERNEL);
   if (info->refcount == NULL) {
      printk(KERN_ERR "ipt_geoip: failed to allocate `refcount' memory\n");
      return 0;
   }
   *(info->refcount) = 1;
   
   return 1;
}

static void geoip_destroy(void *matchinfo, unsigned int matchsize)
{
   struct ipt_geoip_info *info = matchinfo;
   struct geoip_info *node; /* this keeps the code sexy */
   u_int8_t i;
   
   /* Decrease the previously increased refcount in checkentry()
    * If it's equal to 1, we know this entry is just moving
    * but not removed. We simply return to avoid useless destroy()
    * processing.
    */
   atomic_dec((atomic_t *)info->refcount);
   if (*info->refcount)
      return;

   /* Don't leak my memory, you idiot.
    * Bug found with nfsim.. the netfilter's best
    * friend. --peejix */
   kfree(info->refcount);
   
   /* This entry has been removed from the table so
    * decrease the refcount of all countries it is
    * using.
    */
  
   for (i = 0; i < info->count; i++)
      if ((node = info->mem[i]) != NULL) {
         atomic_dec((atomic_t *)&node->ref);

         /* Free up some memory if that node isn't used
          * anymore. */
         if (node->ref < 1)
            remove_node(node);
      }
      else
         /* Something strange happened. There's no memory allocated for this
          * country.  Please send this bug to the mailing list. */
         printk(KERN_ERR
               "ipt_geoip: What happened peejix ? What happened acidmen ?\n"
               "ipt_geoip: please report this bug to the maintainers\n");
   return;
}

static struct ipt_match geoip_match
= { { NULL, NULL }, "geoip", &match, &geoip_checkentry, &geoip_destroy, THIS_MODULE };

static int __init init(void)
{
   return ipt_register_match(&geoip_match);
}

static void __exit fini(void)
{
  ipt_unregister_match(&geoip_match);
  return;
}

module_init(init);
module_exit(fini);
