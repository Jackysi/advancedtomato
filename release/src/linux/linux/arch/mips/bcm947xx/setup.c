/*
 *  Generic setup routines for Broadcom MIPS boards
 *
 *  Copyright (C) 2005 Felix Fietkau <nbd@openwrt.org>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/serialP.h>
#include <linux/ide.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/time.h>
#include <asm/reboot.h>

#include <typedefs.h>
#include <osl.h>
#include <sbutils.h>
#include <bcmutils.h>
#include <bcmnvram.h>
#include <bcmdevs.h>
#include <sbhndmips.h>
#include <hndmips.h>
#include <trxhdr.h>
#include "bcm947xx.h"
#include <linux/mtd/mtd.h>
#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/partitions.h>
#endif
#include <linux/romfs_fs.h>
#include <linux/cramfs_fs.h>
#include <linux/minix_fs.h>
#include <linux/ext2_fs.h>

/* Global SB handle */
sb_t *bcm947xx_sbh = NULL;
spinlock_t bcm947xx_sbh_lock = SPIN_LOCK_UNLOCKED;
EXPORT_SYMBOL(bcm947xx_sbh);
EXPORT_SYMBOL(bcm947xx_sbh_lock);

/* CPU freq Tomato RAF features */
int bcm947xx_cpu_clk; 
EXPORT_SYMBOL(bcm947xx_cpu_clk);

/* Convenience */
#define sbh bcm947xx_sbh
#define sbh_lock bcm947xx_sbh_lock

extern void bcm947xx_time_init(void);
extern void bcm947xx_timer_setup(struct irqaction *irq);

#ifdef CONFIG_REMOTE_DEBUG
extern void set_debug_traps(void);
extern void rs_kgdb_hook(struct serial_state *);
extern void breakpoint(void);
#endif

#if defined(CONFIG_BLK_DEV_IDE) || defined(CONFIG_BLK_DEV_IDE_MODULE)
extern struct ide_ops std_ide_ops;
#endif

/* Kernel command line */
char arcs_cmdline[CL_SIZE] __initdata = CONFIG_CMDLINE;
extern void sb_serial_init(sb_t *sbh, void (*add)(void *regs, uint irq, uint baud_base, uint reg_shift));

void
bcm947xx_machine_restart(char *command)
{
	printk("Please stand by while rebooting the system...\n");

	if (sb_chip(sbh) == BCM4785_CHIP_ID)
		MTC0(C0_BROADCOM, 4, (1 << 22));

	/* Set the watchdog timer to reset immediately */
	__cli();
	sb_watchdog(sbh, 1);

	if (sb_chip(sbh) == BCM4785_CHIP_ID) {
		__asm__ __volatile__(
			".set\tmips3\n\t"
			"sync\n\t"
			"wait\n\t"
			".set\tmips0");
	}

	while (1);
}

void
bcm947xx_machine_halt(void)
{
	printk("System halted\n");

	/* Disable interrupts and watchdog and spin forever */
	__cli();
	sb_watchdog(sbh, 0);
	while (1);
}

#ifdef CONFIG_SERIAL

static int ser_line = 0;

typedef struct {
        void *regs;
        uint irq;
        uint baud_base;
        uint reg_shift;
} serial_port;

static serial_port ports[4];
static int num_ports = 0;

static void
serial_add(void *regs, uint irq, uint baud_base, uint reg_shift)
{
        ports[num_ports].regs = regs;
        ports[num_ports].irq = irq;
        ports[num_ports].baud_base = baud_base;
        ports[num_ports].reg_shift = reg_shift;
        num_ports++;
}

static void
do_serial_add(serial_port *port)
{
        void *regs;
        uint irq;
        uint baud_base;
        uint reg_shift;
        struct serial_struct s;
        
        regs = port->regs;
        irq = port->irq;
        baud_base = port->baud_base;
        reg_shift = port->reg_shift;

        memset(&s, 0, sizeof(s));

        s.line = ser_line++;
        s.iomem_base = regs;
        s.irq = irq + 2;
        s.baud_base = baud_base / 16;
        s.flags = ASYNC_BOOT_AUTOCONF;
        s.io_type = SERIAL_IO_MEM;
        s.iomem_reg_shift = reg_shift;

        if (early_serial_setup(&s) != 0) {
                printk(KERN_ERR "Serial setup failed!\n");
        }
}

#endif /* CONFIG_SERIAL */

