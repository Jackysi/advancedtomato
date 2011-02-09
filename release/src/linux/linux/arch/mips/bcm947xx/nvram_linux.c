/*
 * NVRAM variable manipulation (Linux kernel half)
 *
 * Copyright 2006, Broadcom Corporation
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/bootmem.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mtd/mtd.h>
#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <typedefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <sbconfig.h>
#include <sbchipc.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <siutils.h>
#else	// K24
#include <linux/wrapper.h>
#include <sbutils.h>
#endif
#include <hndmips.h>
#include <sflash.h>
#include <linux/vmalloc.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <asm/cacheflush.h>
#include <bcmdefs.h>
#include <hndsoc.h>
#include <linux/nls.h>
#ifdef MODULE
/* This isn't right, but I can't figure out how to make the link error go away. */
#define flush_cache_all() do { hndcrc8(nvram_buf, sizeof(nvram_buf), 0); hndcrc8(nvram_buf, sizeof(nvram_buf), 0); } while (0)
#endif
#endif	// K26

MODULE_LICENSE("GPL");

#define KB * 1024

static int hdr_valid(struct nvram_header *header, int max);

/* In BSS to minimize text size and page aligned so it can be mmap()-ed */
/* Used for:
 *	In early... as nvram (read) staging buffer.
 *	In normal.. to hold the values of items.
 */
char nvram_buf[NVRAM_VAL_SIZE] __attribute__((aligned(PAGE_SIZE)));

/* This is the staging buffer for data going to/from the flash.
 * Also as work buffer for compactify.
 * It is large enough to hold all the NVRAM data and is 1 or more EBs is size.
 * The first chunk (before the nvram areaa) in the flash eb is preserved. */
unsigned char *nvram_commit_buf = NULL;
static int erasesize;	/* The size of flash eraseblock & commit_buf.
			 * 32KB rounded up to mtd->erasesise. (64KB or 128KB) */

int oflow_area_present = 0;

/* The nvram area is the last 32KB (or 60kb for E3000) of the last eraseblock of the flash chip.
 * Normally this is the mtdN partition named "nvram".
 * Normally this paritition is the entire last eraseblock. Do "cat /proc/mtd" to see this.
 * The first part of the last EB, from the start up to the NVRAM area, is
 * not used by pmon/cfe.
 * We use the next-to-last 32kb for overflow (extended) nvram area.  On a
 * smallish flash chip these 2 areas are the entire EB.  Some larger routers
 * have 128KB EB size, and on these the 1st 64KB of the last EB is unused.
 *
 * The implementation of this code is one main area of 32KB and one oflow area
 * of 32KB, for a total available nvram of 64KB.
 * Period.
 *
 * Some routers have pmon/cfe that uses 60KB for nvram.  On these, there is no
 * overflow area.  The total nvram area of these is 60kb.
 *
 */

#if NVRAM_SPACE != (32 * 1024)
#error	Attempt to redefine NVRAM_SPACE to something other than 32K.
#endif

/* This is the size of pmon/cfe (and for us: "main") nvram area.
 * Normally 32kb, but a few routers it is 60kb.
 */
int nvram_space = NVRAM_32K;	/* Determined at probe time. */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#define MTD_ERASE(mtd, args...) (*(mtd->erase))(mtd, args)
#define MTD_READ(mtd, args...) (*(mtd->read))(mtd, args)
#define MTD_WRITE(mtd, args...) (*(mtd->write))(mtd, args)

#define mem_map_reserve(a) SetPageReserved(a)
#define mem_map_unreserve(a) ClearPageReserved(a)

#define bcm947xx_sbh bcm947xx_sih
extern void *bcm947xx_sih;

#define sb_setcore	si_setcore
#define SB_CC		CC_CORE_ID
#define SB_FLASH2_SZ	SI_FLASH2_SZ
#define SB_FLASH1_SZ	SI_FLASH1_SZ
#define SB_FLASH1	SI_FLASH1
#define SB_FLASH2	SI_FLASH2
#define SB_BUS		SI_BUS
#define sb_setosh	si_setosh
#define sb_memc_get_ncdl	si_memc_get_ncdl

