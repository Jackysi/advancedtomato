/*
 * Copyright (c) 2001-2002 by David Brownell
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

/* this file is part of ehci-hcd.c */

/*-------------------------------------------------------------------------*/

/*
 * EHCI Root Hub ... the nonsharable stuff
 *
 * Registers don't need cpu_to_le32, that happens transparently
 */

/*-------------------------------------------------------------------------*/

#define PORT_WAKE_BITS  (PORT_WKOC_E|PORT_WKDISC_E|PORT_WKCONN_E)

#ifndef STUB_DEBUG_FILES

/* Display the ports dedicated to the companion controller */
static ssize_t show_companion(struct usb_device *udev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct ehci_hcd		*ehci;
	int			nports, index, n;
	int			count = PAGE_SIZE;
	char			*ptr = buf;

	ehci = hcd_to_ehci((struct usb_hcd *)udev->hcpriv);
	nports = HCS_N_PORTS(ehci->hcs_params);

	for (index = 0; index < nports; ++index) {
		if (test_bit(index, &ehci->companion_ports)) {
			n = snprintf(ptr, count, "%d\n", index + 1);
			ptr += n;
			count -= n;
		}
	}
	return ptr - buf;
}

/*
 * Sets the owner of a port
 */
static void set_owner(struct ehci_hcd *ehci, int portnum, int new_owner)
{
	u32 __iomem		*status_reg;
	u32			port_status;
	int 			try;

	status_reg = &ehci->regs->port_status[portnum];

	/*
	 * The controller won't set the OWNER bit if the port is
	 * enabled, so this loop will sometimes require at least two
	 * iterations: one to disable the port and one to set OWNER.
	 */
	for (try = 4; try > 0; --try) {
		spin_lock_irq(&ehci->lock);
		port_status = readl(status_reg);
		if ((port_status & PORT_OWNER) == new_owner
				|| (port_status & (PORT_OWNER | PORT_CONNECT))
					== 0)
			try = 0;
		else {
			port_status ^= PORT_OWNER;
			port_status &= ~(PORT_PE | PORT_RWC_BITS);
			writel(port_status, status_reg);
		}
		spin_unlock_irq(&ehci->lock);
		if (try > 1)
			msleep(5);
	}
}

/*
 * Dedicate or undedicate a port to the companion controller.
 * Syntax is "[-]portnum", where a leading '-' sign means
 * return control of the port to the EHCI controller.
 */
static ssize_t store_companion(struct usb_device *udev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct ehci_hcd		*ehci;
	int			portnum, new_owner;

	ehci = hcd_to_ehci((struct usb_hcd *)udev->hcpriv);
	new_owner = PORT_OWNER;		/* Owned by companion */
	if (sscanf(buf, "%d", &portnum) != 1)
		return -EINVAL;
	if (portnum < 0) {
		portnum = - portnum;
		new_owner = 0;		/* Owned by EHCI */
	}
	if (portnum <= 0 || portnum > HCS_N_PORTS(ehci->hcs_params))
		return -ENOENT;
	portnum--;
	if (new_owner)
		set_bit(portnum, &ehci->companion_ports);
	else
		clear_bit(portnum, &ehci->companion_ports);
	set_owner(ehci, portnum, new_owner);
	return count;
}
static DEVICE_ATTR (companion, 0644, show_companion, store_companion);

#endif	/* STUB_DEBUG_FILES */

/*-------------------------------------------------------------------------*/

static int check_reset_complete (
	struct ehci_hcd	*ehci,
	int		index,
	int		port_status
) {
	if (!(port_status & PORT_CONNECT)) {
		ehci->reset_done [index] = 0;
		return port_status;
	}

	/* if reset finished and it's still not enabled -- handoff */
	if (!(port_status & PORT_PE)) {
		/* with integrated TT, there's nobody to hand it to! */
		if (ehci_is_TDI(ehci)) {
			ehci_dbg (ehci,
				"Failed to enable port %d on root hub TT\n",
				index+1);
			return port_status;
		}

		ehci_dbg (ehci, "port %d full speed --> companion\n",
			index + 1);

		// what happens if HCS_N_CC(params) == 0 ?
		port_status |= PORT_OWNER;
		writel (port_status, &ehci->regs->port_status [index]);
		port_status &= ~PORT_RWC_BITS;

	} else
		ehci_dbg (ehci, "port %d high speed\n", index + 1);

	return port_status;
}

/*-------------------------------------------------------------------------*/


/* build "status change" packet (one or two bytes) from HC registers */

