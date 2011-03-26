/*
 * HND MIPS boards setup routines
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: setup.c,v 1.14 2010/02/26 04:43:25 Exp $
 */

#include <linux/types.h>
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/serial.h>
#include <linux/serialP.h>
#include <linux/serial_core.h>
#if defined(CONFIG_BLK_DEV_IDE) || defined(CONFIG_BLK_DEV_IDE_MODULE)
#include <linux/blkdev.h>
#include <linux/ide.h>
#endif
#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/time.h>
#include <asm/reboot.h>

#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/romfs_fs.h>
#include <linux/cramfs_fs.h>
#include <linux/squashfs_fs.h>
#endif

#include <typedefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmnvram.h>
#include <siutils.h>
#include <hndsoc.h>
#include <hndcpu.h>
#include <mips33_core.h>
#include <mips74k_core.h>
#include <sbchipc.h>
#include <hndchipc.h>
#include <trxhdr.h>
#ifdef HNDCTF
#include <ctf/hndctf.h>
#endif /* HNDCTF */
#include "bcm947xx.h"

extern void bcm947xx_time_init(void);
extern void bcm947xx_timer_setup(struct irqaction *irq);

#ifdef CONFIG_KGDB
extern void set_debug_traps(void);
extern void rs_kgdb_hook(struct uart_port *);
extern void breakpoint(void);
#endif

#if defined(CONFIG_BLK_DEV_IDE) || defined(CONFIG_BLK_DEV_IDE_MODULE)
extern struct ide_ops std_ide_ops;
#endif

/* Global SB handle */
si_t *bcm947xx_sih = NULL;
spinlock_t bcm947xx_sih_lock = SPIN_LOCK_UNLOCKED;
EXPORT_SYMBOL(bcm947xx_sih);
EXPORT_SYMBOL(bcm947xx_sih_lock);

/* CPU freq */
int bcm947xx_cpu_clk;
EXPORT_SYMBOL(bcm947xx_cpu_clk);

/* Convenience */
#define sih bcm947xx_sih
#define sih_lock bcm947xx_sih_lock

#ifdef HNDCTF
ctf_t *kcih = NULL;
EXPORT_SYMBOL(kcih);
ctf_attach_t ctf_attach_fn = NULL;
EXPORT_SYMBOL(ctf_attach_fn);
#endif /* HNDCTF */

/* Kernel command line */
extern char arcs_cmdline[CL_SIZE];

void
bcm947xx_machine_restart(char *command)
{
	printk("Please stand by while rebooting the system...\n");

	/* Set the watchdog timer to reset immediately */
	local_irq_disable();
	hnd_cpu_reset(sih);
}

void
bcm947xx_machine_halt(void)
{
	printk("System halted\n");

	/* Disable interrupts and watchdog and spin forever */
	local_irq_disable();
	si_watchdog(sih, 0);
	while (1);
}

#ifdef CONFIG_SERIAL_CORE

static int num_ports = 0;

static void __init
serial_add(void *regs, uint irq, uint baud_base, uint reg_shift)
{
	struct uart_port rs;

	memset(&rs, 0, sizeof(rs));
	rs.line = num_ports++;
	rs.flags = UPF_BOOT_AUTOCONF | UPF_SHARE_IRQ;
	rs.iotype = UPIO_MEM;

	rs.mapbase = (unsigned int) regs;
	rs.membase = regs;
	rs.irq = irq + 2;
	rs.uartclk = baud_base;
	rs.regshift = reg_shift;

	if (early_serial_setup(&rs) != 0) {
		printk(KERN_ERR "Serial setup failed!\n");
	}
}

static void __init
serial_setup(si_t *sih)
{
	si_serial_init(sih, serial_add);

#ifdef CONFIG_KGDB
	/* Use the last port for kernel debugging */
	if (rs.membase)
		rs_kgdb_hook(&rs);
#endif
}

#endif /* CONFIG_SERIAL_CORE */

void __init
brcm_setup(void)
{
	char *value;

	/* Get global SB handle */
	sih = si_kattach(SI_OSH);

	/* Initialize clocks and interrupts */
	si_mips_init(sih, SBMIPS_VIRTIRQ_BASE);

	if (BCM330X(current_cpu_data.processor_id) &&
		(read_c0_diag() & BRCM_PFC_AVAIL)) {
		/* 
		 * Now that the sih is inited set the  proper PFC value 
		 */	
		printk("Setting the PFC to its default value\n");
		enable_pfc(PFC_AUTO);
	}


#ifdef CONFIG_SERIAL_CORE
	/* Initialize UARTs */
	serial_setup(sih);
#endif /* CONFIG_SERIAL_CORE */

#if defined(CONFIG_BLK_DEV_IDE) || defined(CONFIG_BLK_DEV_IDE_MODULE)
	ide_ops = &std_ide_ops;
#endif

	/* Override default command line arguments */
	value = nvram_get("kernel_args");
	if (value && strlen(value) && strncmp(value, "empty", 5))
		strncpy(arcs_cmdline, value, sizeof(arcs_cmdline));


	/* Generic setup */
	_machine_restart = bcm947xx_machine_restart;
	_machine_halt = bcm947xx_machine_halt;
	pm_power_off = bcm947xx_machine_halt;

	board_time_init = bcm947xx_time_init;
}

