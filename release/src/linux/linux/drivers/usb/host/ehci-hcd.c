/*
 * Copyright (c) 2000-2002 by David Brownell
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/reboot.h>
#include <linux/bitops.h> /* for generic_ffs */

#ifdef CONFIG_USB_DEBUG
	#define DEBUG
#else
	#undef DEBUG
#endif

#include <linux/usb.h>

#include <linux/version.h>
#include "../hcd.h"

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>


/*-------------------------------------------------------------------------*/

/*
 * EHCI hc_driver implementation ... experimental, incomplete.
 * Based on the final 1.0 register interface specification.
 *
 * USB 2.0 shows up in upcoming www.pcmcia.org technology.
 * First was PCMCIA, like ISA; then CardBus, which is PCI.
 * Next comes "CardBay", using USB 2.0 signals.
 *
 * Contains additional contributions by Brad Hards, Rory Bolt, and others.
 * Special thanks to Intel and VIA for providing host controllers to
 * test this driver on, and Cypress (including In-System Design) for
 * providing early devices for those host controllers to talk to!
 *
 * HISTORY:
 *
 * 2009-01-31 Backport major parts from 2.6.27 (by Leonid Lisovskiy,
 *	<lly@sf.net>).
 *
 * 2003-12-29 Rewritten high speed iso transfer support (by Michal Sojka,
 *	<sojkam@centrum.cz>, updates by DB).
 *
 * 2002-11-29	Correct handling for hw async_next register.
 * 2002-08-06	Handling for bulk and interrupt transfers is mostly shared;
 *	only scheduling is different, no arbitrary limitations.
 * 2002-07-25	Sanity check PCI reads, mostly for better cardbus support,
 * 	clean up HC run state handshaking.
 * 2002-05-24	Preliminary FS/LS interrupts, using scheduling shortcuts
 * 2002-05-11	Clear TT errors for FS/LS ctrl/bulk.  Fill in some other
 *	missing pieces:  enabling 64bit dma, handoff from BIOS/SMM.
 * 2002-05-07	Some error path cleanups to report better errors; wmb();
 *	use non-CVS version id; better iso bandwidth claim.
 * 2002-04-19	Control/bulk/interrupt submit no longer uses giveback() on
 *	errors in submit path.  Bugfixes to interrupt scheduling/processing.
 * 2002-03-05	Initial high-speed ISO support; reduce ITD memory; shift
 *	more checking to generic hcd framework (db).  Make it work with
 *	Philips EHCI; reduce PCI traffic; shorten IRQ path (Rory Bolt).
 * 2002-01-14	Minor cleanup; version synch.
 * 2002-01-08	Fix roothub handoff of FS/LS to companion controllers.
 * 2002-01-04	Control/Bulk queuing behaves.
 *
 * 2001-12-12	Initial patch version for Linux 2.5.1 kernel.
 * 2001-June	Works with usb-storage and NEC EHCI on 2.4
 */

#define DRIVER_VERSION "10 Dec 2004/2.4"
#define DRIVER_AUTHOR "David Brownell"
#define DRIVER_DESC "USB 2.0 'Enhanced' Host Controller (EHCI) Driver"

static const char	hcd_name [] = "ehci_hcd";


#undef EHCI_VERBOSE_DEBUG
#undef EHCI_URB_TRACE


#ifdef DEBUG
#define EHCI_STATS
#endif

#define INTR_AUTOMAGIC		/* urb lifecycle mode, gone in 2.5 */

/* magic numbers that can affect system performance */
#define	EHCI_TUNE_CERR		3	/* 0-3 qtd retries; 0 == don't stop */
#define	EHCI_TUNE_RL_HS		4	/* nak throttle; see 4.9 */
#define	EHCI_TUNE_RL_TT		0
#define	EHCI_TUNE_MULT_HS	1	/* 1-3 transactions/uframe; 4.10.3 */
#define	EHCI_TUNE_MULT_TT	1
#define	EHCI_TUNE_FLS		2	/* (small) 256 frame schedule */

#define EHCI_IAA_MSECS		10		/* arbitrary */
#define EHCI_IO_JIFFIES		(HZ/10)		/* io watchdog > irq_thresh */
#define EHCI_ASYNC_JIFFIES	(HZ/20)		/* async idle timeout */
#define EHCI_SHRINK_FRAMES	5		/* async qh unlink delay */

/* Initial IRQ latency:  lower than default */
static int log2_irq_thresh = 0;		// 0 to 6
MODULE_PARM (log2_irq_thresh, "i");
MODULE_PARM_DESC (log2_irq_thresh, "log2 IRQ latency, 1-64 microframes");

/* initial park setting:  slower than hw default */
static unsigned park = 0;
MODULE_PARM (park, "i");
MODULE_PARM_DESC (park, "park setting; 1-3 back-to-back async packets");

/* for flakey hardware, ignore overcurrent indicators */
static int ignore_oc = 0;
MODULE_PARM (ignore_oc, "i");
MODULE_PARM_DESC (ignore_oc, "ignore bogus hardware overcurrent indications");

#define	INTR_MASK (STS_IAA | STS_FATAL | STS_PCD | STS_ERR | STS_INT)

/*-------------------------------------------------------------------------*/

#include "ehci.h"
#include "ehci-dbg.c"

/*-------------------------------------------------------------------------*/

