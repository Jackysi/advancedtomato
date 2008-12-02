/*
 *  linux/drivers/ide/pdc202xx.c	Version 0.32	Feb. 27, 2002
 *
 *  Copyright (C) 1998-2000	Andre Hedrick <andre@linux-ide.org>
 *  May be copied or modified under the terms of the GNU General Public License
 *
 *  Promise Ultra66 cards with BIOS v1.11 this
 *  compiled into the kernel if you have more than one card installed.
 *
 *  Promise Ultra100 cards with BIOS v2.01 this
 *  compiled into the kernel if you have more than one card installed.
 *
 *  Promise Ultra100TX2 with BIOS v2.10 & Ultra133TX2 with BIOS v2.20
 *  support 8 hard drives on UDMA mode.
 *
 *  Linux kernel will misunderstand FastTrak ATA-RAID series as Ultra
 *  IDE Controller, UNLESS you enable "CONFIG_PDC202XX_FORCE"
 *  That's you can use FastTrak ATA-RAID controllers as IDE controllers.
 *
 *  History :
 *  05/22/01    v1.20 b1
 *           (1) support PDC20268
 *           (2) fix cable judge function
 *  08/22/01    v1.20 b2
 *           (1) support ATA-133 PDC20269/75
 *           (2) support UDMA Mode 6
 *           (3) fix proc report information
 *           (4) set ATA133 timing
 *           (5) fix ultra dma bit 14 selectable
 *           (6) support 32bit LBA
 *  09/11/01    v1.20 b3 
 *           (1) fix eighty_ninty_three()
 *           (2) fix offset address 0x1c~0x1f
 *  10/30/01    v1.20 b4
 *           (1) fix 48bit LBA HOB bit
 *           (2) force rescan drive under PIO modes if need
 *  11/02/01    v1.20.0.5
 *           (1) could be patched with ext3 filesystem code
 *  11/06/01    v1.20.0.6
 *           (1) fix LBA48 drive running without Promise controllers
 *           (2) fix LBA48 drive running under PIO modes
 *  01/28/02    v1.20.0.6
 *           (1) release for linux IDE Group kernel 2.4.18
 *           (2) add version and controller info to proc
 *  05/23/02    v1.20.0.7
 *           (1) disable PDC20262 running with 48bit
 *           (2) Add quirk drive lists for PDC20265/67
 *
 *  Copyright (C) 1999-2002 Promise Technology, Inc.
 *  Author: Frank Tiernan <frankt@promise.com>
 *          PROMISE pdc202xx IDE Controller driver MAINTAINERS
 *  Released under terms of General Public License
 */
 
#define VERSION	"1.20.0.7"
#define VERDATE "2002-05-23"
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/ide.h>

#include <asm/io.h>
#include <asm/irq.h>

#include "ide_modes.h"

#define PDC202XX_DEBUG_DRIVE_INFO		0
#define PDC202XX_DECODE_REGISTER_INFO		0

#define DISPLAY_PDC202XX_TIMINGS

#ifndef SPLIT_BYTE
#define SPLIT_BYTE(B,H,L)	((H)=(B>>4), (L)=(B-((B>>4)<<4)))
#endif

#if defined(DISPLAY_PDC202XX_TIMINGS) && defined(CONFIG_PROC_FS)
#include <linux/stat.h>
#include <linux/proc_fs.h>

static int pdc202xx_get_info(char *, char **, off_t, int);
extern int (*pdc202xx_display_info)(char *, char **, off_t, int); /* ide-proc.c */
extern char *ide_media_verbose(ide_drive_t *);
static struct pci_dev *bmide_dev;
static struct hd_driveid *id[4];
static int speed_rate[4];

static char * pdc202xx_info (char *buf, struct pci_dev *dev)
{
	char *p = buf;

	u32 bibma  = (u32)ioremap_nocache(pci_resource_start(dev, 5), 64);
	u32 reg60h = 0, reg64h = 0, reg68h = 0, reg6ch = 0;
	u16 reg50h = 0;
	u16 word88 = 0;
	int udmasel[4] = {0,0,0,0}, piosel[4] = {0,0,0,0};
	int i = 0, hd = 0;

        /*
         * at that point bibma+0x2 et bibma+0xa are byte registers
         * to investigate:
         */
	u8 c0	= readb(bibma + 0x02);
	u8 c1	= readb(bibma + 0x0a);

	u8 sc11	= readb(bibma + 0x11);
	u8 sc1a	= readb(bibma + 0x1a);
	u8 sc1b	= readb(bibma + 0x1b);
	/* u8 sc1c	= readb(bibma + 0x1c); 
	u8 sc1d	= readb(bibma + 0x1d);
	u8 sc1e	= readb(bibma + 0x1e);
	u8 sc1f	= readb(bibma + 0x1f); */

	iounmap((void*)bibma);

	pci_read_config_word(dev, 0x50, &reg50h);
	pci_read_config_dword(dev, 0x60, &reg60h);
	pci_read_config_dword(dev, 0x64, &reg64h);
	pci_read_config_dword(dev, 0x68, &reg68h);
	pci_read_config_dword(dev, 0x6c, &reg6ch);

	p+=sprintf(p, "\nPROMISE Ultra series driver Ver %s %s Adapter: ", VERSION, VERDATE);
	switch(dev->device) {
		case PCI_DEVICE_ID_PROMISE_20275:
			p += sprintf(p, "MBUltra133\n");
			break;
		case PCI_DEVICE_ID_PROMISE_20269:
			p += sprintf(p, "Ultra133 TX2\n");
			break;
		case PCI_DEVICE_ID_PROMISE_20268:
			p += sprintf(p, "Ultra100 TX2\n");
			break;
		case PCI_DEVICE_ID_PROMISE_20267:
			p += sprintf(p, "Ultra100\n");
			break;
		case PCI_DEVICE_ID_PROMISE_20265:
			p += sprintf(p, "Ultra100 on M/B\n");
			break;
		case PCI_DEVICE_ID_PROMISE_20262:
			p += sprintf(p, "Ultra66\n");
			break;
		case PCI_DEVICE_ID_PROMISE_20246:
			p += sprintf(p, "Ultra33\n");
			reg50h |= 0x0c00;
			break;
		default:
			p += sprintf(p, "Ultra Series\n");
			break;
	}

	p += sprintf(p, "--------------- Primary Channel ---------------- Secondary Channel -------------\n");
	p += sprintf(p, "                %s                         %s\n",
		(c0&0x80)?"disabled":"enabled ",
		(c1&0x80)?"disabled":"enabled ");
	p += sprintf(p, "66 Clocking     %s                         %s\n",
		(sc11&0x02)?"enabled ":"disabled",
		(sc11&0x08)?"enabled ":"disabled");
	p += sprintf(p, "Mode            %s                           %s\n",
		(sc1a & 0x01) ? "MASTER" : "PCI   ",
		(sc1b & 0x01) ? "MASTER" : "PCI   ");
	p += sprintf(p, "--------------- drive0 --------- drive1 -------- drive0 ---------- drive1 ------\n");
	p += sprintf(p, "DMA enabled:    %s              %s             %s               %s\n",
		(id[0]!=NULL && (c0&0x20))?"yes":"no ",(id[1]!=NULL && (c0&0x40))?"yes":"no ",
		(id[2]!=NULL && (c1&0x20))?"yes":"no ",(id[3]!=NULL && (c1&0x40))?"yes":"no ");
	for( hd = 0; hd < 4 ; hd++) {
		if (id[hd] == NULL)
			continue;
		word88 = id[hd]->dma_ultra;
		for ( i = 7 ; i >= 0 ; i--)
			if (word88 >> (i+8)) {
				udmasel[hd] = i;	/* get select UDMA mode */
				break;
			}
		piosel[hd] = (id[hd]->eide_pio_modes >= 0x02) ? 4 : 3;
        }
	p += sprintf(p, "UDMA Mode:      %d                %d               %d                 %d\n",
		udmasel[0], udmasel[1], udmasel[2], udmasel[3]);
	p += sprintf(p, "PIO Mode:       %d                %d               %d                 %d\n",
		piosel[0], piosel[1], piosel[2], piosel[3]);
	return (char *)p;
}