const char *
get_system_type(void)
{
	static char s[32];
	char cn[8];

	if (bcm947xx_sih) {
		bcm_chipname(bcm947xx_sih->chip, cn, 8);
		sprintf(s, "Broadcom BCM%s chip rev %d pkg %d", cn,
			bcm947xx_sih->chiprev, bcm947xx_sih->chippkg);
		return s;
	}
	else
		return "Broadcom BCM947XX";
}

void __init
bus_error_init(void)
{
}

void __init
plat_mem_setup(void)
{
	brcm_setup();
	return;
}

#ifdef CONFIG_MTD_PARTITIONS

enum {
	RT_UNKNOWN,
	RT_DIR320,	// D-Link DIR-320
	RT_WNR3500L,	// Netgear WNR3500v2/U/L
	RT_WNR2000V2,	// Netgear WNR2000v2
	RT_BELKIN_F7D	// Belkin F7D3301, F7D3302, F7D4302, F7D8235V3
};

static int get_router(void)
{
	uint boardnum = bcm_strtoul(nvram_safe_get("boardnum"), NULL, 0);
	uint boardrev = bcm_strtoul(nvram_safe_get("boardrev"), NULL, 0);
	uint boardtype = bcm_strtoul(nvram_safe_get("boardtype"), NULL, 0);

	if ((boardnum == 1 || boardnum == 3500) && boardtype == 0x4CF && (boardrev == 0x1213 || boardrev == 2)) {
		return RT_WNR3500L;
	}
	else if (boardnum == 1 && boardtype == 0xE4CD && boardrev == 0x1700) {
		return RT_WNR2000V2;
	}
#ifdef DIR320_BOARD
	else if (boardnum == 0 && boardtype == 0x48E && boardrev == 0x35) {
		return RT_DIR320;
	}
#endif
	else if (boardtype == 0xA4CF && (boardrev == 0x1102 || boardrev == 0x1100)) {
		return RT_BELKIN_F7D;
	}

	return RT_UNKNOWN;
}

#ifdef DIR320_BOARD
static size_t get_erasesize(struct mtd_info *mtd, size_t offset, size_t size)
{
	int i;
	struct mtd_erase_region_info *regions;
	size_t erasesize = 0;

	if (mtd->numeraseregions > 1) {
		regions = mtd->eraseregions;

		// Find the first erase regions which is part of this partition
		for (i = 0; i < mtd->numeraseregions && offset >= regions[i].offset; i++);

		for (i--; i < mtd->numeraseregions && offset + size > regions[i].offset; i++) {
			if (erasesize < regions[i].erasesize)
				erasesize = regions[i].erasesize;
		}
	}
	else {
		erasesize = mtd->erasesize;
	}

	return erasesize;
}
#endif

/*
	new layout -- zzz 04/2006

	+--------------+
	| boot         |
	+---+----------+	< search for HDR0
	|   |          |
	|   | (kernel) |
	| l |          |
	| i +----------+	< + trx->offset[1]
	| n |          |
	| u | rootfs   |
	| x |          |
	+   +----------+	< + trx->len
	|   | jffs2    |
	+---+----------+	< size - NVRAM_SPACE - board_data_size()
	| board_data   |
	+--------------+ 	< size - NVRAM_SPACE
	| nvram        |
	+--------------+	< size
*/

static struct mtd_partition bcm947xx_parts[] = {
	{ name: "pmon", offset: 0, size: 0, mask_flags: MTD_WRITEABLE, },
	{ name: "linux", offset: 0, size: 0, },
	{ name: "rootfs", offset: 0, size: 0, mask_flags: MTD_WRITEABLE, },
	{ name: "jffs2", offset: 0, size: 0, },
	{ name: "nvram", offset: 0, size: 0, },
	{ name: "board_data", offset: 0, size: 0, },
	{ name: NULL, },
};

#define PART_BOOT	0
#define PART_LINUX	1
#define PART_ROOTFS	2
#define PART_JFFS2	3
#define PART_NVRAM	4
#define PART_BOARD	5