/*
 * handshake - spin reading hc until handshake completes or fails
 * @ptr: address of hc register to be read
 * @mask: bits to look at in result of read
 * @done: value of those bits when handshake succeeds
 * @usec: timeout in microseconds
 *
 * Returns negative errno, or zero on success
 *
 * Success happens when the "mask" bits have the specified value (hardware
 * handshake done).  There are two failure modes:  "usec" have passed (major
 * hardware flakeout), or the register reads as all-ones (hardware removed).
 *
 * That last failure should_only happen in cases like physical cardbus eject
 * before driver shutdown. But it also seems to be caused by bugs in cardbus
 * bridge shutdown:  shutting down the bridge before the devices using it.
 */
static int handshake (u32 __iomem *ptr, u32 mask, u32 done, int usec)
{
	u32	result;

	do {
		result = readl (ptr);
		if (result == ~(u32)0)		/* card removed */
			return -ENODEV;
		result &= mask;
		if (result == done)
			return 0;
		udelay (1);
		usec--;
	} while (usec > 0);
	return -ETIMEDOUT;
}

/*
 * hc states include: unknown, halted, ready, running
 * transitional states are messy just now
 * trying to avoid "running" unless urbs are active
 * a "ready" hc can be finishing prefetched work
 */

/* force HC to halt state from unknown (EHCI spec section 2.3) */
static int ehci_halt (struct ehci_hcd *ehci)
{
	u32	temp = readl (&ehci->regs->status);

	/* disable any irqs left enabled by previous code */
	writel (0, &ehci->regs->intr_enable);

	if ((temp & STS_HALT) != 0)
		return 0;

	temp = readl (&ehci->regs->command);
	temp &= ~CMD_RUN;
	writel (temp, &ehci->regs->command);
	return handshake (&ehci->regs->status, STS_HALT, STS_HALT, 16 * 125);
}

static int handshake_on_error_set_halt(struct ehci_hcd *ehci, void __iomem *ptr,
				       u32 mask, u32 done, int usec)
{
	int error;

	error = handshake(ptr, mask, done, usec);
	if (error) {
		ehci_halt(ehci);
		ehci->hcd.state = USB_STATE_HALT;
		ehci_err(ehci, "force halt; handhake %p %08x %08x -> %d\n",
			ptr, mask, done, error);
	}

	return error;
}

/* put TDI/ARC silicon into EHCI mode */
static void tdi_reset (struct ehci_hcd *ehci)
{
	u32 __iomem	*reg_ptr;
	u32		tmp;

	reg_ptr = (u32 __iomem *)(((u8 __iomem *)ehci->regs) + USBMODE);
	tmp = readl(reg_ptr);
	tmp |= USBMODE_CM_HC;
	writel(tmp, reg_ptr);
}

/* reset a non-running (STS_HALT == 1) controller */
static int ehci_reset (struct ehci_hcd *ehci)
{
	int	retval;
	u32	command = readl (&ehci->regs->command);

	command |= CMD_RESET;
	dbg_cmd (ehci, "reset", command);
	writel (command, &ehci->regs->command);
	ehci->hcd.state = USB_STATE_HALT;
	ehci->next_statechange = jiffies;
	retval = handshake (&ehci->regs->command, CMD_RESET, 0, 250 * 1000);

	if (retval)
		return retval;

	if (ehci_is_TDI(ehci))
		tdi_reset (ehci);

	return retval;
}

/* idle the controller (from running) */
static void ehci_quiesce (struct ehci_hcd *ehci)
{
	u32	temp;

#ifdef DEBUG
	if (!HCD_IS_RUNNING (ehci->hcd.state))
		BUG ();
#endif

	/* wait for any schedule enables/disables to take effect */
	temp = readl(&ehci->regs->command) << 10;
	temp &= STS_ASS | STS_PSS;
	if (handshake_on_error_set_halt(ehci, &ehci->regs->status,
					STS_ASS | STS_PSS, temp, 16 * 125))
		return;

	/* then disable anything that's still active */
	temp = readl (&ehci->regs->command);
	temp &= ~(CMD_ASE | CMD_IAAD | CMD_PSE);
	writel (temp, &ehci->regs->command);

	/* hardware can take 16 microframes to turn off ... */
	if (handshake_on_error_set_halt(ehci, &ehci->regs->status,
				    STS_ASS | STS_PSS, 0, 16 * 125) != 0)
		return;
	ehci->hcd.state = USB_STATE_READY;
}

/*-------------------------------------------------------------------------*/

static void end_unlink_async(struct ehci_hcd *ehci);
static void ehci_work(struct ehci_hcd *ehci);

#include "ehci-hub.c"
#include "ehci-mem.c"
#include "ehci-q.c"
#include "ehci-sched.c"

