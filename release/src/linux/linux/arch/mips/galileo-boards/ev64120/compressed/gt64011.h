/*
 * gt64011.h: Galileo PCI system controller
 * Copyright (c) 1998 Algorithmics Ltd
 */

#ifdef __ASSEMBLY__

/* offsets from base register */
#define GT64011(x)		(x)

/* device is littleendian, so data may need to be swapped */
#ifdef GALILEO_PORT
#define htoll(x) ((((x) & 0x00ff) << 24) | \
		  (((x) & 0xff00) <<  8) | \
		  (((x) >> 8)  & 0xff00) | \
		  (((x) >> 24) & 0x00ff))
/*#define ltohl(x) htoll(x)*/
#else
#define htoll(x) (x)
/* #define ltohl(x) (x) */
#endif

#else /* !__ASSEMBLY__ */

/* offsets from base pointer, this construct allows optimisation */
static char *const _gt64011p = (char *) PA_TO_KVA1(GT64011_BASE);

#define GT64011(x)		*(volatile unsigned long *)(_gt64011p + (x))

/* device is littleendian, so data may need to be swapped */
#ifdef GALILEO_PORT
#define htoll(x)     ({			\
  unsigned int v = (unsigned int)(x);	\
  v = (((v & 0x00ff) << 24) |		\
       ((v & 0xff00) <<  8) |		\
       ((v >>  8) & 0xff00) |		\
       ((v >> 24) & 0x00ff));		\
    v;					\
    })
#define ltohl(x) htoll(x)

#else
asjdsajd
#define htoll(x) (x)
#define ltohl(x) (x)
#endif
#endif /* __ASSEMBLY__ */
/* CPU configuration */
#define GT_CPU_CFG	GT64011(0x000)
#define GT_CPU_CFG_WriteMode	(1<<11)
#define GT_CPU_CFG_Endianess	(1<<12)
/* Processor Address Space */
#define GT_PAS_RAS10LO	GT64011(0x008)
#define GT_PAS_RAS10HI	GT64011(0x010)
#define GT_PAS_RAS32LO	GT64011(0x018)
#define GT_PAS_RAS32HI	GT64011(0x020)
#define GT_PAS_CS20LO	GT64011(0x028)
#define GT_PAS_CS20HI	GT64011(0x030)
#define GT_PAS_CS3BOOTLO GT64011(0x038)
#define GT_PAS_CS3BOOTHI GT64011(0x040)
#define GT_PAS_PCIIOLO	GT64011(0x048)
#define GT_PAS_PCIIOHI	GT64011(0x050)
#define GT_PAS_PCIMEMLO	GT64011(0x058)
#define GT_PAS_PCIMEMHI	GT64011(0x060)
#define GT_PAS_INTDEC	GT64011(0x068)
#define GT_PAS_BUSERRLO	GT64011(0x070)
#define GT_PAS_PCIMEM1LO GT64011(0x080)
#define GT_PAS_PCIMEM1HI GT64011(0x088)
#define GT_PAS_LOMASK_Low	0x7ff
#define GT_PAS_LOSHIFT_Low	0
#define GT_PAS_HIMASK_High	0x07f
#define GT_PAS_HISHIFT_High	0
/* DRAM and  Device Address Space */
#define GT_DDAS_RAS0LO	GT64011(0x400)
#define GT_DDAS_RAS0HI	GT64011(0x404)
#define GT_DDAS_RAS1LO	GT64011(0x408)
#define GT_DDAS_RAS1HI	GT64011(0x40c)
#define GT_DDAS_RAS2LO	GT64011(0x410)
#define GT_DDAS_RAS2HI	GT64011(0x414)
#define GT_DDAS_RAS3LO	GT64011(0x418)
#define GT_DDAS_RAS3HI	GT64011(0x41c)
#define GT_DDAS_CS0LO	GT64011(0x420)
#define GT_DDAS_CS0HI	GT64011(0x424)
#define GT_DDAS_CS1LO	GT64011(0x428)
#define GT_DDAS_CS1HI	GT64011(0x42c)
#define GT_DDAS_CS2LO	GT64011(0x430)
#define GT_DDAS_CS2HI	GT64011(0x434)
#define GT_DDAS_CS3LO	GT64011(0x438)
#define GT_DDAS_CS3HI	GT64011(0x43c)
#define GT_DDAS_BOOTCSLO	GT64011(0x440)
#define GT_DDAS_BOOTCSHI	GT64011(0x444)
#define GT_DDAS_ERROR	GT64011(0x470)
#define GT_DDAS_LOMASK_Low	0xff
#define GT_DDAS_LOSHIFT_Low	0
#define GT_DDAS_HIMASK_High	0xff
#define GT_DDAS_HISHIFT_High	0
/* DRAM Configuration */
#define GT_DRAM_CFG	GT64011(0x448)
#define GT_DRAM_CFG_RefIntCntMASK	0x00003fff
#define GT_DRAM_CFG_RefIntCntSHIFT	0
#define GT_DRAM_CFG_RefIntCnt(x)	(((x)<<GT_DRAM_CFG_RefIntCntSHIFT)&\
					 GT_DRAM_CFG_RefIntCntMASK)