#define sih bcm947xx_sih
#endif	// KERNEL 2.6

#if 0
static int
nvram_valid(struct nvram_header *header)
{
	return 
	    header->magic == NVRAM_MAGIC &&
	    header->len >= sizeof(struct nvram_header) && 
	    header->len <= NVRAM_SPACE &&
#ifdef MIPSEB
	    1;	/* oleg -- no crc check for now */
#else
	    (header->crc_ver_init & 255) ==
		hndcrc8((char *) header + NVRAM_CRC_START_POSITION,
		header->len - NVRAM_CRC_START_POSITION, CRC8_INIT_VALUE);
#endif
}
#endif

#ifdef MODULE

#define early_nvram_get(name) nvram_get(name)
#define early_nvram_getall(name,c) _nvram_getall(name,c)
extern void *bcm947xx_sbh;   
#define sbh bcm947xx_sbh
#define sbh_lock bcm947xx_sbh_lock

#define NVR_DEVNAME "nvram2"
#define NVR_DEVNUM  1

#else /* !MODULE */
#define NVR_DEVNAME "nvram"
#define NVR_DEVNUM  0

/* Global SB handle */
extern void *bcm947xx_sbh;
extern spinlock_t bcm947xx_sbh_lock;

/* Convenience */
#define sbh bcm947xx_sbh
#define sbh_lock bcm947xx_sbh_lock

/* Early (before mm or mtd) read-only access to NVRAM */
/* Probe for NVRAM header */
static void
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
__init
#endif
early_nvram_init(void)
{
	struct nvram_header *header;
	chipcregs_t *cc;
	struct sflash *info = NULL;
	int i;
	int j;
	uint32 base, off, lim;
	u32 *src, *dst;

	if ((cc = sb_setcore(sbh, SB_CC, 0)) != NULL) {
		base = KSEG1ADDR(SB_FLASH2);
		switch (readl(&cc->capabilities) & CC_CAP_FLASH_MASK) {
		case PFLASH:
			lim = SB_FLASH2_SZ;
			break;

		case SFLASH_ST:
		case SFLASH_AT:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
			if ((info = sflash_init(sih, cc)) == NULL)
				return;
#else
			if ((info = sflash_init(cc)) == NULL)
				return;
#endif
			lim = info->size;
			break;

		case FLASH_NONE:
		default:
			return;
		}
	} else {
		/* extif assumed, Stop at 4 MB */
		base = KSEG1ADDR(SB_FLASH1);
		lim = SB_FLASH1_SZ;
	}

	/* XXX: hack for supporting the CFE environment stuff on WGT634U */
	src = (u32 *) KSEG1ADDR(base + 8 * 1024 * 1024 - 0x2000);
	dst = (u32 *) nvram_buf;
	if ((lim == 0x02000000) && ((*src & 0xff00ff) == 0x000001)) {
		printk("early_nvram_init: WGT634U NVRAM found.\n");

		for (i = 0; i < 0x1ff0; i++) {
			if (*src == 0xFFFFFFFF)
				break;
			*dst++ = *src++;
		}
		return;
	}

	off = FLASH_MIN;
	while (off <= lim) {
		/* Windowed flash access */
		j = 32 KB;
		header = (struct nvram_header *) KSEG1ADDR(base + off - NVRAM_32K);
		if (hdr_valid(header, NVRAM_32K))
			goto found;
		j = 4 KB;
		header = (struct nvram_header *) KSEG1ADDR(base + off - (NVRAM_32K + 28 KB));
		if (hdr_valid(header, NVRAM_32K + 28 KB))
			goto found;
		off <<= 1;
	}

	printk("Probing didn't find nvram, assuming 32K.\n");
	j = 32 KB;
	/* Try embedded NVRAM at 4 KB and 1 KB as last resorts */
	header = (struct nvram_header *) KSEG1ADDR(base + 4 KB);
	if (header->magic == NVRAM_MAGIC)
		goto found;
	
	header = (struct nvram_header *) KSEG1ADDR(base + 1 KB);
	if (header->magic == NVRAM_MAGIC)
		goto found;
	
	printk("early_nvram_init: NVRAM not found\n");
	return;

found:
	src = (u32 *) header;
	dst = (u32 *) nvram_buf;
	nvram_space = 64 KB - j;
	printk("early_nvram_init detected %d KB NVRAM area\n", nvram_space/1024);
	bzero(nvram_buf, sizeof(nvram_buf));
	for (i = 0; i < sizeof(struct nvram_header); i += 4)
		*dst++ = *src++;
	for (; i < header->len && i < nvram_space; i += 4)
		*dst++ = *src++;
}