/*-------------------------------------------------------------------------*/
static void ehci_iaa_watchdog(unsigned long param)
{
	struct ehci_hcd		*ehci = (struct ehci_hcd *) param;
	unsigned long		flags;

	spin_lock_irqsave (&ehci->lock, flags);

	/* Lost IAA irqs wedge things badly; seen first with a vt8235.
	 * So we need this watchdog, but must protect it against both
	 * (a) SMP races against real IAA firing and retriggering, and
	 * (b) clean HC shutdown, when IAA watchdog was pending.
	 */
	if (ehci->reclaim
			&& !timer_pending(&ehci->iaa_watchdog)
			&& HCD_IS_RUNNING (ehci->hcd.state)) {
		u32 cmd, status;

		/* If we get here, IAA is *REALLY* late.  It's barely
		 * conceivable that the system is so busy that CMD_IAAD
		 * is still legitimately set, so let's be sure it's
		 * clear before we read STS_IAA.  (The HC should clear
		 * CMD_IAAD when it sets STS_IAA.)
		 */
		cmd = readl(&ehci->regs->command);
		if (cmd & CMD_IAAD)
			writel(cmd & ~CMD_IAAD,
					&ehci->regs->command);

		/* If IAA is set here it either legitimately triggered
		 * before we cleared IAAD above (but _way_ late, so we'll
		 * still count it as lost) ... or a silicon erratum:
		 * - VIA seems to set IAA without triggering the IRQ;
		 * - IAAD potentially cleared without setting IAA.
		 */
		status = readl(&ehci->regs->status);
		if ((status & STS_IAA) || !(cmd & CMD_IAAD)) {
			COUNT (ehci->stats.lost_iaa);
			writel(STS_IAA, &ehci->regs->status);
		}

		ehci_vdbg(ehci, "IAA watchdog: status %x cmd %x\n",
				status, cmd);
		end_unlink_async(ehci);
	}

	spin_unlock_irqrestore(&ehci->lock, flags);
}

static void ehci_watchdog (unsigned long param)
{
	struct ehci_hcd		*ehci = (struct ehci_hcd *) param;
	unsigned long		flags;

	spin_lock_irqsave (&ehci->lock, flags);

 	/* stop async processing after it's idled a bit */
	if (test_bit (TIMER_ASYNC_OFF, &ehci->actions))
 		start_unlink_async (ehci, ehci->async);

	/* ehci could run by timer, without IRQs ... */
	ehci_work (ehci);

	spin_unlock_irqrestore (&ehci->lock, flags);
}

/* On some systems, leaving remote wakeup enabled prevents system shutdown.
 * The firmware seems to think that powering off is a wakeup event!
 * This routine turns off remote wakeup and everything else, on all ports.
 */
static void ehci_turn_off_all_ports(struct ehci_hcd *ehci)
{
	int	port = HCS_N_PORTS(ehci->hcs_params);

	while (port--)
		writel(PORT_RWC_BITS,
				&ehci->regs->port_status[port]);
}

/*
 * Halt HC, turn off all ports, and let the BIOS use the companion controllers.
 * Should be called with ehci->lock held.
 */
static void ehci_silence_controller(struct ehci_hcd *ehci)
{
	ehci_halt(ehci);
	ehci_turn_off_all_ports(ehci);

	/* make BIOS/etc use companion controller during reboot */
	writel(0, &ehci->regs->configured_flag);

	/* unblock posted writes */
	readl(&ehci->regs->configured_flag);
}

/* EHCI 0.96 (and later) section 5.1 says how to kick BIOS/SMM/...
 * off the controller (maybe it can boot from highspeed USB disks).
 */
static int bios_handoff (struct ehci_hcd *ehci, int where, u32 cap)
{
	if (cap & (1 << 16)) {
		int msec = 500;

		/* request handoff to OS */
		cap |= 1 << 24;
		pci_write_config_dword (ehci->hcd.pdev, where, cap);

		/* and wait a while for it to happen */
		do {
			wait_ms (10);
			msec -= 10;
			pci_read_config_dword (ehci->hcd.pdev, where, &cap);
		} while ((cap & (1 << 16)) && msec);
		if (cap & (1 << 16)) {
			ehci_err (ehci, "BIOS handoff failed (%d, %04x)\n",
				where, cap);
			pci_write_config_dword (ehci->hcd.pdev, where, 0);
			return 0;
		} 
		ehci_dbg (ehci, "BIOS handoff succeeded\n");
	}
	return 0;
}

static int
ehci_reboot (struct notifier_block *self, unsigned long code, void *null)
{
	struct ehci_hcd		*ehci;

	ehci = container_of (self, struct ehci_hcd, reboot_notifier);

	/* make BIOS/etc use companion controller during reboot */
	writel (0, &ehci->regs->configured_flag);
	return 0;
}


/* called by khubd or root hub init threads */

