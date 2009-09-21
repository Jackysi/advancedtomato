/* Driver/API for AMD Geode Multi-Function General Purpose Timers (MFGPT)
 *
 * Copyright (C) 2006, Advanced Micro Devices, Inc.
 * Copyright (C) 2007, Andres Salomon <dilinger@debian.org>
 * Backported to 2.4 by Willy Tarreau <w@1wt.eu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/interrupt.h>
#include <asm/geode-mfgpt.h>
#include <asm/io.h>
#include <asm/msr.h>

/* for pci_iomap/pci_iounmap */
#include "mfgpt-compat.h"

static void *mfgpt_iobase;

static struct mfgpt_timer_t {
	unsigned int avail:1;
} mfgpt_timers[MFGPT_MAX_TIMERS];

void geode_mfgpt_write(int i, u16 r, u16 v)
{
	outw(v, (unsigned long)(mfgpt_iobase + (r + (i * 8))));
}
EXPORT_SYMBOL(geode_mfgpt_write);

u16 geode_mfgpt_read(int i, u16 r)
{
	return inw((unsigned long)(mfgpt_iobase + (r + (i * 8))));
}
EXPORT_SYMBOL(geode_mfgpt_read);

int geode_mfgpt_toggle_event(int timer, int cmp, int event, int setup)
{
	u32 msr, mask, value, dummy;
	int shift = (cmp == MFGPT_CMP1) ? 0 : 8;

	if (timer < 0 || timer >= MFGPT_MAX_TIMERS)
		return -EIO;

	switch(event) {
	case MFGPT_EVENT_RESET:
		msr = MSR_MFGPT_NR;
		mask = 1 << (timer + 24);
		break;

	case MFGPT_EVENT_NMI:
		msr = MSR_MFGPT_NR;
		mask = 1 << (timer + shift);
		break;

	case MFGPT_EVENT_IRQ:
		msr = MSR_MFGPT_IRQ;
		mask = 1 << (timer + shift);
		break;

	default:
		return -EIO;
	}

	rdmsr(msr, value, dummy);

	if (setup)
		value |= mask;
	else
		value &= ~mask;

	wrmsr(msr, value, dummy);
	return 0;
}

EXPORT_SYMBOL(geode_mfgpt_toggle_event);

/* Allow for disabling of MFGPTs */
static int disable;
static int __init mfgpt_disable(char *s)
{
	disable = 1;
	return 1;
}
__setup("nomfgpt", mfgpt_disable);

/* Reset the MFGPT timers. This is required by some broken BIOSes which already
 * do the same and leave the system in an unstable state. TinyBIOS 0.98 is
 * affected at least (0.99 is OK with MFGPT workaround left to off).
 */
static int __init mfgpt_fix(char *s)
{
	u32 val, dummy;

	/* The following udocumented bit resets the MFGPT timers */
	val = 0xFF; dummy = 0;
	wrmsr(MSR_MFGPT_SETUP, val, dummy);
	return 1;
}
__setup("mfgptfix", mfgpt_fix);


/*
 * Check whether any MFGPTs are available for the kernel to use.  In most
 * cases, firmware that uses AMD's VSA code will claim all timers during
 * bootup; we certainly don't want to take them if they're already in use.
 * In other cases (such as with VSAless OpenFirmware), the system firmware
 * leaves timers available for us to use.
 */

static struct pci_device_id geode_sbdevs[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_NS, PCI_DEVICE_ID_NS_CS5535_ISA) },
	{ PCI_DEVICE(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_CS5536_ISA) }
};

static int timers = -1;

static void geode_mfgpt_detect(void)
{
	struct pci_dev *pdev = NULL;
	int i, ret, dev, count = 0;
	u16 val;

	timers = 0;

	if (disable) {
		printk(KERN_INFO "geode-mfgpt:  MFGPT support is disabled\n");
		goto done;
	}

	if (!is_geode()) {
		printk(KERN_INFO "geode-mfgpt:  Not a Geode GX/LX processor\n");
		goto done;
	}

	for (dev = 0; dev < ARRAY_SIZE(geode_sbdevs); dev++) {
		pdev = pci_find_device(geode_sbdevs[dev].vendor,
				       geode_sbdevs[dev].device, NULL);

		if (pdev != NULL)
			break;
	}

	if (pdev == NULL) {
		printk(KERN_ERR "geode-mfgpt:  No PCI devices found\n");
		goto done;
	}

	if ((ret = pci_enable_device_bars(pdev, 1 << MFGPT_PCI_BAR)))
		goto err;

	if ((ret = pci_request_region(pdev, MFGPT_PCI_BAR, "geode-mfgpt")))
		goto err;

	mfgpt_iobase = pci_iomap(pdev, MFGPT_PCI_BAR, 64);

	if (mfgpt_iobase == NULL)
		goto ereq;

	for (i = 0; i < MFGPT_MAX_TIMERS; i++) {
		val = geode_mfgpt_read(i, MFGPT_REG_SETUP);
		if (!(val & MFGPT_SETUP_SETUP)) {
			mfgpt_timers[i].avail = 1;
			timers++;
		}
	}

 done:
	printk(KERN_INFO "geode-mfgpt:  %d MFGPT timers available.\n", timers);
	return;
 ereq:
	pci_release_region(pdev, MFGPT_PCI_BAR);
 err:
	printk(KERN_ERR  "geode-mfgpt:  Error initalizing the timers\n");
	return;
}

static int mfgpt_get(int timer)
{
	mfgpt_timers[timer].avail = 0;
	printk(KERN_INFO "geode-mfgpt:  Registered timer %d\n", timer);
	return timer;
}

int geode_mfgpt_alloc_timer(int timer, int domain)
{
	int i;

	if (timers == -1) {
		/* timers haven't been detected yet */
		geode_mfgpt_detect();
	}

	if (!timers)
		return -1;

	if (timer >= MFGPT_MAX_TIMERS)
		return -1;

	if (timer < 0) {
		/* Try to find an available timer */
		for (i = 0; i < MFGPT_MAX_TIMERS; i++) {
			if (mfgpt_timers[i].avail)
				return mfgpt_get(i);

			if (i == 5 && domain == MFGPT_DOMAIN_WORKING)
				break;
		}
	} else {
		/* If they requested a specific timer, try to honor that */
		if (mfgpt_timers[timer].avail)
			return mfgpt_get(timer);
	}

	/* No timers available - too bad */
	return -1;
}
EXPORT_SYMBOL(geode_mfgpt_alloc_timer);