void __init
brcm_setup(void)
{
	int i;
	char *value;

	/* Get global SB handle */
	sbh = sb_kattach(SB_OSH);

	/* Initialize clocks and interrupts */
	sb_mips_init(sbh, SBMIPS_VIRTIRQ_BASE);

	if (BCM330X(current_cpu_data.processor_id) &&
		(read_c0_diag() & BRCM_PFC_AVAIL)) {
		/* 
		 * Now that the sbh is inited set the  proper PFC value 
		 */	
		printk("Setting the PFC to its default value\n");
		enable_pfc(PFC_AUTO);
	}


	value = nvram_get("kernel_args");
#ifdef CONFIG_SERIAL
	sb_serial_init(sbh, serial_add);

	/* reverse serial ports if nvram variable contains console=ttyS1 */
	/* Initialize UARTs */
	if (value && strlen(value) && strstr(value, "console=ttyS1")!=NULL) {
		for (i = num_ports; i; i--)
			do_serial_add(&ports[i - 1]);
	} else {
		for (i = 0; i < num_ports; i++)
			do_serial_add(&ports[i]);
	}
#endif

#if defined(CONFIG_BLK_DEV_IDE) || defined(CONFIG_BLK_DEV_IDE_MODULE)
	ide_ops = &std_ide_ops;
#endif

	/* Override default command line arguments */
	if (value && strlen(value) && strncmp(value, "empty", 5))
		strncpy(arcs_cmdline, value, sizeof(arcs_cmdline));


	/* Generic setup */
	_machine_restart = bcm947xx_machine_restart;
	_machine_halt = bcm947xx_machine_halt;
	_machine_power_off = bcm947xx_machine_halt;

	board_time_init = bcm947xx_time_init;
	board_timer_setup = bcm947xx_timer_setup;
}

const char *
get_system_type(void)
{
	static char s[40];

	if (bcm947xx_sbh) {
		sprintf(s, "Broadcom BCM%X chip rev %d pkg %d", sb_chip(bcm947xx_sbh),
			sb_chiprev(bcm947xx_sbh), sb_chippkg(bcm947xx_sbh));
		return s;
	}
	else
		return "Broadcom BCM947XX";
}

void __init
bus_error_init(void)
{
}


#ifdef CONFIG_MTD_PARTITIONS

enum {
	RT_UNKNOWN,
	RT_DIR320	// D-Link DIR-320
};