static int
ehci_hub_status_data (struct usb_hcd *hcd, char *buf)
{
	struct ehci_hcd	*ehci = hcd_to_ehci (hcd);
	u32		temp, status = 0;
	u32		mask;
	int		ports, i, retval = 1;
	unsigned long	flags;

	/* if !USB_SUSPEND, root hub timers won't get shut down ... */
	if (!HCD_IS_RUNNING(hcd->state))
		return 0;

	/* init status to no-changes */
	buf [0] = 0;
	ports = HCS_N_PORTS (ehci->hcs_params);
	if (ports > 7) {
		buf [1] = 0;
		retval++;
	}
	
	/* Some boards (mostly VIA?) report bogus overcurrent indications,
	 * causing massive log spam unless we completely ignore them.  It
	 * may be relevant that VIA VT8235 controllers, where PORT_POWER is
	 * always set, seem to clear PORT_OCC and PORT_CSC when writing to
	 * PORT_POWER; that's surprising, but maybe within-spec.
	 */
	if (!ignore_oc)
		mask = PORT_CSC | PORT_PEC | PORT_OCC;
	else
		mask = PORT_CSC | PORT_PEC;
	// PORT_RESUME from hardware ~= PORT_STAT_C_SUSPEND

	/* no hub change reports (bit 0) for now (power, ...) */

	/* port N changes (bit N)? */
	spin_lock_irqsave (&ehci->lock, flags);
	for (i = 0; i < ports; i++) {
		temp = readl (&ehci->regs->port_status [i]);
		if (temp & PORT_OWNER) {
			/* don't report this in GetPortStatus */
			if (temp & PORT_CSC) {
				temp &= ~PORT_CSC;
				writel (temp, &ehci->regs->port_status [i]);
			}
			continue;
		}
		if (!(temp & PORT_CONNECT))
			ehci->reset_done [i] = 0;
		if ((temp & mask) != 0
				|| ((temp & PORT_RESUME) != 0
					&& time_after_eq(jiffies,
						ehci->reset_done[i]))) {
			if (i < 7)
			    buf [0] |= 1 << (i + 1);
			else
			    buf [1] |= 1 << (i - 7);
			status = STS_PCD;
		}
	}
	/* FIXME autosuspend idle root hubs */
	spin_unlock_irqrestore (&ehci->lock, flags);
	return status ? retval : 0;
}

/*-------------------------------------------------------------------------*/

static void
ehci_hub_descriptor (
	struct ehci_hcd			*ehci,
	struct usb_hub_descriptor	*desc
) {
	int		ports = HCS_N_PORTS (ehci->hcs_params);
	u16		temp;

	desc->bDescriptorType = 0x29;
	desc->bPwrOn2PwrGood = 10;	/* ehci 1.0, 2.3.9 says 20ms max */
	desc->bHubContrCurrent = 0;

	desc->bNbrPorts = ports;
	temp = 1 + (ports / 8);
	desc->bDescLength = 7 + 2 * temp;

	/* two bitmaps:  ports removable, and usb 1.0 legacy PortPwrCtrlMask */
	memset (&desc->bitmap [0], 0, temp);
	memset (&desc->bitmap [temp], 0xff, temp);

	temp = 0x0008;			/* per-port overcurrent reporting */
	if (HCS_PPC (ehci->hcs_params))
		temp |= 0x0001;		/* per-port power control */
	if (HCS_INDICATOR (ehci->hcs_params))
		temp |= 0x0080;		/* per-port indicators (LEDs) */
	desc->wHubCharacteristics = cpu_to_le16 (temp);
}

/*-------------------------------------------------------------------------*/

