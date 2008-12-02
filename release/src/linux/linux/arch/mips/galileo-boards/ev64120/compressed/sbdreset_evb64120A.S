/*
 * Copyright 1997 Algorithmics Ltd
 *	All Rights Reserved
 *
 * gal9/sbdreset.sx -- low level board dependent routines
 */

#ifdef EVB64120A

#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/cacheops.h>
#include <asm/current.h>
#include <asm/offset.h>
#include <asm/processor.h>
#include <asm/regdef.h>
#include <asm/cachectl.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#include <asm/cpu.h>
#include "sbd.h"


#include "gt64011.h"
#include "ns16550.h"

#ifdef GALILEO_PORT     // miniBios crack
#define C0_CONFIG CP0_CONFIG
#define C0_STATUS CP0_STATUS
#define C0_TLBLO0 CP0_ENTRYLO0
#define C0_TLBLO1 CP0_ENTRYLO1
#define C0_PGMASK CP0_PAGEMASK
#define C0_TLBHI CP0_ENTRYHI
#define C0_INX CP0_INDEX
#define NTLBENTRIES     48


#define CFG_IB CONF_IB
#define CFG_DB CONF_DB
#define CFG_C_NONCOHERENT CONF_CM_CACHABLE_NONCOHERENT
#define C0_SR CP0_STATUS
#define SR_DE ST0_DE


        #define SLEAF(x) LEAF(x)
        #define SEND(x) END(x)
        #define XLEAF(x) LEAF(x)
        #define SBD_DISPLAY(a,b,c,d,e) ;

#define K0BASE          0x80000000
#define K0SIZE          0x20000000
#define K1BASE          0xa0000000
#define K1SIZE          0x20000000
#define K2BASE          0xc0000000

#define PHYS_TO_K0(pa)  ((pa)|K0BASE)
#define PHYS_TO_K1(pa)  ((pa)|K1BASE)
#define K0_TO_PHYS(va)  ((va)&(K0SIZE-1))
#define K1_TO_PHYS(va)  ((va)&(K1SIZE-1))
#define K0_TO_K1(va)    ((va)|K1SIZE)
#define K1_TO_K0(va)    ((va)&~K1SIZE)

#define PA_TO_KVA0(pa)  PHYS_TO_K0(pa)
#define PA_TO_KVA1(pa)  PHYS_TO_K1(pa)
#define KVA_TO_PA(pa)   K1_TO_PHYS(pa)
#define KSEG0_BASE      K0BASE
#define KSEG1_BASE      K1BASE


#endif

#define MB	0x100000

#define MemTypeNone		0x8000
#define MemRasMask		0x0f00
#define MemRasShift		8
#define MemCasMask		0x000f
#define MemCasShift 		0

#define rasave	s0
#define p64011	s1
#define bank0	s2
#define bank1	s3
#define bank2	s4
#define bank3	s5
#define memtop	s6
#define membase	s7

/*#if #endian(big)	*/
#ifdef __MIPSEB__

#define HTOLL(sr,tr) \
	.set noat ; \
	srl	AT,sr,24 ; \
	srl	tr,sr,8 ; \
	and	tr,0xff00 ; \
	or	AT,tr ; \
	and	tr,sr,0xff00 ; \
	sll	tr,8 ; \
	or	AT,tr ; \
	sll	tr,sr,24 ; \
	or	sr,AT,tr ; \
	.set at
#else
#define HTOLL(sr,tr)
#endif

#undef DBGSBD

#ifdef DBGSBD
#define DBG(s) \
	.rdata ; \
88:	.asciiz	s ; \
	.text ; \
	la	a0, 88b ; \
	jal	_dbgmsg

LEAF(_dbgmsg)
	.set noat
	li	AT,PHYS_TO_K1(NS16550_CHANB)
waitrdy:
	lbu	v1,LSR(AT)
	.set noreorder;	nop; nop; nop; nop; nop; nop; nop; nop; .set reorder
	and	v1,LSR_TXRDY
	beqz	v1,waitrdy

	lbu	v1,(a0)
	addu	a0,1
	beqz	v1,9f
	sb	v1,DATA(AT)
	.set noreorder;	nop; nop; nop; nop; nop; nop; nop; nop; .set reorder
	b	waitrdy
9:	j	ra
	.set at
END(_dbgmsg)

LEAF(_dbghex)
	li	a1,PHYS_TO_K1(NS16550_CHANB)
	li	t0,8
1:
	lbu	t1,LSR(a1)
	.set noreorder;	nop; nop; nop; nop; nop; nop; nop; nop; .set reorder
	and	t1,LSR_TXRDY
	beqz	t1,1b

	srl	t1,a0,28
	addu	t1,'0'
	ble	t1,'9',2f
	addu	t1,'a'-'0'-10
