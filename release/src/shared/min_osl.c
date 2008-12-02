/*
 * Initialization and support routines for self-booting
 * compressed image.
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: min_osl.c,v 1.1.1.3 2005/03/07 07:31:12 kanki Exp $
 */

#include <osl.h>
#include <sbutils.h>
#include <bcmutils.h>
#include <hndmips.h>

/* Cache support */

/* Cache and line sizes */
static uint icache_size, ic_lsize, dcache_size, dc_lsize;

static void
_change_cachability(uint32 cm)
{
	uint32 prid, c0reg;

	c0reg = MFC0(C0_CONFIG, 0);
	c0reg &= ~CONF_CM_CMASK;
	c0reg |= (cm & CONF_CM_CMASK);
	MTC0(C0_CONFIG, 0, c0reg);
	prid = MFC0(C0_PRID, 0);
	if ((prid & (PRID_COMP_MASK | PRID_IMP_MASK)) ==
	    (PRID_COMP_BROADCOM | PRID_IMP_BCM3302)) {
		c0reg = MFC0(C0_BROADCOM, 0);
		/* Enable icache */
		c0reg |= (1 << 31);
		/* Enable dcache */
		c0reg |= (1 << 30);
		MTC0(C0_BROADCOM, 0 , c0reg);
	}
}	
static void (*change_cachability)(uint32);

void
caches_on(void)
{
	uint32 config1;
	uint start, end, size, lsize;

	/* Save cache config */
	config1 = MFC0(C0_CONFIG, 1);
	
	icache_probe(config1, &size, &lsize);
	icache_size = size;
	ic_lsize = lsize;
	
	dcache_probe(config1, &size, &lsize);
	dcache_size = size;
	dc_lsize = lsize;
	
	if ((MFC0(C0_CONFIG, 0) & CONF_CM_CMASK) != CONF_CM_UNCACHED) {
		blast_dcache();
		blast_icache();
		return;
	}

	/* init icache */
	start = KSEG0;
	end = (start + icache_size);
	MTC0(C0_TAGLO, 0, 0);
	MTC0(C0_TAGHI, 0, 0);
	while (start < end) {
		cache_unroll(start, Index_Store_Tag_I);
		start += ic_lsize;
	}
	
	/* init dcache */
	start = KSEG0;
	end = (start + dcache_size);
	MTC0(C0_TAGLO, 0, 0);
	MTC0(C0_TAGHI, 0, 0);
	while (start < end) {
		cache_unroll(start, Index_Store_Tag_D);
		start += dc_lsize;
	}

	/* Must be in KSEG1 to change cachability */
	change_cachability = (void (*)(uint32)) KSEG1ADDR(_change_cachability);
	change_cachability(CONF_CM_CACHABLE_NONCOHERENT);
}


#define BCM4710_DUMMY_RREG() (((sbconfig_t *)(KSEG1ADDR(0x18000000 + SBCONFIGOFF)))->sbimstate)

void
blast_dcache(void)
{
	uint32 start, end;

	start = KSEG0;
	end = start + dcache_size;

	while(start < end) {
		BCM4710_DUMMY_RREG();
		cache_unroll(start, Index_Writeback_Inv_D);
		start += dc_lsize;
	}
}

void
blast_icache(void)
{
	uint32 start, end;

	start = KSEG0;
	end = start + icache_size;

	while(start < end) {
		cache_unroll(start, Index_Invalidate_I);
		start += ic_lsize;
	}
}

/* uart output */

struct serial_struct {
	unsigned char	*iomem_base;
	unsigned short	iomem_reg_shift;
	int	irq;
	int	baud_base;
};

static struct serial_struct hndrte_uart;

#define LOG_BUF_LEN	(1024)
#define LOG_BUF_MASK	(LOG_BUF_LEN-1)
static char log_buf[LOG_BUF_LEN];
static unsigned long log_start;


static inline int
serial_in(struct serial_struct *info, int offset)
{
	return ((int)R_REG((uint8 *)(info->iomem_base + (offset << info->iomem_reg_shift))));
}