static int ehci_hub_control (
	struct usb_hcd	*hcd,
	u16		typeReq,
	u16		wValue,
	u16		wIndex,
	char		*buf,
	u16		wLength
) {
	struct ehci_hcd	*ehci = hcd_to_ehci (hcd);
	int		ports = HCS_N_PORTS (ehci->hcs_params);
	u32 __iomem	*status_reg = &ehci->regs->port_status[
				(wIndex & 0xff) - 1];
	u32		temp, status;
	unsigned long	flags;
	int		retval = 0;
	unsigned	selector;

	/*
	 * FIXME:  support SetPortFeatures USB_PORT_FEAT_INDICATOR.
	 * HCS_INDICATOR may say we can change LEDs to off/amber/green.
	 * (track current state ourselves) ... blink for diagnostics,
	 * power, "this is the one", etc.  EHCI spec supports this.
	 */

	spin_lock_irqsave (&ehci->lock, flags);
	switch (typeReq) {
	case ClearHubFeature:
		switch (wValue) {
		case C_HUB_LOCAL_POWER:
		case C_HUB_OVER_CURRENT:
			/* no hub-wide feature/status flags */
			break;
		default:
			goto error;
		}
		break;
	case ClearPortFeature:
		if (!wIndex || wIndex > ports)
			goto error;
		wIndex--;
		temp = readl (status_reg);
		if (temp & PORT_OWNER)
			break;

		switch (wValue) {
		case USB_PORT_FEAT_ENABLE:
			writel (temp & ~PORT_PE, status_reg);
			break;
		case USB_PORT_FEAT_C_ENABLE:
			writel ((temp & ~PORT_RWC_BITS) | PORT_PEC,
				 status_reg);
			break;
		case USB_PORT_FEAT_SUSPEND:
			if (temp & PORT_RESET)
				goto error;
			if (temp & PORT_SUSPEND) {
				if ((temp & PORT_PE) == 0)
					goto error;
				/* resume signaling for 20 msec */
				temp &= ~(PORT_RWC_BITS | PORT_WAKE_BITS);
				writel(temp | PORT_RESUME,
						status_reg);
				ehci->reset_done [wIndex] = jiffies
						+ msecs_to_jiffies (20);
			}
			break;
		case USB_PORT_FEAT_C_SUSPEND:
			clear_bit(wIndex, &ehci->port_c_suspend);
			break;
		case USB_PORT_FEAT_POWER:
			if (HCS_PPC (ehci->hcs_params))
				writel (temp & ~(PORT_RWC_BITS | PORT_POWER),
					status_reg);
			break;
		case USB_PORT_FEAT_C_CONNECTION:
			writel ((temp & ~PORT_RWC_BITS) | PORT_CSC,
				status_reg);
			break;
		case USB_PORT_FEAT_C_OVER_CURRENT:
			writel ((temp & ~PORT_RWC_BITS) | PORT_OCC,
				status_reg);
			break;
		case USB_PORT_FEAT_C_RESET:
			/* GetPortStatus clears reset */
			break;
		default:
			goto error;
		}
		readl (&ehci->regs->command);	/* unblock posted write */
		break;
	case GetHubDescriptor:
		ehci_hub_descriptor (ehci, (struct usb_hub_descriptor *)
			buf);
		break;
	case GetHubStatus:
		/* no hub-wide feature/status flags */
		memset (buf, 0, 4);
		//cpu_to_le32s ((u32 *) buf);
		break;
	case GetPortStatus:
		if (!wIndex || wIndex > ports)
			goto error;
		wIndex--;
		status = 0;
		temp = readl (status_reg);

		// wPortChange bits
		if (temp & PORT_CSC)
			status |= 1 << USB_PORT_FEAT_C_CONNECTION;
		if (temp & PORT_PEC)
			status |= 1 << USB_PORT_FEAT_C_ENABLE;
		// USB_PORT_FEAT_C_SUSPEND
		if ((temp & PORT_OCC) && !ignore_oc) {
			status |= 1 << USB_PORT_FEAT_C_OVER_CURRENT;

			/*
			 * Hubs should disable port power on over-current.
			 * However, not all EHCI implementations do this
			 * automatically, even if they _do_ support per-port
			 * power switching; they're allowed to just limit the
			 * current.  khubd will turn the power back on.
			 */
			if (HCS_PPC (ehci->hcs_params)){
				writel(temp & ~(PORT_RWC_BITS | PORT_POWER),
					status_reg);
			}
		}

		/* whoever resumes must GetPortStatus to complete it!! */
		if (temp & PORT_RESUME) {

			/* Remote Wakeup received? */
			if (!ehci->reset_done[wIndex]) {
				/* resume signaling for 20 msec */
				ehci->reset_done[wIndex] = jiffies
						+ msecs_to_jiffies(20);
				/* check the port again */
				mod_timer(&ehci->hcd.rh_timer,
						ehci->reset_done[wIndex]);
			}

			/* resume completed? */
			else if (time_after_eq(jiffies,
					ehci->reset_done[wIndex])) {
				set_bit(wIndex, &ehci->port_c_suspend);
				ehci->reset_done[wIndex] = 0;

				/* stop resume signaling */
				temp = readl(status_reg);
				writel(temp & ~(PORT_RWC_BITS | PORT_RESUME),
					status_reg);
				retval = handshake(status_reg,
					   PORT_RESUME, 0, 2000 /* 2msec */);
				if (retval != 0) {
					ehci_err(ehci,
						"port %d resume error %d\n",
						wIndex + 1, retval);
					goto error;
				}
				temp &= ~(PORT_SUSPEND|PORT_RESUME|(3<<10));
			}
		}

		/* whoever resets must GetPortStatus to complete it!! */
		if ((temp & PORT_RESET)
				&& time_after (jiffies,
					ehci->reset_done [wIndex])) {
			status |= 1 << USB_PORT_FEAT_C_RESET;
			ehci->reset_done [wIndex] = 0;

			/* force reset to complete */
			writel (temp & ~PORT_RESET,
					status_reg);
			/* REVISIT:  some hardware needs 550+ usec to clear
			 * this bit; seems too long to spin routinely...
			 */
			retval = handshake(status_reg,
					PORT_RESET, 0, 750);
			if (retval != 0) {
				ehci_err (ehci, "port %d reset error %d\n",
					wIndex + 1, retval);
				goto error;
			}

			/* see what we found out */
			temp = check_reset_complete (ehci, wIndex, readl(status_reg));
		}

		/* transfer dedicated ports to the companion hc */
		if ((temp & PORT_CONNECT) &&
				test_bit(wIndex, &ehci->companion_ports)) {
			temp &= ~PORT_RWC_BITS;
			temp |= PORT_OWNER;
			writel(temp, status_reg);
			ehci_dbg(ehci, "port %d --> companion\n", wIndex + 1);
			temp = readl(status_reg);
		}

		// don't show wPortStatus if it's owned by a companion hc
		if (!(temp & PORT_OWNER)) {
			if (temp & PORT_CONNECT) {
				status |= 1 << USB_PORT_FEAT_CONNECTION;
			// status may be from integrated TT
			status |= ehci_port_speed(ehci, temp);
			}
			if (temp & PORT_PE)
				status |= 1 << USB_PORT_FEAT_ENABLE;
		if (temp & (PORT_SUSPEND|PORT_RESUME))
				status |= 1 << USB_PORT_FEAT_SUSPEND;
			if (temp & PORT_OC)
				status |= 1 << USB_PORT_FEAT_OVER_CURRENT;
			if (temp & PORT_RESET)
				status |= 1 << USB_PORT_FEAT_RESET;
			if (temp & PORT_POWER)
				status |= 1 << USB_PORT_FEAT_POWER;
		}
		if (test_bit(wIndex, &ehci->port_c_suspend))
			status |= 1 << USB_PORT_FEAT_C_SUSPEND;


#ifndef	EHCI_VERBOSE_DEBUG
	if (status & ~0xffff)	/* only if wPortChange is interesting */
#endif
		dbg_port (ehci, "GetStatus", wIndex + 1, temp);
		// we "know" this alignment is good, caller used kmalloc()...
		*((u32 *) buf) = cpu_to_le32 (status);
		break;
	case SetHubFeature:
		switch (wValue) {
		case C_HUB_LOCAL_POWER:
		case C_HUB_OVER_CURRENT:
			/* no hub-wide feature/status flags */
			break;
		default:
			goto error;
		}
		break;
	case SetPortFeature:
		selector = wIndex >> 8;
		wIndex &= 0xff;
		if (!wIndex || wIndex > ports)
			goto error;
		wIndex--;
		temp = readl (status_reg);
		if (temp & PORT_OWNER)
			break;

		temp &= ~PORT_RWC_BITS;
		switch (wValue) {
		case USB_PORT_FEAT_SUSPEND:
			if ((temp & PORT_PE) == 0
					|| (temp & PORT_RESET) != 0)
				goto error;
			writel (temp | PORT_SUSPEND, status_reg);
			break;
		case USB_PORT_FEAT_POWER:
			if (HCS_PPC (ehci->hcs_params))
				writel (temp | PORT_POWER,
					status_reg);
			break;
		case USB_PORT_FEAT_RESET:
			if (temp & PORT_RESUME)
				goto error;
			/* line status bits may report this as low speed,
			 * which can be fine if this root hub has a
			 * transaction translator built in.
			 */
			if ((temp & (PORT_PE|PORT_CONNECT)) == PORT_CONNECT
					&& !ehci_is_TDI(ehci)
					&& PORT_USB11 (temp)) {
				ehci_dbg (ehci,
					"port %d low speed --> companion\n",
					wIndex + 1);
				temp |= PORT_OWNER;
			} else {
				ehci_vdbg (ehci, "port %d reset\n", wIndex + 1);
				temp |= PORT_RESET;
				temp &= ~PORT_PE;

				/*
				 * caller must wait, then call GetPortStatus
				 * usb 2.0 spec says 50 ms resets on root
				 */
				ehci->reset_done [wIndex] = jiffies
						+ msecs_to_jiffies (50);
			}
			writel (temp, status_reg);
			break;

		/* For downstream facing ports (these):  one hub port is put
		 * into test mode according to USB2 11.24.2.13, then the hub
		 * must be reset (which for root hub now means rmmod+modprobe,
		 * or else system reboot).  See EHCI 2.3.9 and 4.14 for info
		 * about the EHCI-specific stuff.
		 */
		case USB_PORT_FEAT_TEST:
			if (!selector || selector > 5)
				goto error;
			ehci_quiesce(ehci);
			ehci_halt(ehci);
			temp |= selector << 16;
			writel(temp, status_reg);
			break;

		default:
			goto error;
		}
		readl (&ehci->regs->command);	/* unblock posted writes */
		break;

	default:
error:
		/* "stall" on error */
		retval = -EPIPE;
	}
	spin_unlock_irqrestore (&ehci->lock, flags);
	return retval;
}
