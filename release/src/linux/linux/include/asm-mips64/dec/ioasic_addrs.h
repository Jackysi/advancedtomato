/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Definitions for the address map in the JUNKIO Asic
 *
 * Created with Information from:
 *
 * "DEC 3000 300/400/500/600/700/800/900 AXP Models System Programmer's Manual"
 *
 * and the Mach Sources
 */

#ifndef IOASIC_ADDRS_H
#define IOASIC_ADDRS_H

#define IOASIC_SLOT_SIZE 0x00040000

#define SYSTEM_ROM 	(0*IOASIC_SLOT_SIZE)	/* board ROM */
#define IOCTL 		(1*IOASIC_SLOT_SIZE)	/* I/O ASIC */
#define ESAR 		(2*IOASIC_SLOT_SIZE)	/* LANCE MAC address chip */
#define LANCE 		(3*IOASIC_SLOT_SIZE)	/* LANCE Ethernet */
#define SCC0 		(4*IOASIC_SLOT_SIZE)	/* SCC #0 */
#define VDAC_HI		(5*IOASIC_SLOT_SIZE)	/* VDAC (maxine) */
#define SCC1 		(6*IOASIC_SLOT_SIZE)	/* SCC #1 (3min, 3max+) */
#define VDAC_LO		(7*IOASIC_SLOT_SIZE)	/* VDAC (maxine) */
#define TOY 		(8*IOASIC_SLOT_SIZE)	/* RTC */
#define ISDN 		(9*IOASIC_SLOT_SIZE)	/* ISDN (maxine) */
#define ERRADDR		(9*IOASIC_SLOT_SIZE)	/* bus error address (3max+) */
#define CHKSYN 		(10*IOASIC_SLOT_SIZE)	/* ECC syndrome (3max+) */
#define ACCESS_BUS	(10*IOASIC_SLOT_SIZE)	/* Access.Bus (maxine) */
#define MCR 		(11*IOASIC_SLOT_SIZE)	/* memory control (3max+) */
#define FLOPPY 		(11*IOASIC_SLOT_SIZE)	/* FDC (maxine) */
#define SCSI 		(12*IOASIC_SLOT_SIZE)	/* ASC SCSI */
#define FLOPPY_DMA 	(13*IOASIC_SLOT_SIZE)	/* FDC DMA (maxine) */
#define SCSI_DMA 	(14*IOASIC_SLOT_SIZE)	/* ??? */
#define RESERVED_4 	(15*IOASIC_SLOT_SIZE)	/* unused? */

/*
 * Offsets for IOCTL registers (relative to (system_base + IOCTL))
 */
#define SCSI_DMA_P	0x00			/* SCSI DMA Pointer */
#define SCSI_DMA_BP	0x10			/* SCSI DMA Buffer Pointer */
#define LANCE_DMA_P	0x20			/* LANCE DMA Pointer */
#define SCC0_T_DMA_P	0x30			/* Communication Port 1 Transmit DMA Pointer */
#define SCC0_R_DMA_P	0x40			/* Communication Port 1 Receive DMA Pointer */
#define SCC1_T_DMA_P	0x50			/* Communication Port 2 Transmit DMA Pointer */
#define SCC1_R_DMA_P	0x60			/* Communication Port 2 Receive DMA Pointer */
#define FLOPPY_DMA_P	0x70			/* Floppy DMA Pointer */
#define ISDN_T_DMA_P	0x80			/* ISDN Transmit DMA Pointer */
#define ISDN_T_DMA_BP	0x90			/* ISDN Transmit DMA Buffer Pointer */
#define ISDN_R_DMA_P	0xa0			/* ISDN Receive DMA Pointer */
#define ISDN_R_DMA_BP	0xb0			/* ISDN Receive DMA Buffer Pointer */

#define SSR		0x100			/* System Support Register */
#define SIR		0x110			/* System Interrupt Register */
#define SIMR		0x120			/* System Interrupt Mask Register */
#define FCTR		0x1e0			/* Free-Running Counter */

/*
 * Handle partial word SCSI DMA transfers
 */
#define	SCSI_SCR	0x1b0
#define	SCSI_SDR0	0x1c0
#define	SCSI_SDR1	0x1d0

/*
 * DMA defines for the System Support Register
 */
#define LANCE_DMA_EN	(1UL<<16)			/* LANCE DMA enable */
#define SCSI_DMA_EN	(1UL<<17)			/* SCSI DMA enable */
#define SCSI_DMA_DIR	(1UL<<18)			/* SCSI DMA direction */
#define ISDN_REC_DMA_EN (1UL<<19)			/* ISDN receive DMA enable */
#define ISDN_TRN_DMA_EN (1UL<<20)			/* ISDN transmit DMA enable */
#define FLOPPY_DMA_EN	(1UL<<21)			/* Floppy DMA enable */
#define FLOPPY_DMA_DIR	(1UL<<22)			/* Floppy DMA direction */
#define SCC1A_DMA_EN	(1UL<<28)			/* SCC1 Channel A DMA enable */
#define SCC1B_DMA_EN	(1UL<<29)			/* SCC1 Channel B DMA enable */
#define SCC0A_DMA_EN	(1UL<<30)			/* SCC0 Channel A DMA enable */
#define SCC0B_DMA_EN	(1UL<<31)			/* Scc0 Channel B DMA enable */

#endif
