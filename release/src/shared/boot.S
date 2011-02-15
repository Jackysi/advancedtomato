/*
 * BCM947XX Boot code for standalone apps.
 *
 * Code should be position-independent until it copies itself to SDRAM.
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */

#include "mipsinc.h"
#include "sbconfig.h"
#include "sbchipc.h"
#include "bcmdevs.h"

	.text
	LEAF(startup)
	.set	noreorder


	li	a0,KSEG1ADDR(SB_ENUM_BASE)

       	# XXX: the following code snipet sets clk frequency to 200M
        # correct pll clk freq to real speed in the 5350 case.
	# unless its vsim which we detect as pkg option 1 (should be 0xe)
        # It is Ugly...but
	li	a3,BCM5350_CHIP_ID		# 5350 ChipID
	lw	t1,CC_CHIPID(a0)		# ChipID register
	li	t2,CID_ID_MASK			# chip id is bit 0-15
	and	t2,t1,t2
	bne	t2,a3,2f			# if not 5350 then skip
	nop
	
	li	t2,CID_PKG_MASK			# if it is a vsim 5350, also skip
	and	t2,t1,t2
	li	a3,(HDLSIM5350_PKG_ID << CID_PKG_SHIFT)
	beq	t2,a3,2f			# if pkg opt 1 then skip
	nop

	li	a3,CLKC_5350_N
	lw	t1,CC_CLKC_N(a0)
	beq	a3,t1,2f			# move ahead if clk freq set correctly
	nop
	sw	a3,CC_CLKC_N(a0)		# set control N1 to select 6
	li	t1,1
	sw	t1,CC_WATCHDOG(a0)		# set WatchDog Reset
1:	b	1b
	nop

2:
#ifdef	BCM5354
	lw	t1,CC_CHIPID(a0)		# ChipID register
	li	t2,CID_ID_MASK			# chip id is bit 0-15
	li	a3,BCM5354_CHIP_ID		# 5354 ChipID
	and	t2,t1,t2
	bne	t2,a3,ramcheck			# if not 5354 then skip
	nop

	li	t2,0x1				# Need define
	sw	t2,PMU_REG_CONTROL_ADDR(a0)
	li 	t3,0x6800000			# Should only affect the switch bits
	sw	t3,PMU_REG_CONTROL_DATA(a0)
	
	/* 
	 * Trim the output voltage of the 1.2V BB switcher and 2.5V
	 * regulator to the correct value.
	 */
	li	t2,0x0
	sw	t2,PMU_REG_CONTROL_ADDR(a0)
	li 	t3,0x2000			# Reduce the output voltage of
	sw	t3,PMU_REG_CONTROL_DATA(a0)	# BB switcher to get 1.2V
	li	t2,0x3
	sw	t2,PMU_REG_CONTROL_ADDR(a0)
	li 	t3,0x02000000			# Increase the output voltage
	sw	t3,PMU_REG_CONTROL_DATA(a0)	# of VDDP LDO to get 2.5V

	lw	t2,PMU_CTL(a0)			# Check if PLL has been programmed
	andi	t2,t2,PCTL_XTALFREQ_MASK
	bnez	t2,3f				# Yup, leave it alone
	nop
	li	t2,0x7ffff			# Should only turn off the PLL bit
	sw	t2,PMU_MIN_RES_MASK(a0)		# Disable base band PLL
	sw	t2,PMU_MAX_RES_MASK(a0)
	nop

	/* Init code for FF4 space without TLB, enabling RAC */
3:	li 	t0,0x1fa0000c			# Set up CBR to 0x1fax_xxxx
	mtc0 	t0,$22,6
	li 	t1,0x1fa00000
	lw 	t2,0x14(t1)
	or 	t3,t2,0xc0000000		# enable ffxx_xxxx space # without programming TLB
	sw 	t3,0x14(t1) 
	li 	t0,0xff40000c			# change CBR to ff4x_xxxx
	mtc0 	t0,$22,6
#endif	/* BCM5354 */

ramcheck:
	/* Check if we booted from SDRAM */
	bal	1f
	nop