static int pdc202xx_get_info (char *buffer, char **addr, off_t offset, int count)
{
	char *p = buffer;
	
	p = pdc202xx_info(buffer, bmide_dev);
	return p-buffer;	/* => must be less than 4k! */
}
#endif  /* defined(DISPLAY_PDC202XX_TIMINGS) && defined(CONFIG_PROC_FS) */

byte pdc202xx_proc = 0;

const char *pdc_quirk_drives[] = {
	"QUANTUM FIREBALLlct08 08",
	"QUANTUM FIREBALLP KA6.4",
	"QUANTUM FIREBALLP KA9.1",
	"QUANTUM FIREBALLP LM20.4",
	"QUANTUM FIREBALLP KX13.6",
	"QUANTUM FIREBALLP KX20.5",
	"QUANTUM FIREBALLP KX27.3",
	"QUANTUM FIREBALLP LM20.5",
	NULL
};

extern char *ide_xfer_verbose (byte xfer_rate);

/* A Register */
#define	SYNC_ERRDY_EN	0xC0

#define	SYNC_IN		0x80	/* control bit, different for master vs. slave drives */
#define	ERRDY_EN	0x40	/* control bit, different for master vs. slave drives */
#define	IORDY_EN	0x20	/* PIO: IOREADY */
#define	PREFETCH_EN	0x10	/* PIO: PREFETCH */

#define	PA3		0x08	/* PIO"A" timing */
#define	PA2		0x04	/* PIO"A" timing */
#define	PA1		0x02	/* PIO"A" timing */
#define	PA0		0x01	/* PIO"A" timing */

/* B Register */

#define	MB2		0x80	/* DMA"B" timing */
#define	MB1		0x40	/* DMA"B" timing */
#define	MB0		0x20	/* DMA"B" timing */

#define	PB4		0x10	/* PIO_FORCE 1:0 */

#define	PB3		0x08	/* PIO"B" timing */	/* PIO flow Control mode */
#define	PB2		0x04	/* PIO"B" timing */	/* PIO 4 */
#define	PB1		0x02	/* PIO"B" timing */	/* PIO 3 half */
#define	PB0		0x01	/* PIO"B" timing */	/* PIO 3 other half */

/* C Register */
#define	IORDYp_NO_SPEED	0x4F
#define	SPEED_DIS	0x0F

#define	DMARQp		0x80
#define	IORDYp		0x40
#define	DMAR_EN		0x20
#define	DMAW_EN		0x10

#define	MC3		0x08	/* DMA"C" timing */
#define	MC2		0x04	/* DMA"C" timing */
#define	MC1		0x02	/* DMA"C" timing */
#define	MC0		0x01	/* DMA"C" timing */

#if PDC202XX_DECODE_REGISTER_INFO

#define REG_A		0x01
#define REG_B		0x02
#define REG_C		0x04
#define REG_D		0x08

static void decode_registers (byte registers, byte value)
{
	byte	bit = 0, bit1 = 0, bit2 = 0;

	switch(registers) {
		case REG_A:
			bit2 = 0;
			printk("A Register ");
			if (value & 0x80) printk("SYNC_IN ");
			if (value & 0x40) printk("ERRDY_EN ");
			if (value & 0x20) printk("IORDY_EN ");
			if (value & 0x10) printk("PREFETCH_EN ");
			if (value & 0x08) { printk("PA3 ");bit2 |= 0x08; }
			if (value & 0x04) { printk("PA2 ");bit2 |= 0x04; }
			if (value & 0x02) { printk("PA1 ");bit2 |= 0x02; }
			if (value & 0x01) { printk("PA0 ");bit2 |= 0x01; }
			printk("PIO(A) = %d ", bit2);
			break;
		case REG_B:
			bit1 = 0;bit2 = 0;
			printk("B Register ");
			if (value & 0x80) { printk("MB2 ");bit1 |= 0x80; }
			if (value & 0x40) { printk("MB1 ");bit1 |= 0x40; }
			if (value & 0x20) { printk("MB0 ");bit1 |= 0x20; }
			printk("DMA(B) = %d ", bit1 >> 5);
			if (value & 0x10) printk("PIO_FORCED/PB4 ");
			if (value & 0x08) { printk("PB3 ");bit2 |= 0x08; }
			if (value & 0x04) { printk("PB2 ");bit2 |= 0x04; }
			if (value & 0x02) { printk("PB1 ");bit2 |= 0x02; }
			if (value & 0x01) { printk("PB0 ");bit2 |= 0x01; }
			printk("PIO(B) = %d ", bit2);
			break;
		case REG_C:
			bit2 = 0;
			printk("C Register ");
			if (value & 0x80) printk("DMARQp ");
			if (value & 0x40) printk("IORDYp ");
			if (value & 0x20) printk("DMAR_EN ");
			if (value & 0x10) printk("DMAW_EN ");

			if (value & 0x08) { printk("MC3 ");bit2 |= 0x08; }
			if (value & 0x04) { printk("MC2 ");bit2 |= 0x04; }
			if (value & 0x02) { printk("MC1 ");bit2 |= 0x02; }
			if (value & 0x01) { printk("MC0 ");bit2 |= 0x01; }
			printk("DMA(C) = %d ", bit2);
			break;
		case REG_D:
			printk("D Register ");
			break;
		default:
			return;
	}
	printk("\n        %s ", (registers & REG_D) ? "DP" :
				(registers & REG_C) ? "CP" :
				(registers & REG_B) ? "BP" :
				(registers & REG_A) ? "AP" : "ERROR");
	for (bit=128;bit>0;bit/=2)
		printk("%s", (value & bit) ? "1" : "0");
	printk("\n");
}