static int ehci_start (struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	u32			temp;
	struct usb_device	*udev;
	struct usb_bus		*bus;
	int			retval;
	u32			hcc_params;
	u8                      tempbyte;

	spin_lock_init (&ehci->lock);

	ehci->caps = (struct ehci_caps *) hcd->regs;
 	ehci->regs = (struct ehci_regs *) (hcd->regs +
 				HC_LENGTH (readl (&ehci->caps->hc_capbase)));
	dbg_hcs_params (ehci, "ehci_start");
	dbg_hcc_params (ehci, "ehci_start");

	hcc_params = readl (&ehci->caps->hcc_params);

	/* EHCI 0.96 and later may have "extended capabilities" */
	temp = HCC_EXT_CAPS (hcc_params);
	while (temp) {
		u32		cap;

		pci_read_config_dword (ehci->hcd.pdev, temp, &cap);
		ehci_dbg (ehci, "capability %04x at %02x\n", cap, temp);
		switch (cap & 0xff) {
		case 1:			/* BIOS/SMM/... handoff */
			if (bios_handoff (ehci, temp, cap) != 0)
				return -EOPNOTSUPP;
			break;
		case 0:			/* illegal reserved capability */
			cap = 0;
			/* FALLTHROUGH */
		default:		/* unknown */
			break;
		}
		temp = (cap >> 8) & 0xff;
	}

	/* cache this readonly data; minimize PCI reads */
	ehci->hcs_params = readl (&ehci->caps->hcs_params);

	/* force HC to halt state */
	if ((retval = ehci_halt (ehci)) != 0)
		return retval;

	/*
	 * hw default: 1K periodic list heads, one per frame.
	 * periodic_size can shrink by USBCMD update if hcc_params allows.
	 */
	ehci->periodic_size = DEFAULT_I_TDPS;
	INIT_LIST_HEAD(&ehci->cached_itd_list);
	INIT_LIST_HEAD(&ehci->cached_sitd_list);
	if ((retval = ehci_mem_init (ehci, SLAB_KERNEL)) < 0)
		return retval;

	/* controllers may cache some of the periodic schedule ... */
	if (HCC_ISOC_CACHE (hcc_params)) 	// full frame cache
		ehci->i_thresh = 8;
	else					// N microframes cached
		ehci->i_thresh = 2 + HCC_ISOC_THRES (hcc_params);

	ehci->reclaim = NULL;
	ehci->next_uframe = -1;
	ehci->clock_frame = -1;

	/* controller state:  unknown --> reset */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
	hcd->uses_new_polling = 1;
	hcd->poll_rh = 0;
#endif
	/* EHCI spec section 4.1 */
	if ((retval = ehci_reset (ehci)) != 0) {
		ehci_mem_cleanup (ehci);
		return retval;
	}

{
	u8 misc_reg;
	u32 vendor_id;
	
	pci_read_config_dword (ehci->hcd.pdev, PCI_VENDOR_ID, &vendor_id);
	if (vendor_id == 0x31041106) {
		/* VIA 6212 */
		printk(KERN_INFO "EHCI: Enabling VIA 6212 workarounds\n");
		pci_read_config_byte(ehci->hcd.pdev, 0x49, &misc_reg);
		misc_reg &= ~0x20;
		pci_write_config_byte(ehci->hcd.pdev, 0x49, misc_reg);
		pci_read_config_byte(ehci->hcd.pdev, 0x49, &misc_reg);

		pci_read_config_byte(ehci->hcd.pdev, 0x4b, &misc_reg);
		misc_reg |= 0x20;
		pci_write_config_byte(ehci->hcd.pdev, 0x4b, misc_reg);
		pci_read_config_byte(ehci->hcd.pdev, 0x4b, &misc_reg);
	}
}

	writel (INTR_MASK, &ehci->regs->intr_enable);
	writel (ehci->periodic_dma, &ehci->regs->frame_list);

	/*
	 * dedicate a qh for the async ring head, since we couldn't unlink
	 * a 'real' qh without stopping the async schedule [4.8].  use it
	 * as the 'reclamation list head' too.
	 * its dummy is used in hw_alt_next of many tds, to prevent the qh
	 * from automatically advancing to the next td after short reads.
	 */
	ehci->async->qh_next.qh = NULL;
	ehci->async->hw_next = QH_NEXT (ehci->async->qh_dma);
	ehci->async->hw_info1 = cpu_to_le32 (QH_HEAD);
	ehci->async->hw_token = cpu_to_le32 (QTD_STS_HALT);
	ehci->async->hw_qtd_next = EHCI_LIST_END;
	ehci->async->qh_state = QH_STATE_LINKED;
	ehci->async->hw_alt_next = QTD_NEXT (ehci->async->dummy->qtd_dma);
	writel ((u32)ehci->async->qh_dma, &ehci->regs->async_next);

	/*
	 * hcc_params controls whether ehci->regs->segment must (!!!)
	 * be used; it constrains QH/ITD/SITD and QTD locations.
	 * pci_pool consistent memory always uses segment zero.
	 * streaming mappings for I/O buffers, like pci_map_single(),
	 * can return segments above 4GB, if the device allows.
	 *
	 * NOTE:  the dma mask is visible through dma_supported(), so
	 * drivers can pass this info along ... like NETIF_F_HIGHDMA,
	 * Scsi_Host.highmem_io, and so forth.  It's readonly to all
	 * host side drivers though.
	 */
	if (HCC_64BIT_ADDR (hcc_params)) {
		writel (0, &ehci->regs->segment);
		if (!pci_set_dma_mask (ehci->hcd.pdev, 0xffffffffffffffffULL))
			ehci_info (ehci, "enabled 64bit PCI DMA\n");
	}

	/* help hc dma work well with cachelines */
	pci_set_mwi (ehci->hcd.pdev);

	/* clear interrupt enables, set irq latency */
	temp = readl (&ehci->regs->command) & 0x0fff;
	if (log2_irq_thresh < 0 || log2_irq_thresh > 6)
		log2_irq_thresh = 0;
	temp |= 1 << (16 + log2_irq_thresh);
	// if hc can park (ehci >= 0.96), default is 3 packets per async QH 
	if (HCC_CANPARK(hcc_params)) {
		/* HW default park == 3, on hardware that supports it (like
		 * NVidia and ALI silicon), maximizes throughput on the async
		 * schedule by avoiding QH fetches between transfers.
		 *
		 * With fast usb storage devices and NForce2, "park" seems to
		 * make problems:  throughput reduction (!), data errors...
		 */
		if (park) {
			park = min(park, (unsigned) 3);
			temp |= CMD_PARK;
			temp |= park << 8;
		}
		ehci_dbg(ehci, "park %d\n", park);
	}
	if (HCC_PGM_FRAMELISTLEN (hcc_params)) {
		/* periodic schedule size can be smaller than default */
		temp &= ~(3 << 2);
		temp |= (EHCI_TUNE_FLS << 2);
		switch (EHCI_TUNE_FLS) {
		case 0: ehci->periodic_size = 1024; break;
		case 1: ehci->periodic_size = 512; break;
		case 2: ehci->periodic_size = 256; break;
		default:	BUG ();
		}
	}
	temp &= ~(CMD_IAAD | CMD_ASE | CMD_PSE),
	// Philips, Intel, and maybe others need CMD_RUN before the
	// root hub will detect new devices (why?); NEC doesn't
	temp |= CMD_RUN;
	writel (temp, &ehci->regs->command);
	dbg_cmd (ehci, "init", temp);

	/* set async sleep time = 10 us ... ? */

	init_timer (&ehci->watchdog);
	ehci->watchdog.function = ehci_watchdog;
	ehci->watchdog.data = (unsigned long) ehci;

	init_timer(&ehci->iaa_watchdog);
	ehci->iaa_watchdog.function = ehci_iaa_watchdog;
	ehci->iaa_watchdog.data = (unsigned long) ehci;

	/* wire up the root hub */
	bus = hcd_to_bus (hcd);
	bus->root_hub = udev = usb_alloc_dev (NULL, bus);
	if (!udev) {
done2:
		ehci_mem_cleanup (ehci);
		return -ENOMEM;
	}

	/*
	 * Start, enabling full USB 2.0 functionality ... usb 1.1 devices
	 * are explicitly handed to companion controller(s), so no TT is
	 * involved with the root hub.
	 */
	ehci->reboot_notifier.notifier_call = ehci_reboot;
	register_reboot_notifier (&ehci->reboot_notifier);

	ehci->hcd.state = USB_STATE_READY;
	writel (FLAG_CF, &ehci->regs->configured_flag);
	readl (&ehci->regs->command);	/* unblock posted write */

        /* PCI Serial Bus Release Number is at 0x60 offset */
	pci_read_config_byte (hcd->pdev, 0x60, &tempbyte);
	temp = HC_VERSION(readl (&ehci->caps->hc_capbase));
	ehci_info (ehci,
		"USB %x.%x enabled, EHCI %x.%02x, driver %s\n",
		((tempbyte & 0xf0)>>4), (tempbyte & 0x0f),
		temp >> 8, temp & 0xff, DRIVER_VERSION);

	/*
	 * From here on, khubd concurrently accesses the root
	 * hub; drivers will be talking to enumerated devices.
	 *
	 * Before this point the HC was idle/ready.  After, khubd
	 * and device drivers may start it running.
	 */
	usb_connect (udev);
	udev->speed = USB_SPEED_HIGH;
	if (hcd_register_root (hcd) != 0) {
		if (hcd->state == USB_STATE_RUNNING)
			ehci_quiesce (ehci);
		ehci_reset (ehci);
		bus->root_hub = 0;
		usb_free_dev (udev); 
		retval = -ENODEV;
		goto done2;
	}

	create_debug_files (ehci);

	return 0;
}

