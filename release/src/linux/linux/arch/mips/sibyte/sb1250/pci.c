/*
 * Copyright (C) 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * BCM1250-specific PCI support
 *
 * This module provides the glue between Linux's PCI subsystem
 * and the hardware.  We basically provide glue for accessing
 * configuration space, and set up the translation for I/O
 * space accesses.
 *
 * To access configuration space, we call some assembly-level
 * stubs that flip the KX bit on and off in the status
 * register, and do XKSEG addressed memory accesses there.
 * It's slow (7 SSNOPs to guarantee that KX is set!) but
 * fortunately, config space accesses are rare.
 *
 * We could use the ioremap functionality for the confguration
 * space as well as I/O space, but I'm not sure of the
 * implications of setting aside 16MB of KSEG2 for something
 * that is used so rarely (how much space in the page tables?)
 *
 */
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/console.h>

#include <asm/sibyte/sb1250_defs.h>
#include <asm/sibyte/sb1250_regs.h>
#include <asm/sibyte/sb1250_scd.h>
#include <asm/io.h>

#include "lib_hssubr.h"

/*
 * This macro calculates the offset into config space where
 * a given bus, device/function, and offset live on the sb1250
 */

#define CFGOFFSET(bus,devfn,where) (((bus)<<16)+((devfn)<<8)+(where))

/*
 * Using the above offset, this macro calcuates the physical address in the
 * config space.
 */
#define CFGADDR(dev,where) (A_PHYS_LDTPCI_CFG_MATCH_BITS + \
			    CFGOFFSET(dev->bus->number,dev->devfn,where))

/*
 * Read/write 32-bit values in config space.
 */
static inline u32 READCFG32(u32 addr)
{
	return hs_read32(addr & ~3);
}

static inline void WRITECFG32(u32 addr, u32 data)
{
	hs_write32(addr & ~3,(data));
}

/*
 * This variable is the KSEG2 (kernel virtual) mapping of the ISA/PCI I/O
 * space area.  We map 64K here and the offsets from this address get treated
 * with "match bytes" policy to make everything look little-endian.  So, you
 * need to also set CONFIG_SWAP_IO_SPACE, but this is the combination that
 * works correctly with most of Linux's drivers.
 */

#define PCI_BUS_ENABLED	1
#define LDT_BUS_ENABLED	2

static int sb1250_bus_status = 0;

#define MATCH_BITS	0x20000000	/* really belongs in an include file */

#define LDT_BRIDGE_START ((A_PCI_TYPE01_HEADER|MATCH_BITS)+0x00)
#define LDT_BRIDGE_END   ((A_PCI_TYPE01_HEADER|MATCH_BITS)+0x20)

/*
 * Read/write access functions for various sizes of values
 * in config space.
 */

static int
sb1250_pci_read_config_byte(struct pci_dev *dev, int where, u8 * val)
{
	u32 data = 0;
	u32 cfgaddr = CFGADDR(dev, where);

	data = READCFG32(cfgaddr);

	/*
	 * If the LDT was not configured, make it look like the bridge
	 * header is not there.
	 */
	if (!(sb1250_bus_status & LDT_BUS_ENABLED) &&
	    (cfgaddr >= LDT_BRIDGE_START) && (cfgaddr < LDT_BRIDGE_END)) {
		data = 0xFFFFFFFF;
	}

	*val = (data >> ((where & 3) << 3)) & 0xff;

	return PCIBIOS_SUCCESSFUL;
}


static int
sb1250_pci_read_config_word(struct pci_dev *dev, int where, u16 * val)
{
	u32 data = 0;
	u32 cfgaddr = CFGADDR(dev, where);

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	data = READCFG32(cfgaddr);

	/*
	 * If the LDT was not configured, make it look like the bridge
	 * header is not there.
	 */
	if (!(sb1250_bus_status & LDT_BUS_ENABLED) &&
	    (cfgaddr >= LDT_BRIDGE_START) && (cfgaddr < LDT_BRIDGE_END)) {
		data = 0xFFFFFFFF;
	}

	*val = (data >> ((where & 3) << 3)) & 0xffff;

	return PCIBIOS_SUCCESSFUL;
}

static int
sb1250_pci_read_config_dword(struct pci_dev *dev, int where, u32 * val)
{
	u32 data = 0;
	u32 cfgaddr = CFGADDR(dev, where);

	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	data = READCFG32(cfgaddr);

	/*
	 * If the LDT was not configured, make it look like the bridge
	 * header is not there.
	 */
	if (!(sb1250_bus_status & LDT_BUS_ENABLED) &&
	    (cfgaddr >= LDT_BRIDGE_START) && (cfgaddr < LDT_BRIDGE_END)) {
		data = 0xFFFFFFFF;
	}

	*val = data;

	return PCIBIOS_SUCCESSFUL;
}


