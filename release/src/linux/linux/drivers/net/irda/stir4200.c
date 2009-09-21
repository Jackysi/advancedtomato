/*****************************************************************************
*
* Filename:      stir4200.c
* Version:       0.2
* Description:   IrDA-USB Driver
* Status:        Experimental 
* Author:        Paul Stewart <stewart@parc.com>
*
*	Copyright (C) 2000, Roman Weissgaerber <weissg@vienna.at>
*	Copyright (C) 2001, Dag Brattli <dag@brattli.net>
*	Copyright (C) 2001, Jean Tourrilhes <jt@hpl.hp.com>
*          
*	This program is free software; you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation; either version 2 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program; if not, write to the Free Software
*	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/

/*
* This driver is based on usb-irda.c.  The STIr4200 has bulk in and out
* endpoints just like usr-irda devices, but the data it sends and receives
* is raw; like irtty, it needs to call the wrap and unwrap functions to add
* and remove SOF/BOF and escape characters to/from the frame.  It doesn't 
* have an interrupt endpoint like the IrDA-USB devices.
*/

/*------------------------------------------------------------------*/

#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/rtnetlink.h>
#include <linux/usb.h>
#include <net/irda/irda.h>
#include <net/irda/irlap.h>
#include <net/irda/irda_device.h>
#include <net/irda/wrapper.h>
#include <net/irda/crc.h>

/* crc32.h was introduced at or around 2.4.19.  If it exists, use it. 
 * Otherwise, roll our own.  Delete this when it is included into a 
 * specific kernel
 */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,19)
#include <linux/crc32.h>
#else

#ifndef _LINUX_CRC32_H
#define _LINUX_CRC32_H

static unsigned const ethernet_polynomial = 0x04c11db7U;
static inline __u32 ether_crc(int length, unsigned char *data)
{
        int crc = -1;
        while (--length >= 0) {
                unsigned char current_octet = *data++;
                int bit;
                for (bit = 0; bit < 8; bit++, current_octet >>= 1) {
                        crc = (crc << 1) ^
                                ((crc < 0) ^ (current_octet & 1) ?
                                 ethernet_polynomial : 0);
                }
        }
        return crc;
}
#endif /* _LINUX_CRC32_H */

#endif


#include "stir4200.h"

/*------------------------------------------------------------------*/

static int qos_mtt_bits = 0;
static int rx_sensitivity = 0;
static int tx_power = 0;

/* Master instance for each hardware found */
#define NIRUSB 4		/* Max number of USB-IrDA dongles */
static struct stir_cb stir_instance[NIRUSB];

/* These are the currently known IrDA USB dongles. Add new dongles here */
static struct usb_device_id dongles[] = {
    /* SigmaTel, Inc,  STIr4200 IrDA/USB Bridge */
    { USB_DEVICE(0x066f, 0x4200), .driver_info = 0 },
    { }, /* The end */
};

MODULE_DEVICE_TABLE(usb, dongles);

/*------------------------------------------------------------------*/

static void stir_disconnect(struct usb_device *dev, void *ptr);
static void stir_change_speed(struct stir_cb *self, int speed);
static int stir_hard_xmit(struct sk_buff *skb, struct net_device *dev);
static int stir_open(struct stir_cb *self);
static int stir_close(struct stir_cb *self);
static void stir_setup_receive_timer(struct stir_cb *self);
static void stir_write_bulk_callback(struct urb *urb);
static void stir_change_speed_callback(struct urb *urb);
static void stir_receive(struct urb *urb);
static int stir_net_init(struct net_device *dev);
static int stir_net_open(struct net_device *dev);
static int stir_net_close(struct net_device *dev);
static int stir_net_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
static void stir_net_timeout(struct net_device *dev);
static struct net_device_stats *stir_net_get_stats(struct net_device *dev);


/************************ REGISTER OPERATIONS ************************/

static int stir_write_reg(struct stir_cb *self, unsigned short reg,
		      unsigned char value) {
	struct usb_device *dev = self->usbdev;
	int ret;
	
	if (reg >= 0x10) {
		ERROR("%s(), Ignoring bogus register read request %d\n",
		      __FUNCTION__, reg);
		return 0;
	}
	
	ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			      STIR_REQ_WRITEREG_SINGLE, STIR_REQ_WRITEREG,
			      value, reg, self->ctrl_buf, 1,
			      MSECS_TO_JIFFIES(STIR_CTRL_TIMEOUT));
	
	if (ret != 1) {
		ERROR("%s(), cannot write register %d = 0x%x (%d)\n",
		      __FUNCTION__, reg, value, ret);
		return 0;
	}
	
	return ret;
}

#if 0
static int stir_get_regs(struct stir_cb *self, unsigned char startReg,
		     unsigned char nRegs, unsigned char *data) {
	struct usb_device *dev = self->usbdev;
	unsigned char b = self->ctrl_buf;
	int ret;
	
	if ((startReg + nRegs) > 0x10 || nRegs == 0) {
		ERROR("%s(), Ignoring bogus register read request %d/%d\n",
		      __FUNCTION__, startReg, nRegs);
		return 0;
	}
	
	ret = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			      STIR_REQ_READ_REG, STIR_REQ_READ,
			      0, startReg, b, nRegs,
			      MSECS_TO_JIFFIES(STIR_CTRL_TIMEOUT));
	
	if (ret != nRegs) {
		ERROR("%s(), cannot read registers %d-%d (%d)\n", __FUNCTION__,
		      startReg, startReg + nRegs, ret);
		return 0;
	}
	
	memcpy(data, b, nRegs);
	
	return ret;
}
#endif

/************************ BUFFER ROUTINES ************************/
static int stir_irda_init_iobuf(iobuff_t *io, int size)
{
    io->head = kmalloc(size, GFP_KERNEL);
    if (io->head != NULL) {
	    io->truesize = size;
	    io->in_frame = FALSE;
	    io->state    = OUTSIDE_FRAME;
	    io->data     = io->head;
    }
    return io->head ? 0 : -ENOMEM;
}

/*********************** FIR WRAPPER ROUTINES ***********************/
/*
 * The STIr4220 use a proprietary wrapping of data when communicating
 * with the driver at FIR. It's very similar to the SIR wrapping,
 * the only difference is the start/stop bytes and CRC.
 * Those minor differences force us to use our own wrappers.
 * Jean II
 */

/*------------------------------------------------------------------*/
/*
 * Prepare a SIR IrDA frame for transmission to the USB dongle.  We
 * use the standard async_wrap_skb() code used with most of the
 * serial-based IrDA modules, and prepend the header as required by
 * the SigmaTel datasheet: a two byte 0x55 0xAA sequence and two
 * little-endian length bytes.
 */
static inline int stir_wrap_sir_skb(struct sk_buff *skb, iobuff_t *buf) {
	__u8 *ptr;
	__u16 wraplen;

	ptr = buf->data = buf->head;
	*ptr++ = 0x55;
	*ptr++ = 0xAA;
	
	wraplen = async_wrap_skb(skb, buf->data + STIR_IRDA_HEADER,
				 buf->truesize - STIR_IRDA_HEADER);
	
	*ptr++ = wraplen & 0xff;
	*ptr++ = (wraplen >> 8) & 0xff;

	buf->len = wraplen + STIR_IRDA_HEADER;

	return buf->len;
}

/*------------------------------------------------------------------*/
/*
 * Prepare a FIR IrDA frame for transmission to the USB dongle.  The
 * FIR transmit frame is documented in the datasheet.  It consists of
 * a two byte 0x55 0xAA sequence, two little-endian length bytes, a
 * sequence of exactly 16 XBOF bytes of 0x7E, two BOF bytes of 0x7E,
 * then the data escaped as follows:
 *
 *    0x7D -> 0x7D 0x5D
 *    0x7E -> 0x7D 0x5E
 *    0x7F -> 0x7D 0x5F
 *
 * Then, 4 bytes of little endian (stuffed) FCS follow, then two 
 * trailing EOF bytes of 0x7E.
 */
static inline int stir_stuff_fir_byte(__u8 *buf, __u8 c) {
	if (c == 0x7d || c == 0x7e || c == 0x7f) {
		*buf++ = 0x7d;
		*buf = c ^ 0x20;
		return 2;
	}
	*buf = c;
	return 1;
}

