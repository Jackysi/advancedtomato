/*
 *  acpitable.c - x86-64-specific ACPI (1.0 & 2.0) boot-time initialization
 *
 *  Copyright (C) 1999 Andrew Henroid
 *  Copyright (C) 2001 Richard Schaal
 *  Copyright (C) 2001 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *  Copyright (C) 2001 Jun Nakajima <jun.nakajima@intel.com>
 *  Copyright (C) 2001 Arjan van de Ven <arjanv@redhat.com>
 *  Copyright (C) 2002 Vojtech Pavlik <vojtech@suse.cz>
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <asm/mpspec.h>
#include <asm/io.h>
#include <asm/apic.h>
#include <asm/apicdef.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/fixmap.h>

#include "acpitable.h"

static acpi_table_handler acpi_boot_ops[ACPI_TABLE_COUNT];

extern unsigned long end_pfn;
static inline int bad_ptr(void *p) 
{ 
	if ((unsigned long)p  >> PAGE_SHIFT >= end_pfn) 
		return 1; 
	return 0; 	
} 

/*
 * Checksum an ACPI table.
 */

static unsigned char __init acpi_checksum(void *buffer, int length)
{
	unsigned char sum = 0;
	while (length--)
		sum += *(unsigned char *)buffer++;
	return sum;
}

/*
 * Print an ACPI table header for debugging.
 */

static char __init *acpi_kill_spaces(char *t, char *s, int m)
{
	int l = strnlen(s, m);
	strncpy(t, s, m);
	t[l] = 0;
	while (l > 0 && (t[l - 1] == ' ' || t[l - 1] == '\t')) t[--l] = 0;
	while (t[0] == ' ' || t[0] == '\t') t++;
	return t;
}

static void __init acpi_print_table_header(struct acpi_table_header * header)
{
	char oem[7], id[9];

	printk(KERN_INFO "acpi: %.4s rev: %d oem: %s id: %s build: %d.%d\n",
		header->signature, header->revision, acpi_kill_spaces(oem, header->oem_id, 6),
		acpi_kill_spaces(id, header->oem_table_id, 8), header->oem_revision >> 16,
		header->oem_revision & 0xffff);
}

/*
 * Search a block of memory for the RSDP signature
 */

static void* __init acpi_tb_scan_memory_for_rsdp(void *address, int length)
{
	u32 offset = 0;

	while (offset < length) {
		if (strncmp(address, "RSD PTR ", 8) == 0 &&
		    acpi_checksum(address, RSDP_CHECKSUM_LENGTH) == 0) {
			printk(KERN_INFO "acpi: RSDP found at address %p\n", address);
			return address;
		}
		offset += RSDP_SCAN_STEP;
		address += RSDP_SCAN_STEP;
	}
	return NULL;
}

/*
 * Search lower 1_mbyte of memory for the root system descriptor
 * pointer structure. If it is found, set *RSDP to point to it.
 */

static struct acpi_table_rsdp* __init acpi_find_root_pointer(void)
{
	struct acpi_table_rsdp *rsdp;

	if ((rsdp = acpi_tb_scan_memory_for_rsdp(__va(LO_RSDP_WINDOW_BASE), LO_RSDP_WINDOW_SIZE)))
		return rsdp;

	if ((rsdp = acpi_tb_scan_memory_for_rsdp(__va(HI_RSDP_WINDOW_BASE), HI_RSDP_WINDOW_SIZE)))
		return rsdp;

	return NULL;
}

static int __init acpi_process_table(u64 table)
{
	struct acpi_table_header *header;
	int type;

	header = __va(table);

	acpi_print_table_header(header);

	if (acpi_checksum(header, header->length)) {
		printk(KERN_WARNING "acpi: ACPI table at %#lx has invalid checksum.\n", table);
		return -1;
	}

	for (type = 0; type < ACPI_TABLE_COUNT; type++)
		if (!strncmp(header->signature, acpi_table_signatures[type], 4))
			break;

	if (type == ACPI_TABLE_COUNT || !acpi_boot_ops[type])
		return 0;

	return acpi_boot_ops[type](header, table);
}

static int __init acpi_tables_init(void)
{
	struct acpi_table_rsdp *rsdp;
	struct acpi_table_xsdt *xsdt = NULL;
	struct acpi_table_rsdt *rsdt = NULL;
	char oem[7];
	int i;

	if (!(rsdp = acpi_find_root_pointer())) {
		printk(KERN_ERR "acpi: Couldn't find ACPI root pointer!\n");
		return -1;
	}

	printk(KERN_INFO "acpi: RSDP rev: %d oem: %s\n",
		rsdp->revision, acpi_kill_spaces(oem, rsdp->oem_id, 6));

	if (!acpi_checksum(rsdp, RSDP2_CHECKSUM_LENGTH)
		&& rsdp->length >= RSDP2_CHECKSUM_LENGTH) {	/* ACPI 2.0 might be present */
		xsdt = __va(rsdp->xsdt_address);
		if (bad_ptr(xsdt))
			return -1; 
		if (!strncmp(xsdt->header.signature, "XSDT", 4)) {
			acpi_print_table_header(&xsdt->header);
			for (i = 0; i < (xsdt->header.length - sizeof(struct acpi_table_header)) / sizeof(u64); i++)
				if (acpi_process_table(xsdt->entry[i]))
					return -1;
			return 0;
		}
	}

	rsdt = __va(rsdp->rsdt_address);
	if (bad_ptr(rsdt))
		return -1;
	if (!strncmp(rsdt->header.signature, "RSDT", 4)) {
		acpi_print_table_header(&rsdt->header);
		for (i = 0; i < (rsdt->header.length - sizeof(struct acpi_table_header)) / sizeof(u32); i++)
			if (acpi_process_table(rsdt->entry[i]))
				return -1;
		return 0;
	}

	printk(KERN_WARNING "acpi: No ACPI table directory found.\n");
	return -1;
}

