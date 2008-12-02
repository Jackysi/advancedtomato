/*
 * pSeries_lpar.c
 * Copyright (C) 2001 Todd Inglett, IBM Corporation
 *
 * pSeries LPAR support.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/processor.h>
#include <asm/semaphore.h>
#include <asm/mmu.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/machdep.h>
#include <asm/abs_addr.h>
#include <asm/mmu_context.h>
#include <asm/ppcdebug.h>
#include <asm/pci_dma.h>
#include <linux/pci.h>
#include <asm/naca.h>
#include <asm/hvcall.h>

long plpar_tce_get(unsigned long liobn,
		   unsigned long ioba,
		   unsigned long *tce_ret)
{
	unsigned long dummy;
	return plpar_hcall(H_GET_TCE, liobn, ioba, 0, 0,
			   tce_ret, &dummy, &dummy);
}


long plpar_tce_put(unsigned long liobn,
		   unsigned long ioba,
		   unsigned long tceval)
{
	return plpar_hcall_norets(H_PUT_TCE, liobn, ioba, tceval);
}

long plpar_get_term_char(unsigned long termno,
			 unsigned long *len_ret,
			 char *buf_ret)
{
	unsigned long *lbuf = (unsigned long *)buf_ret;  /* ToDo: alignment? */
	return plpar_hcall(H_GET_TERM_CHAR, termno, 0, 0, 0,
			   len_ret, lbuf+0, lbuf+1);
}

long plpar_put_term_char(unsigned long termno,
			 unsigned long len,
			 const char *buffer)
{
	unsigned long dummy;
	unsigned long *lbuf = (unsigned long *)buffer;  /* ToDo: alignment? */
	return plpar_hcall(H_PUT_TERM_CHAR, termno, len,
			   lbuf[0], lbuf[1], &dummy, &dummy, &dummy);
}

long plpar_eoi(unsigned long xirr)
{
	return plpar_hcall_norets(H_EOI, xirr);
}

long plpar_cppr(unsigned long cppr)
{
	return plpar_hcall_norets(H_CPPR, cppr);
}

long plpar_ipi(unsigned long servernum,
	       unsigned long mfrr)
{
	return plpar_hcall_norets(H_IPI, servernum, mfrr);
}

long plpar_xirr(unsigned long *xirr_ret)
{
	unsigned long dummy;
	return plpar_hcall(H_XIRR, 0, 0, 0, 0,
			   xirr_ret, &dummy, &dummy);
}

long plpar_ipoll(unsigned long servernum, unsigned long* xirr_ret, unsigned long* mfrr_ret)
{
	unsigned long dummy;
	return plpar_hcall(H_IPOLL, servernum, 0, 0, 0,
			   xirr_ret, mfrr_ret, &dummy);
}


static void tce_build_pSeriesLP(struct TceTable *tbl, long tcenum, 
				unsigned long uaddr, int direction )
{
	u64 set_tce_rc;
	union Tce tce;
	
	PPCDBG(PPCDBG_TCE, "build_tce: uaddr = 0x%lx\n", uaddr);
	PPCDBG(PPCDBG_TCE, "\ttcenum = 0x%lx, tbl = 0x%lx, index=%lx\n", 
	       tcenum, tbl, tbl->index);

	tce.wholeTce = 0;
	tce.tceBits.rpn = (virt_to_absolute(uaddr)) >> PAGE_SHIFT;

	tce.tceBits.readWrite = 1;
	if ( direction != PCI_DMA_TODEVICE ) tce.tceBits.pciWrite = 1;

	set_tce_rc = plpar_tce_put((u64)tbl->index, 
				 (u64)tcenum << 12, 
				 tce.wholeTce );

	if(set_tce_rc) {
		printk("tce_build_pSeriesLP: plpar_tce_put failed. rc=%ld\n", set_tce_rc);
		printk("\tindex   = 0x%lx\n", (u64)tbl->index);
		printk("\ttcenum  = 0x%lx\n", (u64)tcenum);
		printk("\ttce val = 0x%lx\n", tce.wholeTce );
	}
}

static void tce_free_one_pSeriesLP(struct TceTable *tbl, long tcenum)
{
	u64 set_tce_rc;
	union Tce tce;

	tce.wholeTce = 0;
	set_tce_rc = plpar_tce_put((u64)tbl->index, 
				 (u64)tcenum << 12,
				 tce.wholeTce );
	if ( set_tce_rc ) {
		printk("tce_free_one_pSeriesLP: plpar_tce_put failed\n");
		printk("\trc      = %ld\n", set_tce_rc);
		printk("\tindex   = 0x%lx\n", (u64)tbl->index);
		printk("\ttcenum  = 0x%lx\n", (u64)tcenum);
		printk("\ttce val = 0x%lx\n", tce.wholeTce );
	}

}

/* PowerPC Interrupts for lpar. */
/* NOTE: this typedef is duplicated (for now) from xics.c! */
typedef struct {
	int (*xirr_info_get)(int cpu);
	void (*xirr_info_set)(int cpu, int val);
	void (*cppr_info)(int cpu, u8 val);
	void (*qirr_info)(int cpu, u8 val);
} xics_ops;
static int pSeriesLP_xirr_info_get(int n_cpu)
{
	unsigned long lpar_rc;
	unsigned long return_value; 

	lpar_rc = plpar_xirr(&return_value);
	if (lpar_rc != H_Success) {
		panic(" bad return code xirr - rc = %lx \n", lpar_rc); 
	}
	return ((int)(return_value));
}

static void pSeriesLP_xirr_info_set(int n_cpu, int value)
{
	unsigned long lpar_rc;
	unsigned long val64 = value & 0xffffffff;

	lpar_rc = plpar_eoi(val64);
	if (lpar_rc != H_Success) {
		panic(" bad return code EOI - rc = %ld, value=%lx \n", lpar_rc, val64); 
	}
}