2:	sb	t1,DATA(a1)
	.set noreorder;	nop; nop; nop; nop; nop; nop; nop; nop; .set reorder

	sll	a0,4
	sub	t0,1
	bnez	t0,1b

	j	ra
	.set at
END(_dbghex)

	.rdata
initb_str:
	.byte	9,0x40	/* Reset CH B */
	.byte	1,0x00	/* Interrupt disabled */
	.byte	3,0xc1	/* 8 bits/char rx enable */
	.byte	4,0x44	/* x16 clk mode 1 stop bit */
	.byte	5,0x6a	/* tx 8/bit RTS & tx enable */
	.byte	9,0x0a	/* MIE Master int enab. and NV No Vector */
	.byte	11,0x50	/* Select BR gen. out for both rx and ts */
	.byte	0,0x10
	.byte	0,0x10
	.byte	14,0x01	/* enable baud rate gen. */
	.byte	15,0x00	/* known state for reg 15 */

	.byte	14,0x00	/* disable baud rate gen. */
	.byte	12,0x0a /* 0x0a	= 9600 baud time const. - lower 8 bits */
	.byte	13,0x00	/* 9600 buad time const. - upper 8 bits */
	.byte	14,0x01	/* enable baud rate gen. */
	.byte	0xff

	.text

SLEAF(_dbginit)
        /*
        li	v0,PHYS_TO_K1(NS16550_CHANB)
	la	a0,initb_str
	or	a0,K1BASE
1:	lbu	t0,0(a0)
	beq	t0,0xff,1f
	sb	t0,LSR(v0)
	.set noreorder;	nop; nop; nop; nop; nop; nop; nop; nop; .set reorder
	addu	a0,1
	b	1b
        */
        jal     init_ns16550_chan_b # Debug channel
	j	ra
SEND(_dbginit)
#else
#define DBG(s)
#endif

LEAF(sbdreset)
	move	rasave,ra

	/* if launched by ITROM, leave Config alone */
#ifndef ITBASE
	/* set config register for 32b/32b cachelines, kseg0 cacheable */
	mfc0	t1,C0_CONFIG
	and	t1,~0x3f		# set bits 5..0 only
	or	t1,CFG_IB | CFG_DB | CFG_C_NONCOHERENT
	mtc0	t1,C0_CONFIG
#endif

       /* Initialize stack pointer to 6MB address */
         li sp,0xa0600000


        /*
	 * slight amount of kludgery here to stop RAM resident
	 * program from overwriting itself...
	 */
//	li	v1,0x1fc00000		/* check return address is in ROM */
//	and	v0,ra,v1
//	bne	v0,v1,.noinit

        /* table driven hardware register initialization */
        la      a0, reginittab
        or      a0, K1BASE              /* force to kseg1 */

1:      lw      v0,0(a0)
        lw      v1,4(a0)
        addu    a0,8
        beqz    v0,8f

        sw      v1,0(v0)
        b       1b
8:

#ifdef DBGSBD
	jal     init_ns16550_chan_b   # was - _dbginit
	DBG("sbdreset\r\n")
#endif
#define DEVICE_BANK0PARAMETERS                                                          0x45C
#define DEVICE_BANK1PARAMETERS                                                          0x460
#define DEVICE_BANK2PARAMETERS                                                          0x464
#define DEVICE_BANK3PARAMETERS                                                          0x468
#define DEVICE_BOOT_BANK_PARAMETERS                                                     0x46C
#define GT_INTERNAL_REG_BASE 0xb4000000

        li      p64011, PA_TO_KVA1(GT64011_BASE)

        li  v0,0xb400046c       /* Boot Device */
        lw  t0,0(v0)
        and t0,0x00003000       /* Keep the correct boot size */
	or  t0,htoll(0x3847de70)
	sw  t0,0(v0)

        li  v0,0xb4000468       /* CS3 Device - 16 bit FLASH memory */
        li  t0,htoll(0x3859e6e8)
	sw  t0,0(v0)


        li  v0,0xb4000c84       /* PCI 1 timeout register */
        li  t0,htoll(0xffff)
	sw  t0,0(v0)


        li  v0,0xb4000c3c       /* Enable I/O response on PCI0 */
        li  t0,htoll(0x7)
	sw  t0,0(v0)

        li  v0,0xb4000cbc       /* Enable I/O response on PCI1 */
        li  t0,htoll(0x7)
	sw  t0,0(v0)

        /* GT-64120 Initialization */

        li      p64011, PA_TO_KVA1(GT64011_BASE)

        /*********************************************************************/
        /************************* SDRAM initializing ************************/
        /******************************* START *******************************/


                                        /* SDRAM banks 0,1,2,3 parameters               */
        li      t0,htoll(0x01908200)    /* - Standard Monitor: Interleave enabled       */
        li      v0,0xb4000448           /* - Registered SDRAM (Bit 23)                  */
        sw      t0,0(v0)                /* - Duplicate Dadr11,BankSel1 and Dadr12       */
                                        /* - Cas latency: 2 Cycles                      */
                                        /* - Flow Through enable: One sample            */
                                        /* - SRAS - precharge time: 3 Cycles            */
                                        /* - No ECC                                     */
                                        /* - No ByPass                                  */
                                        /* - Burst length: 8                            */

        /* Detect whether we have a 16,64,128 or 256 Mbit SDRAM on DIMM0 */
        /* Set bank0`s range to: 0 - 0x10000000 (256 MByte)     */
