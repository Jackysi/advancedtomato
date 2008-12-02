/* $Id: ide.h,v 1.1.1.4 2003/10/14 08:09:23 sparq Exp $
 * ide.h: Ultra/PCI specific IDE glue.
 *
 * Copyright (C) 1997  David S. Miller (davem@caip.rutgers.edu)
 * Copyright (C) 1998  Eddie C. Dost   (ecd@skynet.be)
 */

#ifndef _SPARC64_IDE_H
#define _SPARC64_IDE_H

#ifdef __KERNEL__

#include <linux/config.h>
#include <asm/pgalloc.h>
#include <asm/io.h>
#include <asm/hdreg.h>
#include <asm/page.h>
#include <asm/spitfire.h>

#undef  MAX_HWIFS
#define MAX_HWIFS	2

#define	ide__sti()	__sti()

static __inline__ int ide_default_irq(ide_ioreg_t base)
{
	return 0;
}

static __inline__ ide_ioreg_t ide_default_io_base(int index)
{
	return 0;
}

static __inline__ void ide_init_hwif_ports(hw_regs_t *hw, ide_ioreg_t data_port, ide_ioreg_t ctrl_port, int *irq)
{
	ide_ioreg_t reg =  data_port;
	int i;

	for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++) {
		hw->io_ports[i] = reg;
		reg += 1;
	}
	if (ctrl_port) {
		hw->io_ports[IDE_CONTROL_OFFSET] = ctrl_port;
	} else {
		hw->io_ports[IDE_CONTROL_OFFSET] = 0;
	}
	if (irq != NULL)
		*irq = 0;
	hw->io_ports[IDE_IRQ_OFFSET] = 0;
}

/*
 * This registers the standard ports for this architecture with the IDE
 * driver.
 */
static __inline__ void ide_init_default_hwifs(void)
{
#ifndef CONFIG_BLK_DEV_IDEPCI
	hw_regs_t hw;
	int index;

	for (index = 0; index < MAX_HWIFS; index++) {
		ide_init_hwif_ports(&hw, ide_default_io_base(index), 0, NULL);
		hw.irq = ide_default_irq(ide_default_io_base(index));
		ide_register_hw(&hw, NULL);
	}
#endif /* CONFIG_BLK_DEV_IDEPCI */
}

typedef union {
	unsigned int		all	: 8;	/* all of the bits together */
	struct {
		unsigned int	bit7	: 1;
		unsigned int	lba	: 1;
		unsigned int	bit5	: 1;
		unsigned int	unit	: 1;
		unsigned int	head	: 4;
	} b;
} select_t;

typedef union {
	unsigned int all		: 8;	/* all of the bits together */
	struct {
		unsigned int HOB	: 1;	/* 48-bit address ordering */
		unsigned int reserved456: 3;
		unsigned bit3		: 1;	/* ATA-2 thingy */
		unsigned int SRST	: 1;	/* host soft reset bit */
		unsigned int nIEN	: 1;	/* device INTRQ to host */
		unsigned int bit0	: 1;
	} b;
} control_t;

static __inline__ int ide_request_irq(unsigned int irq,
				      void (*handler)(int, void *, struct pt_regs *),
				      unsigned long flags, const char *name, void *devid)
{
	return request_irq(irq, handler, SA_SHIRQ, name, devid);
}

static __inline__ void ide_free_irq(unsigned int irq, void *dev_id)
{
	free_irq(irq, dev_id);
}

static __inline__ int ide_check_region(ide_ioreg_t base, unsigned int size)
{
	return check_region(base, size);
}

static __inline__ void ide_request_region(ide_ioreg_t base, unsigned int size,
					  const char *name)
{
	request_region(base, size, name);
}

static __inline__ void ide_release_region(ide_ioreg_t base, unsigned int size)
{
	release_region(base, size);
}

#undef  SUPPORT_SLOW_DATA_PORTS
#define SUPPORT_SLOW_DATA_PORTS 0

#undef  SUPPORT_VLB_SYNC
#define SUPPORT_VLB_SYNC 0

#undef  HD_DATA
#define HD_DATA ((ide_ioreg_t)0)

/* From m68k code... */

#ifdef insl
#undef insl
#endif
#ifdef outsl
#undef outsl
#endif
#ifdef insw
#undef insw
#endif
#ifdef outsw
#undef outsw
#endif

