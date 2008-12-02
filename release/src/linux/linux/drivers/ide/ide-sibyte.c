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

/*  Derived loosely from ide-pmac.c, so:
 *  
 *  Copyright (C) 1998 Paul Mackerras.
 *  Copyright (C) 1995-1998 Mark Lord
 */
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/ide.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/sibyte/sb1250_regs.h>
#include <asm/sibyte/sb1250_int.h>
#include <asm/sibyte/sb1250_genbus.h>
#include <asm/sibyte/64bit.h>
#include <asm/sibyte/board.h>

/* Note: this should be general for any board using IDE on GenBus */

extern struct ide_ops std_ide_ops;

static ide_hwif_t *sb_ide_hwif = NULL;
static unsigned long ide_base;

#define SIBYTE_IDE_BASE        (KSEG1ADDR(ide_base)-mips_io_port_base)
#define SIBYTE_IDE_REG(pcaddr) (SIBYTE_IDE_BASE + ((pcaddr)<<5))

/*
 * We are limiting the number of PCI-IDE devices to leave room for
 * GenBus IDE (and possibly PCMCIA/CF?)
 */
static int sibyte_ide_default_irq(ide_ioreg_t base)
{
	return 0;
}

static ide_ioreg_t sibyte_ide_default_io_base(int index)
{
	return 0;
}

static void sibyte_ide_init_hwif_ports (hw_regs_t *hw, ide_ioreg_t data_port,
				       ide_ioreg_t ctrl_port, int *irq)
{
	std_ide_ops.ide_init_hwif_ports(hw, data_port, ctrl_port, irq);
}

static int sibyte_ide_request_irq(unsigned int irq,
                                void (*handler)(int,void *, struct pt_regs *),
                                unsigned long flags, const char *device,
                                void *dev_id)
{
	return request_irq(irq, handler, flags, device, dev_id);
}

static void sibyte_ide_free_irq(unsigned int irq, void *dev_id)
{
	free_irq(irq, dev_id);
}

static inline int is_sibyte_ide(ide_ioreg_t from)
{
	return (sb_ide_hwif &&
		((from == sb_ide_hwif->io_ports[IDE_DATA_OFFSET]) ||
		 (from == sb_ide_hwif->io_ports[IDE_ERROR_OFFSET]) ||
		 (from == sb_ide_hwif->io_ports[IDE_NSECTOR_OFFSET]) ||
		 (from == sb_ide_hwif->io_ports[IDE_SECTOR_OFFSET]) ||
		 (from == sb_ide_hwif->io_ports[IDE_LCYL_OFFSET]) ||
		 (from == sb_ide_hwif->io_ports[IDE_HCYL_OFFSET]) ||
		 (from == sb_ide_hwif->io_ports[IDE_SELECT_OFFSET]) ||
		 (from == sb_ide_hwif->io_ports[IDE_STATUS_OFFSET]) ||
		 (from == sb_ide_hwif->io_ports[IDE_CONTROL_OFFSET])));
}

static int sibyte_ide_check_region(ide_ioreg_t from, unsigned int extent)
{
	/* Figure out if it's the SiByte IDE; if so, don't do anything
           since our I/O space is in a weird place. */
	if (is_sibyte_ide(from))
		return 0;
	else
#ifdef CONFIG_BLK_DEV_IDE
		return std_ide_ops.ide_check_region(from, extent);
#else
		return 0;
#endif
}

static void sibyte_ide_request_region(ide_ioreg_t from, unsigned int extent,
				     const char *name)
{
#ifdef CONFIG_BLK_DEV_IDE
	if (!is_sibyte_ide(from))
		std_ide_ops.ide_request_region(from, extent, name);
#endif
}

static void sibyte_ide_release_region(ide_ioreg_t from, unsigned int extent)
{
#ifdef CONFIG_BLK_DEV_IDE
	if (!is_sibyte_ide(from))
		std_ide_ops.ide_release_region(from, extent);
#endif
}

struct ide_ops sibyte_ide_ops = {
	&sibyte_ide_default_irq,
	&sibyte_ide_default_io_base,
	&sibyte_ide_init_hwif_ports,
	&sibyte_ide_request_irq,
	&sibyte_ide_free_irq,
	&sibyte_ide_check_region,
	&sibyte_ide_request_region,
	&sibyte_ide_release_region
};

/*
 * I/O operations. The FPGA for SiByte generic bus IDE deals with
 * byte-swapping for us, so we can't share the I/O macros with other
 * IDE (e.g. PCI-IDE) devices.
 */

#define sibyte_outb(val,port)					\
do {								\
	*(volatile u8 *)(mips_io_port_base + (port)) = val;	\
} while(0)

#define sibyte_outw(val,port)					\
do {								\
	*(volatile u16 *)(mips_io_port_base + (port)) = val;	\
} while(0)

#define sibyte_outl(val,port)					\
do {								\
	*(volatile u32 *)(mips_io_port_base + (port)) = val;	\
} while(0)

static inline unsigned char sibyte_inb(unsigned long port)
{
	return (*(volatile u8 *)(mips_io_port_base + (port)));
}

static inline unsigned short sibyte_inw(unsigned long port)
{
	return (*(volatile u16 *)(mips_io_port_base + (port)));
}

static inline unsigned int sibyte_inl(unsigned long port)
{
	return (*(volatile u32 *)(mips_io_port_base + (port)));
}


static inline void sibyte_outsb(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		sibyte_outb(*(u8 *)addr, port);
		addr++;
	}
}

static inline void sibyte_insb(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		*(u8 *)addr = sibyte_inb(port);
		addr++;
	}
}

