/*****************************************************************************
*
* Filename:      stir4200.h
* Version:       0.2
* Description:   IrDA-USB Driver
* Status:        Experimental 
* Author:        Paul Stewart <stewart@parc.com>
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

/* These are definitions out of the SigmaTel STIr4200 datasheet, plus a 
 * sprinkling of additional constants and data structs inspired by irda-usb.h
 */

#define STIR_IRDA_HEADER  4
#define STIR_MIN_RTT 500
#define STIR_CTRL_TIMEOUT 100		   /* milliseconds */
#define STIR_BULK_TIMEOUT 200		   /* milliseconds */
#define STIR_IRDA_RXFRAME_LEN 64
#define STIR_FIFO_SIZE 4096
#define STIR_IDLE_TIMEOUT 10		   /* milliseconds */
#define STIR_IDLE_PERIODS 700		   /* roughly 7 second idle window */
#define STIR_MIN_SPEED_DELAY 75		   /* milliseconds */

#define STIR_MAX_ACTIVE_RX_URBS   1       /* Don't touch !!! */
#define STIR_MAX_RX_URBS  (STIR_MAX_ACTIVE_RX_URBS + 1)

#define STIR_BOF  0x7E /* Beginning of frame */
#define STIR_XBOF 0x7F
#define STIR_EOF  0x7E /* End of frame */

#define STIR_REQ_WRITEREG		0x40
#define STIR_REQ_WRITEREG_MULTI		0x00
#define STIR_REQ_WRITEREG_SINGLE	0x03
#define STIR_REQ_READ			0xC0
#define STIR_REQ_READ_REG		0x01
#define STIR_REQ_READ_ROM		0x02

#define STIR_REG_MODE 1
#define STIR_MODE_FIR     0x80
#define STIR_MODE_SIR     0x20
#define STIR_MODE_ASK     0x10
#define STIR_MODE_FASTRX  0x08
#define STIR_MODE_FFRSTEN 0x04
#define STIR_MODE_NRESET  0x02
#define STIR_MODE_2400    0x01

#define STIR_REG_PDCLK 2
#define STIR_PDCLK_4000000 0x02
#define STIR_PDCLK_115200  0x09
#define STIR_PDCLK_57600   0x13
#define STIR_PDCLK_38400   0x1D
#define STIR_PDCLK_19200   0x3B
#define STIR_PDCLK_9600    0x77
#define STIR_PDCLK_2400    0xDF /* also set bit 0 of STIR_REG_MODE */

#define STIR_REG_CTRL1 3
#define STIR_CTRL1_SDMODE  0x80
#define STIR_CTRL1_RXSLOW  0x40
#define STIR_CTRL1_TXPWD   0x10
#define STIR_CTRL1_RXPWD   0x08
#define STIR_CTRL1_TXPWR0  0x00 /* 0 = highest power */
#define STIR_CTRL1_TXPWR1  0x02
#define STIR_CTRL1_TXPWR2  0x04
#define STIR_CTRL1_TXPWR3  0x06 /* 3 = lowest power */
#define STIR_CTRL1_SRESET  0x01

#define STIR_REG_CTRL2 4
#define STIR_CTRL2_FIR_1   0x00
#define STIR_CTRL2_FIR_2   0x20
#define STIR_CTRL2_FIR_3   0x40
#define STIR_CTRL2_FIR_4   0x60
#define STIR_CTRL2_FIR_5   0x80
#define STIR_CTRL2_SIR_4   0x00
#define STIR_CTRL2_SIR_8   0x20
#define STIR_CTRL2_SIR_12  0x40
#define STIR_CTRL2_SIR_16  0x60
#define STIR_CTRL2_SIR_20  0x80
#define STIR_CTRL2_SIR_24  0xA0
#define STIR_CTRL2_SIR_28  0xC0
#define STIR_CTRL2_SPWITDH 0x08
#define STIR_CTRL2_REVID   0x03

#define STIR_REG_FIFOCTL 5
#define STIR_FIFOCTL_DIR   0x10
#define STIR_FIFOCTL_CLR   0x08
#define STIR_FIFOCTL_EMPTY 0x04

#define STIR_REG_FIFOCNT1  6 
#define STIR_REG_FIFOCNT2  7

#define STIR_REG_IRDIG 9
#define STIR_IRDIG_RXHIGH  0x80
#define STIR_IRDIG_RXLOW   0x40

#define STIR_REG_TEST 15
#define STIR_TEST_PLLDOWN  0x80
#define STIR_TEST_LOOPIR   0x40
#define STIR_TEST_LOOPUSB  0x20
#define STIR_TEST_TSTENA   0x10
#define STIR_TEST_TSTOSC   0x0F

struct stir_cb {
        struct usb_device *usbdev;      /* init: probe_irda */
	struct usb_interface *usbintf;	/* init: probe_irda */
        int netopen;                    /* Device is active for network */
        int present;                    /* Device is present on the bus */
        __u8  bulk_in_ep;               /* Rx Endpoint assignments */
        __u8  bulk_out_ep;              /* Tx Endpoint assignments */
        __u16 bulk_out_mtu;             /* Max Tx packet size in bytes */

        wait_queue_head_t wait_q;       /* for timeouts */

        struct urb *rx_urb[STIR_MAX_RX_URBS];/* used to receive data frames */
        struct urb *idle_rx_urb;        /* Pointer to idle URB in Rx path */
        struct urb *tx_urb;              /* URB used to send data frames */
        
        struct net_device *netdev;      /* Yes! we are some kind of netdev. */
        struct net_device_stats stats;
        struct irlap_cb   *irlap;       /* The link layer we are binded to */
        
        struct qos_info qos;

        struct timeval stamp;
        struct timeval now;

        spinlock_t lock;                /* For serializing operations */

        __u32 speed;                    /* Current speed */
	__u32 new_speed;
    
        iobuff_t                tx_buff;
        iobuff_t                rx_buff;
	unsigned char		*rxdata;

	unsigned int idle_periods;
	unsigned int rx_timer_active;
	struct timer_list submit_rx_timer;

	struct urb *speed_urb[2];
	unsigned char *ctrl_buf;
	unsigned int speed_timer_active;
	struct timer_list speed_timer;
};