_DIMM0:
        li  v0,0xb4000008
	li  t0,htoll(0x0)
	sw  t0,0(v0)

        li  v0,0xb4000010
	li  t0,htoll(0x7f)
	sw  t0,0(v0)

        /* Close banks 2 and 3 */
        li  v0,0xb4000018
	li  t0,htoll(0x7ff)
	sw  t0,0(v0)
        li  v0,0xb4000020
	li  t0,htoll(0x00)
	sw  t0,0(v0)

        /* Extend bank0 to 0x10000000 and Close bank1,2 and 3 */
        DBG("Extend bank0 to 0x10000000 and Close bank1,2 and 3...\r\n")
        li  v0,0xb4000400
	li  t0,htoll(0x0)
	sw  t0,0(v0)
        li  v0,0xb4000404
	li  t0,htoll(0xff)
	sw  t0,0(v0)
        li  v0,0xb4000408
	li  t0,htoll(0xff)
	sw  t0,0(v0)
        li  v0,0xb400040c
	li  t0,htoll(0x00)
	sw  t0,0(v0)
        li  v0,0xb4000410
	li  t0,htoll(0xff)
	sw  t0,0(v0)
        li  v0,0xb4000414
	li  t0,htoll(0x00)
	sw  t0,0(v0)
        li  v0,0xb4000418
	li  t0,htoll(0xff)
	sw  t0,0(v0)
        li  v0,0xb400041c
	li  t0,htoll(0x00)
	sw  t0,0(v0)

        /* Configure bank0 to 256 Mbit */
        DBG("Configure bank0 to 256 Mbit...\r\n")
        li  v0,0xb400044c
	li  t0,htoll(0x00004c69)
        sw  t0,0(v0)

        /* Config the SDRAM banks decode system */
	li  v0,0xb400047c
	li  t0,htoll(2)
	sw  t0,0(v0)

        li  v0,0xb4000474
	li  t0,htoll(0x3)
	sw  t0,0(v0)

        li  v0,0xa0000000
        li  t0,0
        sw  t0,0(v0)

        li  v0,0xb4000474
	li  t0,htoll(0x0)
	sw  t0,0(v0)

        /* Write to address 0x2000000 and check if 0x00000000 is being written too */
        DBG("Write to address 0x2000000 and check if 0x00000000 is being written too...\r\n")
        li  v0,0xa0000000
        li  t1,0xa0000010
        li  t0,htoll(0x0)
1:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,1b

        /* The address should activate Dadr12 */
        li  v0,0xa2000000
	li  t0,0x11111111
        sw  t0,0(v0)
        li  v0,0xa0000010
        li  t1,0xa0000100
        li  t0,0x22222222
2:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,2b

        DBG("Check address 0x00000000 for duplications...\r\n")
        li    t0,0xa0000000
        li    v0,0x11111111
        lw    t0,(t0)
        bne   t0,v0,_256MBIT

        /* Write to address 0x1000 and check if 0x00000000 is being written too */
        DBG("Write to address 0x1000 and check if 0x00000000 is being written too...\r\n")
        li  v0,0xa0000000
        li  t1,0xa0000010
        li  t0,htoll(0x0)
1:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,1b

        /* The address should activate bank select1*/
        li  v0,0xa0001000
	li  t0,0x11111111
        sw  t0,0(v0)
        li  v0,0xa0000010
        li  t1,0xa0000100
        li  t0,0x22222222
2:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,2b

        DBG("Check address 0x00000000 for duplications...\r\n")
        li    t0,0xa0000000
        li    v0,0x11111111
        lw    t0,(t0)
        beq   t0,v0,_16MBIT

        /* Write to address 0x8000000 and check if 0x00000000 is being written too */
        DBG("Write to address 0x8000000 and check if 0x00000000 is being written too...\r\n")
        li  v0,0xa0000000
        li  t1,0xa0000010
        li  t0,htoll(0x0)
1:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,1b

        /* The address should activate Dadr9 which on the column cycle is in active with 64 Mbit
           device */
        li  v0,0xa8000000
	li  t0,0x11111111
        sw  t0,0(v0)
        li  v0,0xa0000010
        li  t1,0xa0000100
        li  t0,0x22222222
2:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,2b

        DBG("Check address 0x00000000 for duplications...\r\n")
        li    t0,0xa0000000
        li    v0,0x11111111
        lw    t0,(t0)
        beq   t0,v0,_64MBIT
        b     _128MBIT

_16MBIT:
        DBG("16 Mbit SDRAM detected...\r\n")
        /* In 16 Mbit SDRAM we must use 2 way bank interleaving!!! */
        li  v0,0xb4000810
	li  t0,htoll(16)
	sw  t0,0(v0)
        li  t1,htoll(0x00000449)
        b   _DIMM1

_64MBIT:
        DBG("64 Mbit SDRAM detected...\r\n")
        /* In 64 Mbit SDRAM we must use 4 way bank interleaving!!! */
        li  v0,0xb4000810
	li  t0,htoll(64)
	sw  t0,0(v0)
        li  t1,htoll(0x00000c69)
        b   _DIMM1

_128MBIT:
        DBG("128 Mbit SDRAM detected...\r\n")
        /* In 128 Mbit SDRAM we must use 4 way bank interleaving!!! */
        li  v0,0xb4000810
	li  t0,htoll(128)
	sw  t0,0(v0)
        li  t1,htoll(0x00000c69)
        b   _DIMM1

_256MBIT:
        DBG("256 Mbit SDRAM detected...\r\n")
        /* In 256 Mbit SDRAM we must use 4 way bank interleaving!!! */
        li  v0,0xb4000810
	li  t0,htoll(256)
	sw  t0,0(v0)
        li  t1,htoll(0x00004c69)
        b   _DIMM1

_DIMM1:
        li  v0,0xb400044c
        sw  t1,0(v0)  # Bank0
        sw  t1,4(v0)  # Bank1

        /* Detect whether we have a 16,64,128 or 256 Mbit SDRAM on DIMM1 */
        /* Close banks 0 and 1 */
        li  v0,0xb4000008
	li  t0,htoll(0xff)
	sw  t0,0(v0)

        li  v0,0xb4000010
	li  t0,htoll(0x0)
	sw  t0,0(v0)

        /* Set bank2`s range to: 0 - 0x10000000 (256 MByte)     */
        li  v0,0xb4000018
	li  t0,htoll(0x0)
	sw  t0,0(v0)
        li  v0,0xb4000020
	li  t0,htoll(0x7f)
	sw  t0,0(v0)

        /* Extend bank2 to 0x10000000 and Close bank0,1 and 3 */
        DBG("Extend bank2 to 0x10000000 and Close banks 0,1 and 3...\r\n")
        li  v0,0xb4000400
	li  t0,htoll(0xff)
	sw  t0,0(v0)
        li  v0,0xb4000404
	li  t0,htoll(0x00)
	sw  t0,0(v0)
        li  v0,0xb4000408
	li  t0,htoll(0xff)
	sw  t0,0(v0)
        li  v0,0xb400040c
	li  t0,htoll(0x00)
	sw  t0,0(v0)
        li  v0,0xb4000410
	li  t0,htoll(0x00)
	sw  t0,0(v0)
        li  v0,0xb4000414
	li  t0,htoll(0xff)
	sw  t0,0(v0)
        li  v0,0xb4000418
	li  t0,htoll(0xff)
	sw  t0,0(v0)
        li  v0,0xb400041c
	li  t0,htoll(0x00)
	sw  t0,0(v0)

        /* Configure bank2 to 256 Mbit */
        DBG("Configure bank2 to 256 Mbit...\r\n")
        li  v0,0xb4000454
	li  t0,htoll(0x00004c69)
        sw  t0,0(v0)

        /* Config the SDRAM banks decode system */
	li  v0,0xb400047c
	li  t0,htoll(2)
	sw  t0,0(v0)

        li  v0,0xb4000474
	li  t0,htoll(0x3)
	sw  t0,0(v0)

        li  v0,0xa0000000
        li  t0,0
        sw  t0,0(v0)

        li  v0,0xb4000474
	li  t0,htoll(0x0)
	sw  t0,0(v0)

        /* Write to address 0x2000000 and check if 0x00000000 is being written too */
        DBG("Write to address 0x2000000 and check if 0x00000000 is being written too...\r\n")
        li  v0,0xa0000000
        li  t1,0xa0000010
        li  t0,htoll(0x0)
1:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,1b

        /* The address should activate Dadr12 */
        li  v0,0xa2000000
	li  t0,0x11111111
        sw  t0,0(v0)
        li  v0,0xa0000010
        li  t1,0xa0000100
        li  t0,0x22222222
2:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,2b

        DBG("Check address 0x00000000 for duplications...\r\n")
        li    t0,0xa0000000
        li    v0,0x11111111
        lw    t0,(t0)
        bne   t0,v0,_256MBIT2

        /* Write to address 0x1000 and check if 0x00000000 is being written too */
        DBG("Write to address 0x1000 and check if 0x00000000 is being written too...\r\n")
        li  v0,0xa0000000
        li  t1,0xa0000010
        li  t0,htoll(0x0)
1:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,1b

        /* The address should activate bank select1*/
        li  v0,0xa0001000
	li  t0,0x11111111
        sw  t0,0(v0)
        li  v0,0xa0000010
        li  t1,0xa0000100
        li  t0,0x22222222
2:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,2b

        DBG("Check address 0x00000000 for duplications...\r\n")
        li    t0,0xa0000000
        li    v0,0x11111111
        lw    t0,(t0)
        beq   t0,v0,_16MBIT2

        /* Write to address 0x8000000 and check if 0x00000000 is being written too */
        DBG("Write to address 0x8000000 and check if 0x00000000 is being written too...\r\n")
        li  v0,0xa0000000
        li  t1,0xa0000010
        li  t0,htoll(0x0)
1:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,1b

        /* The address should activate Dadr9 which on the column cycle is in active with 64 Mbit
           device */
        li  v0,0xa8000000
	li  t0,0x11111111
        sw  t0,0(v0)
        li  v0,0xa0000010
        li  t1,0xa0000100
        li  t0,0x22222222
2:      sw  t0,0(v0)
        addu v0,4
        bne t1,v0,2b

        DBG("Check address 0x00000000 for duplications...\r\n")
        li    t0,0xa0000000
        li    v0,0x11111111
        lw    t0,(t0)
        beq   t0,v0,_64MBIT2
        b     _128MBIT2

_16MBIT2:
        DBG("16 Mbit SDRAM detected...\r\n")
        /* In 16 Mbit SDRAM we must use 2 way bank interleaving!!! */
        li  v0,0xb4000814
	li  t0,htoll(16)
	sw  t0,0(v0)
        li  t1,htoll(0x00000449)
        b   _INIT_SDRAM

_64MBIT2:
        DBG("64 Mbit SDRAM detected...\r\n")
        /* In 64 Mbit SDRAM we must use 4 way bank interleaving!!! */
        li  v0,0xb4000814
	li  t0,htoll(64)
	sw  t0,0(v0)
        li  t1,htoll(0x00000c69)
        b   _INIT_SDRAM

_128MBIT2:
        DBG("128 Mbit SDRAM detected...\r\n")
        /* In 128 Mbit SDRAM we must use 4 way bank interleaving!!! */
        li  v0,0xb4000814
	li  t0,htoll(128)
	sw  t0,0(v0)
        li  t1,htoll(0x00000c69)
        b   _INIT_SDRAM

_256MBIT2:
        DBG("256 Mbit SDRAM detected...\r\n")
        /* In 256 Mbit SDRAM we must use 4 way bank interleaving!!! */
        li  v0,0xb4000814
	li  t0,htoll(256)
	sw  t0,0(v0)
        li  t1,htoll(0x00004c69)
        b   _INIT_SDRAM

