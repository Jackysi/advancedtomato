/*
 * sbd.h: cpu board definitions for Galileo-9
 * Copyright (c) 1997	Algorithmics Ltd
 */

#ifndef MHZ
/* fastest possible pipeline clock */
#define MHZ		150
#endif

#define RAMCYCLE	60	/* 60ns dram cycle */
#define ROMCYCLE	750	/* ~750ns rom cycle */
#define CACHECYCLE	(1000/MHZ)	/* internal clock */
#define CYCLETIME	CACHECYCLE
#define CACHEMISS	(CYCLETIME * 6)

/*
 * rough scaling factors for 2 instruction DELAY loop to get 1ms and 1us delays
 */
#define ASMDELAY(ns,icycle)	\
	(((ns) + (icycle)) / ((icycle) * 2))

#define CACHEUS		ASMDELAY(1000, CACHECYCLE)
#define RAMUS		ASMDELAY(1000, CACHEMISS+RAMCYCLE)
#define ROMUS		ASMDELAY(1000, CACHEMISS+ROMCYCLE)
#define CACHEMS		ASMDELAY(1000000, CACHECYCLE)
#define RAMMS		ASMDELAY(1000000, CACHEMISS+RAMCYCLE)
#define ROMMS		ASMDELAY(1000000, CACHEMISS+ROMCYCLE)

#ifndef __ASSEMBLY__
#define nsdelay(ns)	mips_cycle (ASMDELAY (ns, CACHECYCLE))
#define usdelay(us)	mips_cycle (ASMDELAY ((us)*1000, CACHECYCLE))
#endif

#define DRAM_BASE		0x00000000
#define PCI_IO_BASE		0x10000000
#define PCI_IO_SIZE		0x02000000
#define PCI_MEM_BASE		0x12000000
#define PCI_MEM_SIZE		0x02000000
#define GT64011_BASE		0x14000000
#define DUART_BASE		0x1d000000
#define FLASH_BASE		0x1f000000
#define PROM_BASE		0x1fc00000


#define LOCAL_MEM		DRAM_BASE
#define LOCAL_MEM_SIZE		(128*1024*1024)	/* SDRAM size (16MB) */

#define BOOTPROM_BASE		PROM_BASE

#define DUART_CLOCK		3686400