/* Early (before mm or mtd) read-only access to NVRAM */
static char *
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
__init
#endif
early_nvram_get(const char *name)
{
	char *var, *value, *end, *eq;

	if (!name)
		return NULL;

	/* Too early? */
	if (sbh == NULL)
		return NULL;

	if (!nvram_buf[0])
		early_nvram_init();

	/* Look for name=value and return value */
	var = &nvram_buf[sizeof(struct nvram_header)];
	end = nvram_buf + sizeof(nvram_buf) - 2;
	end[0] = end[1] = '\0';
	for (; *var; var = value + strlen(value) + 1) {
		if (!(eq = strchr(var, '=')))
			break;
		value = eq + 1;
		if ((eq - var) == strlen(name) && strncmp(var, name, (eq - var)) == 0)
			return value;
	}

	return NULL;
}

static int
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
__init
#endif
early_nvram_getall(char *buf, int count)
{
	char *var, *end;
	int len = 0;
	
	/* Too early? */
	if (sbh == NULL)
		return -1;

	if (!nvram_buf[0])
		early_nvram_init();

	bzero(buf, count);

	/* Write name=value\0 ... \0\0 */
	var = &nvram_buf[sizeof(struct nvram_header)];
	end = nvram_buf + sizeof(nvram_buf) - 2;
	end[0] = end[1] = '\0';
	for (; *var; var += strlen(var) + 1) {
		if ((count - len) <= (strlen(var) + 1))
			break;
		len += sprintf(buf + len, "%s", var) + 1;
	}

	return 0;
}
#endif /* !MODULE */

extern char * _nvram_get(const char *name);
extern int _nvram_set(const char *name, const char *value);
extern int _nvram_unset(const char *name);
extern int _nvram_getall(char *buf, int count);
extern int _nvram_commit(struct nvram_header *header);
extern int _nvram_init(void *sbh);
extern void _nvram_exit(void);

/* Globals */
static spinlock_t nvram_lock = SPIN_LOCK_UNLOCKED;
static struct semaphore nvram_sem;
static int nvram_major = -1;
static struct mtd_info *nvram_mtd = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static struct class *nvram_class = NULL;
#else
static devfs_handle_t nvram_handle = NULL;
#endif

static int
hdr_valid(struct nvram_header *header, int max)
{
	return (header->magic == NVRAM_MAGIC &&
		header->len >= sizeof(struct nvram_header) && 
		header->len <= max &&
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
		(nvram_calc_crc(header) == (uint8) header->crc_ver_init));
#else
		(header->crc_ver_init & 255) ==
		hndcrc8((char *) header + NVRAM_CRC_START_POSITION,
		header->len - NVRAM_CRC_START_POSITION, CRC8_INIT_VALUE));
#endif
}