static inline int stir_wrap_fir_skb(struct sk_buff *skb, iobuff_t *buf) {
	__u8 *ptr;
	__u8 *hdr;
	__u32 fcs = ~(crc32_le(~0, skb->data, skb->len));
	__u16 wraplen;
	int i;

	/* Size of header (2) + size (2) + preamble (16) + bofs (2) + 
	 * stuffed fcs bytes (8) + eofs (2) 
	 */
	if (buf->truesize < 32) return 0;

	/* Header */
	ptr = buf->data = buf->head;
	*ptr++ = 0x55;
	*ptr++ = 0xAA;
	
	hdr = ptr;
	ptr += 2;

	/* Preamble */
	for (i = 0; i < 16; i++)
		*ptr++ = 0x7f;

	/* BOFs */
	*ptr++ = 0x7e;
	*ptr++ = 0x7e;

	/* Address / Control / Information */
	for (i = 0; i < skb->len; i++) {
		/* Must be able to fit possibly stuffed byte, 4
		 * possibly stuffed FCS bytes, and 2 trailing EOF
		 * bytes.
		 */
		if (ptr - buf->head >= buf->truesize - 12) return 0;
		ptr += stir_stuff_fir_byte(ptr, skb->data[i]);
	}
	
	/* FCS */
	ptr += stir_stuff_fir_byte(ptr, fcs & 0xff);
	ptr += stir_stuff_fir_byte(ptr, (fcs >> 8) & 0xff);
	ptr += stir_stuff_fir_byte(ptr, (fcs >> 16) & 0xff);
	ptr += stir_stuff_fir_byte(ptr, (fcs >> 24) & 0xff);

	/* EOF */
	*ptr++ = 0x7e;
	*ptr++ = 0x7e;

	/* Total lenght, minus the header */
	wraplen = ptr - buf->head - STIR_IRDA_HEADER;

	/* Fill in header length */
	*hdr++ = wraplen & 0xff;
	*hdr++ = (wraplen >> 8) & 0xff;

	buf->len = wraplen + STIR_IRDA_HEADER;

	return buf->len;
}

/*
 * Function async_bump (buf, len, stats)
 *
 *    Got a frame, make a copy of it, and pass it up the stack! We can try
 *    to inline it since it's only called from state_inside_frame
 */
static inline void
stir_fir_bump(struct net_device *dev,
	      struct net_device_stats *stats,
	      iobuff_t *rx_buff)
{
	struct sk_buff *newskb;
	struct sk_buff *dataskb;
	int		docopy;

	/* Check if we need to copy the data to a new skb or not.
	 * If the driver doesn't use ZeroCopy Rx, we have to do it.
	 * With ZeroCopy Rx, the rx_buff already point to a valid
	 * skb. But, if the frame is small, it is more efficient to
	 * copy it to save memory (copy will be fast anyway - that's
	 * called Rx-copy-break). Jean II */
	docopy = ((rx_buff->skb == NULL) ||
		  (rx_buff->len < IRDA_RX_COPY_THRESHOLD));

	/* Allocate a new skb */
	newskb = dev_alloc_skb(docopy ? rx_buff->len + 1 : rx_buff->truesize);
	if (!newskb)  {
		stats->rx_dropped++;
		/* We could deliver the current skb if doing ZeroCopy Rx,
		 * but this would stall the Rx path. Better drop the
		 * packet... Jean II */
		return;
	}

	/* Align IP header to 20 bytes (i.e. increase skb->data)
	 * Note this is only useful with IrLAN, as PPP has a variable
	 * header size (2 or 1 bytes) - Jean II */
	skb_reserve(newskb, 1);

	if(docopy) {
		/* Copy data without CRC (lenght already checked) */
		memcpy(newskb->data, rx_buff->data, rx_buff->len - 4);
		/* Deliver this skb */
		dataskb = newskb;
	} else {
		/* We are using ZeroCopy. Deliver old skb */
		dataskb = rx_buff->skb;
		/* And hook the new skb to the rx_buff */
		rx_buff->skb = newskb;
		rx_buff->head = newskb->data;	/* NOT newskb->head */
	}

	/* Set proper length on skb (without CRC) */
	skb_put(dataskb, rx_buff->len - 4);

	/* Feed it to IrLAP layer */
	dataskb->dev = dev;
	dataskb->mac.raw  = dataskb->data;
	dataskb->protocol = htons(ETH_P_IRDA);

	netif_rx(dataskb);

	stats->rx_packets++;
	stats->rx_bytes += rx_buff->len - 4;

	/* Clean up rx_buff (redundant with async_unwrap_bof() ???) */
	rx_buff->data = rx_buff->head;
	rx_buff->len = 0;
}

/*
 * Function async_unwrap_bof(dev, byte)
 *
 *    Handle Beggining Of Frame character received within a frame
 *
 */
static inline void
stir_unwrap_fir_bof(struct stir_cb *self,
		    iobuff_t *rx_buff, __u8 byte)
{
	/* Not supposed to happen... - Jean II */
	IRDA_DEBUG(0, "%s(), Received STIR-XBOF !\n", __FUNCTION__);
}

/*
 * Function async_unwrap_eof(dev, byte)
 *
 *    Handle End Of Frame character received within a frame
 *
 */
static inline void
stir_unwrap_fir_eof(struct stir_cb *self,
		    iobuff_t *rx_buff, __u8 byte)
{
	__u32 fcs_frame;
	__u32 fcs_calc;

	switch(rx_buff->state) {

	case BEGIN_FRAME:
	case LINK_ESCAPE:
	case INSIDE_FRAME:
	default:
		/* We receive multiple BOF/EOF */
		if(rx_buff->len == 0)
			break;

		/* Note : in the case of BEGIN_FRAME and LINK_ESCAPE,
		 * the fcs will most likely not match and generate an
		 * error, as expected - Jean II */
		rx_buff->state = OUTSIDE_FRAME;
		rx_buff->in_frame = FALSE;

		/* We can't inline the CRC calculation, as we have
		 * nowhere to store it in rx_buff... Jean II */
		if(rx_buff->len > 4) {
			fcs_calc = ~(crc32_le(~0, rx_buff->data,
					      rx_buff->len - 4));
			fcs_frame = (rx_buff->data[rx_buff->len - 4] |
				     (rx_buff->data[rx_buff->len - 3] << 8) |
				     (rx_buff->data[rx_buff->len - 2] << 16) |
				     (rx_buff->data[rx_buff->len - 1] << 24));
			IRDA_DEBUG(0, "%s(), crc = 0x%X, crc  = 0x%X, len = %d\n",
				   __FUNCTION__, fcs_calc, fcs_frame, rx_buff->len);
		} else {
			fcs_calc = 0;
			fcs_frame = 1;
		}

		/* You may see an abnormal number of CRC failures around
		 * there... This is due to a nice hardware bug in
		 * the STIr4200. Quite often, the hardware will
		 * pass us two or more packets in a URB without any
		 * BOF/EOF in between. The CRC is the one of the last
		 * packet, but we will treat those packets as a single
		 * packet, so it won't match.
		 * Of course, because there is no separators, there is
		 * no way we can correct this bug.
		 * Jean II */

		/* Test FCS and signal success if the frame is good */
		if (fcs_calc == fcs_frame) {
			/* Deliver frame */
			stir_fir_bump(self->netdev, &self->stats, rx_buff);
		} else {
			/* Wrong CRC, discard frame!  */
			irda_device_set_media_busy(self->netdev, TRUE);

			IRDA_DEBUG(0, "%s(), crc error\n",
				   __FUNCTION__);
			self->stats.rx_errors++;
			self->stats.rx_crc_errors++;
		}

		/* Fall through : We may receive only a single BOF/EOF */
	case OUTSIDE_FRAME:
		/* BOF == EOF, so beware... */

		/* Now receiving frame */
		rx_buff->state = BEGIN_FRAME;
		rx_buff->in_frame = TRUE;

		/* Time to initialize receive buffer */
		rx_buff->data = rx_buff->head;
		rx_buff->len = 0;
		rx_buff->fcs = INIT_FCS;
		break;
	}
}

/*
 * Function async_unwrap_ce(dev, byte)
 *
 *    Handle Character Escape character received within a frame
 *
 */
static inline void
stir_unwrap_fir_ce(struct stir_cb *self,
		   iobuff_t *rx_buff, __u8 byte)
{
	switch(rx_buff->state) {
	case OUTSIDE_FRAME:
		/* Activate carrier sense */
		irda_device_set_media_busy(self->netdev, TRUE);
		break;

	case LINK_ESCAPE:
		WARNING("%s: state not defined\n", __FUNCTION__);
		break;

	case BEGIN_FRAME:
	case INSIDE_FRAME:
	default:
		/* Stuffed byte comming */
		rx_buff->state = LINK_ESCAPE;
		break;
	}
}

/*
 * Function async_unwrap_other(dev, byte)
 *
 *    Handle other characters received within a frame
 *
 */
static inline void
stir_unwrap_fir_other(struct stir_cb *self,
		      iobuff_t *rx_buff, __u8 byte)
{
	switch(rx_buff->state) {
		/* This is on the critical path, case are ordered by
		 * probability (most frequent first) - Jean II */
	case INSIDE_FRAME:
		/* Must be the next byte of the frame */
		if (rx_buff->len < rx_buff->truesize)  {
			rx_buff->data[rx_buff->len++] = byte;
		} else {
			IRDA_DEBUG(1, "%s(), Rx buffer overflow, aborting\n",
				   __FUNCTION__);
			rx_buff->state = OUTSIDE_FRAME;
		}
		break;

	case LINK_ESCAPE:
		/*
		 *  Stuffed char, complement bit 5 of byte
		 *  following CE, IrLAP p.114
		 */
		byte ^= IRDA_TRANS;
		if (rx_buff->len < rx_buff->truesize)  {
			rx_buff->data[rx_buff->len++] = byte;
			rx_buff->state = INSIDE_FRAME;
		} else {
			IRDA_DEBUG(1, "%s(), Rx buffer overflow, aborting\n",
				   __FUNCTION__);
			rx_buff->state = OUTSIDE_FRAME;
		}
		break;

	case OUTSIDE_FRAME:
		/* Activate carrier sense */
		if(byte != XBOF)
			irda_device_set_media_busy(self->netdev, TRUE);
		break;

	case BEGIN_FRAME:
	default:
		rx_buff->data[rx_buff->len++] = byte;
		rx_buff->state = INSIDE_FRAME;
		break;
	}
}