static void __init acpi_parse_lapic(struct acpi_table_lapic *local_apic)
{
	printk(KERN_INFO "acpi: LAPIC acpi_id: %d id: %d enabled: %d\n",
		local_apic->acpi_id, local_apic->id, local_apic->flags.enabled);
}

static void __init acpi_parse_ioapic(struct acpi_table_ioapic *ioapic)
{
	printk(KERN_INFO "acpi: IOAPIC id: %d address: %#x global_irq_base: %#x\n",
		ioapic->id, ioapic->address, ioapic->global_irq_base);
}

static void __init acpi_parse_int_src_ovr(struct acpi_table_int_src_ovr *intsrc)
{
	printk(KERN_INFO "acpi: INT_SRC_OVR bus: %d irq: %d global_irq: %d polarity: %d trigger: %d\n",
		intsrc->bus, intsrc->bus_irq, intsrc->global_irq, intsrc->flags.polarity, intsrc->flags.trigger);
}

static void __init acpi_parse_nmi_src(struct acpi_table_nmi_src *nmisrc)
{
	printk(KERN_INFO "acpi: NMI_SRC polarity: %d trigger: %d global_irq: %d\n",
		nmisrc->flags.polarity, nmisrc->flags.trigger, nmisrc->global_irq);
}

static void __init acpi_parse_lapic_nmi(struct acpi_table_lapic_nmi *localnmi)
{
	printk(KERN_INFO "acpi: LAPIC_NMI acpi_id: %d polarity: %d trigger: %d lint: %d\n",
		localnmi->acpi_id, localnmi->flags.polarity, localnmi->flags.trigger, localnmi->lint);
}

static void __init acpi_parse_lapic_addr_ovr(struct acpi_table_lapic_addr_ovr *lapic_addr_ovr)
{
	printk(KERN_INFO "acpi: LAPIC_ADDR_OVR address: %#lx\n",
		(unsigned long) lapic_addr_ovr->address);
}

static void __init acpi_parse_plat_int_src(struct acpi_table_plat_int_src *plintsrc)
{
	printk(KERN_INFO "acpi: PLAT_INT_SRC polarity: %d trigger: %d type: %d id: %d eid: %d iosapic_vector: %#x global_irq: %d\n",
		plintsrc->flags.polarity, plintsrc->flags.trigger, plintsrc->type, plintsrc->id, plintsrc->eid,
		plintsrc->iosapic_vector, plintsrc->global_irq);
}

static int __init acpi_parse_madt(struct acpi_table_header *header, unsigned long phys)
{

	struct acpi_table_madt *madt;
	struct acpi_madt_entry_header *entry_header;
	int table_size;

	madt = __va(phys);
	table_size = header->length - sizeof(*madt);
	entry_header = (void *)madt + sizeof(*madt);

	while (entry_header && table_size > 0) {

		switch (entry_header->type) {
			case ACPI_MADT_LAPIC:
				acpi_parse_lapic((void *) entry_header);
				break;
			case ACPI_MADT_IOAPIC:
				acpi_parse_ioapic((void *) entry_header);
				break;
			case ACPI_MADT_INT_SRC_OVR:
				acpi_parse_int_src_ovr((void *) entry_header);
				break;
			case ACPI_MADT_NMI_SRC:
				acpi_parse_nmi_src((void *) entry_header);
				break;
			case ACPI_MADT_LAPIC_NMI:
				acpi_parse_lapic_nmi((void *) entry_header);
				break;
			case ACPI_MADT_LAPIC_ADDR_OVR:
				acpi_parse_lapic_addr_ovr((void *) entry_header);
				break;
			case ACPI_MADT_PLAT_INT_SRC:
				acpi_parse_plat_int_src((void *) entry_header);
				break;
			default:
				printk(KERN_WARNING "acpi: Unsupported MADT entry type 0x%x\n", entry_header->type);
				break;
		}

		table_size -= entry_header->length;
		entry_header = (void *) entry_header + entry_header->length;
	}

	printk(KERN_INFO "acpi: Local APIC address %#x\n", madt->lapic_address);

	return 0;
}

static int __init acpi_parse_hpet(struct acpi_table_header *header, unsigned long phys)
{
	struct acpi_table_hpet *hpet_tbl;

	hpet_tbl = __va(phys);

	if (hpet_tbl->addr.space_id != ACPI_SPACE_MEM) {
		printk(KERN_WARNING "acpi: HPET timers must be located in memory.\n");
		return -1;
	}

	hpet.address = hpet_tbl->addr.addrl | ((long) hpet_tbl->addr.addrh << 32);

	printk(KERN_INFO "acpi: HPET id: %#x base: %#lx\n", hpet_tbl->id, hpet.address);

	return 0;
}

/*
 * Configure the processor info using MADT in the ACPI tables. If we fail to
 * configure that, then we use the MPS tables.
 */

void __init config_acpi_tables(void)
{
	acpi_boot_ops[ACPI_APIC] = acpi_parse_madt;
	acpi_boot_ops[ACPI_HPET] = acpi_parse_hpet;

	if (acpi_tables_init())
		printk(KERN_ERR "acpi: Init failed.\n");
}