#define insl(data_reg, buffer, wcount) insw(data_reg, buffer, (wcount)<<1)
#define outsl(data_reg, buffer, wcount) outsw(data_reg, buffer, (wcount)<<1)

#define insw(port, buf, nr) ide_insw((port), (buf), (nr))
#define outsw(port, buf, nr) ide_outsw((port), (buf), (nr))

static __inline__ unsigned int inw_be(unsigned long addr)
{
	unsigned int ret;

	__asm__ __volatile__("lduha [%1] %2, %0"
			     : "=r" (ret)
			     : "r" (addr), "i" (ASI_PHYS_BYPASS_EC_E));

	return ret;
}

static __inline__ void ide_insw(unsigned long port,
				void *dst,
				unsigned long count)
{
#if (L1DCACHE_SIZE > PAGE_SIZE)		    /* is there D$ aliasing problem */
	unsigned long end = (unsigned long)dst + (count << 1);
#endif
	u16 *ps = dst;
	u32 *pi;

	if(((u64)ps) & 0x2) {
		*ps++ = inw_be(port);
		count--;
	}
	pi = (u32 *)ps;
	while(count >= 2) {
		u32 w;

		w  = inw_be(port) << 16;
		w |= inw_be(port);
		*pi++ = w;
		count -= 2;
	}
	ps = (u16 *)pi;
	if(count)
		*ps++ = inw_be(port);

#if (L1DCACHE_SIZE > PAGE_SIZE)		    /* is there D$ aliasing problem */
	__flush_dcache_range((unsigned long)dst, end);
#endif
}

static __inline__ void outw_be(unsigned short w, unsigned long addr)
{
	__asm__ __volatile__("stha %0, [%1] %2"
			     : /* no outputs */
			     : "r" (w), "r" (addr), "i" (ASI_PHYS_BYPASS_EC_E));
}

static __inline__ void ide_outsw(unsigned long port,
				 const void *src,
				 unsigned long count)
{
#if (L1DCACHE_SIZE > PAGE_SIZE)		    /* is there D$ aliasing problem */
	unsigned long end = (unsigned long)src + (count << 1);
#endif
	const u16 *ps = src;
	const u32 *pi;

	if(((u64)src) & 0x2) {
		outw_be(*ps++, port);
		count--;
	}
	pi = (const u32 *)ps;
	while(count >= 2) {
		u32 w;

		w = *pi++;
		outw_be((w >> 16), port);
		outw_be(w, port);
		count -= 2;
	}
	ps = (const u16 *)pi;
	if(count)
		outw_be(*ps, port);

#if (L1DCACHE_SIZE > PAGE_SIZE)		    /* is there D$ aliasing problem */
	__flush_dcache_range((unsigned long)src, end);
#endif
}