static int
sb1250_pci_write_config_byte(struct pci_dev *dev, int where, u8 val)
{
	u32 data = 0;
	u32 cfgaddr = CFGADDR(dev, where);

	data = READCFG32(cfgaddr);

	data = (data & ~(0xff << ((where & 3) << 3))) |
	    (val << ((where & 3) << 3));

	WRITECFG32(cfgaddr, data);

	return PCIBIOS_SUCCESSFUL;
}

static int
sb1250_pci_write_config_word(struct pci_dev *dev, int where, u16 val)
{
	u32 data = 0;
	u32 cfgaddr = CFGADDR(dev, where);

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	data = READCFG32(cfgaddr);

	data = (data & ~(0xffff << ((where & 3) << 3))) |
	    (val << ((where & 3) << 3));

	WRITECFG32(cfgaddr, data);

	return PCIBIOS_SUCCESSFUL;
}

static int
sb1250_pci_write_config_dword(struct pci_dev *dev, int where, u32 val)
{
	u32 cfgaddr = CFGADDR(dev, where);

	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	WRITECFG32(cfgaddr, val);

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops sb1250_pci_ops = {
	sb1250_pci_read_config_byte,
	sb1250_pci_read_config_word,
	sb1250_pci_read_config_dword,
	sb1250_pci_write_config_byte,
	sb1250_pci_write_config_word,
	sb1250_pci_write_config_dword
};


void __init pcibios_init(void)
{
	uint32_t cmdreg;
	uint64_t reg;

	/*
	 * See if the PCI bus has been configured by the firmware.
	 */

	cmdreg = READCFG32((A_PCI_TYPE00_HEADER | MATCH_BITS) +
			   PCI_COMMAND);

	if (!(cmdreg & PCI_COMMAND_MASTER)) {
		printk
		    ("PCI: Skipping PCI probe.  Bus is not initialized.\n");
		return;
	}

	reg = *((volatile uint64_t *) KSEG1ADDR(A_SCD_SYSTEM_CFG));
	if (!(reg & M_SYS_PCI_HOST)) {
		printk("PCI: Skipping PCI probe.  Processor is in PCI device mode.\n");
		return;
	}

	sb1250_bus_status |= PCI_BUS_ENABLED;

	/*
	 * Establish a mapping from KSEG2 (kernel virtual) to PCI I/O space
	 * Use "match bytes", even though this exposes endianness.
	 * big-endian Linuxes will have CONFIG_SWAP_IO_SPACE set.
	 */

	set_io_port_base((unsigned long)
		ioremap(A_PHYS_LDTPCI_IO_MATCH_BYTES, 65536));
	isa_slot_offset = (unsigned long)
		ioremap(A_PHYS_LDTPCI_IO_MATCH_BYTES_32, 1024*1024);

	/*
	 * Also check the LDT bridge's enable, just in case we didn't
	 * initialize that one.
	 */

	cmdreg = READCFG32((A_PCI_TYPE01_HEADER | MATCH_BITS) +
			   PCI_COMMAND);

	if (cmdreg & PCI_COMMAND_MASTER) {
		sb1250_bus_status |= LDT_BUS_ENABLED;
	}

	/* Probe for PCI hardware */

	printk("PCI: Probing PCI hardware on host bus 0.\n");
	pci_scan_bus(0, &sb1250_pci_ops, NULL);

#ifdef CONFIG_VGA_CONSOLE
	take_over_console(&vga_con,0,MAX_NR_CONSOLES-1,1);
#endif
}

int pcibios_enable_device(struct pci_dev *dev, int mask)
{
	/* Not needed, since we enable all devices at startup.  */
	return 0;
}

void pcibios_align_resource(void *data, struct resource *res,
		       unsigned long size, unsigned long align)
{
}

char *__init pcibios_setup(char *str)
{
	/* Nothing to do for now.  */

	return str;
}

struct pci_fixup pcibios_fixups[] = {
	{0}
};

void pcibios_update_resource(struct pci_dev *dev, struct resource *root,
	struct resource *res, int resource)
{
	unsigned long where, size;
	u32 reg;

	where = PCI_BASE_ADDRESS_0 + (resource * 4);
	size = res->end - res->start;
	pci_read_config_dword(dev, where, &reg);
	reg = (reg & size) | (((u32) (res->start - root->start)) & ~size);
	pci_write_config_dword(dev, where, reg);
}

/*
 *  Called after each bus is probed, but before its children
 *  are examined.
 */
void __devinit pcibios_fixup_bus(struct pci_bus *b)
{
	pci_read_bridge_bases(b);
}

unsigned int pcibios_assign_all_busses(void)
{
	return 1;
}