#if 0
int
_nvram_read(char *buf)
{
	struct nvram_header *header = (struct nvram_header *) buf;
	size_t len;

	if (!nvram_mtd ||
	    MTD_READ(nvram_mtd, nvram_mtd->size - nvram_space, nvram_space, &len, buf) ||
	    len != nvram_space ||
	    !nvram_valid(header)) {
		printk("_nvram_read: invalid nvram image\n");
		/* Maybe we can recover some data from early initialization */
		memcpy(buf, nvram_buf, nvram_space);
	}

	return 0;
}
#endif

/* Read the entire /dev/nvram block. Works only if EB is >= 32KB.
 * Uses the beginning of commit_buf.
 * Returns: < 0 for error.
 * >= 0 -- offset into nvram_commit_buf of the NVRAM header.
 * N.B.,  nvram_commit_buf[] has the last 64KB of the flash nvram partition.
 */
int
_nvram_init_read(void)
{
	size_t len;
	int j;
	int ret;
	struct nvram_header *header;
	u_int32_t offset;	/* fseek position of the last EB in the /mtd/nvram partition. */
	unsigned int i;

	if (!nvram_mtd) {
		printk("nvram_init: NVRAM not found\n");
		return -ENODEV;
	}

	oflow_area_present = 0;
	if (erasesize < 2 * NVRAM_32K)
		return -ENODEV;

	i = 2 * NVRAM_32K;
	/* seek offset to the last 64KB.  Normally 0. 64k on 128k EB size. */
	offset = nvram_mtd->size - i;
	len = 0;
	/* Read the last 64kb of flash */
	ret = MTD_READ(nvram_mtd, offset, i, &len, nvram_commit_buf);
	if (ret || len != i) {
		printk("nvram_init: read error ret = %d, len = %d/%d\n", ret, len, i);
		return -EIO;
	}
	/* Probe various spots to find the header. Every 4K down from 32K from the end.*/
	for (j = 32 KB; j >= 0 ; j -= 4 KB) {
		header = (struct nvram_header *)(nvram_commit_buf + j);
		if (hdr_valid(header, 64 KB - j))
			break;
	}
	if (j >= 0)
		printk("Probing found nvram header at %dK, size %dK\n", (j)/1024, (64 KB - j)/1024);
	else {
		printk("Probing didn't find nvram header. Assuming 32K\n");
		j = 32 KB;
	}
	nvram_space = 64 KB - j;
	return j;
}


/* Called in early initialization. */
int
nvram_init(void *sbh)
{
	return 0;
}

int
nvram_set(const char *name, const char *value)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&nvram_lock, flags);
	ret = _nvram_set(name, value);
	spin_unlock_irqrestore(&nvram_lock, flags);

	return ret;
}

char *
real_nvram_get(const char *name)
{
	unsigned long flags;
	char *value;

	spin_lock_irqsave(&nvram_lock, flags);
	value = _nvram_get(name);
	spin_unlock_irqrestore(&nvram_lock, flags);

	return value;
}

char *
nvram_get(const char *name)
{
	if (nvram_major >= 0)
		return real_nvram_get(name);
	else
		return early_nvram_get(name);
}

int
nvram_unset(const char *name)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&nvram_lock, flags);
	ret = _nvram_unset(name);
	spin_unlock_irqrestore(&nvram_lock, flags);

	return ret;
}

static void
erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *) done->priv;
	wake_up(wait_q);
}