/*
 * Called when the ehci_hcd module is removed.
 */
static void ehci_stop (struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);

	ehci_dbg (ehci, "stop\n");

	/* no more interrupts ... */
	del_timer_sync (&ehci->watchdog);
	del_timer_sync(&ehci->iaa_watchdog);

	spin_lock_irq(&ehci->lock);
	if (HCD_IS_RUNNING (hcd->state))
		ehci_quiesce (ehci);

	ehci_silence_controller(ehci);
	ehci_reset (ehci);
	spin_unlock_irq(&ehci->lock);

	/* let companion controllers work when we aren't */
	writel (0, &ehci->regs->configured_flag);
	unregister_reboot_notifier (&ehci->reboot_notifier);

	remove_debug_files (ehci);

	/* root hub is shut down separately (first, when possible) */
	spin_lock_irq (&ehci->lock);
	if (ehci->async)
		ehci_work (ehci);
	spin_unlock_irq (&ehci->lock);
	ehci_mem_cleanup (ehci);

#ifdef	EHCI_STATS
	ehci_dbg (ehci, "irq normal %ld err %ld reclaim %ld (lost %ld)\n",
		ehci->stats.normal, ehci->stats.error, ehci->stats.reclaim,
		ehci->stats.lost_iaa);
	ehci_dbg (ehci, "complete %ld unlink %ld\n",
		ehci->stats.complete, ehci->stats.unlink);
#endif

	dbg_status (ehci, "ehci_stop completed", readl (&ehci->regs->status));
}

static int ehci_get_frame (struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	return (readl (&ehci->regs->frame_index) >> 3) % ehci->periodic_size;
}

/*-------------------------------------------------------------------------*/

#ifdef	CONFIG_PM

/* suspend/resume, section 4.3 */