struct mtd_partition *
init_mtd_partitions(struct mtd_info *mtd, size_t size)
{
	int router;
	struct trx_header *trx;
	unsigned char buf[512];
	size_t off, trxoff, boardoff;
	size_t crclen;
	size_t len;
	size_t trxsize;

	crclen = 0;

	/* Find and size nvram */
	bcm947xx_parts[PART_NVRAM].size = ROUNDUP(NVRAM_SPACE, mtd->erasesize);
	bcm947xx_parts[PART_NVRAM].offset = size - bcm947xx_parts[PART_NVRAM].size;

	/* Size board_data */
	boardoff = bcm947xx_parts[PART_NVRAM].offset;
	router = get_router();
	switch (router) {
#ifdef DIR320_BOARD
	case RT_DIR320:
		if (get_erasesize(mtd, bcm947xx_parts[PART_NVRAM].offset, bcm947xx_parts[PART_NVRAM].size) == 0x2000) {
			bcm947xx_parts[PART_NVRAM].size = ROUNDUP(NVRAM_SPACE, 0x2000);
			bcm947xx_parts[PART_NVRAM].offset = size - bcm947xx_parts[PART_NVRAM].size;
			bcm947xx_parts[PART_BOARD].size = 0x2000; // 8 KB
			bcm947xx_parts[PART_BOARD].offset = bcm947xx_parts[PART_NVRAM].offset - bcm947xx_parts[PART_BOARD].size;
		}
		else bcm947xx_parts[PART_BOARD].name = NULL;
		break;
#endif
	case RT_WNR3500L:
	case RT_WNR2000V2:
		bcm947xx_parts[PART_BOARD].size = mtd->erasesize;
		boardoff -= bcm947xx_parts[PART_BOARD].size;
		bcm947xx_parts[PART_BOARD].offset = boardoff;
		boardoff -= 5 * mtd->erasesize;
		if (size <= 4 * 1024 * 1024) {
			// 4MB flash
			bcm947xx_parts[PART_JFFS2].offset = boardoff;
			bcm947xx_parts[PART_JFFS2].size = bcm947xx_parts[PART_BOARD].offset - bcm947xx_parts[PART_JFFS2].offset;
		}
		else {
			// 8MB or larger flash, exclude 1 block for Netgear CRC
			crclen = mtd->erasesize;
		}
		break;
	default:
		bcm947xx_parts[PART_BOARD].name = NULL;
		break;
	}

	trxsize = 0;
	trx = (struct trx_header *) buf;
	for (off = 0; off < size; off += mtd->erasesize) {
		/*
		 * Read block 0 to test for rootfs
		 */
		if ((mtd->read(mtd, off, sizeof(buf), &len, buf)) || (len != sizeof(buf)))
			continue;

		/* Try looking at TRX header for rootfs offset */
		switch (le32_to_cpu(trx->magic)) {
		case TRX_MAGIC_F7D3301:
		case TRX_MAGIC_F7D3302:
		case TRX_MAGIC_F7D4302:
		case TRX_MAGIC_F5D8235V3:
		case TRX_MAGIC_QA:
			if (router != RT_BELKIN_F7D)
				continue;
			// fall through
		case TRX_MAGIC:
			trxsize = ROUNDUP(le32_to_cpu(trx->len), mtd->erasesize);	// kernel + rootfs
			break;
		}

		if (trxsize) {
			/* Size pmon */
			bcm947xx_parts[PART_BOOT].size = off;

			/* Size linux (kernel and rootfs) */
			bcm947xx_parts[PART_LINUX].offset = off;
			bcm947xx_parts[PART_LINUX].size = boardoff - off;

			/* Find and size rootfs */
			trxoff = (le32_to_cpu(trx->offsets[2]) > off) ? trx->offsets[2] : trx->offsets[1];
			bcm947xx_parts[PART_ROOTFS].offset = trxoff + off;
			bcm947xx_parts[PART_ROOTFS].size = trxsize - trxoff;

			/* Find and size jffs2 */
			if (bcm947xx_parts[PART_JFFS2].size == 0) {
				bcm947xx_parts[PART_JFFS2].offset = off + trxsize;
				if ((boardoff - crclen) > bcm947xx_parts[PART_JFFS2].offset) {
					bcm947xx_parts[PART_JFFS2].size = boardoff - crclen - bcm947xx_parts[PART_JFFS2].offset;
				}
			}

			break;
		}
	}

	if (trxsize == 0) {
		// uh, now what...
		printk(KERN_NOTICE "%s: Unable to find a valid linux partition\n", mtd->name);
	}

#if 0
	int i;
	for (i = 0; bcm947xx_parts[i].name; ++i) {
		printk(KERN_NOTICE "%8x %8x (%8x) %s\n",
			bcm947xx_parts[i].offset,
			(bcm947xx_parts[i].offset + bcm947xx_parts[i].size) - 1,
			bcm947xx_parts[i].size,
			bcm947xx_parts[i].name);
	}
#endif

	return bcm947xx_parts;
}