static inline void
serial_out(struct serial_struct *info, int offset, int value)
{
	W_REG((uint8 *)(info->iomem_base + (offset << info->iomem_reg_shift)), value);
	*((volatile unsigned int *) KSEG1ADDR(0x18000000));
}

void
putc(int c)
{
	/* CR before LF */
	if (c == '\n')
		putc('\r');

	/* Store in log buffer */
	*((char *) KSEG1ADDR(&log_buf[log_start])) = (char) c;
	log_start = (log_start + 1) & LOG_BUF_MASK;

	/* No UART */
	if (!hndrte_uart.iomem_base)
		return;

	while (!(serial_in(&hndrte_uart, UART_LSR) & UART_LSR_THRE));
	serial_out(&hndrte_uart, UART_TX, c);
}

/* assert & debugging */


/* general purpose memory allocation */

extern char text_start[], text_end[];
extern char data_start[], data_end[];
extern char bss_start[], bss_end[];

static ulong free_mem_ptr = 0;
static ulong free_mem_ptr_end = 0;

void *
malloc(uint size)
{
	void *p;

	/* Sanity check */
	if (size < 0)
		printf("Malloc error");
	if (free_mem_ptr == 0)
		printf("Memory error");

	/* Align */
	free_mem_ptr = (free_mem_ptr + 3) & ~3;

	p = (void *) free_mem_ptr;
	free_mem_ptr += size;

	if (free_mem_ptr >= free_mem_ptr_end)
		printf("Out of memory");

	return p;
}

int
free(void *where)
{
	return 0;
}

/* microsecond delay */

/* Default to 125 MHz */
static unsigned long cpu_clock = 125000000;

static inline void
__delay(uint loops)
{
        __asm__ __volatile__ (
                ".set\tnoreorder\n"
                "1:\tbnez\t%0,1b\n\t"
                "subu\t%0,1\n\t"
                ".set\treorder"
                :"=r" (loops)
                :"0" (loops));
}

void
udelay(uint us)
{
	uint loops;

	loops = cpu_clock / 5;

	__delay(loops);
}

extern char *
getvar(char *vars, char *name)
{
	return NULL;
}

extern int
getintvar(char *vars, char *name)
{
	return 0;
}

/* No trap handling in self-decompressing boots */
extern void trap_init(void);

void
trap_init(void)
{
}


static void
serial_add(void *regs, uint irq, uint baud_base, uint reg_shift)
{
	int quot;

	if (hndrte_uart.iomem_base)
		return;

	hndrte_uart.iomem_base = regs;
	hndrte_uart.irq = irq;
	hndrte_uart.baud_base = baud_base / 16;
	hndrte_uart.iomem_reg_shift = reg_shift;

	/* Set baud and 8N1 */
	quot = (hndrte_uart.baud_base + 57600) / 115200;
	serial_out(&hndrte_uart, UART_LCR, UART_LCR_DLAB);
	serial_out(&hndrte_uart, UART_DLL, quot & 0xff);
	serial_out(&hndrte_uart, UART_DLM, quot >> 8);
	serial_out(&hndrte_uart, UART_LCR, UART_LCR_WLEN8);

	/* According to the Synopsys website: "the serial clock
	 * modules must have time to see new register values
	 * and reset their respective state machines. This
	 * total time is guaranteed to be no more than
	 * (2 * baud divisor * 16) clock cycles of the slower
	 * of the two system clocks. No data should be transmitted
	 * or received before this maximum time expires."
	 */
	udelay(1000);
}


void *
osl_init()
{
	uint32 c0reg;
	void *sbh;

	/* Disable interrupts */
	c0reg = MFC0(C0_STATUS, 0);
	c0reg &= ~ST0_IE;
	MTC0(C0_STATUS, 0 , c0reg);

	/* Scan backplane */
	sbh = sb_kattach();

	sb_mips_init(sbh);
	sb_serial_init(sbh, serial_add);

	/* Init malloc */
	free_mem_ptr = (ulong) bss_end;
	free_mem_ptr_end = ((ulong)&c0reg) - 8192;	/* Enough stack? */

	return (sbh);
}
