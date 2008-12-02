/*
 * linux/drivers/usb/gadget/pxa2xx_udc.h
 * Intel PXA2xx on-chip full speed USB device controller
 * 
 * Copyright (C) 2003 Robert Schwebel <r.schwebel@pengutronix.de>, Pengutronix
 * Copyright (C) 2003 David Brownell
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINUX_USB_GADGET_PXA25X_H
#define __LINUX_USB_GADGET_PXA25X_H

#include <linux/types.h>

/*-------------------------------------------------------------------------*/

/* pxa2xx has this (move to include/asm-arm/arch-pxa/pxa-regs.h) */
#define UFNRH_SIR	(1 << 7)	/* SOF interrupt request */
#define UFNRH_SIM	(1 << 6)	/* SOF interrupt mask */
#define UFNRH_IPE14	(1 << 5)	/* ISO packet error, ep14 */
#define UFNRH_IPE9	(1 << 4)	/* ISO packet error, ep9 */
#define UFNRH_IPE4	(1 << 3)	/* ISO packet error, ep4 */

/* pxa255 has this (move to include/asm-arm/arch-pxa/pxa-regs.h) */
#define	UDCCFR		UDC_RES2	/* UDC Control Function Register */
#define UDCCFR_AREN	(1 << 7)	/* ACK response enable (now) */
#define UDCCFR_ACM	(1 << 2)	/* ACK control mode (wait for AREN) */

/* for address space reservation */
#define	REGISTER_FIRST	((unsigned long)(&UDCCR))
#define	REGISTER_LAST	((unsigned long)(&UDDR14))	/* not UDDR15! */
#define REGISTER_LENGTH	((REGISTER_LAST - REGISTER_FIRST) + 4)

/*-------------------------------------------------------------------------*/

struct pxa2xx_udc;

struct pxa2xx_ep {
	struct usb_ep				ep;
	struct pxa2xx_udc			*dev;

	const struct usb_endpoint_descriptor	*desc;
	struct list_head			queue;
	int					dma; 

	u8					bEndpointAddress;
	u8					bmAttributes;

	unsigned				stopped : 1;
							 
	/* UDCCS = UDC Control/Status for this EP
	 * UBCR = UDC Byte Count Remaining (contents of OUT fifo)
	 * UDDR = UDC Endpoint Data Register (the fifo)
	 * DRCM = DMA Request Channel Map
	 */
	volatile u32				*reg_udccs;
	volatile u32				*reg_ubcr;
	volatile u32				*reg_uddr;
	volatile u32				*reg_drcmr;
};

struct pxa2xx_request {
	struct usb_request			req;
	struct list_head			queue;
};

enum ep0_state { 
	EP0_IDLE,
	EP0_IN_DATA_PHASE,
	EP0_OUT_DATA_PHASE,
	EP0_END_XFER,
	EP0_STALL,
};

#define EP0_FIFO_SIZE	((unsigned)16)
#define BULK_FIFO_SIZE	((unsigned)64)
#define ISO_FIFO_SIZE	((unsigned)256)
#define INT_FIFO_SIZE	((unsigned)8)

struct udc_stats {
	struct ep0stats {
		unsigned long		ops;
		unsigned long		bytes;
	} read, write;
	unsigned long			irqs;
};

struct pxa2xx_udc {
	struct usb_gadget			gadget;
	struct usb_gadget_driver		*driver;

	enum ep0_state				ep0state;
	struct udc_stats			stats;
	unsigned				got_irq : 1,
						got_disc : 1,
						has_cfr : 1,
						req_pending : 1,
						req_std : 1,
						req_config : 1;

#define start_watchdog(dev) mod_timer(&dev->timer, jiffies + (HZ/200))
	struct timer_list			timer;

	struct pxa2xx_ep			ep [16];
};

/* 2.5 changes ... */

#ifndef container_of
#define container_of    list_entry
#endif

#ifndef WARN_ON
#define WARN_ON BUG_ON
#endif

/*-------------------------------------------------------------------------*/

#ifdef CONFIG_ARCH_INNOKOM
#include <asm/arch/innokom.h>
#define is_usb_connected	(GPLR(GPIO_INNOKOM_USB_DISC) \
					& GPIO_bit(GPIO_INNOKOM_USB_DISC))
#define make_usb_disappear() ({ \
	(GPSR(GPIO_INNOKOM_USB_ONOFF) | GPIO_bit(GPIO_INNOKOM_USB_ONOFF));\
	printk("RS: disappear\n"); \
	udelay(5); \
	})
#define let_usb_appear() ({ \
	(GPCR(GPIO_INNOKOM_USB_ONOFF) | GPIO_bit(GPIO_INNOKOM_USB_ONOFF)); \
	printk("RS: disappear\n"); \
	udelay(5); \
	})
#endif

/*-------------------------------------------------------------------------*/

#ifdef CONFIG_ARCH_LUBBOCK
#include <asm/arch/lubbock.h>
#define	is_usb_connected	((LUB_MISC_RD & (1 << 9)) == 0)
/* lubbock can also report usb connect/disconnect irqs */

#define LED_CONNECTED_ON	(DISCRETE_LED_ON(D26))
#define LED_CONNECTED_OFF	(DISCRETE_LED_OFF(D26) /*, LUB_HEXLED = 0*/)
#define LED_EP0_ON		(DISCRETE_LED_ON(D25))
#define LED_EP0_OFF		(DISCRETE_LED_OFF(D25))
#endif