int
nvram_commit(void)
{
	size_t erasesize, len, magic_len;
	unsigned int i;
	int ret;
	struct nvram_header *header;
	unsigned long flags;
	u_int32_t offset;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	struct erase_info erase;
	u_int32_t magic_offset = 0; /* Offset for writing MAGIC # */

	if (!nvram_mtd) {
		printk("nvram_commit: NVRAM not found\n");
		return -ENODEV;
	}

	if (in_interrupt()) {
		printk("nvram_commit: not committing in interrupt\n");
		return -EINVAL;
	}

	/* Backup sector blocks to be erased */
	erasesize = ROUNDUP(nvram_space, nvram_mtd->erasesize);
	down(&nvram_sem);

	//#warning no commit
	//_nvram_commit(nvram_commit_buf);  ret = -ENODEV; goto done;	//temp!!!

	if ((i = erasesize - nvram_space) > 0) {
		offset = nvram_mtd->size - erasesize;
		len = 0;
		ret = MTD_READ(nvram_mtd, offset, i, &len, nvram_commit_buf);
		if (ret || len != i) {
			printk("nvram_commit: read error ret = %d, len = %d/%d\n", ret, len, i);
			ret = -EIO;
			goto done;
		}
	}
	else {
		offset = nvram_mtd->size - nvram_space;
		i = 0;
	}
	header = (struct nvram_header *)(nvram_commit_buf + i);
	magic_offset = i + offsetof(struct nvram_header, magic);

	/* clear the existing magic # to mark the NVRAM as unusable 
	 * we can pull MAGIC bits low without erase
	 */
	header->magic = NVRAM_CLEAR_MAGIC; /* All zeros magic */

	/* Unlock sector blocks (for Intel 28F320C3B flash) , 20060309 */
	if (nvram_mtd->unlock)
		nvram_mtd->unlock(nvram_mtd, offset, nvram_mtd->erasesize);

	ret = MTD_WRITE(nvram_mtd, offset + magic_offset, sizeof(header->magic),
		&magic_len, (char *)&header->magic);
	if (ret || magic_len != sizeof(header->magic)) {
		printk("nvram_commit: clear MAGIC error\n");
		ret = -EIO;
		goto done;
	}

	/* reset MAGIC before we regenerate the NVRAM,
	 * otherwise we'll have an incorrect CRC
	 */
	header->magic = NVRAM_MAGIC;
	/* Regenerate NVRAM */
	spin_lock_irqsave(&nvram_lock, flags);
	ret = _nvram_commit(header);
	spin_unlock_irqrestore(&nvram_lock, flags);
	if (ret)
		goto done;

	/* Erase sector blocks */
	init_waitqueue_head(&wait_q);
	for (; offset < nvram_mtd->size - nvram_space + header->len; offset += nvram_mtd->erasesize) {
		erase.mtd = nvram_mtd;
		erase.addr = offset;
		erase.len = nvram_mtd->erasesize;
		erase.callback = erase_callback;
		erase.priv = (u_long) &wait_q;

		set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&wait_q, &wait);

		/* Unlock sector blocks */
		if (nvram_mtd->unlock)
			nvram_mtd->unlock(nvram_mtd, offset, nvram_mtd->erasesize);

		if ((ret = MTD_ERASE(nvram_mtd, &erase))) {
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			printk("nvram_commit: erase error\n");
			goto done;
		}

		/* Wait for erase to finish */
		schedule();
		remove_wait_queue(&wait_q, &wait);
	}

	/* Write partition up to end of data area */
	header->magic = NVRAM_INVALID_MAGIC; /* All ones magic */
	offset = nvram_mtd->size - erasesize;
	i = erasesize - nvram_space + header->len;
	ret = MTD_WRITE(nvram_mtd, offset, i, &len, nvram_commit_buf);
	if (ret || len != i) {
		printk("nvram_commit: write error\n");
		ret = -EIO;
		goto done;
	}

	/* Now mark the NVRAM in flash as "valid" by setting the correct MAGIC # */
	header->magic = NVRAM_MAGIC;
	ret = MTD_WRITE(nvram_mtd, offset + magic_offset, sizeof(header->magic),
		&magic_len, (char *)&header->magic);
	if (ret || magic_len != sizeof(header->magic)) {
		printk("nvram_commit: write MAGIC error\n");
		ret = -EIO;
		goto done;
	}

	/*
	 * Reading a few bytes back here will put the device
	 * back to the correct mode on certain flashes
	 */
	offset = nvram_mtd->size - erasesize;
	ret = MTD_READ(nvram_mtd, offset, 4, &len, nvram_commit_buf);