static inline void sibyte_outsw(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		sibyte_outw(*(u16 *)addr, port);
		addr += 2;
	}
}

static inline void sibyte_insw(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		*(u16 *)addr = sibyte_inw(port);
		addr += 2;
	}
}

static inline void sibyte_outsl(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		sibyte_outl(*(u32 *)addr, port);
		addr += 4;
	}
}

static inline void sibyte_insl(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		*(u32 *)addr = sibyte_inl(port);
		addr += 4;
	}
}

static void sibyte_ideproc(ide_ide_action_t action, ide_drive_t *drive,
			   void *buffer, unsigned int count)
{
	/*  slow? vlb_sync? */
	switch (action) {
	case ideproc_ide_input_data:
		if (drive->io_32bit) {
			sibyte_insl(IDE_DATA_REG, buffer, count);
		} else {
			sibyte_insw(IDE_DATA_REG, buffer, count<<1);
		}
		break;
	case ideproc_ide_output_data:
		if (drive->io_32bit) {
			sibyte_outsl(IDE_DATA_REG, buffer, count);
		} else {
			sibyte_outsw(IDE_DATA_REG, buffer, count<<1);
		}
		break;
	case ideproc_atapi_input_bytes:
		count++;
		if (drive->io_32bit) {
			sibyte_insl(IDE_DATA_REG, buffer, count>>2);
		} else {
			sibyte_insw(IDE_DATA_REG, buffer, count>>1);
		}
		if ((count & 3) >= 2)
			sibyte_insw(IDE_DATA_REG, (char *)buffer + (count & ~3), 1);
		break;
	case ideproc_atapi_output_bytes:
		count++;
		if (drive->io_32bit) {
			sibyte_outsl(IDE_DATA_REG, buffer, count>>2);
		} else {
			sibyte_outsw(IDE_DATA_REG, buffer, count>>1);
		}
		if ((count & 3) >= 2)
			sibyte_outsw(IDE_DATA_REG, (char *)buffer + (count & ~3), 1);
		break;
	}
}

/*
 * selectproc and intrproc aren't really necessary, since
 * byte-swapping doesn't affect byte ops; they are included for
 * consistency.
 */
static void sibyte_selectproc(ide_drive_t *drive)
{
	sibyte_outb(drive->select.all, IDE_SELECT_REG);
}

static void sibyte_intrproc(ide_drive_t *drive)
{
	sibyte_outb(drive->ctl|2, IDE_CONTROL_REG);
}

void __init sibyte_ide_probe(void)
{
	int i;
	ide_hwif_t *hwif;
	/* 
	 * Find the first untaken slot in hwifs 
	 */
	for (i = 0; i < MAX_HWIFS; i++) {
		if (!ide_hwifs[i].io_ports[IDE_DATA_OFFSET]) {
			break;
		}
	}
	if (i == MAX_HWIFS) {
		printk("No space for SiByte onboard IDE driver in ide_hwifs[].  Not enabled.\n");
		return;
	}

	/* Find memory base address */
#ifdef __MIPSEL__
	if (sb1250_pass == K_SYS_REVISION_PASS1)
		ide_base = G_IO_START_ADDR(csr_in32(4+(IO_SPACE_BASE|A_IO_EXT_REG(R_IO_EXT_REG(R_IO_EXT_START_ADDR, IDE_CS))))) << S_IO_ADDRBASE;
	else
#endif
		ide_base = G_IO_START_ADDR(csr_in32(IO_SPACE_BASE|A_IO_EXT_REG(R_IO_EXT_REG(R_IO_EXT_START_ADDR, IDE_CS)))) << S_IO_ADDRBASE;

	/*
	 * Set up our stuff; we're a little odd because our io_ports
	 * aren't in the usual place, and byte-swapping isn't
	 * necessary.
	 */
	hwif = &ide_hwifs[i];
	hwif->hw.io_ports[IDE_DATA_OFFSET]    = SIBYTE_IDE_REG(0x1f0);
	hwif->hw.io_ports[IDE_ERROR_OFFSET]   = SIBYTE_IDE_REG(0x1f1);
	hwif->hw.io_ports[IDE_NSECTOR_OFFSET] = SIBYTE_IDE_REG(0x1f2);
	hwif->hw.io_ports[IDE_SECTOR_OFFSET]  = SIBYTE_IDE_REG(0x1f3);
	hwif->hw.io_ports[IDE_LCYL_OFFSET]    = SIBYTE_IDE_REG(0x1f4);
	hwif->hw.io_ports[IDE_HCYL_OFFSET]    = SIBYTE_IDE_REG(0x1f5);
	hwif->hw.io_ports[IDE_SELECT_OFFSET]  = SIBYTE_IDE_REG(0x1f6);
	hwif->hw.io_ports[IDE_STATUS_OFFSET]  = SIBYTE_IDE_REG(0x1f7);
	hwif->hw.io_ports[IDE_CONTROL_OFFSET] = SIBYTE_IDE_REG(0x3f6);
	hwif->hw.irq                          = K_INT_GB_IDE;
	hwif->irq                             = K_INT_GB_IDE;
	hwif->noprobe                         = 0;
	/* Use our own non-byte-swapping routines */
	hwif->ideproc                         = sibyte_ideproc;
	hwif->selectproc                      = sibyte_selectproc;
	hwif->intrproc                        = sibyte_intrproc;

	memcpy(hwif->io_ports, hwif->hw.io_ports, sizeof(hwif->io_ports));
	printk("SiByte onboard IDE configured as device %i\n", i);
	sb_ide_hwif = hwif;
}