#define GT_DRAM_CFG_StagRef		(1<<16)
#define GT_DRAM_CFG_StagRefOn		0
#define GT_DRAM_CFG_StagRefAll		GT_DRAM_CFG_StagRef
#define GT_DRAM_CFG_ADSFunct		(1<<17)
#define GT_DRAM_CFG_ADSFunctDRAM	0
#define GT_DRAM_CFG_ADSFunctOnly	GT_DRAM_CFG_ADSFunct
#define GT_DRAM_CFG_DRAMLatch		(1<<18)
#define GT_DRAM_CFG_DRAMLatchActive	0
#define GT_DRAM_CFG_DRAMLatchTransparent GT_DRAM_CFG_DRAMLatch
/* DRAM Parameters */
#define GT_DRAMPAR_BANK0 GT64011(0x44c)
#define GT_DRAMPAR_BANK1 GT64011(0x450)
#define GT_DRAMPAR_BANK2 GT64011(0x454)
#define GT_DRAMPAR_BANK3 GT64011(0x458)
#define GT_DRAMPAR_CASWr		(1<<0)
#define GT_DRAMPAR_CASWr1		0
#define GT_DRAMPAR_CASWr2		GT_DRAMPAR_CASWr
#define GT_DRAMPAR_RAStoCASWr		(1<<1)
#define GT_DRAMPAR_RAStoCASWr2		0
#define GT_DRAMPAR_RAStoCASWr3		GT_DRAMPAR_RAStoCASWr
#define GT_DRAMPAR_CASRd		(1<<2)
#define GT_DRAMPAR_CASRd1		0
#define GT_DRAMPAR_CASRd2		GT_DRAMPAR_CASRd
#define GT_DRAMPAR_RAStoCASRd		(1<<3)
#define GT_DRAMPAR_RAStoCASRd2		0
#define GT_DRAMPAR_RAStoCASRd3		GT_DRAMPAR_RAStoCASRd
#define GT_DRAMPAR_RefreshSHIFT		4
#define GT_DRAMPAR_RefreshMASK		(3<<4)
#define GT_DRAMPAR_Refresh512		(0<<4)
#define GT_DRAMPAR_Refresh1024		(1<<4)
#define GT_DRAMPAR_Refresh2048		(2<<4)
#define GT_DRAMPAR_Refresh4096		(3<<4)
#define GT_DRAMPAR_BankWidth		(1<<6)
#define GT_DRAMPAR_BankWidth32		0
#define GT_DRAMPAR_BankWidth64		GT_DRAMPAR_BankWidth
#define GT_DRAMPAR_BankLoc		(1<<7)
#define GT_DRAMPAR_BankLocEven		0
#define GT_DRAMPAR_BankLocOdd		GT_DRAMPAR_BankLoc
#define GT_DRAMPAR_Parity		(1<<8)
#define GT_DRAMPAR_ParityDisable	0
#define GT_DRAMPAR_ParityEnable		GT_DRAMPAR_Parity
#define GT_DRAMPAR_MBZ			(1<<9)
/* Device Parameters */
#define GT_DEVPAR_BANK0	GT64011(0x45c)
#define GT_DEVPAR_BANK1	GT64011(0x460)
#define GT_DEVPAR_BANK2	GT64011(0x464)
#define GT_DEVPAR_BANK3	GT64011(0x468)
#define GT_DEVPAR_BOOT	GT64011(0x46c)
#define GT_DEVPAR_TurnOffMASK		(7<<0)
#define GT_DEVPAR_TurnOffSHIFT		0
#define GT_DEVPAR_TurnOff(x)		((x)<<0)
#define GT_DEVPAR_AccToFirstMASK	(15<<3)
#define GT_DEVPAR_AccToFirstSHIFT	3
#define GT_DEVPAR_AccToFirst(x)		((x)<<3)
#define GT_DEVPAR_AccToNextMASK		(15<<7)
#define GT_DEVPAR_AccToNextSHIFT	7
#define GT_DEVPAR_AccToNext(x)		((x)<<7)
#define GT_DEVPAR_ADStoWrMASK		(7<<11)
#define GT_DEVPAR_ADStoWrSHIFT		11
#define GT_DEVPAR_ADStoWr(x)		((x)<<11)
#define GT_DEVPAR_WrActiveMASK		(7<<14)
#define GT_DEVPAR_WrActiveSHIFT		14
#define GT_DEVPAR_WrActive(x)		((x)<<14)
#define GT_DEVPAR_WrHighMASK		(7<<17)
#define GT_DEVPAR_WrHighSHIFT		17
#define GT_DEVPAR_WrHigh(x)		((x)<<17)
#define GT_DEVPAR_DevWidthMASK		(3<<20)
#define GT_DEVPAR_DevWidthSHIFT		20
#define GT_DEVPAR_DevWidth8		(0<<20)
#define GT_DEVPAR_DevWidth16		(1<<20)
#define GT_DEVPAR_DevWidth32		(2<<20)
#define GT_DEVPAR_DevWidth64		(3<<20)
#define GT_DEVPAR_DevLoc		(1<<23)
#define GT_DEVPAR_DevLocEven		0
#define GT_DEVPAR_DevLocOdd		GT_DEVPAR_DevLoc
#define GT_DEVPAR_LatchFunct		(1<<25)
#define GT_DEVPAR_LatchFunctTransparent 0
#define GT_DEVPAR_LatchFunctEnable	GT_DEVPAR_LatchFunct
#define GT_DEVPAR_Parity		(1<<30)
#define GT_DEVPAR_ParityDisable		0
#define GT_DEVPAR_ParityEnable		GT_DEVPAR_Parity
#define GT_DEVPAR_ReservedMASK		0x3d400000
#define GT_DEVPAR_Reserved		0x14400000
/* PCI Internal */
#define GT_IPCI_CMD GT64011(0xc00)
#define GT_IPCI_CMD_ByteSwap		(1<<0)
#define GT_IPCI_CMD_ByteSwapOn		0
#define GT_IPCI_CMD_ByteSwapOff		GT_INTPCI_CMD_ByteSwap
#define GT_IPCI_CMD_SyncModeMASK	(3<<1)
#define GT_IPCI_CMD_SyncModeSHIFT	1
#define GT_IPCI_CMD_SyncModeStd		(0<<1)
#define GT_IPCI_CMD_SyncMode1		(1<<1)
#define GT_IPCI_CMD_SyncMode2		(2<<1)
#define GT_IPCI_TOR	GT64011(0xc04)
#define GT_IPCI_TOR_Timeout0MASK	(255<<0)
#define GT_IPCI_TOR_Timeout0SHIFT	0
#define GT_IPCI_TOR_Timeout0(x)	((x)<<0)
#define GT_IPCI_TOR_Timeout1MASK	(255<<8)
#define GT_IPCI_TOR_Timeout1SHIFT	8
#define GT_IPCI_TOR_Timeout1(x)	((x)<<8)
#define GT_IPCI_TOR_RetryCtrMASK	(255<<16)
#define GT_IPCI_TOR_RetryCtrSHIFT	16
#define GT_IPCI_TOR_RetryCtr(x)		((x)<<16)
#define GT_IPCI_RAS10SIZE	GT64011(0xc08)
#define GT_IPCI_RAS32SIZE	GT64011(0xc0c)
#define GT_IPCI_CS20SIZE	GT64011(0xc10)
#define GT_IPCI_CS3BOOTSIZE	GT64011(0xc14)
#define GT_IPCI_SIZE_BankSizeMASK	(0xfffff<<12)
#define GT_IPCI_SIZE_BankSizeSHIFT	12
#define GT_IPCI_INTRCAUSE	GT64011(0xc18)
#define GT_IPCI_INTRMASK	GT64011(0xc1c)
#define  GT_INTR_INTSUM			0x0000001
#define  GT_INTR_MEMOUT			0x0000002
#define  GT_INTR_DMAOUT			0x0000004
#define  GT_INTR_CPUOUT			0x0000008
#define  GT_INTR_DMA0COMP		0x0000010
#define  GT_INTR_DMA1COMP		0x0000020
#define  GT_INTR_DMA2COMP		0x0000040
#define  GT_INTR_DMA3COMP		0x0000080
#define  GT_INTR_T0EXP			0x0000100
#define  GT_INTR_T1EXP			0x0000200
#define  GT_INTR_T2EXP			0x0000400
#define  GT_INTR_T3EXP			0x0000800
#define  GT_INTR_MASRDERR		0x0001000
#define  GT_INTR_SLVWRERR		0x0002000
#define  GT_INTR_MASWRERR		0x0004000
#define  GT_INTR_SLVRDERR		0x0008000
#define  GT_INTR_ADDRERR		0x0010000
#define  GT_INTR_MEMERR			0x0020000
#define  GT_INTR_MASABORT		0x0040000
#define  GT_INTR_TARABORT		0x0080000
#define  GT_INTR_RETRYCTR		0x0010000
#define  GT_INTR_CPU2PCIA		0x0020000
#define  GT_INTR_CPU2PCIB		0x0040000
#define  GT_INTR_CPU2PCIC		0x0080000
#define  GT_INTR_CPU2PCID		0x0100000
#define  GT_INTR_CPU2PCIE		0x0200000
#define  GT_INTR_PCI2CPUA		0x0400000
#define  GT_INTR_PCI2CPUB		0x0800000
#define  GT_INTR_PCI2CPUC		0x1000000
#define  GT_INTR_PCI2CPUD		0x2000000
#define  GT_INTR_CPUINTSUM		0x4000000
#define  GT_INTR_PCIINTSUM		0x8000000
#define GT_IPCI_PCIINTMASK	GT64011(0xc24)
#define GT_IPCI_SERMASK		GT64011(0xc28)
#define GT_IPCI_SERMASK_AddrErr		(1<<0)
#define GT_IPCI_SERMASK_MasWrErr	(1<<1)
#define GT_IPCI_SERMASK_MasRdErr	(1<<2)
#define GT_IPCI_SERMASK_MemErr		(1<<3)
#define GT_IPCI_SERMASK_MasAbort	(1<<4)
#define GT_IPCI_SERMASK_TarAbort	(1<<5)
#define GT_IPCI_INTACK		GT64011(0xc34)
#define GT_IPCI_BAREN		GT64011(0xc3c)
#define GT_IPCI_BAREN_SwCs3BootDis	(1<<0)
#define GT_IPCI_BAREN_SwRas32Dis	(1<<1)
#define GT_IPCI_BAREN_SwRas10Dis	(1<<2)
#define GT_IPCI_BAREN_IntIODis		(1<<3)
#define GT_IPCI_BAREN_IntMemDis		(1<<4)
#define GT_IPCI_BAREN_Cs3BootDis	(1<<5)
#define GT_IPCI_BAREN_Cs20Dis		(1<<6)
#define GT_IPCI_BAREN_Ras32Dis		(1<<7)
#define GT_IPCI_BAREN_Ras10Dis		(1<<8)
#define GT_IPCI_CFGADDR		GT64011(0xcf8)
#define GT_IPCI_CFGDATA		GT64011(0xcfc)
#define GT_IPCI_CFGADDR_RegNumMASK	(0x3f<<2)
#define GT_IPCI_CFGADDR_RegNumSHIFT	2
#define GT_IPCI_CFGADDR_RegNum(x)	((x)<<2)
#define GT_IPCI_CFGADDR_FunctNumMASK	(0x7<<8)
#define GT_IPCI_CFGADDR_FunctNumSHIFT	8
#define GT_IPCI_CFGADDR_FunctNum(x)	((x)<<8)
#define GT_IPCI_CFGADDR_DevNumMASK	(0x1f<<11)
#define GT_IPCI_CFGADDR_DevNumSHIFT	11
#define GT_IPCI_CFGADDR_DevNum(x)	((x)<<11)
#define GT_IPCI_CFGADDR_BusNumMASK	(0xff<<16)
#define GT_IPCI_CFGADDR_BusNumSHIFT	16
#define GT_IPCI_CFGADDR_BusNum(x)	((x)<<16)
#define GT_IPCI_CFGADDR_ConfigEn	(1<<31)