done:
	up(&nvram_sem);
	return ret;
}

int
nvram_getall(char *buf, int count)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&nvram_lock, flags);
	if (nvram_major >= 0)
		ret = _nvram_getall(buf, count);
	else
		ret = early_nvram_getall(buf, count);
	spin_unlock_irqrestore(&nvram_lock, flags);

	return ret;
}

#ifndef MODULE
EXPORT_SYMBOL(nvram_get);
EXPORT_SYMBOL(nvram_getall);
EXPORT_SYMBOL(nvram_set);
EXPORT_SYMBOL(nvram_unset);
EXPORT_SYMBOL(nvram_commit);
#endif

/* User mode interface below */

static ssize_t
dev_nvram_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	char tmp[100], *name = tmp, *value;
	ssize_t ret;
	unsigned long off;

	if (count > sizeof(tmp)) {
		if (!(name = kmalloc(count, GFP_KERNEL)))
			return -ENOMEM;
	}

	if (copy_from_user(name, buf, count)) {
		ret = -EFAULT;
		goto done;
	}

	if (*name == '\0') {
		/* Get all variables */
		ret = nvram_getall(name, count);
		if (ret == 0) {
			if (copy_to_user(buf, name, count)) {
				ret = -EFAULT;
				goto done;
			}
			ret = count;
		}
	} else {
		if (!(value = nvram_get(name))) {
			ret = 0;
			goto done;
		}

		/* Provide the offset into mmap() space */
		off = (unsigned long) value - (unsigned long) nvram_buf;

		if (put_user(off, (unsigned long *) buf)) {
			ret = -EFAULT;
			goto done;
		}

		ret = sizeof(char *);
	}

	flush_cache_all();

done:
	if (name != tmp)
		kfree(name);

	return ret;
}

static ssize_t
dev_nvram_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	char tmp[100], *name = tmp, *value;
	ssize_t ret;

	if (count > sizeof(tmp)) {
		if (!(name = kmalloc(count, GFP_KERNEL)))
			return -ENOMEM;
	}

	if (copy_from_user(name, buf, count)) {
		ret = -EFAULT;
		goto done;
	}

	value = name;
	name = strsep(&value, "=");
	if (value)
		ret = nvram_set(name, value) ? : count;
	else
		ret = nvram_unset(name) ? : count;

done:
	if (name != tmp)
		kfree(name);

	return ret;
}	

static int
dev_nvram_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	if (cmd != NVRAM_MAGIC)
		return -EINVAL;

	return nvram_commit();
}

#ifdef MODULE
/*  This maps the vmalloced module buffer to user space. */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static int do_vm_mmap(struct vm_area_struct *vma, char *adr, unsigned long siz)
{
	unsigned int start = vma->vm_start;
	int pfn;
	int ret;

	while (siz > 0) {
		pfn = vmalloc_to_pfn(adr);
		if ((ret = remap_pfn_range(vma, start, pfn, PAGE_SIZE, PAGE_READONLY)) < 0) {
			return ret;
		}
		start += PAGE_SIZE;
		adr += PAGE_SIZE;
		siz -= PAGE_SIZE;
	}

	return 0;
}

#else	// K24
/* From bttv-driver.c
 * Here we want the physical address of the memory.
 * This is used when initializing the contents of the
 * area and marking the pages as reserved.
 */
static inline unsigned long kvirt_to_pa(unsigned long adr) 
{
	unsigned long kva;

	kva = (unsigned long)page_address(vmalloc_to_page((void *)adr));
	kva |= adr & (PAGE_SIZE-1); /* restore the offset */
	return __pa(kva);
}

static int do_vm_mmap(struct vm_area_struct *vma, char *adr, unsigned long siz)
{
	unsigned int start = vma->vm_start;
	unsigned long page;

	while (siz > 0) {
		page = kvirt_to_pa((unsigned long)adr);
		if (remap_page_range(start, page, PAGE_SIZE, PAGE_READONLY))
			return -EAGAIN;
		start += PAGE_SIZE;
		adr += PAGE_SIZE;
		siz -= PAGE_SIZE;
	}
	return 0;
}
#endif
#endif	// MODULE