1:	li	t0,PHYSADDR_MASK
	and	t0,t0,ra
	li	t1,SB_FLASH1
	bge	t0,t1,inflash
	nop

	/* Call draminit to size memory */
	jal	board_draminit
	nop
	b	setsp
	nop

inflash:
	/* Is this chipc rev 11 or 12 and a serial flash? */
	li	t0,KSEG1ADDR(SB_ENUM_BASE)
	lw	t1,(SBCONFIGOFF + SBIDHIGH)(t0)
	and	t2,t1,SBIDH_CC_MASK
	srl	t2,t2,SBIDH_CC_SHIFT
	bne	t2,SB_CC,checkcon		/* Not chipc */
	nop
	and	t2,t1,SBIDH_RC_MASK
	and	t3,t1,SBIDH_RCE_MASK
	srl	t3,t3,SBIDH_RCE_SHIFT
	or	t2,t3
	ble	t2,10,checkcon			/* ccrev <= 10 */
	nop
	bge	t2,13,checkcon			/* ccrev >= 13 */
	nop
	lw	t0,CC_CAPABILITIES(t0)
	and	t0,t0,CC_CAP_FLASH_MASK
	beq	t0,SFLASH_AT,switchkseg0	/* Atmel sflash */
	nop
	beq	t0,SFLASH_ST,switchkseg0	/* ST sflash */
	nop


checkcon:
	/* Check if the caches are already on */
	mfc0	t0,C0_CONFIG
	and	t0,CONF_CM_CMASK
	beq	t0,CONF_CM_UNCACHED,initcaches
	nop

	b	switchkseg0
	nop

initcaches:
	/* Turn on the caches in the CP0 register */
	mfc0	t0,C0_DIAGNOSTIC
	or	t0,(BRCM_IC_ENABLE | BRCM_DC_ENABLE) /* Enable both I$ and D$ */
	mtc0	t0,C0_DIAGNOSTIC


1:	/* Get cache sizes */
	.set	mips32
	mfc0	s0,C0_CONFIG,1
	.set	mips0

	li	s1,CONF1_DL_MASK
	and	s1,s0
	beq	s1,zero,nodc
	nop

	srl	s1,CONF1_DL_SHIFT
	li	t0,CONF1_DL_BASE
	sll	s1,t0,s1		/* s1 has D$ cache line size */

	li	s2,CONF1_DA_MASK
	and	s2,s0
	srl	s2,CONF1_DA_SHIFT
	addiu	s2,CONF1_DA_BASE	/* s2 now has D$ associativity */

	li	t0,CONF1_DS_MASK
	and	t0,s0
	srl	t0,CONF1_DS_SHIFT
	li	s3,CONF1_DS_BASE
	sll	s3,s3,t0		/* s3 has D$ sets per way */

	multu	s2,s3			/* sets/way * associativity */
	mflo	t0			/* total cache lines */

	multu	s1,t0			/* D$ linesize * lines */
	mflo	s2			/* s2 is now D$ size in bytes */

	/* Initilize the D$: */
	mtc0	zero,C0_TAGLO
	mtc0	zero,C0_TAGHI

	li	t0,KSEG0		/* Just an address for the first $ line */
	addu	t1,t0,s2		/*  + size of cache == end */

	.set	mips3
1:	cache	Index_Store_Tag_D,0(t0)
	.set	mips0
	bne	t0,t1,1b
	addu	t0,s1

nodc:
	/* Now we get to do it all again for the I$ */

	li	t0,CONF1_IL_MASK
	and	t0,s0
	beq	t0,zero,noic
	nop

	srl	t0,CONF1_IL_SHIFT
	li	s3,CONF1_IL_BASE
	sll	s3,t0			/* s3 has I$ cache line size */

	li	t0,CONF1_IA_MASK
	and	t0,s0
	srl	t0,CONF1_IA_SHIFT
	addiu	s4,t0,CONF1_IA_BASE	/* s4 now has I$ associativity */

	li	t0,CONF1_IS_MASK
	and	t0,s0
	srl	t0,CONF1_IS_SHIFT
	li	s5,CONF1_IS_BASE
	sll	s5,t0			/* s5 has I$ sets per way */

	multu	s4,s5			/* sets/way * associativity */
	mflo	t0			/* s4 is not total cache lines */

	multu	s3,t0			/* I$ linesize * lines */
	mflo	s4			/* s4 is cache size in bytes */

	/* Initilize the I$: */
	mtc0	zero,C0_TAGLO
	mtc0	zero,C0_TAGHI

	li	t0,KSEG0		/* Just an address for the first $ line */
	addu	t1,t0,s4		/*  + size of cache == end */

	.set	mips3