static __inline__ void ide_fix_driveid(struct hd_driveid *id)
{
	int i;
	u16 *stringcast;

	id->config         = __le16_to_cpu(id->config);
	id->cyls           = __le16_to_cpu(id->cyls);
	id->reserved2      = __le16_to_cpu(id->reserved2);
	id->heads          = __le16_to_cpu(id->heads);
	id->track_bytes    = __le16_to_cpu(id->track_bytes);
	id->sector_bytes   = __le16_to_cpu(id->sector_bytes);
	id->sectors        = __le16_to_cpu(id->sectors);
	id->vendor0        = __le16_to_cpu(id->vendor0);
	id->vendor1        = __le16_to_cpu(id->vendor1);
	id->vendor2        = __le16_to_cpu(id->vendor2);
	stringcast = (u16 *)&id->serial_no[0];
	for (i = 0; i < (20/2); i++)
	        stringcast[i] = __le16_to_cpu(stringcast[i]);
	id->buf_type       = __le16_to_cpu(id->buf_type);
	id->buf_size       = __le16_to_cpu(id->buf_size);
	id->ecc_bytes      = __le16_to_cpu(id->ecc_bytes);
	stringcast = (u16 *)&id->fw_rev[0];
	for (i = 0; i < (8/2); i++)
	        stringcast[i] = __le16_to_cpu(stringcast[i]);
	stringcast = (u16 *)&id->model[0];
	for (i = 0; i < (40/2); i++)
	        stringcast[i] = __le16_to_cpu(stringcast[i]);
	id->dword_io       = __le16_to_cpu(id->dword_io);
	id->reserved50     = __le16_to_cpu(id->reserved50);
	id->field_valid    = __le16_to_cpu(id->field_valid);
	id->cur_cyls       = __le16_to_cpu(id->cur_cyls);
	id->cur_heads      = __le16_to_cpu(id->cur_heads);
	id->cur_sectors    = __le16_to_cpu(id->cur_sectors);
	id->cur_capacity0  = __le16_to_cpu(id->cur_capacity0);
	id->cur_capacity1  = __le16_to_cpu(id->cur_capacity1);
	id->lba_capacity   = __le32_to_cpu(id->lba_capacity);
	id->dma_1word      = __le16_to_cpu(id->dma_1word);
	id->dma_mword      = __le16_to_cpu(id->dma_mword);
	id->eide_pio_modes = __le16_to_cpu(id->eide_pio_modes);
	id->eide_dma_min   = __le16_to_cpu(id->eide_dma_min);
	id->eide_dma_time  = __le16_to_cpu(id->eide_dma_time);
	id->eide_pio       = __le16_to_cpu(id->eide_pio);
	id->eide_pio_iordy = __le16_to_cpu(id->eide_pio_iordy);
	for (i = 0; i < 2; i++)
		id->words69_70[i] = __le16_to_cpu(id->words69_70[i]);
        for (i = 0; i < 4; i++)
                id->words71_74[i] = __le16_to_cpu(id->words71_74[i]);
	id->queue_depth	   = __le16_to_cpu(id->queue_depth);
	for (i = 0; i < 4; i++)
		id->words76_79[i] = __le16_to_cpu(id->words76_79[i]);
	id->major_rev_num  = __le16_to_cpu(id->major_rev_num);
	id->minor_rev_num  = __le16_to_cpu(id->minor_rev_num);
	id->command_set_1  = __le16_to_cpu(id->command_set_1);
	id->command_set_2  = __le16_to_cpu(id->command_set_2);
	id->cfsse          = __le16_to_cpu(id->cfsse);
	id->cfs_enable_1   = __le16_to_cpu(id->cfs_enable_1);
	id->cfs_enable_2   = __le16_to_cpu(id->cfs_enable_2);
	id->csf_default    = __le16_to_cpu(id->csf_default);
	id->dma_ultra      = __le16_to_cpu(id->dma_ultra);
	id->word89         = __le16_to_cpu(id->word89);
	id->word90         = __le16_to_cpu(id->word90);
	id->CurAPMvalues   = __le16_to_cpu(id->CurAPMvalues);
	id->word92         = __le16_to_cpu(id->word92);
	id->hw_config      = __le16_to_cpu(id->hw_config);
	id->acoustic       = __le16_to_cpu(id->acoustic);
	for (i = 0; i < 5; i++)
		id->words95_99[i]  = __le16_to_cpu(id->words95_99[i]);
	id->lba_capacity_2 = __le64_to_cpu(id->lba_capacity_2);
	for (i = 0; i < 22; i++)
		id->words104_125[i]   = __le16_to_cpu(id->words104_125[i]);
	id->last_lun       = __le16_to_cpu(id->last_lun);
	id->word127        = __le16_to_cpu(id->word127);
	id->dlf            = __le16_to_cpu(id->dlf);
	id->csfo           = __le16_to_cpu(id->csfo);
	for (i = 0; i < 26; i++)
		id->words130_155[i] = __le16_to_cpu(id->words130_155[i]);
	id->word156        = __le16_to_cpu(id->word156);
	for (i = 0; i < 3; i++)
		id->words157_159[i] = __le16_to_cpu(id->words157_159[i]);
	id->cfa_power      = __le16_to_cpu(id->cfa_power);
	for (i = 0; i < 14; i++)
		id->words161_175[i] = __le16_to_cpu(id->words161_175[i]);
	for (i = 0; i < 31; i++)
		id->words176_205[i] = __le16_to_cpu(id->words176_205[i]);
	for (i = 0; i < 48; i++)
		id->words206_254[i] = __le16_to_cpu(id->words206_254[i]);
	id->integrity_word  = __le16_to_cpu(id->integrity_word);
}

/*
 * The following are not needed for the non-m68k ports
 */
#define ide_ack_intr(hwif)		(1)
#define ide_release_lock(lock)		do {} while (0)
#define ide_get_lock(lock, hdlr, data)	do {} while (0)

#endif /* __KERNEL__ */

#endif /* _SPARC64_IDE_H */
