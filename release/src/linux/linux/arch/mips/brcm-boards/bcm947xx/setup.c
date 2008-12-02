/*
 * Generic setup routines for Broadcom MIPS boards
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: setup.c,v 1.7 2005/03/07 08:35:32 kanki Exp $
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/serialP.h>
#include <linux/ide.h>
#include <asm/bootinfo.h>
#include <asm/time.h>
#include <asm/reboot.h>

#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/minix_fs.h>
#include <linux/ext2_fs.h>
#include <linux/romfs_fs.h>
#include <linux/cramfs_fs.h>
#include <linux/squashfs_fs.h>
#endif

#include <typedefs.h>
#include <bcmutils.h>
#include <bcmnvram.h>
#include <sbmips.h>
#include <sbutils.h>
#include <trxhdr.h>

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

/* Global SB handle */
void *bcm947xx_sbh = NULL;
spinlock_t bcm947xx_sbh_lock = SPIN_LOCK_UNLOCKED;
EXPORT_SYMBOL(bcm947xx_sbh);
EXPORT_SYMBOL(bcm947xx_sbh_lock);

/* Convenience */
#define sbh bcm947xx_sbh
#define sbh_lock bcm947xx_sbh_lock

/* Kernel command line */
char arcs_cmdline[CL_SIZE] __initdata = CONFIG_CMDLINE;

void
bcm947xx_machine_restart(char *command)
{
	printk("Please stand by while rebooting the system...\n");

	/* Set the watchdog timer to reset immediately */
	__cli();
	sb_watchdog(sbh, 1);
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

static struct serial_struct rs = {
	line: 0,
	flags: ASYNC_BOOT_AUTOCONF,
	io_type: SERIAL_IO_MEM,
};

static void __init
serial_add(void *regs, uint irq, uint baud_base, uint reg_shift)
{
	rs.iomem_base = regs;
	rs.irq = irq + 2;
	rs.baud_base = baud_base / 16;
	rs.iomem_reg_shift = reg_shift;

	early_serial_setup(&rs);

	rs.line++;
}

static void __init
serial_setup(void *sbh)
{
	sb_serial_init(sbh, serial_add);

#ifdef CONFIG_REMOTE_DEBUG
	/* Use the last port for kernel debugging */
	if (rs.iomem_base)
		rs_kgdb_hook(&rs);
#endif
}

#endif /* CONFIG_SERIAL */

extern void check_enable_mips_pfc(int val);
void __init
brcm_setup(void)
{
	char *value;
	uint  pfc_val;

	/* Get global SB handle */
	sbh = sb_kattach();

	/* Initialize clocks and interrupts */
	sb_mips_init(sbh);

	/*
	 * Now that the sbh is inited set the  proper PFC value
	 */
	pfc_val = sb_mips_get_pfc(sbh);
	printk("Setting the PFC value as 0x%x\n", pfc_val);
	check_enable_mips_pfc(pfc_val);


#ifdef CONFIG_SERIAL
	/* Initialize UARTs */
	serial_setup(sbh);
#endif

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
	_machine_power_off = bcm947xx_machine_halt;

	board_time_init = bcm947xx_time_init;
	board_timer_setup = bcm947xx_timer_setup;
}

const char *
get_system_type(void)
{
	return "Broadcom BCM947XX";
}

void __init
bus_error_init(void)
{
}

#ifdef CONFIG_MTD_PARTITIONS


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
	+--------------+ 	< size - NVRAM_SPACE
	| nvram        |
	+--------------+	< size
*/

static struct mtd_partition bcm947xx_parts[] = {
	{ name: "pmon",	  offset: 0, size: 0, mask_flags: MTD_WRITEABLE, },
	{ name: "linux",  offset: 0, size: 0, },
	{ name: "rootfs", offset: 0, size: 0, mask_flags: MTD_WRITEABLE, },
	{ name: "jffs2",  offset: 0, size: 0, },
	{ name: "nvram",  offset: 0, size: 0, },
	{ name: NULL, },
};

#define PART_BOOT	0
#define PART_LINUX	1
#define PART_ROOTFS	2
#define PART_JFFS2	3
#define PART_NVRAM	4

struct mtd_partition * __init
init_mtd_partitions(struct mtd_info *mtd, size_t size)
{
	struct trx_header *trx;
	unsigned char buf[512];
	size_t off;
	size_t len;
	size_t trxsize;

	bcm947xx_parts[PART_NVRAM].offset = size - ROUNDUP(NVRAM_SPACE, mtd->erasesize);
	bcm947xx_parts[PART_NVRAM].size = size - bcm947xx_parts[PART_NVRAM].offset;

	trxsize = 0;
	trx = (struct trx_header *) buf;
	for (off = 0; off < size; off += mtd->erasesize) {
		if ((MTD_READ(mtd, off, sizeof(buf), &len, buf)) || (len != sizeof(buf))) continue;

		if (le32_to_cpu(trx->magic) == TRX_MAGIC) {
			bcm947xx_parts[PART_BOOT].size = off;

			bcm947xx_parts[PART_LINUX].offset = off;
			bcm947xx_parts[PART_LINUX].size = bcm947xx_parts[PART_NVRAM].offset - off;			
			
			trxsize = ROUNDUP(le32_to_cpu(trx->len), mtd->erasesize);	// kernel + rootfs

			bcm947xx_parts[PART_ROOTFS].offset = trx->offsets[1] + off;
			bcm947xx_parts[PART_ROOTFS].size = trxsize - trx->offsets[1];
			
			bcm947xx_parts[PART_JFFS2].offset = off + trxsize;
			bcm947xx_parts[PART_JFFS2].size = bcm947xx_parts[PART_NVRAM].offset - bcm947xx_parts[PART_JFFS2].offset;
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


EXPORT_SYMBOL(init_mtd_partitions);

#endif
