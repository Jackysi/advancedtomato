/*
 * BCM47XX support code for some chipcommon (old extif) facilities (uart, jtagm)
 *
 * Copyright 2006, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <sbutils.h>
#include <bcmdevs.h>
#include <bcmnvram.h>
#include <sbconfig.h>
#include <sbextif.h>
#include <sbchipc.h>
#include <hndchipc.h>
#include <hndcpu.h>


/*
 * Returns TRUE if an external UART exists at the given base
 * register.
 */
static bool
BCMINITFN(serial_exists)(osl_t *osh, uint8 *regs)
{
	uint8 save_mcr, status1;

	save_mcr = R_REG(osh, &regs[UART_MCR]);
	W_REG(osh, &regs[UART_MCR], UART_MCR_LOOP | 0x0a);
	status1 = R_REG(osh, &regs[UART_MSR]) & 0xf0;
	W_REG(osh, &regs[UART_MCR], save_mcr);

	return (status1 == 0x90);
}

/*
 * Initializes UART access. The callback function will be called once
 * per found UART.
 */
void
BCMINITFN(sb_serial_init)(sb_t *sbh, void (*add)(void *regs, uint irq, uint baud_base,
                                                 uint reg_shift))
{
	osl_t *osh;
	void *regs;
	ulong base;
	uint irq;
	int i, n;

	osh = sb_osh(sbh);

	if ((regs = sb_setcore(sbh, SB_EXTIF, 0))) {
		extifregs_t *eir = (extifregs_t *) regs;
		sbconfig_t *sb;

		/* Determine external UART register base */
		sb = (sbconfig_t *)((ulong) eir + SBCONFIGOFF);
		base = EXTIF_CFGIF_BASE(sb_base(R_REG(osh, &sb->sbadmatch1)));

		/* Determine IRQ */
		irq = sb_irq(sbh);

		/* Disable GPIO interrupt initially */
		W_REG(osh, &eir->gpiointpolarity, 0);
		W_REG(osh, &eir->gpiointmask, 0);

		/* Search for external UARTs */
		n = 2;
		for (i = 0; i < 2; i++) {
			regs = (void *) REG_MAP(base + (i * 8), 8);
			if (serial_exists(osh, regs)) {
				/* Set GPIO 1 to be the external UART IRQ */
				W_REG(osh, &eir->gpiointmask, 2);
				if (add)
					add(regs, irq, 13500000, 0);
			}
		}

		/* Add internal UART if enabled */
		if (R_REG(osh, &eir->corecontrol) & CC_UE)
			if (add)
				add((void *) &eir->uartdata, irq, sb_clock(sbh), 2);
	} else if ((regs = sb_setcore(sbh, SB_CC, 0))) {
		chipcregs_t *cc = (chipcregs_t *) regs;
		uint32 rev, cap, pll, baud_base, div;

		/* Default value */
		div = 48;

		/* Determine core revision and capabilities */
		rev = sb_corerev(sbh);
		cap = R_REG(osh, &cc->capabilities);
		pll = cap & CC_CAP_PLL_MASK;

		/* Determine IRQ */
		irq = sb_irq(sbh);

		if (pll == PLL_TYPE1) {
			/* PLL clock */
			baud_base = sb_clock_rate(pll,
			                          R_REG(osh, &cc->clockcontrol_n),
			                          R_REG(osh, &cc->clockcontrol_m2));
			div = 1;
		} else {
			/* 5354 chip common uart uses a constant clock
			 * frequency of 25MHz */
			if (sb_corerev(sbh) == 20) {
				/* Set the override bit so we don't divide it */
				W_REG(osh, &cc->corecontrol, CC_UARTCLKO);
				baud_base = 25000000;
			} else if (rev >= 11 && rev != 15) {
				/* Fixed ALP clock */
				baud_base = sb_alp_clock(sbh);
				div = 1;
				/* Set the override bit so we don't divide it */
				W_REG(osh, &cc->corecontrol, CC_UARTCLKO);
			} else if (rev >= 3) {
				/* Internal backplane clock */
				baud_base = sb_clock(sbh);
				div = 2;	/* Minimum divisor */
				W_REG(osh, &cc->clkdiv,
				      ((R_REG(osh, &cc->clkdiv) & ~CLKD_UART) | div));
			} else {
				/* Fixed internal backplane clock */
				baud_base = 88000000;
				div = 48;
			}

			/* Clock source depends on strapping if UartClkOverride is unset */
			if ((rev > 0) &&
			    ((R_REG(osh, &cc->corecontrol) & CC_UARTCLKO) == 0)) {
				if ((cap & CC_CAP_UCLKSEL) == CC_CAP_UINTCLK) {
					/* Internal divided backplane clock */
					baud_base /= div;
				} else {
					/* Assume external clock of 1.8432 MHz */
					baud_base = 1843200;
				}
			}
		}

		/* Add internal UARTs */
		n = cap & CC_CAP_UARTS_MASK;
		for (i = 0; i < n; i++) {
			/* Register offset changed after revision 0 */
			if (rev)
				regs = (void *)((ulong) &cc->uart0data + (i * 256));
			else
				regs = (void *)((ulong) &cc->uart0data + (i * 8));

			if (add)
				add(regs, irq, baud_base, 0);
		}
	}
}

