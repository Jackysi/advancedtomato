/* 
 * AMD K8 NUMA support.
 * Discover the memory map and associated nodes.
 * 
 * Doesn't use the ACPI SRAT table because it has a questionable license.
 * Instead the northbridge registers are read directly. 
 * 
 * Copyright 2002 Andi Kleen, SuSE Labs.
 * $Id: k8topology.c,v 1.1.1.4 2003/10/14 08:07:52 sparq Exp $
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/pci_ids.h>
#include <asm/types.h>
#include <asm/mmzone.h>
#include <asm/proto.h>
#include <asm/e820.h>
#include <asm/pci-direct.h>

int memnode_shift;
u8  memnodemap[NODEMAPSIZE];

static int find_northbridge(void)
{
	int num; 

	for (num = 0; num < 32; num++) { 
		u32 header;
		
		header = read_pci_config(0, num, 0, 0x00);  
		if (header != (PCI_VENDOR_ID_AMD | (0x1100<<16)))
			continue; 	

		header = read_pci_config(0, num, 1, 0x00); 
		if (header != (PCI_VENDOR_ID_AMD | (0x1101<<16)))
			continue;	
		return num; 
	} 

	return -1; 	
}

#define MAXNODE 8 
#define NODEMASK 0xff

struct node { 
	u64 start,end; 
};

#define for_all_nodes(n) \
	for (n=0; n<MAXNODE;n++) if (nodes[n].start!=nodes[n].end)

static int compute_hash_shift(struct node *nodes, int numnodes, u64 maxmem)
{
	int i; 
	int shift = 24;
	u64 addr;
	
	/* When in doubt use brute force. */
	while (shift < 48) { 
		memset(memnodemap,0xff,sizeof(*memnodemap) * NODEMAPSIZE); 
		for_all_nodes (i) { 
			for (addr = nodes[i].start; 
			     addr < nodes[i].end; 
			     addr += (1UL << shift)) {
				if (memnodemap[addr >> shift] != 0xff) { 
					printk("node %d shift %d addr %Lx conflict %d\n", 
					       i, shift, addr, memnodemap[addr>>shift]);
					goto next; 
				} 
				memnodemap[addr >> shift] = i; 
			} 
		} 
		return shift; 
	next:
		shift++; 
	} 
	memset(memnodemap,0,sizeof(*memnodemap) * NODEMAPSIZE); 
	return -1; 
}

extern unsigned long nodes_present;
extern unsigned long end_pfn;

int __init k8_scan_nodes(unsigned long start, unsigned long end)
{ 
	unsigned long prevbase;
	struct node nodes[MAXNODE];
	int nodeid, numnodes, maxnode, i, nb; 

	nb = find_northbridge(); 
	if (nb < 0) 
		return nb;

	printk(KERN_INFO "Scanning NUMA topology in Northbridge %d\n", nb); 

	numnodes = (read_pci_config(0, nb, 0, 0x60 ) >> 4) & 3; 

	memset(&nodes,0,sizeof(nodes)); 
	prevbase = 0;
	maxnode = -1; 
	for (i = 0; i < MAXNODE; i++) { 
		unsigned long base,limit; 

		base = read_pci_config(0, nb, 1, 0x40 + i*8);
		limit = read_pci_config(0, nb, 1, 0x44 + i*8);

		nodeid = limit & 3; 
		if (!limit) { 
			printk(KERN_INFO "Skipping node entry %d (base %lx)\n", i,			       base);
			continue;
		}
		if ((base >> 8) & 3 || (limit >> 8) & 3) {
			printk(KERN_ERR "Node %d using interleaving mode %lx/%lx\n", 
			       nodeid, (base>>8)&3, (limit>>8) & 3); 
			return -1; 
		}	
		if (nodeid > maxnode) 
			maxnode = nodeid; 
		if ((1UL << nodeid) & nodes_present) { 
			printk("Node %d already present. Skipping\n", nodeid);
			continue;
		}

		limit >>= 16; 
		limit <<= 24; 

		if (limit > end_pfn << PAGE_SHIFT) 
			limit = end_pfn << PAGE_SHIFT; 
		if (limit <= base) { 
			printk(KERN_INFO "Node %d beyond memory map\n", nodeid);
			continue; 
		} 
			
		base >>= 16;
		base <<= 24; 

		if (base < start) 
			base = start; 
		if (limit > end) 
			limit = end; 
		if (limit == base) 
			continue; 
		if (limit < base) { 
			printk(KERN_INFO"Node %d bogus settings %lx-%lx. Ignored.\n",
			       nodeid, base, limit); 			       
			continue; 
		} 
		
		/* Could sort here, but pun for now. Should not happen anyroads. */
		if (prevbase > base) { 
			printk(KERN_INFO "Node map not sorted %lx,%lx\n",
			       prevbase,base);
			return -1;
		}
			
		printk(KERN_INFO "Node %d MemBase %016lx Limit %016lx\n", 
		       nodeid, base, limit); 
		
		nodes[nodeid].start = base; 
		nodes[nodeid].end = limit;

		prevbase = base;
	} 

	if (maxnode <= 0)
		return -1; 

	memnode_shift = compute_hash_shift(nodes,maxnode,end);
	if (memnode_shift < 0) { 
		printk(KERN_ERR "No NUMA node hash function found. Contact maintainer\n"); 
		return -1; 
	} 
	printk(KERN_INFO "Using node hash shift of %d\n", memnode_shift); 

	for_all_nodes(i) { 
		setup_node_bootmem(i, nodes[i].start, nodes[i].end); 
	} 

	return 0;
} 

