/*
 *  linux/include/asm-parisc/ide.h
 *
 *  Copyright (C) 1994-1996  Linus Torvalds & authors
 */

/*
 *  This file contains the PARISC architecture specific IDE code.
 */

#ifndef __ASM_PARISC_IDE_H
#define __ASM_PARISC_IDE_H

#ifdef __KERNEL__

#include <linux/config.h>
#include <asm/superio.h>

#ifndef MAX_HWIFS
#define MAX_HWIFS	2
#endif

static __inline__ int ide_default_irq(ide_ioreg_t base)
{
	switch (base) {
#ifdef CONFIG_SUPERIO
		case 0x1f0: 
		case 0x170:
			return superio_get_ide_irq();
#endif /* CONFIG_SUPERIO */
		default:
			return 0;
	}
}

static __inline__ ide_ioreg_t ide_default_io_base(int index)
{
	switch (index) {
#ifdef CONFIG_SUPERIO 
		case 0:	return (superio_get_ide_irq() ? 0x1f0 : 0);
		case 1:	return (superio_get_ide_irq() ? 0x170 : 0);
#endif /* CONFIG_SUPERIO */
		default:
			return 0;
	}
}

static __inline__ void ide_init_hwif_ports(hw_regs_t *hw, ide_ioreg_t data_port, ide_ioreg_t ctrl_port, int *irq)
{
	ide_ioreg_t reg = data_port;
	int i;

	for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++) {
		hw->io_ports[i] = reg;
		reg += 1;
	}
	if (ctrl_port) {
		hw->io_ports[IDE_CONTROL_OFFSET] = ctrl_port;
	} else {
		hw->io_ports[IDE_CONTROL_OFFSET] = hw->io_ports[IDE_DATA_OFFSET] + 0x206;
	}
	if (irq != NULL)
		*irq = 0;
	hw->io_ports[IDE_IRQ_OFFSET] = 0;
}

static __inline__ void ide_init_default_hwifs(void)
{
#ifndef CONFIG_BLK_DEV_IDEPCI
	hw_regs_t hw;
	int index;

	for(index = 0; index < MAX_HWIFS; index++) {
		ide_init_hwif_ports(&hw, ide_default_io_base(index), 0, NULL);
		hw.irq = ide_default_irq(ide_default_io_base(index));
		ide_register_hw(&hw, NULL);
	}
#endif /* CONFIG_BLK_DEV_IDEPCI */
}

#define ide_request_irq(irq,hand,flg,dev,id)	request_irq((irq),(hand),(flg),(dev),(id))
#define ide_free_irq(irq,dev_id)		free_irq((irq), (dev_id))
#define ide_check_region(from,extent)		check_region((from), (extent))
#define ide_request_region(from,extent,name)	request_region((from), (extent), (name))
#define ide_release_region(from,extent)		release_region((from), (extent))

#define T_CHAR          (0x0000)        /* char:  don't touch  */
#define T_SHORT         (0x4000)        /* short: 12 -> 21     */
#define T_INT           (0x8000)        /* int:   1234 -> 4321 */
#define T_TEXT          (0xc000)        /* text:  12 -> 21     */

#define T_MASK_TYPE     (0xc000)
#define T_MASK_COUNT    (0x3fff)

#define D_CHAR(cnt)     (T_CHAR  | (cnt))
#define D_SHORT(cnt)    (T_SHORT | (cnt))
#define D_INT(cnt)      (T_INT   | (cnt))
#define D_TEXT(cnt)     (T_TEXT  | (cnt))

static u_short driveid_types[] = {
	D_SHORT(10),	/* config - vendor2 */
	D_TEXT(20),	/* serial_no */
	D_SHORT(3),	/* buf_type - ecc_bytes */
	D_TEXT(48),	/* fw_rev - model */
	D_CHAR(2),	/* max_multsect - vendor3 */
	D_SHORT(1),	/* dword_io */
	D_CHAR(2),	/* vendor4 - capability */
	D_SHORT(1),	/* reserved50 */
	D_CHAR(4),	/* vendor5 - tDMA */
	D_SHORT(4),	/* field_valid - cur_sectors */
	D_INT(1),	/* cur_capacity */
	D_CHAR(2),	/* multsect - multsect_valid */
	D_INT(1),	/* lba_capacity */
	D_SHORT(194)	/* dma_1word - reservedyy */
};

#define num_driveid_types       (sizeof(driveid_types)/sizeof(*driveid_types))

static __inline__ void ide_fix_driveid(struct hd_driveid *id)
{
	u_char *p = (u_char *)id;
	int i, j, cnt;
	u_char t;

	for (i = 0; i < num_driveid_types; i++) {
		cnt = driveid_types[i] & T_MASK_COUNT;
		switch (driveid_types[i] & T_MASK_TYPE) {
		case T_CHAR:
			p += cnt;
			break;
		case T_SHORT:
			for (j = 0; j < cnt; j++) {
				t = p[0];
				p[0] = p[1];
				p[1] = t;
				p += 2;
			}
			break;
		case T_INT:
			for (j = 0; j < cnt; j++) {
				t = p[0];
				p[0] = p[3];
				p[3] = t;
				t = p[1];
				p[1] = p[2];
				p[2] = t;
				p += 4;
			}
			break;
		case T_TEXT:
			for (j = 0; j < cnt; j += 2) {
				t = p[0];
				p[0] = p[1];
				p[1] = t;
				p += 2;
			}
			break;
		};
	}
}

/*
 * The following are not needed for the non-m68k ports
 */
#define ide_ack_intr(hwif)		(1)
#define ide_release_lock(lock)		do {} while (0)
#define ide_get_lock(lock, hdlr, data)	do {} while (0)

#endif /* __KERNEL__ */

#endif /* __ASM_PARISC_IDE_H */