/*-------------------------------------------------------------------------*/

/* LEDs are only for debug */
#ifndef LED_CONNECTED_ON
#define LED_CONNECTED_ON	do {} while(0)
#define LED_CONNECTED_OFF	do {} while(0)
#endif
#ifndef LED_EP0_ON
#define LED_EP0_ON		do {} while (0)
#define LED_EP0_OFF		do {} while (0)
#endif

/* one GPIO should be used to detect disconnect */
#ifndef is_usb_connected
#define is_usb_connected	(1)
#warning Not able to detect USB host connect/disconnect.
#endif

/* one GPIO should be used to force the UDC off USB */
#ifndef make_usb_disappear
#define make_usb_disappear()	do {} while (0)
#define let_usb_appear()	do {} while (0)
#warning Not able to force USB device to disconnect.
#endif

/*-------------------------------------------------------------------------*/

/*
 * Debugging support vanishes in non-debug builds.  DBG_NORMAL should be
 * mostly silent during normal use/testing, with no timing side-effects.
 */
#define DBG_NORMAL	1	/* error paths, device state transitions */
#define DBG_VERBOSE	2	/* add some success path trace info */
#define DBG_NOISY	3	/* ... even more: request level */
#define DBG_VERY_NOISY	4	/* ... even more: packet level */

#ifdef DEBUG

static const char *state_name[] = {
	"EP0_IDLE",
	"EP0_IN_DATA_PHASE", "EP0_OUT_DATA_PHASE",
	"EP0_END_XFER", "EP0_STALL"
};

#define DMSG(stuff...) printk(KERN_DEBUG "udc: " stuff)

#ifdef VERBOSE
#    define UDC_DEBUG VERBOSE
#else
#    define UDC_DEBUG DBG_NORMAL
#endif

static void __attribute__ ((__unused__))
dump_udccr(const char *label)
{
	u32	udccr = UDCCR;
	DMSG("%s %02X =%s%s%s%s%s%s%s%s\n",
		label, udccr,
		(udccr & UDCCR_REM) ? " rem" : "",
		(udccr & UDCCR_RSTIR) ? " rstir" : "",
		(udccr & UDCCR_SRM) ? " srm" : "",
		(udccr & UDCCR_SUSIR) ? " susir" : "",
		(udccr & UDCCR_RESIR) ? " resir" : "",
		(udccr & UDCCR_RSM) ? " rsm" : "",
		(udccr & UDCCR_UDA) ? " uda" : "",
		(udccr & UDCCR_UDE) ? " ude" : "");
}

static void __attribute__ ((__unused__))
dump_udccs0(const char *label)
{
	u32		udccs0 = UDCCS0;

	DMSG("%s %s %02X =%s%s%s%s%s%s%s%s\n",
		label, state_name[the_controller->ep0state], udccs0,
		(udccs0 & UDCCS0_SA) ? " sa" : "",
		(udccs0 & UDCCS0_RNE) ? " rne" : "",
		(udccs0 & UDCCS0_FST) ? " fst" : "",
		(udccs0 & UDCCS0_SST) ? " sst" : "",
		(udccs0 & UDCCS0_DRWF) ? " dwrf" : "",
		(udccs0 & UDCCS0_FTF) ? " ftf" : "",
		(udccs0 & UDCCS0_IPR) ? " ipr" : "",
		(udccs0 & UDCCS0_OPR) ? " opr" : "");
}

static void __attribute__ ((__unused__))
dump_state(struct pxa2xx_udc *dev)
{
	u32		tmp;
	unsigned	i;

	DMSG("%s %s, uicr %02X.%02X, usir %02X.%02x, ufnr %02X.%02X\n",
		is_usb_connected ? "host " : "disconnected",
		state_name[dev->ep0state],
		UICR1, UICR0, USIR1, USIR0, UFNRH, UFNRL);
	dump_udccr("udccr");
	if (dev->has_cfr) {
		tmp = UDCCFR;
		DMSG("udccfr %02X =%s%s\n", tmp,
			(tmp & UDCCFR_AREN) ? " aren" : "",
			(tmp & UDCCFR_ACM) ? " acm" : "");
	}

	if (!dev->driver) {
		DMSG("no gadget driver bound\n");
		return;
	} else
		DMSG("ep0 driver '%s'\n", dev->driver->driver.name);
	
	if (!is_usb_connected)
		return;

	dump_udccs0 ("udccs0");
	DMSG("ep0 IN %lu/%lu, OUT %lu/%lu\n",
		dev->stats.write.bytes, dev->stats.write.ops,
		dev->stats.read.bytes, dev->stats.read.ops);

	for (i = 1; i <= 15; i++) {
		if (dev->ep [i].desc == 0)
			continue;
		DMSG ("udccs%d = %02x\n", i, *dev->ep->reg_udccs);
	}
}

#else

#define DMSG(stuff...)		do{}while(0)

#define	dump_udccr(x)	do{}while(0)
#define	dump_udccs0(x)	do{}while(0)
#define	dump_state(x)	do{}while(0)

#define UDC_DEBUG ((unsigned)0)

#endif

#define DBG(lvl, stuff...) do{if ((lvl) <= UDC_DEBUG) DMSG(stuff);}while(0)

#define WARN(stuff...) printk(KERN_WARNING "udc: " stuff)


/* 2.4 backport support */
#define irqreturn_t	void
#define	IRQ_HANDLED


#endif /* __LINUX_USB_GADGET_PXA25X_H */