#endif /* PDC202XX_DECODE_REGISTER_INFO */

static int check_in_drive_lists (ide_drive_t *drive, const char **list)
{
	struct hd_driveid *id = drive->id;

	if (pdc_quirk_drives == list) {
		while (*list) {
			if (strstr(id->model, *list++)) {
				return 2;
			}
		}
	} else {
		while (*list) {
			if (!strcmp(*list++,id->model)) {
				return 1;
			}
		}
	}
	return 0;
}

static int pdc202xx_tune_chipset (ide_drive_t *drive, byte speed)
{
	ide_hwif_t *hwif	= HWIF(drive);
	struct pci_dev *dev	= hwif->pci_dev;

	unsigned int		drive_conf;
	int			err = 0, i = 0, j = hwif->channel ? 2 : 0 ;
	byte			drive_pci, AP, BP, CP, DP;
	byte			TA = 0, TB = 0, TC = 0;

	switch (drive->dn) {
		case 0: drive_pci = 0x60; break;
		case 1: drive_pci = 0x64; break;
		case 2: drive_pci = 0x68; break;
		case 3: drive_pci = 0x6c; break;
		default: return -1;
	}

	if ((drive->media != ide_disk) && (speed < XFER_SW_DMA_0))	return -1;

	pci_read_config_dword(dev, drive_pci, &drive_conf);
	pci_read_config_byte(dev, (drive_pci), &AP);
	pci_read_config_byte(dev, (drive_pci)|0x01, &BP);
	pci_read_config_byte(dev, (drive_pci)|0x02, &CP);
	pci_read_config_byte(dev, (drive_pci)|0x03, &DP);

#ifdef CONFIG_BLK_DEV_IDEDMA
	if (speed >= XFER_SW_DMA_0) {
		if ((BP & 0xF0) && (CP & 0x0F)) {
			/* clear DMA modes of upper 842 bits of B Register */
			/* clear PIO forced mode upper 1 bit of B Register */
			pci_write_config_byte(dev, (drive_pci)|0x01, BP & ~0xF0);
			pci_read_config_byte(dev, (drive_pci)|0x01, &BP);

			/* clear DMA modes of lower 8421 bits of C Register */
			pci_write_config_byte(dev, (drive_pci)|0x02, CP & ~0x0F);
			pci_read_config_byte(dev, (drive_pci)|0x02, &CP);
		}
	} else {
#else
	{
#endif /* CONFIG_BLK_DEV_IDEDMA */
		if ((AP & 0x0F) || (BP & 0x07)) {
			/* clear PIO modes of lower 8421 bits of A Register */
			pci_write_config_byte(dev, (drive_pci), AP & ~0x0F);
			pci_read_config_byte(dev, (drive_pci), &AP);

			/* clear PIO modes of lower 421 bits of B Register */
			pci_write_config_byte(dev, (drive_pci)|0x01, BP & ~0x07);
			pci_read_config_byte(dev, (drive_pci)|0x01, &BP);

			pci_read_config_byte(dev, (drive_pci), &AP);
			pci_read_config_byte(dev, (drive_pci)|0x01, &BP);
		}
	}

	pci_read_config_byte(dev, (drive_pci), &AP);
	pci_read_config_byte(dev, (drive_pci)|0x01, &BP);
	pci_read_config_byte(dev, (drive_pci)|0x02, &CP);

	for ( i = 0; i < 2; i++)
		if (hwif->drives[i].present)
	  		id[i+j] = hwif->drives[i].id;	/* get identify structs */

	switch(speed) {
#ifdef CONFIG_BLK_DEV_IDEDMA
		/* case XFER_UDMA_6: */
		case XFER_UDMA_5:
		case XFER_UDMA_4:	TB = 0x20; TC = 0x01; break;	/* speed 8 == UDMA mode 4 */
		case XFER_UDMA_3:	TB = 0x40; TC = 0x02; break;	/* speed 7 == UDMA mode 3 */
		case XFER_UDMA_2:	TB = 0x20; TC = 0x01; break;	/* speed 6 == UDMA mode 2 */
		case XFER_UDMA_1:	TB = 0x40; TC = 0x02; break;	/* speed 5 == UDMA mode 1 */
		case XFER_UDMA_0:	TB = 0x60; TC = 0x03; break;	/* speed 4 == UDMA mode 0 */
		case XFER_MW_DMA_2:	TB = 0x60; TC = 0x03; break;	/* speed 4 == MDMA mode 2 */
		case XFER_MW_DMA_1:	TB = 0x60; TC = 0x04; break;	/* speed 3 == MDMA mode 1 */
		case XFER_MW_DMA_0:	TB = 0x60; TC = 0x05; break;	/* speed 2 == MDMA mode 0 */
		case XFER_SW_DMA_2:	TB = 0x60; TC = 0x05; break;	/* speed 0 == SDMA mode 2 */
		case XFER_SW_DMA_1:	TB = 0x80; TC = 0x06; break;	/* speed 1 == SDMA mode 1 */
		case XFER_SW_DMA_0:	TB = 0xC0; TC = 0x0B; break;	/* speed 0 == SDMA mode 0 */
#endif /* CONFIG_BLK_DEV_IDEDMA */
		case XFER_PIO_4:	TA = 0x01; TB = 0x04; break;
		case XFER_PIO_3:	TA = 0x02; TB = 0x06; break;
		case XFER_PIO_2:	TA = 0x03; TB = 0x08; break;
		case XFER_PIO_1:	TA = 0x05; TB = 0x0C; break;
		case XFER_PIO_0:
		default:		TA = 0x09; TB = 0x13; break;
	}

#ifdef CONFIG_BLK_DEV_IDEDMA
        if (speed >= XFER_SW_DMA_0) {
		pci_write_config_byte(dev, (drive_pci)|0x01, BP|TB);
		pci_write_config_byte(dev, (drive_pci)|0x02, CP|TC);
	} else {
#else
	{
#endif /* CONFIG_BLK_DEV_IDEDMA */
		pci_write_config_byte(dev, (drive_pci), AP|TA);
		pci_write_config_byte(dev, (drive_pci)|0x01, BP|TB);
	}

#if PDC202XX_DECODE_REGISTER_INFO
	pci_read_config_byte(dev, (drive_pci), &AP);
	pci_read_config_byte(dev, (drive_pci)|0x01, &BP);
	pci_read_config_byte(dev, (drive_pci)|0x02, &CP);
	pci_read_config_byte(dev, (drive_pci)|0x03, &DP);

	decode_registers(REG_A, AP);
	decode_registers(REG_B, BP);
	decode_registers(REG_C, CP);
	decode_registers(REG_D, DP);
#endif /* PDC202XX_DECODE_REGISTER_INFO */

	if (!drive->init_speed)
		drive->init_speed = speed;
	err = ide_config_drive_speed(drive, speed);
	drive->current_speed = speed;

#if PDC202XX_DEBUG_DRIVE_INFO
	printk("%s: %s drive%d 0x%08x ",
		drive->name, ide_xfer_verbose(speed),
		drive->dn, drive_conf);
		pci_read_config_dword(dev, drive_pci, &drive_conf);
	printk("0x%08x\n", drive_conf);
#endif /* PDC202XX_DEBUG_DRIVE_INFO */
	return err;
}

static int pdc202xx_new_tune_chipset (ide_drive_t *drive, byte speed)
{
	ide_hwif_t *hwif	= HWIF(drive);
#ifdef CONFIG_BLK_DEV_IDEDMA
	unsigned long indexreg	= (hwif->dma_base + 1);
	unsigned long datareg	= (hwif->dma_base + 3);
#else
	struct pci_dev *dev	= hwif->pci_dev;
	unsigned long high_16	= (unsigned long)ioremap_nocache(pci_resource_start(dev, 5), 64);
	unsigned long indexreg	= high_16 + (hwif->channel ? 0x09 : 0x01);
	unsigned long datareg	= (indexreg + 2);
#endif /* CONFIG_BLK_DEV_IDEDMA */
	byte thold		= 0x10;
	byte adj		= (drive->dn%2) ? 0x08 : 0x00;
	int set_speed		= 0, i=0, j=hwif->channel ? 2:0;
	int                     err;

	/* Setting tHOLD bit to 0 if using UDMA mode 2 */
	if (speed == XFER_UDMA_2) {
		writeb((thold + adj), indexreg);
		writeb((readb(datareg) & 0x7f), datareg);
	}
	
	/* We need to set ATA133 timing if ATA133 drives exist */
	if (speed>=XFER_UDMA_6)
		set_speed=1;

	if (!drive->init_speed)
		drive->init_speed = speed;
#if PDC202XX_DEBUG_DRIVE_INFO
	printk("%s: Before set_feature = %s, word88 = %#x\n",
		drive->name, ide_xfer_verbose(speed), drive->id->dma_ultra );
#endif /* PDC202XX_DEBUG_DRIVE_INFO */
	err = ide_config_drive_speed(drive, speed);
	drive->current_speed = speed;	
	for ( i = 0 ; i < 2 ; i++)
		if (hwif->drives[i].present) {
	  		id[i+j] = hwif->drives[i].id;	/* get identify structs */
	 		speed_rate[i+j] = speed;	/* get current speed */
	 	}
	if (set_speed) {
		for (i=0; i<4; i++) {
			if (id[i]==NULL)
				continue;
			switch(speed_rate[i]) {
#ifdef CONFIG_BLK_DEV_IDEDMA
				case XFER_UDMA_6:
					writeb((0x10 + adj), indexreg);
					writeb(0x1a, datareg);
					writeb((0x11 + adj), indexreg);
					writeb(0x01, datareg);
					writeb((0x12 + adj), indexreg);
					writeb(0xcb, datareg);
					break;
				case XFER_UDMA_5:
					writeb((0x10 + adj), indexreg);
					writeb(0x1a, datareg);
					writeb((0x11 + adj), indexreg);
					writeb(0x02, datareg);
					writeb((0x12 + adj), indexreg);
					writeb(0xcb, datareg);
					break;
				case XFER_UDMA_4:
					writeb((0x10 + adj), indexreg);
					writeb(0x1a, datareg);
					writeb((0x11 + adj), indexreg);
					writeb(0x03, datareg);
					writeb((0x12 + adj), indexreg);
					writeb(0xcd, datareg);
					break;
				case XFER_UDMA_3:
					writeb((0x10 + adj), indexreg);
					writeb(0x1a, datareg);
					writeb((0x11 + adj), indexreg);
					writeb(0x05, datareg);
					writeb((0x12 + adj), indexreg);
					writeb(0xcd, datareg);
					break;
				case XFER_UDMA_2:
					writeb((0x10 + adj), indexreg);
					writeb(0x2a, datareg);
					writeb((0x11 + adj), indexreg);
					writeb(0x07, datareg);
					writeb((0x12 + adj), indexreg);
					writeb(0xcd, datareg);
					break;
				case XFER_UDMA_1:
					writeb((0x10 + adj), indexreg);
					writeb(0x3a, datareg);
					writeb((0x11 + adj), indexreg);
					writeb(0x0a, datareg);
					writeb((0x12 + adj), indexreg);
					writeb(0xd0, datareg);
					break;
				case XFER_UDMA_0:
					writeb((0x10 + adj), indexreg);
					writeb(0x4a, datareg);
					writeb((0x11 + adj), indexreg);
					writeb(0x0f, datareg);
					writeb((0x12 + adj), indexreg);
					writeb(0xd5, datareg);
					break;
				case XFER_MW_DMA_2:
					writeb((0x0e + adj), indexreg);
					writeb(0x69, datareg);
					writeb((0x0f + adj), indexreg);
					writeb(0x25, datareg);
					break;
				case XFER_MW_DMA_1:
					writeb((0x0e + adj), indexreg);
					writeb(0x6b, datareg);
					writeb((0x0f+ adj), indexreg);
					writeb(0x27, datareg);
					break;
				case XFER_MW_DMA_0:
					writeb((0x0e + adj), indexreg);
					writeb(0xdf, datareg);
					writeb((0x0f + adj), indexreg);
					writeb(0x5f, datareg);
					break;
#endif /* CONFIG_BLK_DEV_IDEDMA */
				case XFER_PIO_4:
					writeb((0x0c + adj), indexreg);
					writeb(0x23, datareg);
					writeb((0x0d + adj), indexreg);
					writeb(0x09, datareg);
					writeb((0x13 + adj), indexreg);
					writeb(0x25, datareg);
					break;
				case XFER_PIO_3:
					writeb((0x0c + adj), indexreg);
					writeb(0x27, datareg);
					writeb((0x0d + adj), indexreg);
					writeb(0x0d, datareg);
					writeb((0x13 + adj), indexreg);
					writeb(0x35, datareg);
					break;
				case XFER_PIO_2:
					writeb((0x0c + adj), indexreg);
					writeb(0x23, datareg);
					writeb((0x0d + adj), indexreg);
					writeb(0x26, datareg);
					writeb((0x13 + adj), indexreg);
					writeb(0x64, datareg);
					break;
				case XFER_PIO_1:
					writeb((0x0c + adj), indexreg);
					writeb(0x46, datareg);
					writeb((0x0d + adj), indexreg);
					writeb(0x29, datareg);
					writeb((0x13 + adj), indexreg);
					writeb(0xa4, datareg);
					break;
				case XFER_PIO_0:
					writeb((0x0c + adj), indexreg);
					writeb(0xfb, datareg);
					writeb((0x0d + adj), indexreg);
					writeb(0x2b, datareg);
					writeb((0x13 + adj), indexreg);
					writeb(0xac, datareg);
					break;
				default:
					break;
			}
		}
	}
#ifndef CONFIG_BLK_DEV_IDEDMA
	iounmap((void*)high_16);
#endif
	return err;
}

/*   0    1    2    3    4    5    6   7   8
 * 960, 480, 390, 300, 240, 180, 120, 90, 60
 *           180, 150, 120,  90,  60
 * DMA_Speed
 * 180, 120,  90,  90,  90,  60,  30
 *  11,   5,   4,   3,   2,   1,   0
 */
static int config_chipset_for_pio (ide_drive_t *drive, byte pio)
{
	byte speed = 0x00;

	pio = (pio == 5) ? 4 : pio;
	speed = XFER_PIO_0 + ide_get_best_pio_mode(drive, 255, pio, NULL);
        
	return ((int) pdc202xx_tune_chipset(drive, speed));
}

static void pdc202xx_tune_drive (ide_drive_t *drive, byte pio)
{
	(void) config_chipset_for_pio(drive, pio);
}

#ifdef CONFIG_BLK_DEV_IDEDMA
static int config_chipset_for_dma (ide_drive_t *drive, byte ultra)
{
	struct hd_driveid *id	= drive->id;
	ide_hwif_t *hwif	= HWIF(drive);
	struct pci_dev *dev	= hwif->pci_dev;
	unsigned long high_16   = (unsigned long)ioremap_nocache(pci_resource_start(dev, 5), 64);
	unsigned long dma_base  = hwif->dma_base;
	unsigned long indexreg	= dma_base + 1;
	unsigned long datareg	= dma_base + 3;
	byte iordy		= 0x13;
	byte adj		= (drive->dn%2) ? 0x08 : 0x00;
	byte cable		= 0;
	byte new_chip		= 0;
	byte unit		= (drive->select.b.unit & 0x01);
	unsigned int		drive_conf;
	byte			drive_pci = 0;
	byte			test1, test2, speed = -1;
	byte			AP;
	unsigned short		EP;
	byte CLKSPD		= 0;
	unsigned long clockreg	= high_16 + 0x11;
	byte udma_33		= ultra;
	byte udma_66		= ((eighty_ninty_three(drive)) && udma_33) ? 1 : 0;
	byte udma_100		= 0;
	byte udma_133		= 0;
	byte mask			= hwif->channel ? 0x08 : 0x02;
	unsigned short c_mask	= hwif->channel ? (1<<11) : (1<<10);

	byte ultra_66		= ((id->dma_ultra & 0x0010) ||
				   (id->dma_ultra & 0x0008)) ? 1 : 0;
	byte ultra_100		= ((id->dma_ultra & 0x0020) ||
				   (ultra_66)) ? 1 : 0;
	byte ultra_133		= ((id->dma_ultra & 0x0040) ||
				   (ultra_100)) ? 1 : 0;

	switch(dev->device) {
		case PCI_DEVICE_ID_PROMISE_20276:
		case PCI_DEVICE_ID_PROMISE_20275:
		case PCI_DEVICE_ID_PROMISE_20269:
			udma_133 = (udma_66) ? 1 : 0;
			udma_100 = (udma_66) ? 1 : 0;
			OUT_BYTE(0x0b, (hwif->dma_base + 1));
			cable = ((IN_BYTE((hwif->dma_base + 3)) & 0x04));
			new_chip = 1;
			break;
		case PCI_DEVICE_ID_PROMISE_20268:
		case PCI_DEVICE_ID_PROMISE_20270:
			udma_100 = (udma_66) ? 1 : 0;
			OUT_BYTE(0x0b, (hwif->dma_base + 1));
			cable = ((IN_BYTE((hwif->dma_base + 3)) & 0x04));
			new_chip = 1;
			break;
		case PCI_DEVICE_ID_PROMISE_20267:
		case PCI_DEVICE_ID_PROMISE_20265:
			udma_100 = (udma_66) ? 1 : 0;
			pci_read_config_word(dev, 0x50, &EP);
			cable = (EP & c_mask);
			new_chip = 0;
			CLKSPD = readb(clockreg);
			break;
		case PCI_DEVICE_ID_PROMISE_20262:
			pci_read_config_word(dev, 0x50, &EP);
			cable = (EP & c_mask);
			new_chip = 0;
			CLKSPD = readb(clockreg);
			break;
		default:
			udma_100 = 0; udma_133 = 0; cable = 0; new_chip = 1;
			break;
	}

	/*
	 * Set the control register to use the 66Mhz system
	 * clock for UDMA 3/4 mode operation. If one drive on
	 * a channel is U66 capable but the other isn't we
	 * fall back to U33 mode. The BIOS INT 13 hooks turn
	 * the clock on then off for each read/write issued. I don't
	 * do that here because it would require modifying the
	 * kernel, seperating the fop routines from the kernel or
	 * somehow hooking the fops calls. It may also be possible to
	 * leave the 66Mhz clock on and readjust the timing
	 * parameters.
	 */

	if (((ultra_66) || (ultra_100) || (ultra_133)) && (cable)) {
#ifdef DEBUG
		printk("ULTRA66: %s channel of Ultra 66 requires an 80-pin cable for Ultra66 operation.\n", hwif->channel ? "Secondary" : "Primary");
		printk("         Switching to Ultra33 mode.\n");
#endif /* DEBUG */
		/* Primary   : zero out second bit */
		/* Secondary : zero out fourth bit */
		//if (!new_chip)
		writeb(CLKSPD & ~mask, clockreg);
		printk("Warning: %s channel requires an 80-pin cable for operation.\n", hwif->channel ? "Secondary":"Primary");
		printk("%s reduced to Ultra33 mode.\n", drive->name);
		udma_66 = 0; udma_100 = 0; udma_133 = 0;
	} else {
		if ((ultra_66) || (ultra_100) || (ultra_133)) {
			/*
			 * check to make sure drive on same channel
			 * is u66 capable
			 */
			if (hwif->drives[!(drive->dn%2)].id) {
				if ((hwif->drives[!(drive->dn%2)].id->dma_ultra & 0x0040) ||
				    (hwif->drives[!(drive->dn%2)].id->dma_ultra
& 0x0020) ||
				    (hwif->drives[!(drive->dn%2)].id->dma_ultra & 0x0010) ||
				    (hwif->drives[!(drive->dn%2)].id->dma_ultra & 0x0008)) {
					writeb(CLKSPD | mask, clockreg);
				} else {
					writeb(CLKSPD & ~mask, clockreg);
				}
			} else { /* udma4 drive by itself */
				writeb(CLKSPD | mask, clockreg);
			}
		}
	}

	iounmap((void*)high_16);

	if (new_chip)	goto chipset_is_set;

	switch(drive->dn) {
		case 0:	drive_pci = 0x60;
			pci_read_config_dword(dev, drive_pci, &drive_conf);
			if ((drive_conf != 0x004ff304) && (drive_conf != 0x004ff3c4))
				goto chipset_is_set;
			pci_read_config_byte(dev, (drive_pci), &test1);
			if (!(test1 & SYNC_ERRDY_EN))
				pci_write_config_byte(dev, (drive_pci), test1|SYNC_ERRDY_EN);
			break;
		case 1:	drive_pci = 0x64;
			pci_read_config_dword(dev, drive_pci, &drive_conf);
			if ((drive_conf != 0x004ff304) && (drive_conf != 0x004ff3c4))
				goto chipset_is_set;
			pci_read_config_byte(dev, 0x60, &test1);
			pci_read_config_byte(dev, (drive_pci), &test2);
			if ((test1 & SYNC_ERRDY_EN) && !(test2 & SYNC_ERRDY_EN))
				pci_write_config_byte(dev, (drive_pci), test2|SYNC_ERRDY_EN);
			break;
		case 2:	drive_pci = 0x68;
			pci_read_config_dword(dev, drive_pci, &drive_conf);
			if ((drive_conf != 0x004ff304) && (drive_conf != 0x004ff3c4))
				goto chipset_is_set;
			pci_read_config_byte(dev, (drive_pci), &test1);
			if (!(test1 & SYNC_ERRDY_EN))
				pci_write_config_byte(dev, (drive_pci), test1|SYNC_ERRDY_EN);
			break;
		case 3:	drive_pci = 0x6c;
			pci_read_config_dword(dev, drive_pci, &drive_conf);
			if ((drive_conf != 0x004ff304) && (drive_conf != 0x004ff3c4))
				goto chipset_is_set;
			pci_read_config_byte(dev, 0x68, &test1);
			pci_read_config_byte(dev, (drive_pci), &test2);
			if ((test1 & SYNC_ERRDY_EN) && !(test2 & SYNC_ERRDY_EN))
				pci_write_config_byte(dev, (drive_pci), test2|SYNC_ERRDY_EN);
			break;
		default:
			return ide_dma_off;
	}

chipset_is_set:

	if (drive->media != ide_disk)
		return ide_dma_off_quietly;
	
	if (new_chip) {
		if (id->capability & 4) {	/* IORDY_EN & PREFETCH_EN */
			OUT_BYTE((iordy + adj), indexreg);
			OUT_BYTE((IN_BYTE(datareg)|0x03), datareg);
		}
	}
	else {
		pci_read_config_byte(dev, (drive_pci), &AP);
		if (id->capability & 4)	/* IORDY_EN */
			pci_write_config_byte(dev, (drive_pci), AP|IORDY_EN);
		pci_read_config_byte(dev, (drive_pci), &AP);
		if (drive->media == ide_disk)	/* PREFETCH_EN */
			pci_write_config_byte(dev, (drive_pci), AP|PREFETCH_EN);
	}

	if ((id->dma_ultra & 0x0040)&&(udma_133))	speed = XFER_UDMA_6;
	else if ((id->dma_ultra & 0x0020)&&(udma_100))	speed = XFER_UDMA_5;
	else if ((id->dma_ultra & 0x0010)&&(udma_66))	speed = XFER_UDMA_4;
	else if ((id->dma_ultra & 0x0008)&&(udma_66))	speed = XFER_UDMA_3;
	else if ((id->dma_ultra & 0x0004)&&(udma_33))	speed = XFER_UDMA_2;
	else if ((id->dma_ultra & 0x0002)&&(udma_33))	speed = XFER_UDMA_1;
	else if ((id->dma_ultra & 0x0001)&&(udma_33))	speed = XFER_UDMA_0;
	else if (id->dma_mword & 0x0004)		speed = XFER_MW_DMA_2;
	else if (id->dma_mword & 0x0002)		speed = XFER_MW_DMA_1;
	else if (id->dma_mword & 0x0001)		speed = XFER_MW_DMA_0;
	else if ((id->dma_1word & 0x0004)&&(!new_chip))	speed = XFER_SW_DMA_2;
	else if ((id->dma_1word & 0x0002)&&(!new_chip))	speed = XFER_SW_DMA_1;
	else if ((id->dma_1word & 0x0001)&&(!new_chip))	speed = XFER_SW_DMA_0;
	else {
		/* restore original pci-config space */
		if (!new_chip)
			pci_write_config_dword(dev, drive_pci, drive_conf);
		return ide_dma_off_quietly;
	}

	outb(inb(dma_base+2) & ~(1<<(5+unit)), dma_base+2);
	(void) hwif->speedproc(drive, speed);

	return ((int)	((id->dma_ultra >> 14) & 3) ? ide_dma_on :
			((id->dma_ultra >> 11) & 7) ? ide_dma_on :
			((id->dma_ultra >> 8) & 7) ? ide_dma_on :
			((id->dma_mword >> 8) & 7) ? ide_dma_on : 
			((id->dma_1word >> 8) & 7) ? ide_dma_on :
						     ide_dma_off_quietly);
}

static int config_drive_xfer_rate (ide_drive_t *drive)
{
	struct hd_driveid *id = drive->id;
	ide_hwif_t *hwif = HWIF(drive);
	ide_dma_action_t dma_func = ide_dma_off_quietly;

	if (id && (id->capability & 1) && hwif->autodma) {
		/* Consult the list of known "bad" drives */
		if (ide_dmaproc(ide_dma_bad_drive, drive)) {
			dma_func = ide_dma_off;
			goto fast_ata_pio;
		}
		dma_func = ide_dma_off_quietly;
		if (id->field_valid & 4) {
			if (id->dma_ultra & 0x007F) {
				/* Force if Capable UltraDMA */
				dma_func = config_chipset_for_dma(drive, 1);
				if ((id->field_valid & 2) &&
				    (dma_func != ide_dma_on))
					goto try_dma_modes;
			}
		} else if (id->field_valid & 2) {
try_dma_modes:
			if ((id->dma_mword & 0x0007) ||
			    (id->dma_1word & 0x0007)) {
				/* Force if Capable regular DMA modes */
				dma_func = config_chipset_for_dma(drive, 0);
				if (dma_func != ide_dma_on)
					goto no_dma_set;
			}
		} else if (ide_dmaproc(ide_dma_good_drive, drive)) {
			if (id->eide_dma_time > 150) {
				goto no_dma_set;
			}
			/* Consult the list of known "good" drives */
			dma_func = config_chipset_for_dma(drive, 0);
			if (dma_func != ide_dma_on)
				goto no_dma_set;
		} else {
			goto fast_ata_pio;
		}
	} else if ((id->capability & 8) || (id->field_valid & 2)) {
fast_ata_pio:
		dma_func = ide_dma_off_quietly;
no_dma_set:
		(void) config_chipset_for_pio(drive, 5);
	}

	return HWIF(drive)->dmaproc(dma_func, drive);
}

int pdc202xx_quirkproc (ide_drive_t *drive)
{
	return ((int) check_in_drive_lists(drive, pdc_quirk_drives));
}

/*
 * pdc202xx_dmaproc() initiates/aborts (U)DMA read/write operations on a drive.
 */
int pdc202xx_dmaproc (ide_dma_action_t func, ide_drive_t *drive)
{
	byte dma_stat		= 0;
	byte sc1d		= 0;
	byte newchip		= 0;
	byte clock		= 0;
	byte hardware48fix	= 0;
	ide_hwif_t *hwif	= HWIF(drive);
	struct pci_dev *dev	= hwif->pci_dev;
	unsigned long high_16	= (unsigned long)ioremap_nocache(pci_resource_start(dev, 5), 64);
	unsigned long dma_base	= hwif->dma_base;
	unsigned long atapi_port= hwif->channel ? high_16+0x24 : high_16+0x20;

	switch (dev->device) {
		case PCI_DEVICE_ID_PROMISE_20276:
		case PCI_DEVICE_ID_PROMISE_20275:
		case PCI_DEVICE_ID_PROMISE_20269:
		case PCI_DEVICE_ID_PROMISE_20268:
		case PCI_DEVICE_ID_PROMISE_20270:
			newchip = 1;
			break;
		case PCI_DEVICE_ID_PROMISE_20267:
		case PCI_DEVICE_ID_PROMISE_20265:
			hardware48fix = 1;
			clock = readb(high_16 + 0x11);
		default:
			break;
	}

	switch (func) {
		case ide_dma_check:
			return config_drive_xfer_rate(drive);
		case ide_dma_begin:
			/* Note that this is done *after* the cmd has
			 * been issued to the drive, as per the BM-IDE spec.
			 * The Promise Ultra33 doesn't work correctly when
			 * we do this part before issuing the drive cmd.
			 */
			/* Enable ATAPI UDMA port for 48bit data on PDC20267 */
			if ((drive->addressing) && (hardware48fix)) {
				struct request *rq = HWGROUP(drive)->rq;
				unsigned long word_count = 0;
				unsigned long hankval = 0;
				unsigned long clockreg = high_16 + 0x11;
				
				writeb(clock|(hwif->channel ? 0x08:0x02), clockreg);
				word_count = (rq->nr_sectors << 8);
				hankval = (rq->cmd == READ) ? 0x05<<24 : 0x06<<24;
				hankval = hankval | word_count ;
				writel(hankval, atapi_port);
			}  
			break;
		case ide_dma_end:
			/* Disable ATAPI UDMA port for 48bit data on PDC20267 */
			if ((drive->addressing) && (hardware48fix)) {
				unsigned long hankval = 0;
				unsigned long clockreg = high_16 + 0x11;
				
			    	writel(hankval, atapi_port);	/* zero out extra */
				clock = readb(clockreg);
				writeb(clock & ~(hwif->channel ? 0x08:0x02), clockreg);
			}
			break;
		case ide_dma_test_irq:	/* returns 1 if dma irq issued, 0 otherwise */
			dma_stat = IN_BYTE(dma_base+2);
			if (newchip) {
				iounmap((void*)high_16);
				return (dma_stat & 4) == 4;
			}

			sc1d = readb(high_16 + 0x001d);
			if (HWIF(drive)->channel) {
				if ((sc1d & 0x50) == 0x50) goto somebody_else;
				else if ((sc1d & 0x40) == 0x40) {
					iounmap((void*)high_16);
					return (dma_stat & 4) == 4;
				}
			} else {
				if ((sc1d & 0x05) == 0x05) goto somebody_else;
				else if ((sc1d & 0x04) == 0x04) {
					iounmap((void*)high_16);
					return (dma_stat & 4) == 4;
				}
			}
somebody_else:
			iounmap((void*)high_16);
			return (dma_stat & 4) == 4;	/* return 1 if INTR asserted */
		case ide_dma_lostirq:
		case ide_dma_timeout:
			if (HWIF(drive)->resetproc != NULL)
				HWIF(drive)->resetproc(drive);
		default:
			break;
	}
	iounmap((void*)high_16);
	return ide_dmaproc(func, drive);	/* use standard DMA stuff */
}
#endif /* CONFIG_BLK_DEV_IDEDMA */

void pdc202xx_reset (ide_drive_t *drive)
{
	OUT_BYTE(0x04,IDE_CONTROL_REG);
	mdelay(1000);
	OUT_BYTE(0x00,IDE_CONTROL_REG);
	mdelay(1000);
	printk("PDC202XX: %s channel reset.\n",
		HWIF(drive)->channel ? "Secondary" : "Primary");
}

/*
 * Since SUN Cobalt is attempting to do this operation, I should disclose
 * this has been a long time ago Thu Jul 27 16:40:57 2000 was the patch date
 * HOTSWAP ATA Infrastructure.
 */
static int pdc202xx_tristate (ide_drive_t * drive, int state)
{
	printk("%s: %s\n", __FUNCTION__, drive->name);
	return 0;
}

unsigned int __init pci_init_pdc202xx (struct pci_dev *dev, const char *name)
{
	unsigned long high_16	= (unsigned long)ioremap_nocache(pci_resource_start(dev, 5), 64);
	byte udma_speed_flag	= readb(high_16 + 0x001f);
	byte primary_mode	= readb(high_16 + 0x001a);
	byte secondary_mode	= readb(high_16 + 0x001b);

	writeb(udma_speed_flag | 0x10, high_16 + 0x001f);
	mdelay(100);
	writeb(udma_speed_flag & ~0x10, high_16 + 0x001f);
	mdelay(2000);	/* 2 seconds ?! */

	if (dev->resource[PCI_ROM_RESOURCE].start) {
		pci_write_config_dword(dev, PCI_ROM_ADDRESS, dev->resource[PCI_ROM_RESOURCE].start | PCI_ROM_ADDRESS_ENABLE);
		printk("%s: ROM enabled at 0x%08lx\n", name, dev->resource[PCI_ROM_RESOURCE].start);
	}
	
	printk("%s: (U)DMA Burst Bit %sABLED " \
		"Primary %s Mode " \
		"Secondary %s Mode.\n",
		name,
		(udma_speed_flag & 1) ? "EN" : "DIS",
		(primary_mode & 1) ? "MASTER" : "PCI",
		(secondary_mode & 1) ? "MASTER" : "PCI" );

#ifdef CONFIG_PDC202XX_BURST
	if (!(udma_speed_flag & 1)) {
		printk("%s: FORCING BURST BIT 0x%02x -> 0x%02x ", name, udma_speed_flag, (udma_speed_flag|1));
		writeb(udma_speed_flag|1, high_16 + 0x001f);
		printk("%sCTIVE\n", (readb(high_16 + 0x001f) & 1) ? "A" : "INA");
	}
#endif /* CONFIG_PDC202XX_BURST */

#ifdef CONFIG_PDC202XX_MASTER
	if (!(primary_mode & 1)) {
		printk("%s: FORCING PRIMARY MODE BIT 0x%02x -> 0x%02x ",
			name, primary_mode, (primary_mode|1));
		writeb(primary_mode|1, high_16 + 0x001a);
		printk("%s\n", (readb(high_16 + 0x001a) & 1) ? "MASTER" : "PCI");
	}

	if (!(secondary_mode & 1)) {
		printk("%s: FORCING SECONDARY MODE BIT 0x%02x -> 0x%02x ",
			name, secondary_mode, (secondary_mode|1));
		writeb(secondary_mode|1, high_16 + 0x001b);
		printk("%s\n", (readb(high_16 + 0x001b) & 1) ? "MASTER" : "PCI");
	}
#endif /* CONFIG_PDC202XX_MASTER */

#if defined(DISPLAY_PDC202XX_TIMINGS) && defined(CONFIG_PROC_FS)
	if (!pdc202xx_proc) {
		pdc202xx_proc = 1;
		bmide_dev = dev;
		pdc202xx_display_info = &pdc202xx_get_info;
	}
#endif /* DISPLAY_PDC202XX_TIMINGS && CONFIG_PROC_FS */
	iounmap((void*)high_16);
	return dev->irq;
}

unsigned int __init ata66_pdc202xx (ide_hwif_t *hwif)
{
	unsigned short mask = (hwif->channel) ? (1<<11) : (1<<10);
	unsigned short CIS;

        	switch(hwif->pci_dev->device) {
		case PCI_DEVICE_ID_PROMISE_20276:
		case PCI_DEVICE_ID_PROMISE_20275:
		case PCI_DEVICE_ID_PROMISE_20269:
		case PCI_DEVICE_ID_PROMISE_20268:
		case PCI_DEVICE_ID_PROMISE_20270:
			OUT_BYTE(0x0b, (hwif->dma_base + 1));
			return (!(IN_BYTE((hwif->dma_base + 3)) & 0x04));
			/* check 80pin cable */
		default:
			pci_read_config_word(hwif->pci_dev, 0x50, &CIS);
			return (!(CIS & mask));
			/* check 80pin cable */
	}
}

void __init ide_init_pdc202xx (ide_hwif_t *hwif)
{
	hwif->tuneproc  = &pdc202xx_tune_drive;
	hwif->quirkproc = &pdc202xx_quirkproc;
	hwif->resetproc = &pdc202xx_reset;

        switch(hwif->pci_dev->device) {
		case PCI_DEVICE_ID_PROMISE_20276:
		case PCI_DEVICE_ID_PROMISE_20275:
		case PCI_DEVICE_ID_PROMISE_20269:
		case PCI_DEVICE_ID_PROMISE_20268:
		case PCI_DEVICE_ID_PROMISE_20270:
			hwif->speedproc = &pdc202xx_new_tune_chipset;
			break;
		case PCI_DEVICE_ID_PROMISE_20267:
		case PCI_DEVICE_ID_PROMISE_20265:
		case PCI_DEVICE_ID_PROMISE_20262:
			hwif->busproc   = &pdc202xx_tristate;
		case PCI_DEVICE_ID_PROMISE_20246:
			hwif->speedproc = &pdc202xx_tune_chipset;
		default:
			break;
	}

#undef CONFIG_PDC202XX_32_UNMASK
#ifdef CONFIG_PDC202XX_32_UNMASK
	hwif->drives[0].io_32bit = 1;
	hwif->drives[1].io_32bit = 1;
	hwif->drives[0].unmask = 1;
	hwif->drives[1].unmask = 1;
#endif /* CONFIG_PDC202XX_32_UNMASK */

#ifdef CONFIG_BLK_DEV_IDEDMA
	if (hwif->dma_base) {
		hwif->dmaproc = &pdc202xx_dmaproc;
		if (!noautodma)
			hwif->autodma = 1;
	} else {
		hwif->drives[0].autotune = 1;
		hwif->drives[1].autotune = 1;
		hwif->autodma = 0;
	}
#else /* !CONFIG_BLK_DEV_IDEDMA */
	hwif->drives[0].autotune = 1;
	hwif->drives[1].autotune = 1;
	hwif->autodma = 0;
#endif /* CONFIG_BLK_DEV_IDEDMA */
}