/*
 * Function stir_async_unwrap_fir_chars (dev, rx_buff, byte)
 *
 *    Parse and de-stuff frame received from the IrDA-port
 *
 */
void stir_async_fir_chars(struct stir_cb *self,
			  iobuff_t *rx_buff,
			  __u8 *bytes, int len)
{
	__u8	byte;
	int	i;

	/* Having the loop here is more efficient - Jean II */
	for (i = 0; i < len; i++) {
		byte = bytes[i];
		switch(byte) {
		case CE:
			stir_unwrap_fir_ce(self, rx_buff, byte);
			break;
		case STIR_XBOF:
			stir_unwrap_fir_bof(self, rx_buff, byte);
			break;
		case STIR_EOF:
			stir_unwrap_fir_eof(self, rx_buff, byte);
			break;
		default:
			stir_unwrap_fir_other(self, rx_buff, byte);
			break;
		}
	}
}

/************************ TRANSMIT ROUTINES ************************/
/*
 * Receive packets from the IrDA stack and send them on the USB pipe.
 * Handle speed change, timeout and lot's of uglyness...
 */


/*------------------------------------------------------------------*/
/*
 * This function returns the bytes that should be programmed into the
 * MODE and PDCLK registers, respectively, in order to get a desired
 * transmit and receive bitrate. 
 */
static void stir_get_speed_bytes(int speed, __u8 *bytes)
{
	switch (speed) {
	case 2400:
		bytes[0] = STIR_MODE_SIR | STIR_MODE_NRESET | STIR_MODE_2400;
		bytes[1] = STIR_PDCLK_2400;
		break;
	default:
	case 9600:
		bytes[0] = STIR_MODE_SIR | STIR_MODE_NRESET;
		bytes[1] = STIR_PDCLK_9600;
		break;
	case 19200:
		bytes[0] = STIR_MODE_SIR | STIR_MODE_NRESET;
		bytes[1] = STIR_PDCLK_19200;
		break;
	case 38400:
		bytes[0] = STIR_MODE_SIR | STIR_MODE_NRESET;
		bytes[1] = STIR_PDCLK_38400;
		break;
	case 57600:
		bytes[0] = STIR_MODE_SIR | STIR_MODE_NRESET;
		bytes[1] = STIR_PDCLK_57600;
		break;
	case 115200:
		bytes[0] = STIR_MODE_SIR | STIR_MODE_NRESET;
		bytes[1] = STIR_PDCLK_115200;
		break;
	case 4000000:
		bytes[0] = STIR_MODE_FIR | STIR_MODE_NRESET | STIR_MODE_FFRSTEN;
		bytes[1] = STIR_PDCLK_4000000;
		break;
	}
}

/*------------------------------------------------------------------*/
/*
 * Send a command to change the speed of the dongle
 * Need to be called with spinlock on.
 */
static void stir_change_speed(struct stir_cb *self, int speed)
{
	__u8 speed_bytes[2];
	IRDA_DEBUG(2, "%s(), speed=%d\n", __FUNCTION__, speed);

	if (speed == -1) return;

	IRDA_DEBUG(2, "%s(), changing speed to %d\n", __FUNCTION__,
		   speed);
	self->speed = speed;

	stir_get_speed_bytes(speed, speed_bytes);

	stir_write_reg(self, STIR_REG_MODE, speed_bytes[0]);
	stir_write_reg(self, STIR_REG_PDCLK, speed_bytes[1]);
}

/*------------------------------------------------------------------*/
/*
 * Send a frame using the bulk endpoint of the STIr4200.  We wrap the
 * skb in the framing required by the skb and speed we're
 * transmitting.
 */
static int stir_tx_submit(struct stir_cb *self, struct sk_buff *skb) {
	struct urb *purb = self->tx_urb;
	int res, mtt, txlen;

	if (purb->status != 0) {
		WARNING("%s(), URB still in use!\n", __FUNCTION__);
		return 1;
	}

	if (self->speed == 4000000) {
		txlen = stir_wrap_fir_skb(skb, &self->tx_buff);
	} else {
		txlen = stir_wrap_sir_skb(skb, &self->tx_buff);
	}
	
	dev_kfree_skb(skb);

        usb_fill_bulk_urb(purb, self->usbdev, 
			  usb_sndbulkpipe(self->usbdev, self->bulk_out_ep),
			  self->tx_buff.data, txlen,
			  stir_write_bulk_callback, self);

	purb->transfer_buffer_length = txlen;
	/* Note : unlink *must* be Asynchronous because of the code in 
	 * stir_net_timeout() -> call in irq - Jean II */
	purb->transfer_flags = USB_ASYNC_UNLINK;

	/* Timeout need to be shorter than NET watchdog timer */
	purb->timeout = MSECS_TO_JIFFIES(STIR_BULK_TIMEOUT);
	purb->context = self;

	/* Generate min turn time. FIXME: can we do better than this? */
	/* Trying to a turnaround time at this level is trying to measure
	 * processor clock cycle with a wrist-watch, approximate at best...
	 *
	 * What we know is the last time we received a frame over USB.
	 * Due to latency over USB that depend on the USB load, we don't
	 * know when this frame was received over IrDA (a few ms before ?)
	 * Then, same story for our outgoing frame...
	 *
	 * Jean II */

	mtt = irda_get_mtt(skb);
	if (mtt) {
		int diff;
		int sdiff = 0;
		do_gettimeofday(&self->now);
		diff = self->now.tv_usec - self->stamp.tv_usec;
		/* Factor in USB delays -> Get rid of udelay() that
		 * would be lost in the noise - Jean II */
		diff += STIR_MIN_RTT;
		if (diff < 0) {
			diff += 1000000;
			sdiff = -1;
		}
		
		/* Check if the mtt is larger than the time we have
		 * already used by all the protocol processing
		 */
		if (self->now.tv_sec + sdiff == self->stamp.tv_usec && 
		    mtt > diff) {
			mtt -= diff;
			if (mtt > 1000)
				mdelay(mtt/1000);
			else
				udelay(mtt);
		}
	}
	
	/* Ask USB to send the packet */
	if ((res = usb_submit_urb(purb))) {
		WARNING("%s(), failed Tx URB\n", __FUNCTION__);
		self->stats.tx_errors++;
		/* Let USB recover : We will catch that in the watchdog */
		/*netif_start_queue(netdev);*/
	} else {
		/* Increment packet stats */
		self->stats.tx_packets++;
                self->stats.tx_bytes += skb->len;
	}
	
	return 0;
}

/*------------------------------------------------------------------*/
/*
 * This function changes the transmission/receive speed
 * asynchronously.  The STIr4200 documentation mentions a "Write
 * Multiple Registers" call which would work quite nicely in this
 * situation for updating both the MODE and PDCLK registers which
 * happen to be adjacent to each other.  Unfortunately, I haven't been
 * able to get this call to work for me.  Instead, I do a two URB
 * requests in a row, setting each register.  The second URB request
 * is started when the first completes in
 * stir_change_speed_callback().
 */
static int stir_change_speed_async(struct stir_cb *self, int speed)
{
	struct urb *purb;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20)
	struct usb_ctrlrequest *dr;
#else
        devrequest *dr;
#define devrequest struct usb_ctrlrequest
#define bRequestType requesttype
#define bRequest request
#define wValue value
#define wIndex index
#define wLength length
#endif
	__u8 *sbuf;
	int status;

	if (speed == -1) return 0;

	sbuf = (__u8 *) self->ctrl_buf;
	stir_get_speed_bytes(speed, sbuf);

	/* Fill first URB */
	purb = self->speed_urb[0];

        if (purb->status != 0) {
                WARNING("%s(), URB still in use!\n", __FUNCTION__);
                return -EINVAL;
        }

	dr = (void *) (sbuf + 2);
	dr->bRequestType = STIR_REQ_WRITEREG;
	dr->bRequest = STIR_REQ_WRITEREG_SINGLE;
	dr->wValue = cpu_to_le16(sbuf[0]);
	dr->wIndex = cpu_to_le16(STIR_REG_MODE);
	dr->wLength = cpu_to_le16(0);

	usb_fill_control_urb(purb, self->usbdev,
			     usb_sndctrlpipe(self->usbdev, 0), 
			     (unsigned char *) dr, sbuf, 0, 
			     stir_change_speed_callback, self);
	purb->transfer_buffer_length = 0;

	/* Fill second URB */
	purb = self->speed_urb[1];

        if (purb->status != 0) {
                WARNING("%s(), URB still in use!\n", __FUNCTION__);
                return -EINVAL;
        }

	dr = (dr + 1);
	dr->bRequestType = STIR_REQ_WRITEREG;
	dr->bRequest = STIR_REQ_WRITEREG_SINGLE;
	dr->wValue = cpu_to_le16(sbuf[1]);
	dr->wIndex = cpu_to_le16(STIR_REG_PDCLK);
	dr->wLength = cpu_to_le16(0);

	usb_fill_control_urb(purb, self->usbdev,
			     usb_sndctrlpipe(self->usbdev, 0), 
			     (unsigned char *) dr, sbuf, 0, 
			     stir_change_speed_callback, self);
	purb->transfer_buffer_length = 0;

	/* Submit first URB */
	status = usb_submit_urb(self->speed_urb[0]);
	self->speed = speed;

	return status;
}