static int get_router(void)
{
	uint boardnum = bcm_strtoul(nvram_safe_get("boardnum"), NULL, 0);
	uint boardrev = bcm_strtoul(nvram_safe_get("boardrev"), NULL, 0);
	uint boardtype = bcm_strtoul(nvram_safe_get("boardtype"), NULL, 0);

#ifdef DIR320_BOARD
	if (boardnum == 0 && boardtype == 0x48E && boardrev == 0x35) {
		return RT_DIR320;
	}
#endif

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
#endif	// DIR320_BOARD

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
	{ name: "pmon",	  offset: 0, size: 0, mask_flags: MTD_WRITEABLE, },
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

struct mtd_partition * __init
init_mtd_partitions(struct mtd_info *mtd, size_t size)
{
	int router;
	struct trx_header *trx;
	unsigned char buf[512];
	size_t off, trxoff, boardoff;
	size_t len;
	size_t trxsize;

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
		if ((MTD_READ(mtd, off, sizeof(buf), &len, buf)) || (len != sizeof(buf))) continue;

		/* Try looking at TRX header for rootfs offset */
		if (le32_to_cpu(trx->magic) == TRX_MAGIC) {
			/* Size pmon */
			bcm947xx_parts[PART_BOOT].size = off;

			/* Size linux (kernel and rootfs) */
			bcm947xx_parts[PART_LINUX].offset = off;
			bcm947xx_parts[PART_LINUX].size = boardoff - off;

			trxsize = ROUNDUP(le32_to_cpu(trx->len), mtd->erasesize);	// kernel + rootfs

			/* Find and size rootfs */
			trxoff = (le32_to_cpu(trx->offsets[2]) > off) ? trx->offsets[2] : trx->offsets[1];
			bcm947xx_parts[PART_ROOTFS].offset = trxoff + off;
			bcm947xx_parts[PART_ROOTFS].size = trxsize - trxoff;

			/* Find and size jffs2 */
			bcm947xx_parts[PART_JFFS2].offset = off + trxsize;
			if (boardoff > bcm947xx_parts[PART_JFFS2].offset)
				bcm947xx_parts[PART_JFFS2].size = boardoff - bcm947xx_parts[PART_JFFS2].offset;

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
struct mtd_partition * __init
init_mtd_partitions(struct mtd_info *mtd, size_t size)
{
	struct minix_super_block *minixsb;
	struct ext2_super_block *ext2sb;
	struct romfs_super_block *romfsb;
	struct squashfs_super_block *squashfsb;
	struct cramfs_super *cramfsb;
	struct trx_header *trx;
	unsigned char buf[512];
	int off, trx_len = 0;
	size_t len;

	minixsb = (struct minix_super_block *) buf;
	ext2sb = (struct ext2_super_block *) buf;
	romfsb = (struct romfs_super_block *) buf;
	squashfsb = (struct squashfs_super_block *) buf;
	cramfsb = (struct cramfs_super *) buf;
	trx = (struct trx_header *) buf;

	/* Look at every 64 KB boundary */
	for (off = 0; off < size; off += (64 * 1024)) {
		memset(buf, 0xe5, sizeof(buf));

		/*
		 * Read block 0 to test for romfs and cramfs superblock
		 */
		if (MTD_READ(mtd, off, sizeof(buf), &len, buf) ||
		    len != sizeof(buf))
			continue;

		/* Try looking at TRX header for rootfs offset */
		if (le32_to_cpu(trx->magic) == TRX_MAGIC) {
			bcm947xx_parts[PART_LINUX].offset = off;
			trx_len = trx->len;
			if (le32_to_cpu(trx->offsets[2]) > off)
				off = le32_to_cpu(trx->offsets[2]);
			else if (le32_to_cpu(trx->offsets[1]) > off)
				off = le32_to_cpu(trx->offsets[1]);
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
		if (squashfsb->s_magic == SQUASHFS_MAGIC) {
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

		/*
		 * Read block 1 to test for minix and ext2 superblock
		 */
		if (MTD_READ(mtd, off + BLOCK_SIZE, sizeof(buf), &len, buf) ||
		    len != sizeof(buf))
			continue;

		/* Try minix */
		if (minixsb->s_magic == MINIX_SUPER_MAGIC ||
		    minixsb->s_magic == MINIX_SUPER_MAGIC2) {
			printk(KERN_NOTICE
			       "%s: Minix filesystem found at block %d\n",
			       mtd->name, off / BLOCK_SIZE);
			goto done;
		}

		/* Try ext2 */
		if (ext2sb->s_magic == cpu_to_le16(EXT2_SUPER_MAGIC)) {
			printk(KERN_NOTICE
			       "%s: ext2 filesystem found at block %d\n",
			       mtd->name, off / BLOCK_SIZE);
			goto done;
		}
	}

	// uh, now what...
	printk(KERN_NOTICE "%s: Unable to find a valid linux partition\n", mtd->name);

 done:
	/* Find and size nvram */
	bcm947xx_parts[PART_NVRAM].offset = size - ROUNDUP(NVRAM_SPACE, mtd->erasesize);
	bcm947xx_parts[PART_NVRAM].size = size - bcm947xx_parts[PART_NVRAM].offset;

	/* Find and size rootfs */
	if (off < size) {
		bcm947xx_parts[PART_ROOTFS].offset = off;
		bcm947xx_parts[PART_ROOTFS].size = bcm947xx_parts[PART_NVRAM].offset - bcm947xx_parts[PART_ROOTFS].offset;
	}

	/* Size linux (kernel and rootfs) */
	bcm947xx_parts[PART_LINUX].size = bcm947xx_parts[PART_NVRAM].offset - bcm947xx_parts[PART_LINUX].offset;

	/* Size pmon */
	bcm947xx_parts[PART_BOOT].size = bcm947xx_parts[PART_LINUX].offset - bcm947xx_parts[PART_BOOT].offset;

	/* Find and size jffs2 -- nvram reserved + pmon */
	bcm947xx_parts[PART_JFFS2].size = (128*1024 - bcm947xx_parts[PART_NVRAM].size) + 
		(256*1024 - bcm947xx_parts[PART_BOOT].size);
	/* ... add unused space above 4MB */
	if (size > 0x400000) {
		if (trx_len <= 0x3a0000) // Small firmware - fixed amount
			bcm947xx_parts[PART_JFFS2].size += size - 0x400000;
		else {
			bcm947xx_parts[PART_JFFS2].size += size - (trx_len + 128*1024 + 256*1024);
			bcm947xx_parts[PART_JFFS2].size &= (~0xFFFFUL); // Round down 64K
		}
	}
	bcm947xx_parts[PART_JFFS2].offset = bcm947xx_parts[PART_NVRAM].offset - bcm947xx_parts[PART_JFFS2].size;
	if (bcm947xx_parts[PART_JFFS2].size <= 0) {
		bcm947xx_parts[PART_JFFS2].name = NULL;
		bcm947xx_parts[PART_JFFS2].size = 0;
	}
	else {
		bcm947xx_parts[PART_ROOTFS].size = bcm947xx_parts[PART_JFFS2].offset - bcm947xx_parts[PART_ROOTFS].offset;
	}
	
	return bcm947xx_parts;
}
#endif	// 0

EXPORT_SYMBOL(init_mtd_partitions);

#endif