static int ehci_suspend (struct usb_hcd *hcd, u32 state)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	int			ports;
	int			i;

	ehci_dbg (ehci, "suspend to %d\n", state);

	ports = HCS_N_PORTS (ehci->hcs_params);

	// FIXME:  This assumes what's probably a D3 level suspend...

	// FIXME:  usb wakeup events on this bus should resume the machine.
	// pci config register PORTWAKECAP controls which ports can do it;
	// bios may have initted the register...

	/* suspend each port, then stop the hc */
	for (i = 0; i < ports; i++) {
		int	temp = readl (&ehci->regs->port_status [i]);

		if ((temp & PORT_PE) == 0
				|| (temp & PORT_OWNER) != 0)
			continue;
		ehci_dbg (ehci, "suspend port %d", i);
		temp |= PORT_SUSPEND;
		writel (temp, &ehci->regs->port_status [i]);
	}

	if (hcd->state == USB_STATE_RUNNING)
		ehci_quiesce (ehci);
	writel (readl (&ehci->regs->command) & ~CMD_RUN, &ehci->regs->command);

// save pci FLADJ value

	/* who tells PCI to reduce power consumption? */

	return 0;
}

static int ehci_resume (struct usb_hcd *hcd)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	int			ports;
	int			i;

	ehci_dbg (ehci, "resume\n");

	ports = HCS_N_PORTS (ehci->hcs_params);

	// FIXME:  if controller didn't retain state,
	// return and let generic code clean it up
	// test configured_flag ?

	/* resume HC and each port */
// restore pci FLADJ value
	// khubd and drivers will set HC running, if needed;
	hcd->state = USB_STATE_READY;
	// FIXME Philips/Intel/... etc don't really have a "READY"
	// state ... turn on CMD_RUN too
	for (i = 0; i < ports; i++) {
		int	temp = readl (&ehci->regs->port_status [i]);

		if ((temp & PORT_PE) == 0
				|| (temp & PORT_SUSPEND) != 0)
			continue;
		ehci_dbg (ehci, "resume port %d", i);
		temp |= PORT_RESUME;
		writel (temp, &ehci->regs->port_status [i]);
		readl (&ehci->regs->command);	/* unblock posted writes */

		wait_ms (20);
		temp &= ~PORT_RESUME;
		writel (temp, &ehci->regs->port_status [i]);
	}
	readl (&ehci->regs->command);	/* unblock posted writes */
	return 0;
}

#endif

/*-------------------------------------------------------------------------*/

/*
 * ehci_work is called from some interrupts, timers, and so on.
 * it calls driver completion functions, after dropping ehci->lock.
 */
static void ehci_work (struct ehci_hcd *ehci)
{
	timer_action_done (ehci, TIMER_IO_WATCHDOG);

	/* another CPU may drop ehci->lock during a schedule scan while
	 * it reports urb completions.  this flag guards against bogus
	 * attempts at re-entrant schedule scanning.
	 */
	if (ehci->scanning)
		return;
	ehci->scanning = 1;
	scan_async (ehci);
	if (ehci->next_uframe != -1)
		scan_periodic (ehci);
	ehci->scanning = 0;

	/* the IO watchdog guards against hardware or driver bugs that
	 * misplace IRQs, and should let us run completely without IRQs.
	 * such lossage has been observed on both VT6202 and VT8235.
	 */
	if (HCD_IS_RUNNING (ehci->hcd.state) &&
			(ehci->async->qh_next.ptr != NULL ||
			 ehci->periodic_sched != 0))
		timer_action (ehci, TIMER_IO_WATCHDOG);
}

/*-------------------------------------------------------------------------*/

static void ehci_irq (struct usb_hcd *hcd, struct pt_regs *regs)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	u32			status, cmd;
	int			bh;

	spin_lock (&ehci->lock);

	status = readl (&ehci->regs->status);

	/* e.g. cardbus physical eject */
	if (status == ~(u32) 0) {
		ehci_dbg (ehci, "device removed\n");
		goto dead;
	}

	status &= INTR_MASK;
	if (!status) {			/* irq sharing? */
		spin_unlock(&ehci->lock);
		return IRQ_NONE;
	}

	/* clear (just) interrupts */
	writel (status, &ehci->regs->status);
	cmd = readl (&ehci->regs->command);
	bh = 0;

#ifdef	EHCI_VERBOSE_DEBUG
	/* unrequested/ignored: Port Change Detect, Frame List Rollover */
	dbg_status (ehci, "irq", status);
#endif

	/* INT, ERR, and IAA interrupt rates can be throttled */

	/* normal [4.15.1.2] or error [4.15.1.1] completion */
	if (likely ((status & (STS_INT|STS_ERR)) != 0)) {
		if (likely ((status & STS_ERR) == 0))
			COUNT (ehci->stats.normal);
		else
			COUNT (ehci->stats.error);
		bh = 1;
	}

	/* complete the unlinking of some qh [4.15.2.3] */
	if (status & STS_IAA) {
		/* guard against (alleged) silicon errata */
		if (cmd & CMD_IAAD) {
			writel(cmd & ~CMD_IAAD,
					&ehci->regs->command);
			ehci_dbg(ehci, "IAA with IAAD still set?\n");
		}
		if (ehci->reclaim) {
			COUNT(ehci->stats.reclaim);
			end_unlink_async(ehci);
		} else
			ehci_dbg(ehci, "IAA with nothing to reclaim?\n");
	}

	/* PCI errors [4.15.2.4] */
	if (unlikely ((status & STS_FATAL) != 0)) {
		dbg_cmd (ehci, "fatal", readl(&ehci->regs->command));
		dbg_status (ehci, "fatal", status);
		if (status & STS_HALT) {
			ehci_err (ehci, "fatal error\n");
dead:
			ehci_reset (ehci);
			writel(0, &ehci->regs->configured_flag);
			/* generic layer kills/unlinks all urbs, then
			 * uses ehci_stop to clean up the rest
			 */
			bh = 1;
		}
	}

	if (bh)
		ehci_work (ehci);
	spin_unlock (&ehci->lock);
	return IRQ_HANDLED;
}