_INIT_SDRAM:
        /* Restore defaults */
        DBG("Restoring defaults...\r\n")
        li  v0,0xb4000404
	li  t0,htoll(0x07)
	sw  t0,0(v0)
        li  v0,0xb4000408
	li  t0,htoll(0x08)
	sw  t0,0(v0)
        li  v0,0xb400040c
	li  t0,htoll(0x0f)
	sw  t0,0(v0)
        li  v0,0xb4000410
	li  t0,htoll(0x10)
	sw  t0,0(v0)
        li  v0,0xb4000414
	li  t0,htoll(0x17)
	sw  t0,0(v0)
        li  v0,0xb4000418
	li  t0,htoll(0x18)
	sw  t0,0(v0)
        li  v0,0xb400041c
	li  t0,htoll(0x1f)
	sw  t0,0(v0)
        li  v0,0xb4000010
	li  t0,htoll(0x07)
	sw  t0,0(v0)
        li  v0,0xb4000018
	li  t0,htoll(0x008)
	sw  t0,0(v0)
        li  v0,0xb4000020
	li  t0,htoll(0x0f)
	sw  t0,0(v0)

        li  v0,0xb400044c
        sw  t1,8(v0)  # Bank2
        sw  t1,12(v0) # Bank3

        li  v0,0xb4000474
	li  t0,htoll(0x3)
	sw  t0,0(v0)

        li  v0,0xa0000000
        li  t0,0
        sw  t0,0(v0)

        li  v0,0xb4000474
	li  t0,htoll(0x0)
	sw  t0,0(v0)

        li  v0,0xb4000474
	li  t0,htoll(0x3)
	sw  t0,0(v0)

        li  v0,0xa0800000
        li  t0,0
        sw  t0,0(v0)

        li  v0,0xb4000474
	li  t0,htoll(0x0)
	sw  t0,0(v0)

        li  v0,0xb4000474
	li  t0,htoll(0x3)
	sw  t0,0(v0)

        li  v0,0xa1000000
        li  t0,0
        sw  t0,0(v0)

        li  v0,0xb4000474
	li  t0,htoll(0x0)
	sw  t0,0(v0)

        li  v0,0xb4000474
	li  t0,htoll(0x3)
	sw  t0,0(v0)

        li  v0,0xa1800000
        li  t0,0
        sw  t0,0(v0)

        li  v0,0xb4000474
	li  t0,htoll(0x0)
	sw  t0,0(v0)

        /*********************************************************************/
        /************************* SDRAM initializing ************************/
        /******************************* END *********************************/

        li      p64011, PA_TO_KVA1(GT64011_BASE)

        li      t0,htoll(0x00000000)    /* RAS[1:0] low decode address */
        sw      t0,0x008(p64011)

        li      t0,htoll(0x00000007)    /* RAS[1:0] high decode address */
        sw      t0,0x010(p64011)

        li      t0,htoll(0x00000000)    /* RAS[0] Low decode address */
        sw      t0,0x400(p64011)

        li      t0,htoll(0x0000000f)    /* RAS[0] High decode address */
        sw      t0,0x404(p64011)

        li      t0,htoll(0x00000008)    /* RAS[3:2] low decode address */
        sw      t0,0x018(p64011)

        li      t0,htoll(0x0000000f)    /* RAS[3:2] high decode address */
        sw      t0,0x020(p64011)

        li      t0,htoll(0x0000000f)    /* RAS[1] Low Decode Address */
        sw      t0,0x408(p64011)

        li      t0,htoll(0x00000008)    /* RAS[1] High Decode Address */
        sw      t0,0x40c(p64011)

        li      t0,htoll(0x00000010)    /* RAS[2] Low Decode Address */
        sw      t0,0x410(p64011)

        li      t0,htoll(0x00000017)    /* RAS[2] High Decode Address */
        sw      t0,0x414(p64011)

        li      t0,htoll(0x00000018)    /* RAS[3] Low Decode Address */
        sw      t0,0x418(p64011)

        li      t0,htoll(0x0000001f)    /* RAS[3] High Decode Address <<<<<< 1*/
        sw      t0,0x41c(p64011)

#ifdef DBGSBD
#define DREG(str,rname) \
	DBG(str); \
	DBG(":\t") ;			\
	lw	a0,rname(p64011) ;	\
	HTOLL(a0,t0) ;			\
	jal	_dbghex ;		\
	DBG("\r\n")

	DBG("GT-64120 settings:\r\n")
        DREG("DRAMPAR_BANK0   (44c)",GT_DRAMPAR_BANK0)
        DREG("DRAMPAR_BANK1   (450)",GT_DRAMPAR_BANK1)
        DREG("DRAMPAR_BANK2   (454)",GT_DRAMPAR_BANK2)
        DREG("DRAMPAR_BANK3   (458)",GT_DRAMPAR_BANK3)
        DREG("PAS_RAS10LO     (008)",GT_PAS_RAS10LO)
        DREG("PAS_RAS10HI     (010)",GT_PAS_RAS10HI)
        DREG("PAS_RAS32LO     (018)",GT_PAS_RAS32LO)
        DREG("PAS_RAS32HI     (020)",GT_PAS_RAS32HI)
        DREG("DDAS_RAS0LO     (400)",GT_DDAS_RAS0LO)
        DREG("DDAS_RAS0HI     (404)",GT_DDAS_RAS0HI)
        DREG("DDAS_RAS1LO     (408)",GT_DDAS_RAS1LO)
        DREG("DDAS_RAS1HI     (40c)",GT_DDAS_RAS1HI)
        DREG("DDAS_RAS2LO     (410)",GT_DDAS_RAS2LO)
        DREG("DDAS_RAS2HI     (414)",GT_DDAS_RAS2HI)
        DREG("DDAS_RAS3LO     (418)",GT_DDAS_RAS3LO)
        DREG("DDAS_RAS3HI     (41c)",GT_DDAS_RAS3HI)
        DREG("GT_DRAM_CFG     (448)",GT_DRAM_CFG)
        DREG("GT_DEVPAR_BANK0 (45c)",GT_DEVPAR_BANK0)
        DREG("GT_DEVPAR_BANK1 (460)",GT_DEVPAR_BANK1)
        DREG("GT_DEVPAR_BANK2 (464)",GT_DEVPAR_BANK2)
        DREG("GT_DEVPAR_BANK3 (468)",GT_DEVPAR_BANK3)
        DREG("GT_IPCI_TOR     (c04)",GT_IPCI_TOR)