/*------------------------------------------------------------------*/
/*
 * This callback is fired when a speed-change URB is complete.
 */
static void stir_change_speed_callback(struct urb *purb)
{
	struct stir_cb *self = purb->context;
	
	IRDA_DEBUG(2, "%s()\n", __FUNCTION__);

	/* We should always have a context */
	ASSERT(self != NULL, return;);

	if ((!self->netopen) || (!self->present)) {
		IRDA_DEBUG(0, "%s(), Network is gone...\n", __FUNCTION__);
		return;
	}

	IRDA_DEBUG(2, "%s(): Change-speed to %d: phase %d urb status %d\n",
		__FUNCTION__, self->speed, purb == self->speed_urb[1],
		purb->status);

	if (purb->status != 0) {
		/* Wait for net timeout to sort this out */
		return;
	}

	if (purb == self->speed_urb[0]) {
		/* Phase one complete.  Now perform phase 2 */
		usb_submit_urb(self->speed_urb[1]);
		return;
	}

	/* We're ready for more packets */
	netif_wake_queue(self->netdev);
}

/*------------------------------------------------------------------*/
/*
 * This callback is fired when we expect a frame to have completed
 * trasmission from the FIFO.  We are now able to perform a
 * speed-change without affecting untransmitted data.  The callbacks
 * from stir_change_speed_async() will take care of calling
 * netif_wake_queue() when this process has completed.
 */
static void stir_update_speed_callback(unsigned long data)
{
	struct stir_cb *self = (struct stir_cb *) data;

	/* Find ourselves */
	ASSERT(self != NULL, return;);

	self->speed_timer_active = 0;

	/* If the network is closed or the device gone, stop everything */
	if ((!self->netopen) || (!self->present)) {
		IRDA_DEBUG(0, "%s(), Network is gone!\n", __FUNCTION__);
		/* Don't re-submit the URB : will stall the Rx path */
		return;
	}
	
	
	IRDA_DEBUG(2, "%s(): Changing to speed %d now\n", __FUNCTION__,
		   self->new_speed);

	if (stir_change_speed_async(self, self->new_speed) == 0)
		self->new_speed = -1;
}

/*------------------------------------------------------------------*/
/*
 * Send an IrDA frame to the USB dongle (for transmission).  If a
 * speed change is required, handle this first.
 */
static int stir_hard_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct stir_cb *self = netdev->priv;
	unsigned long flags;
	s32 speed;
	int     err = 1;        /* Failed */

	netif_stop_queue(netdev);

	/* Protect us from USB callbacks, net watchdog and else. */
	spin_lock_irqsave(&self->lock, flags);

	/* Check if the device is still there.
	 * We need to check self->present under the spinlock because
	 * of stir_disconnect() is synchronous - Jean II */
	if (!self->present) {
		IRDA_DEBUG(0, "%s(), Device is gone...\n", __FUNCTION__);
		goto drop;
	}

	IRDA_DEBUG(2, "%s(): Xmit: len: %d, speed: %d (%d)\n",
		   __FUNCTION__, skb->len, irda_get_next_speed(skb),
		   self->speed);

        /* Check if we need to change the speed */
	speed = irda_get_next_speed(skb);
	if ((speed != self->speed) && (speed != -1)) {
		if (skb->len == 0) {
			/* Set the desired speed */
			if (stir_change_speed_async(self, speed)) {
				ERROR("%s(), change_speed() returned error\n",
				      __FUNCTION__);
				goto drop;
			}
			
			/* We let the change_speed callback drive the
			 * rest of the sending, and eventual call of
			 * netif_wake_queue */
			netdev->trans_start = jiffies;
			err = 0;        /* No error */
			goto drop;
		} else {
			/* Wait until after the frame is transmitted to 
			 * change speeds */
			self->new_speed = speed;
		}
	}

	if (stir_tx_submit(self, skb) == 0) {
		netdev->trans_start = jiffies;
		spin_unlock_irqrestore(&self->lock, flags);
		return 0;
	}

drop:
	/* Drop silently the skb and exit */
	dev_kfree_skb(skb);
	spin_unlock_irqrestore(&self->lock, flags);
	return err;
}

/*------------------------------------------------------------------*/
/*
 * Note : this function will be called only for tx_urb...
 */
static void stir_write_bulk_callback(struct urb *purb)
{
	unsigned long flags;
	struct stir_cb *self = purb->context;
	
	IRDA_DEBUG(2, "%s()\n", __FUNCTION__);

	/* We should always have a context */
	ASSERT(self != NULL, return;);

	/* Check for timeout and other USB nasties */
	if(purb->status != 0) {
		/* I get a lot of -ECONNABORTED = -103 here - Jean II */
		IRDA_DEBUG(0, "%s(), URB complete status %d, transfer_flags 0x%04X\n", __FUNCTION__, purb->status, purb->transfer_flags);

		/* Don't do anything here, that might confuse the USB layer,
		 * and we could go in recursion and blow the kernel stack...
		 * Instead, we will wait for stir_net_timeout(), the
		 * network layer watchdog, to fix the situation.
		 * Jean II */
		/* A reset of the dongle might be welcomed here - Jean II */
		return;
	}

	/* urb is now available */
	//purb->status = 0; -> tested above

	/* Make sure we read self->present properly */
	spin_lock_irqsave(&self->lock, flags);

	/* If the network is closed, stop everything */
	if ((!self->netopen) || (!self->present)) {
		IRDA_DEBUG(0, "%s(), Network is gone...\n", __FUNCTION__);
		spin_unlock_irqrestore(&self->lock, flags);
		return;
	}

	if (self->new_speed != -1) {
		/* We'll assume in this case that the FIFO was empty
		 * at the time of transmission, and the current URB
		 * has just now begun transmission.  We need to wait
		 * until it has completed sending before we change
		 * speeds.  I use STIR_MIN_SPEED_DELAY as a fudge
		 * factor to make sure we've delayed long enough for a
		 * UA response to an SNRM to get across in my setup.
		 * It is possible that this value may need to be more
		 * conservative (larger).
		 */
		struct timer_list *st = &self->speed_timer;
		int wait_msecs;

		if (self->speed_timer_active) {
			WARNING("%s(), Speed timer found already active\n",
				__FUNCTION__);
			del_timer(st);
		}

		wait_msecs = purb->actual_length * 8 * 1000 / self->speed;
		if (wait_msecs < STIR_MIN_SPEED_DELAY)
			wait_msecs = STIR_MIN_SPEED_DELAY;

		init_timer(st);
		st->function = stir_update_speed_callback;
		st->data = (unsigned long) self;
		st->expires = jiffies + MSECS_TO_JIFFIES(wait_msecs);
		add_timer(st);

		self->speed_timer_active = 1;
	} else {
		/* Guess what, there is another bug ! If we send multiple
		 * packets per window, the first packet get corrupted.
		 * Probably we need an ugly timeout around here, or
		 * maybe check the hardware status or something.
		 * I just can't believe this !
		 * Jean II */

		/* Allow the stack to send more packets */
		netif_wake_queue(self->netdev);
	}

	spin_unlock_irqrestore(&self->lock, flags);
}

/*------------------------------------------------------------------*/
/*
 * Helper function for watchdog timer.  Check out the status of an
 * URB, and respond accordingly, updating device statistics.
 */
static inline int stir_check_urb(struct stir_cb *self, struct urb *purb,
			     char *urb_name) {
	if (purb->status != 0) {
		IRDA_DEBUG(0, "%s: %s timed out, urb->status=%d, "
			   "urb->transfer_flags=0x%04X\n", 
			   self->netdev->name, urb_name, purb->status, 
			   purb->transfer_flags);

		/* Increase error count */
		self->stats.tx_errors++;

		switch (purb->status) {
		case -EINPROGRESS:		/* -115 */
			usb_unlink_urb(purb);
			/* Note : above will  *NOT* call netif_wake_queue()
			 * in completion handler, because purb->status will
			 * be -ENOENT. We will fix that at the next watchdog,
			 * leaving more time to USB to recover...
			 * Also, we are in interrupt, so we need to have
			 * USB_ASYNC_UNLINK to work properly...
			 * Jean II */
			break;
		case -ECONNABORTED:		/* -103 */
		case -ECONNRESET:		/* -104 */
		case -ETIMEDOUT:		/* -110 */
		case -ENOENT:			/* -2 (urb unlinked by us)  */
		default:			/* ??? - Play safe */
			purb->status = 0;
			netif_wake_queue(self->netdev);
			break;
		}
		return 1;
	}
	return 0;
}