/*
 * Initialize jtag master and return handle for
 * jtag_rwreg. Returns NULL on failure.
 */
void *
sb_jtagm_init(sb_t *sbh, uint clkd, bool exttap)
{
	void *regs;

	if ((regs = sb_setcore(sbh, SB_CC, 0)) != NULL) {
		chipcregs_t *cc = (chipcregs_t *) regs;
		uint32 tmp;

		/*
		 * Determine jtagm availability from
		 * core revision and capabilities.
		 */
		tmp = sb_corerev(sbh);
		/*
		 * Corerev 10 has jtagm, but the only chip
		 * with it does not have a mips, and
		 * the layout of the jtagcmd register is
		 * different. We'll only accept >= 11.
		 */
		if (tmp < 11)
			return (NULL);

		tmp = R_REG(osh, &cc->capabilities);
		if ((tmp & CC_CAP_JTAGP) == 0)
			return (NULL);

		/* Set clock divider if requested */
		if (clkd != 0) {
			tmp = R_REG(osh, &cc->clkdiv);
			tmp = (tmp & ~CLKD_JTAG) |
				((clkd << CLKD_JTAG_SHIFT) & CLKD_JTAG);
			W_REG(osh, &cc->clkdiv, tmp);
		}

		/* Enable jtagm */
		tmp = JCTRL_EN | (exttap ? JCTRL_EXT_EN : 0);
		W_REG(osh, &cc->jtagctrl, tmp);
	}

	return (regs);
}

void
sb_jtagm_disable(osl_t *osh, void *h)
{
	chipcregs_t *cc = (chipcregs_t *)h;

	W_REG(osh, &cc->jtagctrl, R_REG(osh, &cc->jtagctrl) & ~JCTRL_EN);
}

/*
 * Read/write a jtag register. Assumes a target with
 * 8 bit IR and 32 bit DR.
 */
#define	IRWIDTH		8	/* Default Instruction Register width */
#define	DRWIDTH		32	/* Default Data Register width */

uint32
jtag_rwreg(osl_t *osh, void *h, uint32 ir, uint32 dr)
{
	chipcregs_t *cc = (chipcregs_t *) h;
	uint32 tmp;

	W_REG(osh, &cc->jtagir, ir);
	W_REG(osh, &cc->jtagdr, dr);
	tmp = JCMD_START | JCMD_ACC_IRDR |
		((IRWIDTH - 1) << JCMD_IRW_SHIFT) |
		(DRWIDTH - 1);
	W_REG(osh, &cc->jtagcmd, tmp);
	while (((tmp = R_REG(osh, &cc->jtagcmd)) & JCMD_BUSY) == JCMD_BUSY) {
		/* OSL_DELAY(1); */
	}

	tmp = R_REG(osh, &cc->jtagdr);
	return (tmp);
}