#endif

	/* we can now initialise the caches for a fast clear_mem */
	SBD_DISPLAY ('C','A','C','H',CHKPNT_CACH)
	DBG("init_cache\r\n")
//	jal	mips_init_cache

.noinit:

	/* initialise tlb */
	SBD_DISPLAY ('I','T','L','B', CHKPNT_ITLB)
	DBG("init_tlb\r\n")
//	bal	init_tlb

//	DBG("sbdreset completed\r\n")
//	move	ra,rasave
        j       GetExtendedMemorySize
        nop

END(sbdreset)

LEAF(_sbd_memfail)
	SBD_DISPLAY ('!','M','E','M',CHKPNT_0MEM)
1:	b	1b
	j	ra
END(_sbd_memfail)

	.rdata
RefreshBits:
	.word	htoll(GT_DRAMPAR_Refresh512)
	.word	htoll(GT_DRAMPAR_Refresh1024)
	.word	htoll(GT_DRAMPAR_Refresh2048)
	.word	htoll(GT_DRAMPAR_Refresh4096)
	.text

/* DRAM: */
#define GT_DRAM_CFG_INIT \
        GT_DRAM_CFG_RefIntCnt(160) | \
        GT_DRAM_CFG_StagRefOn | \
        GT_DRAM_CFG_ADSFunctDRAM | \
        GT_DRAM_CFG_DRAMLatchActive

/* serial port:  widest timings even 8 bit bus, latch enabled no parity */
#define GT_DEVPAR_SERIALINIT \
        GT_DEVPAR_TurnOff(7) | \
        GT_DEVPAR_AccToFirst(15) | \
        GT_DEVPAR_AccToNext(15) | \
        GT_DEVPAR_ADStoWr(7) | \
        GT_DEVPAR_WrActive(7) | \
        GT_DEVPAR_WrHigh(7) | \
        GT_DEVPAR_DevWidth8 | \
        GT_DEVPAR_DevLocEven | \
        GT_DEVPAR_LatchFunctTransparent | \
        GT_DEVPAR_ParityDisable | \
        GT_DEVPAR_Reserved

/* PCI: */
#define GT_IPCI_TOR_INIT \
        GT_IPCI_TOR_Timeout0(255) | \
        GT_IPCI_TOR_Timeout1(255) | \
        GT_IPCI_TOR_RetryCtr(0)

#define INIT(addr,val) \
        .word   addr, val
#define GTINIT(addr,val) \
        INIT(PHYS_TO_K1(GT64011_BASE+(addr)), htoll(val))

        .rdata
reginittab:

        /* disable ras1:0 and ras3:2 decodes */
        GTINIT(GT_PAS_RAS10LO,  GT_PAS_LOMASK_Low);
        GTINIT(GT_PAS_RAS10HI,  0);
        GTINIT(GT_PAS_RAS32LO,  GT_PAS_LOMASK_Low);
        GTINIT(GT_PAS_RAS32HI,  0);

        /* disable RAS[0123] */
        GTINIT(GT_DDAS_RAS0LO,  GT_DDAS_LOMASK_Low)
        GTINIT(GT_DDAS_RAS0HI,  0);
        GTINIT(GT_DDAS_RAS1LO,  GT_DDAS_LOMASK_Low)
        GTINIT(GT_DDAS_RAS1HI,  0);
        GTINIT(GT_DDAS_RAS2LO,  GT_DDAS_LOMASK_Low)
        GTINIT(GT_DDAS_RAS2HI,  0);
        GTINIT(GT_DDAS_RAS3LO,  GT_DDAS_LOMASK_Low)
        GTINIT(GT_DDAS_RAS3HI,  0);

        /* 0x45c, 0x460, 0x464, 0x468 */
	/*GTINIT(GT_DEVPAR_BANK0, GT_DEVPAR_SERIALINIT)*/
        GTINIT(GT_DEVPAR_BANK0, 0x3847de60)
        GTINIT(GT_DEVPAR_BANK1, 0x146fffff)
        GTINIT(GT_DEVPAR_BANK2, 0x144fffff)
        GTINIT(GT_DEVPAR_BANK3, 0x167fffff)

        GTINIT(GT_IPCI_TOR,     GT_IPCI_TOR_INIT)
        INIT(0,0)
        .text

        .globl sbddelay