/*------------------------------------------------------------------*/
/*
 * Watchdog timer from the network layer.
 * After a predetermined timeout, if we don't give confirmation that
 * the packet has been sent (i.e. no call to netif_wake_queue()),
 * the network layer will call this function.
 * Note that URB that we submit have also a timeout. When the URB timeout
 * expire, the normal URB callback is called (write_bulk_callback()).
 */
static void stir_net_timeout(struct net_device *netdev)
{
	unsigned long flags;
	struct stir_cb *self = netdev->priv;
	int	done = 0;	/* If we have made any progress */

	IRDA_DEBUG(0, "%s(), Network layer thinks we timed out!\n", __FUNCTION__);

	/* Protect us from USB callbacks, net Tx and else. */
	spin_lock_irqsave(&self->lock, flags);

	if (!self->present) {
		WARNING("%s(), device not present!\n", __FUNCTION__);
		netif_stop_queue(netdev);
		spin_unlock_irqrestore(&self->lock, flags);
		return;
	}


	done += stir_check_urb(self, self->tx_urb, "Transmit");
	done += stir_check_urb(self, self->speed_urb[0], "Speed 0");
	done += stir_check_urb(self, self->speed_urb[1], "Speed 1");

	spin_unlock_irqrestore(&self->lock, flags);

	/* XXX Deal with speed urb in a similar manner */

	/* Maybe we need a reset */
	/* Note : Some drivers seem to use a usb_set_interface() when they
	 * need to reset the hardware. Hum...
	 */

	/* if(done == 0) */
}

/************************* RECEIVE ROUTINES *************************/
/*
 * Receive packets from the USB layer stack and pass them to the IrDA stack.
 * Try to work around USB failures...
 */

/*
 * Note :
 * Some of you may have noticed that most dongle have an interrupt in pipe
 * that we don't use. Here is the little secret...
 * When we hang a Rx URB on the bulk in pipe, it generates some USB traffic
 * in every USB frame. This is unnecessary overhead.
 * The interrupt in pipe will generate an event every time a packet is
 * received. Reading an interrupt pipe adds minimal overhead, but has some
 * latency (~1ms).
 * If we are connected (speed != 9600), we want to minimise latency, so
 * we just always hang the Rx URB and ignore the interrupt.
 * If we are not connected (speed == 9600), there is usually no Rx traffic,
 * and we want to minimise the USB overhead. In this case we should wait
 * on the interrupt pipe and hang the Rx URB only when an interrupt is
 * received.
 * Jean II
 */

/*------------------------------------------------------------------*/
/*
 * Submit a Rx URB to the USB layer to handle reception of a frame
 * Mostly called by the completion callback of the previous URB.
 *
 * Jean II
 */
static void stir_rx_submit(struct stir_cb *self, struct urb *purb)
{
	int ret;

	IRDA_DEBUG(2, "%s()\n", __FUNCTION__);

	/* Check that we have an urb */
	if (!purb) {
		WARNING("%s(), Bug : purb == NULL\n", __FUNCTION__);
		return;
	}

	/* Reinitialize URB */
	usb_fill_bulk_urb(purb, self->usbdev, 
			  usb_rcvbulkpipe(self->usbdev, self->bulk_in_ep), 
			  self->rxdata, STIR_IRDA_RXFRAME_LEN,
			  stir_receive, self);
	purb->transfer_buffer_length = STIR_IRDA_RXFRAME_LEN;
	purb->status = 0;
	
	ret = usb_submit_urb(purb);
	if (ret) {
		/* If this ever happen, we are in deep s***.
		 * Basically, the Rx path will stop... */
		WARNING("%s(), Failed to submit Rx URB %d\n", __FUNCTION__, ret);
	}
}


/*------------------------------------------------------------------*/
/*
 * Function stir_delayed_rx_submit(unsigned long data)
 *
 *     Called by the kernel timer subsystem to restart receiving after
 *     a delay (because the RX FIFO was empty)
 *
 */
static void stir_delayed_rx_submit(unsigned long data)
{
	struct stir_cb *self = (struct stir_cb *) data;
	int i;

	/* Find ourselves */
	ASSERT(self != NULL, return;);

	self->rx_timer_active = 0;

	/* If the network is closed or the device gone, stop everything */
	if ((!self->netopen) || (!self->present)) {
		IRDA_DEBUG(0, "%s(), Network is gone!\n", __FUNCTION__);
		/* Don't re-submit the URB : will stall the Rx path */
		return;
	}
	
	self->idle_rx_urb = self->rx_urb[STIR_MAX_ACTIVE_RX_URBS];
	self->idle_rx_urb->context = NULL;

	for (i = 0; i < STIR_MAX_ACTIVE_RX_URBS; i++)
		stir_rx_submit(self, self->rx_urb[i]);

}

/*------------------------------------------------------------------*/
/*
 * Setup the receive timer.  This function is called every time a
 * zero-length frame is received from the bulk endpoint.  With the
 * SigmaTel, if the FIFO is empty a zero-length buffer is received
 * immediately.  If we immediately resubmitted the URB, we'd spend a
 * lot of system time setting up and clearing URBs.  Instead, this
 * function sets up a period of moderate attentiveness, followed by a
 * fairly inattentive state where URBs are submitted only often enough
 * so a FIFO overrun is unlikely.  The do_reset parameter allows the
 * transmit code to reset the timer so that a response to a
 * transmitted packet can be received quickly.
 */
static void stir_setup_receive_timer(struct stir_cb *self) {
	struct timer_list *st = &self->submit_rx_timer;

	if (self->rx_timer_active != 0) {
		WARNING("%s(), timer already active!\n", __FUNCTION__);
		return;
	}
		
	init_timer(st);
	st->function = stir_delayed_rx_submit;
	st->data = (unsigned long) self;
	if (self->idle_periods < STIR_IDLE_PERIODS) {
		self->idle_periods++;
		st->expires = jiffies + MSECS_TO_JIFFIES(STIR_IDLE_TIMEOUT);
	} else {
		if (self->speed <= 0 || 
		    self->speed > STIR_FIFO_SIZE * 8) {
			st->expires = jiffies + 2 * HZ;
		} else {
			st->expires = jiffies + 
				STIR_FIFO_SIZE * 8 * HZ / self->speed;
		}
	}
	add_timer(st);
	self->rx_timer_active = 1;
}

/*------------------------------------------------------------------*/
/*
 * Function stir_receive(purb)
 *
 *     Called by the USB subsystem when a frame has been received
 *
 */
static void stir_receive(struct urb *purb) 
{
	struct stir_cb *self = (struct stir_cb *) purb->context;
	int i;
	
	IRDA_DEBUG(2, "%s(), len=%d\n", __FUNCTION__, purb->actual_length);
	
	/* Find ourselves */
	ASSERT(self != NULL, return;);

	/* If the network is closed or the device gone, stop everything */
	if ((!self->netopen) || (!self->present)) {
		IRDA_DEBUG(0, "%s(), Network is gone!\n", __FUNCTION__);
		/* Don't re-submit the URB : will stall the Rx path */
		return;
	}
	
	/* Check the status */
	if(purb->status != 0) {
		switch (purb->status) {
		case -ECONNRESET:		/* -104 */
			IRDA_DEBUG(0, "%s(), Connection Reset (-104), "
				   "transfer_flags 0x%04X \n", 
				   __FUNCTION__, purb->transfer_flags);
			/* uhci_cleanup_unlink() is going to kill the Rx
			 * URB just after we return. No problem, at this
			 * point the URB will be idle ;-) - Jean II */
			break;
		default:
			IRDA_DEBUG(0, "%s(), RX status %d, "
				   "transfer_flags 0x%04X \n", __FUNCTION__, 
				   purb->status, purb->transfer_flags);
			break;
		}
		purb->actual_length = 0; /* Treat it like an empty frame */
	}
	
	/* If we received data in this last urb, submit another right
	 * away.  Otherwise, we perform the submit on a timer, so as
	 * not to flood the device with idle requests.  We should
	 * scale this delay by the speed at which we expec the FIFO to
	 * have filled up.
	 */
	if (purb->actual_length <= 0) {
#if 1
		stir_setup_receive_timer(self);
#else
		/* The code above may not work properly for two reason :
		 * First, the minimum timer granularity we can have is 10ms,
		 * which is 3 or 4 turnarounds or small packets, so it will
		 * show up in term of latency performance.
		 * Second, the hardware FIR Rx FIFO is busted, and will
		 * aggregate Rx packets without properly adding a separator
		 * (BOF/EOF) in between, and our Rx code won't be able
		 * to properly decapsulate that.
		 * The hardware interface is c**p, and there is only so
		 * much we can do to workaround that...
		 * Jean II */
		/* Actually, my test at FIR show that this made absolutely
		 * no difference. This is going to be very ugly. Jean II */

		/* Submit the idle URB to replace the URB we've just received */
		stir_rx_submit(self, self->idle_rx_urb);

		/* Recycle Rx URB : Now, the idle URB is the present one */
		purb->context = NULL;
		self->idle_rx_urb = purb;
#endif
		return;
	}

	self->idle_periods = 0;

	/*  
	 * Remember the time we received this frame, so we can
	 * reduce the min turn time a bit since we will know
	 * how much time we have used for protocol processing
	 */
        do_gettimeofday(&self->stamp);

	if (self->speed == 4000000) {
		stir_async_fir_chars(self, &self->rx_buff,
				     self->rxdata, purb->actual_length);
	} else {
		for (i = 0; i < purb->actual_length; i++) {
			async_unwrap_char(self->netdev, &self->stats, 
					  &self->rx_buff, self->rxdata[i]);
		}
	}

	/* Note : at this point, the URB we've just received (purb)
	 * is still referenced by the USB layer. For example, if we
	 * have received a -ECONNRESET, uhci_cleanup_unlink() will
	 * continue to process it (in fact, cleaning it up).
	 * If we were to submit this URB, disaster would ensue.
	 * Therefore, we submit our idle URB, and put this URB in our
	 * idle slot....
	 * Jean II */
	/* Note : with this scheme, we could submit the idle URB before
	 * processing the Rx URB. Another time... Jean II */

	/* Submit the idle URB to replace the URB we've just received */
	stir_rx_submit(self, self->idle_rx_urb);

	/* Recycle Rx URB : Now, the idle URB is the present one */
	purb->context = NULL;
	self->idle_rx_urb = purb;
}