#if 0
static struct mtd_partition bcm947xx_parts[] =
{
	{
		.name = "boot",
		.size = 0,
		.offset = 0,
	//	.mask_flags = MTD_WRITEABLE
	},
	{
		.name = "linux",
		.size = 0,
		.offset = 0
	},
	{
		.name = "rootfs",
		.size = 0,
		.offset = 0,
		.mask_flags = MTD_WRITEABLE
	},
	{
		.name = "nvram",
		.size = 0,
		.offset = 0
	},
	{
		.name = 0,
		.size = 0,
		.offset = 0
	}
};

struct mtd_partition *
init_mtd_partitions(struct mtd_info *mtd, size_t size)
{
	struct romfs_super_block *romfsb;
	struct cramfs_super *cramfsb;
	struct squashfs_super_block *squashfsb;
	struct trx_header *trx;
	unsigned char buf[512];
	int off;
	size_t len;
	int i;

	romfsb = (struct romfs_super_block *) buf;
	cramfsb = (struct cramfs_super *) buf;
	squashfsb = (struct squashfs_super_block *) buf;
	trx = (struct trx_header *) buf;

	/* Look at every 64 KB boundary */
	for (off = 0; off < size; off += (64 * 1024)) {
		memset(buf, 0xe5, sizeof(buf));

		/*
		 * Read block 0 to test for romfs and cramfs superblock
		 */
		if (mtd->read(mtd, off, sizeof(buf), &len, buf) ||
		    len != sizeof(buf))
			continue;

		/* Try looking at TRX header for rootfs offset */
		if (le32_to_cpu(trx->magic) == TRX_MAGIC) {
			bcm947xx_parts[1].offset = off;
//			if (le32_to_cpu(trx->offsets[1]) > off)
	                if (le32_to_cpu(trx->offsets[2]) > off)
	                        off = le32_to_cpu(trx->offsets[2]);
	                else if (le32_to_cpu(trx->offsets[1]) > off)
				off = le32_to_cpu(trx->offsets[1]);
			/* In case where CFE boots from ROM, we expect
			 * Linux to fit in first flash partition.
			 */
			if (bcm947xx_parts[1].offset == 0 && off)
				off -= (64 * 1024);
			continue;
		}

		/* romfs is at block zero too */
		if (romfsb->word0 == ROMSB_WORD0 &&
		    romfsb->word1 == ROMSB_WORD1) {
			printk(KERN_NOTICE
			       "%s: romfs filesystem found at block %d\n",
			       mtd->name, off / BLOCK_SIZE);
			goto done;
		}

		/* squashfs is at block zero too */
                if (squashfsb->s_magic == SQUASHFS_MAGIC
			|| squashfsb->s_magic == SQUASHFS_MAGIC_LZMA) {
                        printk(KERN_NOTICE
                               "%s: squashfs filesystem found at block %d\n",
                               mtd->name, off / BLOCK_SIZE);
                        goto done;
                }


		/* so is cramfs */
		if (cramfsb->magic == CRAMFS_MAGIC) {
			printk(KERN_NOTICE
			       "%s: cramfs filesystem found at block %d\n",
			       mtd->name, off / BLOCK_SIZE);
			goto done;
		}

	}

	printk(KERN_NOTICE
	       "%s: Couldn't find valid ROM disk image\n",
	       mtd->name);

 done:
	/* Setup NVRAM MTD partition */
	i = (sizeof(bcm947xx_parts)/sizeof(struct mtd_partition)) - 2;

	bcm947xx_parts[i].size = ROUNDUP(NVRAM_SPACE, mtd->erasesize);
	bcm947xx_parts[i].offset = size - bcm947xx_parts[i].size;

	/* Find and size rootfs */
	if (off < size) {
		bcm947xx_parts[2].offset = off;
		bcm947xx_parts[2].size = bcm947xx_parts[3].offset - bcm947xx_parts[2].offset;
	}

	/* Size linux (kernel and rootfs) */
	bcm947xx_parts[1].size = bcm947xx_parts[3].offset - bcm947xx_parts[1].offset;

	/* Size pmon */
	bcm947xx_parts[0].size = bcm947xx_parts[1].offset - bcm947xx_parts[0].offset;

	return bcm947xx_parts;
}
#endif	// 0

EXPORT_SYMBOL(init_mtd_partitions);

#endif