static void pSeriesLP_cppr_info(int n_cpu, u8 value)
{
	unsigned long lpar_rc;

	lpar_rc = plpar_cppr(value);
	if (lpar_rc != H_Success) {
		panic(" bad return code cppr - rc = %lx \n", lpar_rc); 
	}
}

static void pSeriesLP_qirr_info(int n_cpu , u8 value)
{
	unsigned long lpar_rc;

	lpar_rc = plpar_ipi(get_hard_smp_processor_id(n_cpu),value);
	if (lpar_rc != H_Success) {
    udbg_printf("pSeriesLP_qirr_info - plpar_ipi failed!!!!!!!! \n");
		panic(" bad return code qirr -ipi  - rc = %lx \n", lpar_rc); 
	}
}

xics_ops pSeriesLP_ops = {
	pSeriesLP_xirr_info_get,
	pSeriesLP_xirr_info_set,
	pSeriesLP_cppr_info,
	pSeriesLP_qirr_info
};
/* end TAI-LPAR */


int vtermno;	/* virtual terminal# for udbg  */

static void udbg_putcLP(unsigned char c)
{
	char buf[16];
	unsigned long rc;

	if (c == '\n')
		udbg_putcLP('\r');

	buf[0] = c;
	do {
		rc = plpar_put_term_char(vtermno, 1, buf);
	} while(rc == H_Busy);
}

/* Buffered chars getc */
static long inbuflen;
static long inbuf[2];	/* must be 2 longs */

static int udbg_getc_pollLP(void)
{
	/* The interface is tricky because it may return up to 16 chars.
	 * We save them statically for future calls to udbg_getc().
	 */
	char ch, *buf = (char *)inbuf;
	int i;
	long rc;
	if (inbuflen == 0) {
		/* get some more chars. */
		inbuflen = 0;
		rc = plpar_get_term_char(vtermno, &inbuflen, buf);
		if (rc != H_Success)
			inbuflen = 0;	/* otherwise inbuflen is garbage */
	}
	if (inbuflen <= 0 || inbuflen > 16) {
		/* Catch error case as well as other oddities (corruption) */
		inbuflen = 0;
		return -1;
	}
	ch = buf[0];
	for (i = 1; i < inbuflen; i++)	/* shuffle them down. */
		buf[i-1] = buf[i];
	inbuflen--;
	return ch;
}

static unsigned char udbg_getcLP(void)
{
	int ch;
	for (;;) {
		ch = udbg_getc_pollLP();
		if (ch == -1) {
			/* This shouldn't be needed...but... */
			volatile unsigned long delay;
			for (delay=0; delay < 2000000; delay++)
				;
		} else {
			return ch;
		}
	}
}



/* Code for hvc_console.  Should move it back eventually. */

int hvc_get_chars(int index, char *buf, int count)
{
	unsigned long got;

	if (plpar_hcall(H_GET_TERM_CHAR, index, 0, 0, 0, &got,
		(unsigned long *)buf, (unsigned long *)buf+1) == H_Success) {
		/*
		 * Work around a HV bug where it gives us a null
		 * after every \r.  -- paulus
		 */
		if (got > 0) {
			int i;
			for (i = 1; i < got; ++i) {
				if (buf[i] == 0 && buf[i-1] == '\r') {
					--got;
					if (i < got)
						memmove(&buf[i], &buf[i+1],
							got - i);
				}
			}
		}
		return got;
	}
	return 0;
}

int hvc_put_chars(int index, const char *buf, int count)
{
	unsigned long dummy;
	unsigned long *lbuf = (unsigned long *) buf;
	long ret;

	ret = plpar_hcall(H_PUT_TERM_CHAR, index, count, lbuf[0], lbuf[1],
			  &dummy, &dummy, &dummy);
	if (ret == H_Success)
		return count;
	if (ret == H_Busy)
		return 0;
	return -1;
}

int hvc_count(int *start_termno)
{
	u32 *termno;
	struct device_node *dn;

	if ((dn = find_path_device("/rtas")) != NULL) {
		if ((termno = (u32 *)get_property(dn, "ibm,termno", 0)) != NULL) {
			if (start_termno)
				*start_termno = termno[0];
			return termno[1];
		}
	}
	return 0;
}

#ifndef CONFIG_PPC_ISERIES
void pSeries_lpar_mm_init(void);

/* This is called early in setup.c.
 * Use it to setup page table ppc_md stuff as well as udbg.
 */
void pSeriesLP_init_early(void)
{
	pSeries_lpar_mm_init();

	ppc_md.tce_build	 = tce_build_pSeriesLP;
	ppc_md.tce_free_one	 = tce_free_one_pSeriesLP;

#ifdef CONFIG_SMP
	smp_init_pSeries();
#endif
	pSeries_pcibios_init_early();

	/* The keyboard is not useful in the LPAR environment.
	 * Leave all the interfaces NULL.
	 */

	/* lookup the first virtual terminal number in case we don't have a com port.
	 * Zero is probably correct in case someone calls udbg before the init.
	 * The property is a pair of numbers.  The first is the starting termno (the
	 * one we use) and the second is the number of terminals.
	 */
	u32 *termno;
	struct device_node *np = find_path_device("/rtas");
	if (np) {
		termno = (u32 *)get_property(np, "ibm,termno", 0);
		if (termno)
			vtermno = termno[0];
	}
	ppc_md.udbg_putc = udbg_putcLP;
	ppc_md.udbg_getc = udbg_getcLP;
	ppc_md.udbg_getc_poll = udbg_getc_pollLP;
}
#endif