/*------------------------------------------------------------------*/
/*
 * Callbak from IrDA layer. IrDA wants to know if we have
 * started receiving anything.
 */
static int stir_is_receiving(struct stir_cb *self)
{
	/* Note : because of the way UHCI works, it's almost impossible
	 * to get this info. The Controller DMA directly to memory and
	 * signal only when the whole frame is finished. To know if the
	 * first TD of the URB has been filled or not seems hard work...
	 *
	 * The other solution would be to use the "receiving" command
	 * on the default decriptor with a usb_control_msg(), but that
	 * would add USB traffic and would return result only in the
	 * next USB frame (~1ms).
	 *
	 * I've been told that current dongles send status info on their
	 * interrupt endpoint, and that's what the Windows driver uses
	 * to know this info. Unfortunately, this is not yet in the spec...
	 *
	 * Jean II
	 */

	return 0; /* For now */
}

/********************** IRDA DEVICE CALLBACKS **********************/
/*
 * Main calls from the IrDA/Network subsystem.
 * Mostly registering a new irda-usb device and removing it....
 * We only deal with the IrDA side of the business, the USB side will
 * be dealt with below...
 */

/*------------------------------------------------------------------*/
/*
 * Callback when a new IrDA device is created.
 */
static int stir_net_init(struct net_device *dev)
{
	IRDA_DEBUG(1, "%s()\n", __FUNCTION__);
	
	/* Keep track of module usage */
	SET_MODULE_OWNER(dev);

	/* Set up to be a normal IrDA network device driver */
	irda_device_setup(dev);

	/* Insert overrides below this line! */

	return 0;
}

/*------------------------------------------------------------------*/
/*
 * Function stir_net_open (dev)
 *
 *    Network device is taken up. Usually this is done by "ifconfig irda0 up" 
 *   
 * Note : don't mess with self->netopen - Jean II
 */
static int stir_net_open(struct net_device *netdev)
{
	struct stir_cb *self;
	char	hwname[16];
	int i;
	
	IRDA_DEBUG(1, "%s()\n", __FUNCTION__);

	ASSERT(netdev != NULL, return -1;);
	self = (struct stir_cb *) netdev->priv;
	ASSERT(self != NULL, return -1;);

	/* Can only open the device if it's there */
	if(!self->present) {
		WARNING("%s(), device not present!\n", __FUNCTION__);
		return -1;
	}

	/* Initialize internal values */
	self->new_speed = -1;

	/* To do *before* submitting Rx urbs and starting net Tx queue
	 * Jean II */
	self->netopen = 1;

	/* 
	 * Now that everything should be initialized properly,
	 * Open new IrLAP layer instance to take care of us...
	 * Note : will send immediately a speed change...
	 */
	sprintf(hwname, "usb#%d", self->usbdev->devnum);
	self->irlap = irlap_open(netdev, &self->qos, hwname);
	ASSERT(self->irlap != NULL, return -1;);

	/* Allow IrLAP to send data to us */
	netif_start_queue(netdev);

	/* We submit all the Rx URB except for one that we keep idle.
	 * Need to be initialised before submitting other USBs, because
	 * in some cases as soon as we submit the URBs the USB layer
	 * will trigger a dummy receive - Jean II */
	self->idle_rx_urb = self->rx_urb[STIR_MAX_ACTIVE_RX_URBS];
	self->idle_rx_urb->context = NULL;

	/* Now that we can pass data to IrLAP, allow the USB layer
	 * to send us some data... */
	for (i = 0; i < STIR_MAX_ACTIVE_RX_URBS; i++)
		stir_rx_submit(self, self->rx_urb[i]);

	/* Ready to play !!! */
	return 0;
}

/*------------------------------------------------------------------*/
/*
 * Function stir_net_close (self)
 *
 *    Network device is taken down. Usually this is done by 
 *    "ifconfig irda0 down" 
 */
static int stir_net_close(struct net_device *netdev)
{
	struct stir_cb *self;
	int	i;

	IRDA_DEBUG(1, "%s()\n", __FUNCTION__);

	ASSERT(netdev != NULL, return -1;);
	self = (struct stir_cb *) netdev->priv;
	ASSERT(self != NULL, return -1;);

	/* Clear this flag *before* unlinking the urbs and *before*
	 * stopping the network Tx queue - Jean II */
	self->netopen = 0;

	/* Stop network Tx queue */
	netif_stop_queue(netdev);

	/* Deallocate all the Rx path buffers (URBs and skb) */
	for (i = 0; i < STIR_MAX_RX_URBS; i++) {
		struct urb *purb = self->rx_urb[i];
		/* Cancel the receive command */
		usb_unlink_urb(purb);
	}
	/* Cancel Tx and speed URB - need to be synchronous to avoid races */
	self->tx_urb->transfer_flags &= ~USB_ASYNC_UNLINK;
	usb_unlink_urb(self->tx_urb);
	self->speed_urb[0]->transfer_flags &= ~USB_ASYNC_UNLINK;
	usb_unlink_urb(self->speed_urb[0]);
	self->speed_urb[1]->transfer_flags &= ~USB_ASYNC_UNLINK;
	usb_unlink_urb(self->speed_urb[1]);

	/* Stop and remove instance of IrLAP */
	if (self->irlap)
		irlap_close(self->irlap);
	self->irlap = NULL;

	return 0;
}

/*------------------------------------------------------------------*/
/*
 * IOCTLs : Extra out-of-band network commands...
 */
static int stir_net_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	unsigned long flags;
	struct if_irda_req *irq = (struct if_irda_req *) rq;
	struct stir_cb *self;
	int ret = 0;

	ASSERT(dev != NULL, return -1;);
	self = dev->priv;
	ASSERT(self != NULL, return -1;);

	IRDA_DEBUG(2, "%s(), %s, (cmd=0x%X)\n", __FUNCTION__, dev->name, cmd);

	switch (cmd) {
	case SIOCSBANDWIDTH: /* Set bandwidth */
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		/* Protect us from USB callbacks, net watchdog and else. */
		spin_lock_irqsave(&self->lock, flags);
		/* Check if the device is still there */
		if(self->present) {
			/* Set the desired speed */
			stir_change_speed(self, irq->ifr_baudrate);
		}
		spin_unlock_irqrestore(&self->lock, flags);
		break;
	case SIOCSMEDIABUSY: /* Set media busy */
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		/* Check if the IrDA stack is still there */
		if(self->netopen)
			irda_device_set_media_busy(self->netdev, TRUE);
		break;
	case SIOCGRECEIVING: /* Check if we are receiving right now */
		irq->ifr_receiving = stir_is_receiving(self);
		break;
	default:
		ret = -EOPNOTSUPP;
	}
	
	return ret;
}

/*------------------------------------------------------------------*/
/*
 * Get device stats (for /proc/net/dev and ifconfig)
 */
static struct net_device_stats *stir_net_get_stats(struct net_device *dev)
{
	struct stir_cb *self = dev->priv;
	return &self->stats;
}

/********************* IRDA CONFIG SUBROUTINES *********************/
/*
 * Various subroutines dealing with IrDA and network stuff we use to
 * configure and initialise each irda-usb instance.
 * These functions are used below in the main calls of the driver...
 */

/*------------------------------------------------------------------*/
/*
 * Set proper values in the IrDA QOS structure
 */