LEAF(sbdberrenb)
	mfc0	v0,C0_SR
	li	t0,SR_DE
	bnez	a0,1f
	or	t1,v0,t0	# disable cache/parity errors (SR_DE = 1)
	b	2f
1:	not	t1,t0		# enable cache/parity errors (SR_DE = 0)
	and	t1,v0
2:	mtc0	t1,C0_SR
	and	v0,t0		# get old SR_DE bit
	xor	v0,t0		# and invert to make it an enable bit
	j	ra
END(sbdberrenb)


LEAF(sbdberrcnt)
	move	v0,zero
	j	ra
END(sbdberrcnt)

	.lcomm	wbfltmp,4

LEAF(wbflush)
//XLEAF(mips_wbflush)
	sync
	la	t0,wbfltmp
	or	t0,K1BASE
	lw	zero,0(t0)
	j	ra
END(wbflush)


LEAF(sbddelay)
	li	t1,CACHEUS
	and	t0,ra,0x20000000
	beqz	t0,1f
	li	t1,ROMUS
1:	mul	a0,t1
	subu	a0,15		# approx number of loops so far

	.set	noreorder
	.set	nomacro
	nop
2:	bgtz	a0,2b
	subu	a0,1
	.set	macro
	.set	reorder

	j	ra
END(sbddelay)

#include "meminit.S"


LEAF(mips_cycle)
	.set	noreorder
	.set	nomacro
1:	bgtz	a0,1b
	subu	a0,1
	.set	macro
	.set	reorder
	j	ra
END(mips_cycle)

LEAF(init_ns16550_chan_b)
	# enable 16550 fifo if it is there
        li      a0,NS16550_CHANB
        li	t0,FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	t0,FIFO(a0)

	/* convert baud rate in a1 into register value */
        li	t2,NS16550_HZ/(16*115200)	# brtc = CLK/16/speed

	li	t0,CFCR_DLAB			# select brtc divisor
	sb	t0,CFCR(a0)
	sb	t2,DATA(a0)			# store divisor lsb
	srl	t2,8
	sb	t2,IER(a0)			# store divisor msb

	li	t0,CFCR_8BITS			# set 8N1 mode
	sb	t0,CFCR(a0)

	li	t0,MCR_DTR|MCR_RTS # Galileo |MCR_IENABLE	# enable DTR & RTS
  	sb	t0,MCR(a0)
 	li	t0,0 # Galileo IER_ERXRDY			# enable receive interrupt(!)
	sb	t0,IER(a0)

	move	v0,zero				# indicate success
	j	ra

END(init_ns16550_chan_b)

LEAF(init_ns16550_chan_a)
	# enable 16550 fifo if it is there
        li      a0,NS16550_CHANA
        li	t0,FIFO_ENABLE|FIFO_RCV_RST|FIFO_XMT_RST|FIFO_TRIGGER_4
	sb	t0,FIFO(a0)

	/* convert baud rate in a1 into register value */
        li	t2,NS16550_HZ/(16*9600)		# brtc = CLK/16/speed

	li	t0,CFCR_DLAB			# select brtc divisor
	sb	t0,CFCR(a0)
	sb	t2,DATA(a0)			# store divisor lsb
	srl	t2,8
	sb	t2,IER(a0)			# store divisor msb

	li	t0,CFCR_8BITS			# set 8N1 mode
	sb	t0,CFCR(a0)

	li	t0,MCR_DTR|MCR_RTS # Galileo |MCR_IENABLE	# enable DTR & RTS
  	sb	t0,MCR(a0)
 	li	t0,0 # Galileo IER_ERXRDY			# enable receive interrupt(!)
	sb	t0,IER(a0)

	move	v0,zero				# indicate success
	j	ra

END(init_ns16550_chan_a)

#endif /* EVB64120A */