/*-------------------------------------------------------------------------*/

/*
 * non-error returns are a promise to giveback() the urb later
 * we drop ownership so next owner (or urb unlink) can get it
 *
 * urb + dev is in hcd_dev.urb_list
 * we're queueing TDs onto software and hardware lists
 *
 * hcd-specific init for hcpriv hasn't been done yet
 *
 * NOTE:  control, bulk, and interrupt share the same code to append TDs
 * to a (possibly active) QH, and the same QH scanning code.
 */
static int ehci_urb_enqueue (
	struct usb_hcd	*hcd,
	struct urb	*urb,
	int		mem_flags
) {
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	struct list_head	qtd_list;

	urb->transfer_flags &= ~EHCI_STATE_UNLINK;
	INIT_LIST_HEAD (&qtd_list);

	switch (usb_pipetype (urb->pipe)) {
	case PIPE_CONTROL:
		/* qh_completions() code doesn't handle all the fault cases
		 * in multi-TD control transfers.  Even 1KB is rare anyway.
		 */
		if (urb->transfer_buffer_length > (16 * 1024))
			return -EMSGSIZE;
		/* FALLTHROUGH */
	// case PIPE_BULK:
	default:
		if (!qh_urb_transaction (ehci, urb, &qtd_list, mem_flags))
			return -ENOMEM;
		return submit_async (ehci, urb, &qtd_list, mem_flags);

	case PIPE_INTERRUPT:
		if (!qh_urb_transaction (ehci, urb, &qtd_list, mem_flags))
			return -ENOMEM;
		return intr_submit (ehci, urb, &qtd_list, mem_flags);

	case PIPE_ISOCHRONOUS:
		if (urb->dev->speed == USB_SPEED_HIGH)
			return itd_submit (ehci, urb, mem_flags);
		else
			return sitd_submit (ehci, urb, mem_flags);
	}
}

static void unlink_async (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	/* failfast */
	if (!HCD_IS_RUNNING (ehci->hcd.state) && ehci->reclaim)
		end_unlink_async (ehci);

	/* If the QH isn't linked then there's nothing we can do
	 * unless we were called during a giveback, in which case
	 * qh_completions() has to deal with it.
	 */
	if (qh->qh_state != QH_STATE_LINKED) {
		if (qh->qh_state == QH_STATE_COMPLETING)
			qh->needs_rescan = 1;
		return;
	}

	/* defer till later if busy */
	if (ehci->reclaim) {
		struct ehci_qh		*last;

		for (last = ehci->reclaim;
				last->reclaim;
				last = last->reclaim)
			continue;
		qh->qh_state = QH_STATE_UNLINK_WAIT;
		last->reclaim = qh;

	/* start IAA cycle */
	} else 
		start_unlink_async (ehci, qh);
}

/* remove from hardware lists
 * completions normally happen asynchronously
 */

static int ehci_urb_dequeue (struct usb_hcd *hcd, struct urb *urb)
{
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	struct ehci_qh		*qh;
	unsigned long		flags;
	int			rc = 0;

	spin_lock_irqsave (&ehci->lock, flags);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
	rc = usb_hcd_check_unlink_urb(hcd, urb, urb->status);
	if (rc)
		goto done;
#endif

	switch (usb_pipetype (urb->pipe)) {
	// case PIPE_CONTROL:
	// case PIPE_BULK:
	default:
		qh = (struct ehci_qh *) urb->hcpriv;
		if (!qh)
			break;
		switch (qh->qh_state) {
		case QH_STATE_LINKED:
		case QH_STATE_COMPLETING:
			unlink_async(ehci, qh);
			break;
		case QH_STATE_UNLINK:
		case QH_STATE_UNLINK_WAIT:
			/* already started */
			break;
		case QH_STATE_IDLE:
			WARN_ON(1);
			break;
		}
		break;

	case PIPE_INTERRUPT:
		qh = (struct ehci_qh *) urb->hcpriv;
		if (!qh)
			break;
		switch (qh->qh_state) {
		case QH_STATE_LINKED:
		case QH_STATE_COMPLETING:
			intr_deschedule (ehci, qh);
			break;
		case QH_STATE_IDLE:
			qh_completions (ehci, qh);
			break;
		default:
			ehci_dbg (ehci, "bogus qh %p state %d\n",
					qh, qh->qh_state);
			goto done;
		}
		break;

	case PIPE_ISOCHRONOUS:
		// itd or sitd ...

		// wait till next completion, do it then.
		// completion irqs can wait up to 1024 msec,
		break;
	}
done:
	spin_unlock_irqrestore (&ehci->lock, flags);
	return rc;
}

/*-------------------------------------------------------------------------*/

// bulk qh holds the data toggle