static inline void stir_init_qos(struct stir_cb *self)
{
	IRDA_DEBUG(3, "%s()\n", __FUNCTION__);
	
	/* Initialize QoS for this device */
	irda_init_max_qos_capabilies(&self->qos);

	/* That's the Rx capability. */
	self->qos.baud_rate.bits       &= IR_2400 | IR_9600 | IR_19200 |
					 IR_38400 | IR_57600 | IR_115200 |
					 (IR_4000000 << 8);
	self->qos.min_turn_time.bits   &= 0x07; /* >= 1ms turnaround */
	self->qos.additional_bofs.bits &= 0xff; /* Any additional BOFs */
	self->qos.window_size.bits     &= 0x7f; /* Up to 7 unacked frames */
	//self->qos.data_size.bits       &= 0x07; /* Conservative: 256 bytes */
	self->qos.data_size.bits       &= 0x3f;	/* This seems to work OK */
	/* Module parameter can override the rx window size */
	if (qos_mtt_bits)
		self->qos.min_turn_time.bits = qos_mtt_bits;
	/* 
	 * Note : most of those values apply only for the receive path,
	 * the transmit path will be set differently - Jean II 
	 */
	irda_qos_bits_to_value(&self->qos);

	/* We would need to fix the Tx window to 1 - Jean II */
}

/*------------------------------------------------------------------*/
/*
 * Initialise the network side of the irda-usb instance
 * Called when a new USB instance is registered in stir_probe()
 */
static inline int stir_open(struct stir_cb *self)
{
	struct net_device *netdev;
	int err;

	IRDA_DEBUG(1, "%s()\n", __FUNCTION__);

	spin_lock_init(&self->lock);

	stir_init_qos(self);

	/* Bootstrap ZeroCopy Rx */
        if (self->rx_buff.head == NULL) {
		self->rx_buff.truesize = IRDA_SKB_MAX_MTU; 
		self->rx_buff.skb = __dev_alloc_skb(self->rx_buff.truesize, GFP_KERNEL);
		if (self->rx_buff.skb == NULL) {
			ERROR("%s(), dev_alloc_skb() failed for rxbuf!\n",
			      __FUNCTION__);
			goto err;
		}
		skb_reserve(self->rx_buff.skb, 1);
		self->rx_buff.head = self->rx_buff.skb->data;
	}
	/* Create all other necessary buffers */
        if (self->tx_buff.head == NULL) {
		if (stir_irda_init_iobuf(&self->tx_buff, 4000)) {
			ERROR("%s(), init_iobuf() failed for txbuf!\n", 
			      __FUNCTION__);
			goto err;
		}
	}

	if (self->rxdata == NULL) {
		self->rxdata = kmalloc(4096, GFP_KERNEL);
		if (self->rxdata == NULL) {
			ERROR("%s(), Can't allocate rxdata buf\n",
			      __FUNCTION__);
			goto err;
		}
	}

	if (self->ctrl_buf == NULL) {
		self->ctrl_buf = (unsigned char *) __get_free_page(GFP_KERNEL);
		if (self->ctrl_buf == NULL) {
			ERROR("%s(), Can't allocate ctrl buf\n",
			      __FUNCTION__);
			goto err;
		}
	}

	/* Initialize the device -- bring it out of reset, set to 9600 bps */
	stir_change_speed(self, 9600);

	/* Write out sensitivity and power values */
	stir_write_reg(self, STIR_REG_CTRL1, (tx_power & 0x3) << 1);
	stir_write_reg(self, STIR_REG_CTRL2, (rx_sensitivity & 0x7) << 5);

	self->idle_periods = 0;
	self->rx_timer_active = 0;
	self->speed_timer_active = 0;
	/* Move init_timer() in here - Jean II */

	/* Create a network device for us */
	if (!(netdev = dev_alloc("irda%d", &err))) {
		ERROR("%s(), dev_alloc() failed!\n", __FUNCTION__);
		goto err;
	}
	self->netdev = netdev;
 	netdev->priv = (void *) self;

	/* Override the network functions we need to use */
	netdev->init            = stir_net_init;
	netdev->hard_start_xmit = stir_hard_xmit;
	netdev->tx_timeout	= stir_net_timeout;
	netdev->watchdog_timeo  = 250*HZ/1000;	/* 250 ms > USB timeout */
	netdev->open            = stir_net_open;
	netdev->stop            = stir_net_close;
	netdev->get_stats	= stir_net_get_stats;
	netdev->do_ioctl        = stir_net_ioctl;

	rtnl_lock();
	err = register_netdevice(netdev);
	rtnl_unlock();
	if (err) {
		ERROR("%s(), register_netdev() failed!\n", __FUNCTION__);
		goto err;
	}
	MESSAGE("IrDA: Registered SigmaTel device %s\n", netdev->name);
	return 0;

err:
	if(self->rx_buff.head != NULL) {
		kfree_skb(self->rx_buff.skb);
		self->rx_buff.skb = NULL;
		self->rx_buff.head = NULL;
	}
	if(self->tx_buff.head != NULL) {
		kfree(self->tx_buff.head);
		self->tx_buff.head = NULL;
	}
	if(self->rxdata != NULL) {
		kfree(self->rxdata);
		self->rxdata = NULL;
	}
	if (self->ctrl_buf != NULL) {
		free_page((unsigned long) self->ctrl_buf);
		self->ctrl_buf = NULL;
	}
	return -1;
}

/*------------------------------------------------------------------*/
/*
 * Cleanup the network side of the irda-usb instance
 * Called when a USB instance is removed in stir_disconnect()
 */
static inline int stir_close(struct stir_cb *self)
{
	IRDA_DEBUG(1, "%s()\n", __FUNCTION__);

	ASSERT(self != NULL, return -1;);

	/* Remove netdevice */
	if (self->netdev) {
		rtnl_lock();
		unregister_netdevice(self->netdev);
		self->netdev = NULL;
		rtnl_unlock();
	}

	/* Should use del_timer() unconditionally, but we need to make
	 * sure they are always properly initialised in open() - Jean II */
	/* Also think about using timer_pending() elsewhere */
	/* If the device has active timers */
	if (self->rx_timer_active != 0)
		del_timer(&self->submit_rx_timer);

	if (self->speed_timer_active != 0)
		del_timer(&self->speed_timer);

	/* Free any allocated data buffers */
	if(self->rx_buff.head != NULL) {
		kfree_skb(self->rx_buff.skb);
		self->rx_buff.skb = NULL;
		self->rx_buff.head = NULL;
	}
	if(self->tx_buff.head != NULL) {
		kfree(self->tx_buff.head);
		self->tx_buff.head = NULL;
	}
	if(self->rxdata != NULL) {
		kfree(self->rxdata);
		self->rxdata = NULL;
	}
	if (self->ctrl_buf != NULL) {
		free_page((unsigned long) self->ctrl_buf);
		self->ctrl_buf = NULL;
	}

	return 0;
}

/********************** USB CONFIG SUBROUTINES **********************/
/*
 * Various subroutines dealing with USB stuff we use to configure and
 * initialise each irda-usb instance.
 * These functions are used below in the main calls of the driver...
 */

/*------------------------------------------------------------------*/
/*
 * Function stir_parse_endpoints(dev, ifnum)
 *
 *    Parse the various endpoints and find the one we need.
 *
 * The endpoint are the pipes used to communicate with the USB device.
 * The spec defines 2 endpoints of type bulk transfer, one in, and one out.
 * These are used to pass frames back and forth with the dongle.
 * Most dongle have also an interrupt endpoint, that will be probably
 * documented in the next spec...
 */
static inline int stir_parse_endpoints(struct stir_cb *self, struct usb_endpoint_descriptor *endpoint, int ennum)
{
	int i;		/* Endpoint index in table */
		
	/* Init : no endpoints */
	self->bulk_in_ep = 0;
	self->bulk_out_ep = 0;

	/* Let's look at all those endpoints */
	for(i = 0; i < ennum; i++) {
		/* All those variables will get optimised by the compiler,
		 * so let's aim for clarity... - Jean II */
		__u8 ep;	/* Endpoint address */
		__u8 dir;	/* Endpoint direction */
		__u8 attr;	/* Endpoint attribute */
		__u16 psize;	/* Endpoint max packet size in bytes */

		/* Get endpoint address, direction and attribute */
		ep = endpoint[i].bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
		dir = endpoint[i].bEndpointAddress & USB_ENDPOINT_DIR_MASK;
		attr = endpoint[i].bmAttributes;
		psize = endpoint[i].wMaxPacketSize;

		/* Is it a bulk endpoint ??? */
		if(attr == USB_ENDPOINT_XFER_BULK) {
			/* We need to find an IN and an OUT */
			if(dir == USB_DIR_IN) {
				/* This is our Rx endpoint */
				self->bulk_in_ep = ep;
			} else {
				/* This is our Tx endpoint */
				self->bulk_out_ep = ep;
				self->bulk_out_mtu = psize;
			}
		} else {
			ERROR("%s(), Unrecognized endpoint %02X.\n", 
			      __FUNCTION__, ep);
		}
	}

	IRDA_DEBUG(0, "%s(), And our endpoints are : in=%02X, out=%02X (%d)\n",
		__FUNCTION__, self->bulk_in_ep, self->bulk_out_ep, 
		   self->bulk_out_mtu);
	/* Should be 8, 16, 32 or 64 bytes */
	ASSERT(self->bulk_out_mtu == 64, ;);

	return((self->bulk_in_ep != 0) && (self->bulk_out_ep != 0));
}