1:	cache	Index_Store_Tag_I,0(t0)
	.set	mips0
	bne	t0,t1,1b
	addu	t0,s1

noic:	
	/* Caches initialized, change cacheability */
	mfc0	t0,C0_CONFIG
	and	t0,~CONF_CM_CMASK
	or	t0,CONF_CM_CACHABLE_NONCOHERENT
	mtc0	t0,C0_CONFIG
	nop

switchkseg0:
	/* And now jump to KSEG0 */
	li	t0,KSEG1ADDR(SB_FLASH1)
	la	t1,text_start
	la	t2,initdram
	sub	t0,t0,t1
	add	t2,t2,t0
	and	t2,PHYSADDR_MASK
	or	t2,KSEG0
	jr	t2
	nop

initdram:
	/* Initialize SDRAM */
	li	t0,KSEG0ADDR(SB_FLASH1)
	la	t1,text_start
	la	t2,board_draminit
	sub	t0,t0,t1
	add	t2,t2,t0
	jalr	t2
	nop

	/* v0 now contains memory size in bytes */

	/* Copy self to SDRAM */
#ifdef	CONFIG_XIP
	la	a0,text_end
	la	a1,data_start
	la	a2,input_data
#else
	li	a0,KSEG0ADDR(SB_FLASH1)
	la	a1,text_start
	and	a1,PHYSADDR_MASK	/* Uncached writes to avoid a flush */
	or	a1,KSEG1
	la	a2,_end
	and	a2,PHYSADDR_MASK
	or	a2,KSEG1
#endif
1:	lw	t0,0(a0)
	sw	t0,0(a1)
	add	a0,4
	add	a1,4
	blt	a1,a2,1b
	nop
	b	setsp
	nop

	/* Black hole for traps with BEV on */
	.org	0x380
bevtrap: nop
	nop
	.set	mips32
	wait
	.set	mips0
	nop
	nop
	b	bevtrap
	nop

	/* Record the size of the binary */
	.org	BISZ_OFFSET
	.word	BISZ_MAGIC
	.word	text_start
	.word	text_end
	.word	data_start
	.word	data_end
	.word	bss_start
	.word	bss_end
	.word	_end

	/* Embedded NVRAM */
	.balign	0x400	
	.globl	embedded_nvram
embedded_nvram:
	.fill	0x100,4,~(0x48534c46)

setsp:
	/* Record the memory size */
	la	t0,_memsize
	sw	v0,0(t0)

	/* Set up stack pointer */
	or	v0,KSEG0
	sub	sp,v0,4

	/* Clear BSS */	
	la	a0,bss_start
	la	a1,bss_end
1:	sw	zero,0(a0)
	addi	a0,a0,4
	blt	a0,a1,1b
	nop

	/* Setup trap handlers */
#ifdef CONFIG_XIP
	li	t0,KSEG0ADDR(SB_FLASH1)
	la	t1,text_start
	la	t2,trap_init
	sub	t0,t0,t1
	add	t2,t2,t0
	jalr	t2
	nop
#else
	la	t0,trap_init
	jalr	t0
	nop
#endif

	/* Jump to C */
	la	t0,c_main
	jal	t0
	move	a0,ra

	/* In case c_main returns */
theend:	nop
	nop
	.set	mips32
	wait
	.set	mips0
	nop
	nop
	b	theend
	nop

	.set reorder
	END(startup)

	.data
	.globl	_memsize
_memsize:
	.word	0
