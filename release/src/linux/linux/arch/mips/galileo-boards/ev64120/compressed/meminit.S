/*
 * Define BUSWIDTH to usually be real buswidth X 2 (i.e assuming
 * 2-way interleaving).  This is so that the test pattern and
 * inverted pattern are written to the same bank of memory, which
 * prevents us reading back data sitting in the dram buffers and
 * getting a false match.
 */

#ifndef BUSWIDTH
# if #cpu(r3000) || #cpu(r4300) || #cpu(r4650)
#  define BUSWIDTH	8		/* 32 bit memory, bank interleaved */
# elif #cpu(r4000)
#  define BUSWIDTH	16		/* 64 bit memory, bank interleaved */
# endif
#endif

#ifndef RAM_BASE
#define RAM_BASE	KSEG1_BASE
#endif

#ifndef MEMSTART
#define MEMSTART	0x0		/* start of physical memory */
#endif

#ifndef MEMINCR
# define MEMINCR	0x10000		/* work up in 64Kb increments */
#endif

SLEAF(size_mem)
	mfc0	t8,C0_STATUS
#if #cpu(r4000)
	/* disable cache and memory parity checking */
	or	t0,t8,SR_DE
	mtc0	t0,C0_STATUS
#endif

	li	t0,RAM_BASE+MEMSTART	# start at bottom of phys mem
	move	t1,t0			# remember start address
	li	t2,0xaa55aa55		# pattern
	not	t3,t2			# ~pattern

	move	t7,k0
	la	t4,.fail		# bus error exception catcher
	addu	k0,t4,s8		# RELOC

	/* fill first 64Kb with zero (for cache init) */
	move	t4,t0
	li	t5,0x10000
1:	sw	zero,0(t4)
	sw	zero,4(t4)
	sw	zero,8(t4)
	sw	zero,12(t4)
	subu	t5,16
	addu	t4,16
	bnez	t5,1b

.loop:
        addu    t0,MEMINCR
	move	t4,t0

	/* store pattern in bank 0, line 0 */
	sw	t2,0(t4)
	addu	t4,4

#if BUSWIDTH > 4
	/* fill remainder of line with zeros */
	li	t5,BUSWIDTH-4
1:	sw	zero,0(t4)
	subu	t5,4
	addu	t4,4
	bnez	t5,1b
#endif

	/* store inverse pattern in bank 0, line 1 */
	sw	t3,0(t4)
	addu	t4,4

#if BUSWIDTH > 4
	/* fill remainder of line with zeros */
	li	t5,BUSWIDTH-4
1:	sw	zero,0(t4)
	subu	t5,4
	addu	t4,4
	bnez	t5,1b
#endif

	/* defeat write buffering */
#if #cpu(r4000)
	sync
#else
	lw	zero,-4(t4)
#endif

	lw	t4,0(t0)		# read first word of line
	lw	t5,0(t1)		# read start of memory (should be zero)
	bne	t4,t2,.fail		# this line wrong?
        beq     t5,zero,.loop		# start of mem overwritten?

.fail:
	move	k0,t7			# clear exception catcher

	/* restore Status register */
	mtc0	t8,C0_STATUS

	/* return top of memory offset (normally == size) */
	subu 	v0,t0,RAM_BASE
	j	ra
END(size_mem)


/*
 * We must often initialise memory so that it has good parity/ecc,
 * and this must be done before the caches are used.
 */

/*
	clear_mem (size)
	  - clear memory from RAM_BASE+MEMSTART to RAM_BASE+MEMSTART+size
	clear_mem_range (size, start)
	  - clear memory from start to start+size
*/

SLEAF(clear_mem)
	li	a1,RAM_BASE+MEMSTART	# start at bottom of phys mem
clear_mem_range:
	beqz	a0,9f
	addu	a0,a1			# end of memory

	.set noreorder
#if __mips >= 3
1:	sd	zero,0(a1)
	sd	zero,8(a1)
	sd	zero,16(a1)
	sd	zero,24(a1)
	sd	zero,32(a1)
	sd	zero,40(a1)
	sd	zero,48(a1)
	addu	a1,64
	bne	a1,a0,1b
	sd	zero,-8(a1)		# BDSLOT
#else
1:	sw	zero,0(a1)
	sw	zero,4(a1)
	sw	zero,8(a1)
	sw	zero,12(a1)
	sw	zero,16(a1)
	sw	zero,20(a1)
	sw	zero,24(a1)
	sw	zero,28(a1)
	sw	zero,32(a1)
	sw	zero,36(a1)
	sw	zero,40(a1)
	sw	zero,44(a1)
	sw	zero,48(a1)
	sw	zero,52(a1)
	sw	zero,56(a1)
	addu	a1,64
	bne	a1,a0,1b
	sw	zero,-4(a1)		# BDSLOT
#endif
	.set	reorder

9:	j	ra
END(clear_mem)


SLEAF(init_tlb)
	/* initialise tlb */
	mtc0	zero,C0_TLBLO0		/* tlblo0 = invalid */
	mtc0	zero,C0_TLBLO1		/* tlblo1 = invalid */
        mtc0	zero,C0_PGMASK
	li	t8,K1BASE		/* tlbhi  = impossible vpn */
	li	t9,(NTLBENTRIES-1)	/* index */

	.set noreorder
	nop
1:	mtc0	t8,C0_TLBHI
	mtc0	t9,C0_INX
	addu	t8,0x2000		/* inc vpn */
	tlbwi
	bnez	t9,1b
	subu	t9,1			# BDSLOT
	.set reorder

	j	ra
END(init_tlb)