/*********************** USB DEVICE CALLBACKS ***********************/
/*
 * Main calls from the USB subsystem.
 * Mostly registering a new irda-usb device and removing it....
 */

/*------------------------------------------------------------------*/
/*
 * This routine is called by the USB subsystem for each new device
 * in the system. We need to check if the device is ours, and in
 * this case start handling it.
 * Note : it might be worth protecting this function by a global
 * spinlock... Or not, because maybe USB already deal with that...
 */
static void *stir_probe(struct usb_device *dev, unsigned int ifnum,
		      const struct usb_device_id *id)
{
	struct stir_cb *self = NULL;
	struct usb_interface_descriptor *interface;
	int ret;
	int i;

	/* Note : the probe make sure to call us only for devices that
	 * matches the list of dongle (top of the file). So, we
	 * don't need to check if the dongle is really ours.
	 * Jean II */

	MESSAGE("SigmaTel STIr4200 IRDA/USB found at address %d, "
		"Vendor: %x, Product: %x\n",
		dev->devnum, dev->descriptor.idVendor,
		dev->descriptor.idProduct);

	/* Try to cleanup all instance that have a pending disconnect
	 * In theory, it can't happen any longer.
	 * Jean II */
	for (i = 0; i < NIRUSB; i++) {
		struct stir_cb *stir = &stir_instance[i];
		if((stir->usbdev != NULL) &&
		   (stir->present == 0) &&
		   (stir->netopen == 0)) {
			IRDA_DEBUG(0, "%s(), found a zombie instance !!!\n",
				   __FUNCTION__);
			stir_disconnect(stir->usbdev, (void *) stir);
		}
	}

	/* Find an free instance to handle this new device... */
	self = NULL;
	for (i = 0; i < NIRUSB; i++) {
		if(stir_instance[i].usbdev == NULL) {
			self = &stir_instance[i];
			break;
		}
	}
	if(self == NULL) {
		WARNING("Too many STIr IrDA devices !!! (max = %d)\n",
			   NIRUSB);
		return NULL;
	}

	/* Reset the instance */
	self->present = 0;
	self->netopen = 0;

	/* Find our endpoints */
	interface = &dev->actconfig->interface[ifnum].altsetting[0];
	if(!stir_parse_endpoints(self, interface->endpoint,
				 interface->bNumEndpoints)) {
		ERROR("%s(), Bogus endpoints...\n", __FUNCTION__);
		return NULL;
	}

	/* Create all of the needed urbs */
	for (i = 0; i < STIR_MAX_RX_URBS; i++) {
		self->rx_urb[i] = usb_alloc_urb(0);
		if (!self->rx_urb[i]) {
			int j;
			for (j = 0; j < i; j++)
				usb_free_urb(self->rx_urb[j]);
			return NULL;
		}
	}
	self->tx_urb = usb_alloc_urb(0);
	if (!self->tx_urb) {
		for (i = 0; i < STIR_MAX_RX_URBS; i++)
			usb_free_urb(self->rx_urb[i]);
		return NULL;
	}
	self->speed_urb[0] = usb_alloc_urb(0);
	if (!self->speed_urb[0]) {
		for (i = 0; i < STIR_MAX_RX_URBS; i++)
			usb_free_urb(self->rx_urb[i]);
		usb_free_urb(self->tx_urb);
		return NULL;
	}
	self->speed_urb[1] = usb_alloc_urb(0);
	if (!self->speed_urb[1]) {
		for (i = 0; i < STIR_MAX_RX_URBS; i++)
			usb_free_urb(self->rx_urb[i]);
		usb_free_urb(self->tx_urb);
		usb_free_urb(self->speed_urb[0]);
		return NULL;
	}

	self->present = 1;
	self->netopen = 0;
	self->usbdev = dev;
	ret = stir_open(self);
	if (ret)
		return NULL;

//	usb_set_intfdata(intf, self);
	return self;
}

/*------------------------------------------------------------------*/
/*
 * The current irda-usb device is removed, the USB layer tell us
 * to shut it down...
 * One of the constraints is that when we exit this function,
 * we cannot use the usb_device no more. Gone. Destroyed. kfree().
 * Most other subsystem allow you to destroy the instance at a time
 * when it's convenient to you, to postpone it to a later date, but
 * not the USB subsystem.
 * So, we must make bloody sure that everything gets deactivated.
 * Jean II
 */
static void stir_disconnect(struct usb_device *dev, void *ptr)
{
	unsigned long flags;
	struct stir_cb *self = (struct stir_cb *) ptr;
	int i;

	IRDA_DEBUG(1, "%s()\n", __FUNCTION__);

	if (!self)
		return;

	/* Make sure that the Tx path is not executing. - Jean II */
	spin_lock_irqsave(&self->lock, flags);

	/* Oups ! We are not there any more.
	 * This will stop/desactivate the Tx path. - Jean II */
	self->present = 0;

	/* We need to have irq enabled to unlink the URBs. That's OK,
	 * at this point the Tx path is gone - Jean II */
	spin_unlock_irqrestore(&self->lock, flags);

	/* Hum... Check if networking is still active (avoid races) */
	if((self->netopen) || (self->irlap)) {
		/* Accept no more transmissions */
		/*netif_device_detach(self->netdev);*/
		netif_stop_queue(self->netdev);
		/* Stop all the receive URBs */
		for (i = 0; i < STIR_MAX_RX_URBS; i++)
			usb_unlink_urb(self->rx_urb[i]);
		/* Cancel Tx URB.
		 * Toggle flags to make sure it's synchronous. */
		self->tx_urb->transfer_flags &= ~USB_ASYNC_UNLINK;
		usb_unlink_urb(self->tx_urb);
		self->speed_urb[0]->transfer_flags &= ~USB_ASYNC_UNLINK;
		usb_unlink_urb(self->speed_urb[0]);
		self->speed_urb[1]->transfer_flags &= ~USB_ASYNC_UNLINK;
		usb_unlink_urb(self->speed_urb[0]);
	}

	/* Cleanup the device stuff */
	stir_close(self);
	/* No longer attached to USB bus */
	self->usbdev = NULL;
	self->usbintf = NULL;

	/* Clean up our urbs */
	for (i = 0; i < STIR_MAX_RX_URBS; i++)
		usb_free_urb(self->rx_urb[i]);
	/* Clean up Tx and speed URB */
	usb_free_urb(self->tx_urb);
	usb_free_urb(self->speed_urb[0]);
	usb_free_urb(self->speed_urb[1]);

	IRDA_DEBUG(0, "%s(), SigmaTel Disconnected\n", __FUNCTION__);
}

/*------------------------------------------------------------------*/
/*
 * USB device callbacks
 */
static struct usb_driver irda_driver = {
	name:		"stir4200",
	probe:		stir_probe,
	disconnect:	stir_disconnect,
	id_table:	dongles,
};

/************************* MODULE CALLBACKS *************************/
/*
 * Deal with module insertion/removal
 * Mostly tell USB about our existence
 */

/*------------------------------------------------------------------*/
/*
 * Module insertion
 */
int __init stir_init(void)
{
	memset(&stir_instance, 0, sizeof(stir_instance));

	if (usb_register(&irda_driver) < 0)
		return -1;

	MESSAGE("SigmaTel support registered\n");
	return 0;
}
module_init(stir_init);

/*------------------------------------------------------------------*/
/*
 * Module removal
 */
void __exit stir_cleanup(void)
{
	struct stir_cb *stir = NULL;
	int	i;

	/* Find zombie instances and kill them...
	 * In theory, it can't happen any longer. Jean II */
	for (i = 0; i < NIRUSB; i++) {
		stir = &stir_instance[i];
		/* If the Device is zombie */
		if((stir->usbdev != NULL) && (stir->present == 0)) {
			IRDA_DEBUG(0, "%s(), disconnect zombie now !\n",
				   __FUNCTION__);
			stir_disconnect(stir->usbdev, (void *) stir);
		}
	}

	/* Deregister the driver and remove all pending instances */
	usb_deregister(&irda_driver);
}
module_exit(stir_cleanup);

/*------------------------------------------------------------------*/
/*
 * Module parameters
 */
MODULE_PARM(qos_mtt_bits, "i");
MODULE_PARM_DESC(qos_mtt_bits, "Minimum Turn Time");
MODULE_PARM(rx_sensitivity, "i");
MODULE_PARM_DESC(rx_sensitivity, "Set Receiver sensitivity (0-7, 0 is most sensitive)");
MODULE_PARM(tx_power, "i");
MODULE_PARM_DESC(tx_power, "Set Transmitter power (0-3, 0 is highest power)");

MODULE_AUTHOR("Paul Stewart <stewart@parc.com>, Roman Weissgaerber <weissg@vienna.at>, Dag Brattli <dag@brattli.net> and Jean Tourrilhes <jt@hpl.hp.com>");
MODULE_DESCRIPTION("IrDA-USB Dongle Driver for SigmaTel STIr4200"); 
MODULE_LICENSE("GPL");