static int
dev_nvram_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long siz = vma->vm_end - vma->vm_start;

	if (siz > NVRAM_VAL_SIZE) siz = NVRAM_VAL_SIZE;
#ifdef MODULE
	return (do_vm_mmap(vma, nvram_buf, siz));
#else	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	if (remap_pfn_range(vma, vma->vm_start,
			    __pa(nvram_buf) >> PAGE_SHIFT,
			    siz, vma->vm_page_prot))
		return -EAGAIN;
#else
	if (remap_page_range(vma->vm_start, virt_to_phys(nvram_buf),
			     siz, vma->vm_page_prot))
		return -EAGAIN;
#endif
	return 0;
#endif
}

static int
dev_nvram_open(struct inode *inode, struct file * file)
{
	MOD_INC_USE_COUNT;
	return 0;
}

static int
dev_nvram_release(struct inode *inode, struct file * file)
{
	MOD_DEC_USE_COUNT;
	return 0;
}

static struct file_operations dev_nvram_fops = {
	owner:		THIS_MODULE,
	open:		dev_nvram_open,
	release:	dev_nvram_release,
	read:		dev_nvram_read,
	write:		dev_nvram_write,
	ioctl:		dev_nvram_ioctl,
	mmap:		dev_nvram_mmap
};

static void
dev_nvram_exit(void)
{
#ifndef MODULE
	int order = 0;
	struct page *page, *end;
#else
	char *adr = nvram_buf;
	int size = NVRAM_VAL_SIZE;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	if (nvram_handle)
		devfs_unregister(nvram_handle);

	if (nvram_major >= 0)
		devfs_unregister_chrdev(nvram_major, NVR_DEVNAME);
#else	// K26
	if (nvram_class) {
		class_device_destroy(nvram_class, MKDEV(nvram_major, NVR_DEVNUM));
		class_destroy(nvram_class);
	}

	if (nvram_major >= 0)
		unregister_chrdev(nvram_major, NVR_DEVNAME);
#endif
	if (nvram_mtd)
		put_mtd_device(nvram_mtd);

#ifndef MODULE
	while ((PAGE_SIZE << order) < NVRAM_VAL_SIZE)
		order++;
	end = virt_to_page(nvram_buf + (PAGE_SIZE << order) - 1);
	for (page = virt_to_page(nvram_buf); page <= end; page++)
		mem_map_unreserve(page);
#else
	while (size > 0) {
		mem_map_unreserve(vmalloc_to_page((void *)adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
#endif
	_nvram_exit();
	vfree(nvram_commit_buf);
}

static int
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
__init
#endif
dev_nvram_init(void)
{
	int ret = 0;
	unsigned int i;
	osl_t *osh;
#ifndef MODULE
	int order = 0;
	struct page *page, *end;
#else
	char *adr = nvram_buf;
	int size = NVRAM_VAL_SIZE;
#endif

	//printk("---------------------------------------------------------\n");
	printk("----nvram loading -----" __DATE__ " " __TIME__ " --------\n");
#ifndef MODULE
	/* Allocate and reserve memory to mmap() */
	while ((PAGE_SIZE << order) < NVRAM_VAL_SIZE)
		order++;
	end = virt_to_page(nvram_buf + (PAGE_SIZE << order) - 1);
	for (page = virt_to_page(nvram_buf); page <= end; page++)
		mem_map_reserve(page);
#else
	while (size > 0) {
		mem_map_reserve(vmalloc_to_page((void *)adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
#endif
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#ifdef CONFIG_MTD
	/* Find associated MTD device */
	for (i = 0; i < MAX_MTD_DEVICES; i++) {
		nvram_mtd = get_mtd_device(NULL, i);
		if (nvram_mtd) {
			if (!strcmp(nvram_mtd->name, "nvram") &&
			    nvram_mtd->size >= NVRAM_32K)
				break;
			put_mtd_device(nvram_mtd);
		}
	}
	if (i >= MAX_MTD_DEVICES)
		nvram_mtd = NULL;
#endif

	/* Initialize hash table lock */
	spin_lock_init(&nvram_lock);

	/* Initialize commit semaphore */
	init_MUTEX(&nvram_sem);

	/* Register char device */
	if ((nvram_major = devfs_register_chrdev(0, NVR_DEVNAME, &dev_nvram_fops)) < 0) {
		ret = nvram_major;
		goto err;
	}

	if (sb_osh(sbh) == NULL) {
		osh = osl_attach(NULL, SB_BUS, FALSE);
		if (osh == NULL) {
			printk("Error allocating osh\n");
			goto err;
		}
		sb_setosh(sbh, osh);
	}

	/* Create /dev/nvram handle */
	nvram_handle = devfs_register(NULL, NVR_DEVNAME, DEVFS_FL_NONE, nvram_major, NVR_DEVNUM,
				      S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP, &dev_nvram_fops, NULL);

#else	// KERNEL 2.6 
#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
	/* Find associated MTD device */
	for (i = 0; i < MAX_MTD_DEVICES; i++) {
		nvram_mtd = get_mtd_device(NULL, i);
		if (!IS_ERR(nvram_mtd)) {
			if (!strcmp(nvram_mtd->name, "nvram") &&
			    nvram_mtd->size >= NVRAM_32K) {
				break;
			}
			put_mtd_device(nvram_mtd);
		}
	}
	if (i >= MAX_MTD_DEVICES)
		nvram_mtd = NULL;
#endif

	/* Initialize hash table lock */
	spin_lock_init(&nvram_lock);

	/* Initialize commit semaphore */
	init_MUTEX(&nvram_sem);

	/* Register char device */
	if ((nvram_major = register_chrdev(0, NVR_DEVNAME, &dev_nvram_fops)) < 0) {
		ret = nvram_major;
		goto err;
	}

	if (si_osh(sih) == NULL) {
		osh = osl_attach(NULL, SI_BUS, FALSE);
		if (osh == NULL) {
			printk("Error allocating osh\n");
			unregister_chrdev(nvram_major, NVR_DEVNAME);
			goto err;
		}
		si_setosh(sih, osh);
	}

	/* Create /dev/nvram handle */
	nvram_class = class_create(THIS_MODULE, NVR_DEVNAME);
	if (IS_ERR(nvram_class)) {
		printk("Error creating nvram class\n");
		goto err;
	}

	/* Add the device nvram0 */
	class_device_create(nvram_class, NULL, MKDEV(nvram_major, NVR_DEVNUM), NULL, NVR_DEVNAME);
#endif

	/* reserve commit read buffer */
	/* Backup sector blocks to be erased */
	erasesize = ROUNDUP(NVRAM_VAL_SIZE, nvram_mtd->erasesize);
	if (!(nvram_commit_buf = vmalloc(erasesize))) {
		printk("dev_nvram_init: nvram_commit_buf out of memory\n");
		goto err;
	}

	/* Initialize the in-memory database */
	_nvram_init(sbh);

	/* Set the SDRAM NCDL value into NVRAM if not already done */
	if (getintvar(NULL, "sdram_ncdl") == 0) {
		unsigned int ncdl;
		char buf[16];

		if ((ncdl = sb_memc_get_ncdl(sbh))) {
			sprintf(buf, "0x%08x", ncdl);
			nvram_set("sdram_ncdl", buf);
			nvram_commit();
		}
	}

	return 0;
err:
	dev_nvram_exit();
	return ret;
}

module_init(dev_nvram_init);
module_exit(dev_nvram_exit);

/* For the emacs code formatting
Local Variables:
   c-basic-offset: 8
End:
*/