// ehci_endpoint_disable() in 2.6
static void ehci_free_config (struct usb_hcd *hcd, struct usb_device *udev)
{
	struct hcd_dev		*dev = (struct hcd_dev *)udev->hcpriv;
	struct ehci_hcd		*ehci = hcd_to_ehci (hcd);
	int			i;
	unsigned long		flags;

	/* ASSERT:  no requests/urbs are still linked (so no TDs) */
	/* ASSERT:  nobody can be submitting urbs for this any more */

	ehci_dbg (ehci, "free_config %s devnum %d\n",
			udev->devpath, udev->devnum);

	spin_lock_irqsave (&ehci->lock, flags);
	for (i = 0; i < 32; i++) {
		if (dev->ep [i]) {
			struct ehci_qh		*qh;
			char			*why;

			/* dev->ep is a QH unless info1.maxpacket of zero
			 * marks an iso stream head.
			 * FIXME do something smarter here with ISO
			 */
			qh = (struct ehci_qh *) dev->ep [i];
			if (qh->hw_info1 == 0) {
				ehci_err (ehci, "no iso cleanup!!\n");
				continue;
			}


			/* detect/report non-recoverable errors */
			if (in_interrupt ()) 
				why = "disconnect() didn't";
			else if ((qh->hw_info2 & cpu_to_le32 (0xffff)) != 0
					&& qh->qh_state != QH_STATE_IDLE)
				why = "(active periodic)";
			else
				why = 0;
			if (why) {
				err ("dev %s-%s ep %d-%s error: %s",
					hcd_to_bus (hcd)->bus_name,
					udev->devpath,
					i & 0xf, (i & 0x10) ? "IN" : "OUT",
					why);
				BUG ();
			}

			dev->ep [i] = 0;
			if (qh->qh_state == QH_STATE_IDLE)
				goto idle;
			ehci_dbg (ehci, "free_config, async ep 0x%02x qh %p",
					i, qh);

			/* scan_async() empties the ring as it does its work,
			 * using IAA, but doesn't (yet?) turn it off.  if it
			 * doesn't empty this qh, likely it's the last entry.
			 */
			while (qh->qh_state == QH_STATE_LINKED
					&& ehci->reclaim
					&& HCD_IS_RUNNING (ehci->hcd.state)
					) {
				spin_unlock_irqrestore (&ehci->lock, flags);
				/* wait_ms() won't spin, we're a thread;
				 * and we know IRQ/timer/... can progress
				 */
				wait_ms (1);
				spin_lock_irqsave (&ehci->lock, flags);
			}
			if (qh->qh_state == QH_STATE_LINKED)
				start_unlink_async (ehci, qh);
			while (qh->qh_state != QH_STATE_IDLE
					&& ehci->hcd.state != USB_STATE_HALT) {
				spin_unlock_irqrestore (&ehci->lock, flags);
				wait_ms (1);
				spin_lock_irqsave (&ehci->lock, flags);
			}
idle:
			qh_put (ehci, qh);
		}
	}

	spin_unlock_irqrestore (&ehci->lock, flags);
}

/*-------------------------------------------------------------------------*/

static const struct hc_driver ehci_driver = {
	.description =		hcd_name,

	/*
	 * generic hardware linkage
	 */
	.irq =			ehci_irq,
	.flags =		HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.start =		ehci_start,
#ifdef	CONFIG_PM
	.suspend =		ehci_suspend,
	.resume =		ehci_resume,
#endif
	.stop =			ehci_stop,

	/*
	 * memory lifecycle (except per-request)
	 */
	.hcd_alloc =		ehci_hcd_alloc,
	.hcd_free =		ehci_hcd_free,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		ehci_urb_enqueue,
	.urb_dequeue =		ehci_urb_dequeue,
	.free_config =		ehci_free_config,

	/*
	 * scheduling support
	 */
	.get_frame_number =	ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	ehci_hub_status_data,
	.hub_control =		ehci_hub_control,
};

/*-------------------------------------------------------------------------*/

/* EHCI spec says PCI is required. */

/* PCI driver selection metadata; PCI hotplugging uses this */
static const struct pci_device_id __devinitdata pci_ids [] = { {

	/* handle any USB 2.0 EHCI controller */

	.class = 		((PCI_CLASS_SERIAL_USB << 8) | 0x20),
	.class_mask = 	~0,
	.driver_data =	(unsigned long) &ehci_driver,

	/* no matter who makes it */
	.vendor =	PCI_ANY_ID,
	.device =	PCI_ANY_ID,
	.subvendor =	PCI_ANY_ID,
	.subdevice =	PCI_ANY_ID,

}, { /* end: all zeroes */ }
};
MODULE_DEVICE_TABLE (pci, pci_ids);

/* pci driver glue; this is a "new style" PCI driver module */
static struct pci_driver ehci_pci_driver = {
	.name =		(char *) hcd_name,
	.id_table =	pci_ids,

	.probe =	usb_hcd_pci_probe,
	.remove =	usb_hcd_pci_remove,

#ifdef	CONFIG_PM
	.suspend =	usb_hcd_pci_suspend,
	.resume =	usb_hcd_pci_resume,
#endif
};

#define DRIVER_INFO DRIVER_VERSION " " DRIVER_DESC

MODULE_DESCRIPTION (DRIVER_INFO);
MODULE_AUTHOR (DRIVER_AUTHOR);
MODULE_LICENSE ("GPL");

static int __init init (void) 
{
	pr_debug ("%s: block sizes: qh %Zd qtd %Zd itd %Zd sitd %Zd\n",
		hcd_name,
		sizeof (struct ehci_qh), sizeof (struct ehci_qtd),
		sizeof (struct ehci_itd), sizeof (struct ehci_sitd));

	return pci_module_init (&ehci_pci_driver);
}
module_init (init);

static void __exit cleanup (void) 
{	
	pci_unregister_driver (&ehci_pci_driver);
}
module_exit (cleanup);
