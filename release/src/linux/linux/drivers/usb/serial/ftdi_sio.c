

/* Bill Ryder - bryder@sgi.com - wrote the FTDI_SIO implementation */
/* Thanx to FTDI for so kindly providing details of the protocol required */
/*   to talk to the device */
/* Thanx to gkh and the rest of the usb dev group for all code I have assimilated :-) */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <linux/usb.h>
#include <linux/serial.h>
#ifdef CONFIG_USB_SERIAL_DEBUG
	static int debug = 1;
#else
	static int debug;
#endif

#include "usb-serial.h"
#include "ftdi_sio.h"

/*
 * Version Information
 */
#define DRIVER_VERSION "v1.2.1"
#define DRIVER_AUTHOR "Greg Kroah-Hartman <greg@kroah.com>, Bill Ryder <bryder@sgi.com>, Kuba Ober <kuba@mareimbrium.org>"
#define DRIVER_DESC "USB FTDI Serial Converters Driver"

static struct usb_device_id id_table_sio [] = {
	{ USB_DEVICE(FTDI_VID, FTDI_SIO_PID) },
	{ }						/* Terminating entry */
};

/*
 * The 8U232AM has the same API as the sio except for:
 * - it can support MUCH higher baudrates; up to:
 *   o 921600 for RS232 and 2000000 for RS422/485 at 48MHz
 *   o 230400 at 12MHz
 *   so .. 8U232AM's baudrate setting codes are different
 * - it has a two byte status code.
 * - it returns characters every 16ms (the FTDI does it every 40ms)
 */


static struct usb_device_id id_table_8U232AM [] = {
	{ USB_DEVICE(FTDI_VID, FTDI_8U232AM_PID) },
	{ USB_DEVICE(FTDI_NF_RIC_VID, FTDI_NF_RIC_PID) },
	{ }						/* Terminating entry */
};


static __devinitdata struct usb_device_id id_table_combined [] = {
	{ USB_DEVICE(FTDI_VID, FTDI_SIO_PID) },
	{ USB_DEVICE(FTDI_VID, FTDI_8U232AM_PID) },
	{ USB_DEVICE(FTDI_NF_RIC_VID, FTDI_NF_RIC_PID) },
	{ }						/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, id_table_combined);

struct ftdi_private {
	ftdi_chip_type_t chip_type;
				/* type of the device, either SIO or FT8U232AM */
	int baud_base;		/* baud base clock for divisor setting */
	int custom_divisor;	/* custom_divisor kludge, this is for baud_base (different from what goes to the chip!) */
	__u16 last_set_data_urb_value ;
				/* the last data state set - needed for doing a break */
        int write_offset;       /* This is the offset in the usb data block to write the serial data - 
				 * it is different between devices
				 */
	int flags;		/* some ASYNC_xxxx flags are supported */
        wait_queue_head_t delta_msr_wait; /* Used for TIOCMIWAIT */
 	char prev_status, diff_status;        /* Used for TIOCMIWAIT */
};

/* Used for TIOCMIWAIT */
#define FTDI_STATUS_B0_MASK	(FTDI_RS0_CTS | FTDI_RS0_DSR | FTDI_RS0_RI | FTDI_RS0_RLSD)
#define FTDI_STATUS_B1_MASK	(FTDI_RS_BI)
/* End TIOCMIWAIT */

#define FTDI_IMPL_ASYNC_FLAGS = ( ASYNC_SPD_HI | ASYNC_SPD_VHI \
 ASYNC_SPD_CUST | ASYNC_SPD_SHI | ASYNC_SPD_WARP )

/* function prototypes for a FTDI serial converter */
static int  ftdi_SIO_startup		(struct usb_serial *serial);
static int  ftdi_8U232AM_startup	(struct usb_serial *serial);
static void ftdi_shutdown		(struct usb_serial *serial);
static int  ftdi_open			(struct usb_serial_port *port, struct file *filp);
static void ftdi_close			(struct usb_serial_port *port, struct file *filp);
static int  ftdi_write			(struct usb_serial_port *port, int from_user, const unsigned char *buf, int count);
static int  ftdi_write_room		(struct usb_serial_port *port);
static void ftdi_write_bulk_callback	(struct urb *urb);
static void ftdi_read_bulk_callback	(struct urb *urb);
static void ftdi_set_termios		(struct usb_serial_port *port, struct termios * old);
static int  ftdi_ioctl			(struct usb_serial_port *port, struct file * file, unsigned int cmd, unsigned long arg);
static void ftdi_break_ctl		(struct usb_serial_port *port, int break_state );

static struct usb_serial_device_type ftdi_SIO_device = {
	.owner =		THIS_MODULE,
	.name =			"FTDI SIO",
	.id_table =		id_table_sio,
	.num_interrupt_in =	0,
	.num_bulk_in =		1,
	.num_bulk_out =		1,
	.num_ports =		1,
	.open =			ftdi_open,
	.close =		ftdi_close,
	.write =		ftdi_write,
	.write_room =		ftdi_write_room,
	.read_bulk_callback =	ftdi_read_bulk_callback,
	.write_bulk_callback =	ftdi_write_bulk_callback,
	.ioctl =		ftdi_ioctl,
	.set_termios =		ftdi_set_termios,
	.break_ctl =		ftdi_break_ctl,
	.startup =		ftdi_SIO_startup,
	.shutdown =		ftdi_shutdown,
};

static struct usb_serial_device_type ftdi_8U232AM_device = {
	.owner =		THIS_MODULE,
	.name =			"FTDI 8U232AM",
	.id_table =		id_table_8U232AM,
	.num_interrupt_in =	0,
	.num_bulk_in =		1,
	.num_bulk_out =		1,
	.num_ports =		1,
	.open =			ftdi_open,
	.close =		ftdi_close,
	.write =		ftdi_write,
	.write_room =		ftdi_write_room,
	.read_bulk_callback =	ftdi_read_bulk_callback,
	.write_bulk_callback =	ftdi_write_bulk_callback,
	.ioctl =		ftdi_ioctl,
	.set_termios =		ftdi_set_termios,
	.break_ctl =		ftdi_break_ctl,
	.startup =		ftdi_8U232AM_startup,
	.shutdown =		ftdi_shutdown,
};

#define WDR_TIMEOUT (HZ * 5 ) /* default urb timeout */

#define HIGH 1
#define LOW 0

/*
 * ***************************************************************************
 * Utlity functions
 * ***************************************************************************
 */


static int set_rts(struct usb_device *dev,
		   unsigned int pipe,
		   int high_or_low)
{
	static char buf[1];
	unsigned ftdi_high_or_low = (high_or_low? FTDI_SIO_SET_RTS_HIGH : 
				FTDI_SIO_SET_RTS_LOW);
	return(usb_control_msg(dev, pipe,
			       FTDI_SIO_SET_MODEM_CTRL_REQUEST, 
			       FTDI_SIO_SET_MODEM_CTRL_REQUEST_TYPE,
			       ftdi_high_or_low, 0, 
			       buf, 0, WDR_TIMEOUT));
}


static int set_dtr(struct usb_device *dev,
                   unsigned int pipe,
                   int high_or_low)
{
	static char buf[1];
	unsigned ftdi_high_or_low = (high_or_low? FTDI_SIO_SET_DTR_HIGH : 
				FTDI_SIO_SET_DTR_LOW);
	return(usb_control_msg(dev, pipe,
			       FTDI_SIO_SET_MODEM_CTRL_REQUEST, 
			       FTDI_SIO_SET_MODEM_CTRL_REQUEST_TYPE,
			       ftdi_high_or_low, 0, 
			       buf, 0, WDR_TIMEOUT));
}


static __u16 get_ftdi_divisor(struct usb_serial_port * port);


static int change_speed(struct usb_serial_port *port)
{
	char buf[1];
        __u16 urb_value;

	urb_value = get_ftdi_divisor(port);
	
	return (usb_control_msg(port->serial->dev,
			    usb_sndctrlpipe(port->serial->dev, 0),
			    FTDI_SIO_SET_BAUDRATE_REQUEST,
			    FTDI_SIO_SET_BAUDRATE_REQUEST_TYPE,
			    urb_value, 0,
			    buf, 0, 100) < 0);
}


static __u16 get_ftdi_divisor(struct usb_serial_port * port)
{ /* get_ftdi_divisor */
	
	struct ftdi_private * priv = (struct ftdi_private *)port->private;
	__u16 urb_value = 0;
	int baud;

	/*
	 * The logic involved in setting the baudrate can be cleanly split in 3 steps.
	 * Obtaining the actual baud rate is a little tricky since unix traditionally
	 * somehow ignored the possibility to set non-standard baud rates.
	 * 1. Standard baud rates are set in tty->termios->c_cflag
	 * 2. If these are not enough, you can set any speed using alt_speed as follows:
	 *    - set tty->termios->c_cflag speed to B38400
	 *    - set your real speed in tty->alt_speed; it gets ignored when
	 *      alt_speed==0, (or)
	 *    - call TIOCSSERIAL ioctl with (struct serial_struct) set as follows:
	 *      flags & ASYNC_SPD_MASK == ASYNC_SPD_[HI, VHI, SHI, WARP], this just
	 *      sets alt_speed to (HI: 57600, VHI: 115200, SHI: 230400, WARP: 460800)
	 * ** Steps 1, 2 are done courtesy of tty_get_baud_rate
	 * 3. You can also set baud rate by setting custom divisor as follows
	 *    - set tty->termios->c_cflag speed to B38400
	 *    - call TIOCSSERIAL ioctl with (struct serial_struct) set as follows:
	 *      o flags & ASYNC_SPD_MASK == ASYNC_SPD_CUST
	 *      o custom_divisor set to baud_base / your_new_baudrate
	 * ** Step 3 is done courtesy of code borrowed from serial.c - I should really
	 *    spend some time and separate+move this common code to serial.c, it is
	 *    replicated in nearly every serial driver you see.
	 */

	/* 1. Get the baud rate from the tty settings, this observes alt_speed hack */

	baud = tty_get_baud_rate(port->tty);
	dbg("%s - tty_get_baud_rate reports speed %d", __FUNCTION__, baud);

	/* 2. Observe async-compatible custom_divisor hack, update baudrate if needed */

	if (baud == 38400 &&
	    ((priv->flags & ASYNC_SPD_MASK) == ASYNC_SPD_CUST) &&
	     (priv->custom_divisor)) {
		baud = priv->baud_base / priv->custom_divisor;
		dbg("%s - custom divisor %d sets baud rate to %d", __FUNCTION__, priv->custom_divisor, baud);
	}

	/* 3. Convert baudrate to device-specific divisor */

	if (!baud) baud = 9600;	
	switch(priv->chip_type) {
	case SIO: /* SIO chip */
		switch(baud) {
		case 300: urb_value = ftdi_sio_b300; break;
		case 600: urb_value = ftdi_sio_b600; break;
		case 1200: urb_value = ftdi_sio_b1200; break;
		case 2400: urb_value = ftdi_sio_b2400; break;
		case 4800: urb_value = ftdi_sio_b4800; break;
		case 9600: urb_value = ftdi_sio_b9600; break;
		case 19200: urb_value = ftdi_sio_b19200; break;
		case 38400: urb_value = ftdi_sio_b38400; break;
		case 57600: urb_value = ftdi_sio_b57600;  break;
		case 115200: urb_value = ftdi_sio_b115200; break;
		} /* baud */
		if (urb_value == 0)
			dbg("%s - Baudrate (%d) requested is not supported", __FUNCTION__,  baud);
		break;
	case FT8U232AM: /* 8U232AM chip */
		if (baud <= 3000000) {
			urb_value = FTDI_SIO_BAUD_TO_DIVISOR(baud);
		} else {
	                dbg("%s - Baud rate too high!", __FUNCTION__);
		}
		break;
	} /* priv->chip_type */

	if (urb_value == 0) {
		urb_value = ftdi_sio_b9600;
	} else {
		dbg("%s - Baud rate set to %d (divisor %d) on chip %s", __FUNCTION__, baud, urb_value, (priv->chip_type == SIO) ? "SIO" : "FT8U232AM" );
	}

	return(urb_value);
}


static int get_serial_info(struct usb_serial_port * port, struct serial_struct * retinfo)
{
	struct ftdi_private * priv = (struct ftdi_private*) port->private;
	struct serial_struct tmp;

	if (!retinfo)
		return -EFAULT;
	memset(&tmp, 0, sizeof(tmp));
	tmp.flags = priv->flags;
	tmp.baud_base = priv->baud_base;
	tmp.custom_divisor = priv->custom_divisor;
	if (copy_to_user(retinfo, &tmp, sizeof(*retinfo)))
		return -EFAULT;
	return 0;
} /* get_serial_info */


static int set_serial_info(struct usb_serial_port * port, struct serial_struct * newinfo)
{ /* set_serial_info */
	struct ftdi_private * priv = (struct ftdi_private *) port->private;
	struct serial_struct new_serial;
	struct ftdi_private old_priv;

	if (copy_from_user(&new_serial, newinfo, sizeof(new_serial)))
		return -EFAULT;
	old_priv = * priv;

	/* Do error checking and permission checking */

	if (!capable(CAP_SYS_ADMIN)) {
		if (((new_serial.flags & ~ASYNC_USR_MASK) !=
		     (priv->flags & ~ASYNC_USR_MASK)))
			return -EPERM;
		priv->flags = ((priv->flags & ~ASYNC_USR_MASK) |
			       (new_serial.flags & ASYNC_USR_MASK));
		priv->custom_divisor = new_serial.custom_divisor;
		goto check_and_exit;
	}

	if ((new_serial.baud_base != priv->baud_base) ||
	    (new_serial.baud_base < 9600))
		return -EINVAL;

	/* Make the changes - these are privileged changes! */

	priv->flags = ((priv->flags & ~ASYNC_FLAGS) |
	               (new_serial.flags & ASYNC_FLAGS));	
	priv->custom_divisor = new_serial.custom_divisor;

	port->tty->low_latency = (priv->flags & ASYNC_LOW_LATENCY) ? 1 : 0;

check_and_exit:
	if (((old_priv.flags & ASYNC_SPD_MASK) !=
	     (priv->flags & ASYNC_SPD_MASK)) ||
	    (old_priv.custom_divisor != priv->custom_divisor)) {
		if ((priv->flags & ASYNC_SPD_MASK) == ASYNC_SPD_HI)
			port->tty->alt_speed = 57600;
		if ((priv->flags & ASYNC_SPD_MASK) == ASYNC_SPD_VHI)
			port->tty->alt_speed = 115200;
		if ((priv->flags & ASYNC_SPD_MASK) == ASYNC_SPD_SHI)
			port->tty->alt_speed = 230400;
		if ((priv->flags & ASYNC_SPD_MASK) == ASYNC_SPD_WARP)
			port->tty->alt_speed = 460800;
		change_speed(port);
	}
	
	return (0);

} /* set_serial_info */

/*
 * ***************************************************************************
 * FTDI driver specific functions
 * ***************************************************************************
 */

/* Startup for the SIO chip */
static int ftdi_SIO_startup (struct usb_serial *serial)
{
	struct ftdi_private *priv;

	priv = serial->port->private = kmalloc(sizeof(struct ftdi_private), GFP_KERNEL);
	if (!priv){
		err("%s- kmalloc(%Zd) failed.", __FUNCTION__, sizeof(struct ftdi_private));
		return -ENOMEM;
	}

	priv->chip_type = SIO;
	priv->baud_base = 12000000 / 16;
	priv->custom_divisor = 0;
	priv->write_offset = 1;
 	priv->prev_status = priv->diff_status = 0;
	/* This will push the characters through immediately rather
	   than queue a task to deliver them */
	priv->flags = ASYNC_LOW_LATENCY;
	
	return (0);
}

/* Startup for the 8U232AM chip */
static int ftdi_8U232AM_startup (struct usb_serial *serial)
{
	struct ftdi_private *priv;

	priv = serial->port->private = kmalloc(sizeof(struct ftdi_private), GFP_KERNEL);
	if (!priv){
		err("%s- kmalloc(%Zd) failed.", __FUNCTION__, sizeof(struct ftdi_private));
		return -ENOMEM;
	}

	priv->chip_type = FT8U232AM;
	priv->baud_base = 48000000 / 2; /* Would be / 16, but FTDI supports 0.125, 0.25 and 0.5 divisor fractions! */
	priv->custom_divisor = 0;
	priv->write_offset = 0;
        init_waitqueue_head(&priv->delta_msr_wait);
	/* This will push the characters through immediately rather
	   than queue a task to deliver them */
	priv->flags = ASYNC_LOW_LATENCY;
	
	return (0);
}


static void ftdi_shutdown (struct usb_serial *serial)
{
	dbg("%s", __FUNCTION__);

	/* stop reads and writes on all ports */
	while (serial->port[0].open_count > 0) {
	        ftdi_close (&serial->port[0], NULL);
	}
	if (serial->port[0].private){
		kfree(serial->port[0].private);
		serial->port[0].private = NULL;
	}
}


static int  ftdi_open (struct usb_serial_port *port, struct file *filp)
{ /* ftdi_open */
	struct termios tmp_termios;
	struct usb_serial *serial = port->serial;
	struct ftdi_private *priv = port->private;
	
	int result = 0;
	char buf[1]; /* Needed for the usb_control_msg I think */

	dbg("%s", __FUNCTION__);


	port->tty->low_latency = (priv->flags & ASYNC_LOW_LATENCY) ? 1 : 0;

	/* No error checking for this (will get errors later anyway) */
	/* See ftdi_sio.h for description of what is reset */
	usb_control_msg(serial->dev, usb_sndctrlpipe(serial->dev, 0),
			FTDI_SIO_RESET_REQUEST, FTDI_SIO_RESET_REQUEST_TYPE, 
			FTDI_SIO_RESET_SIO, 
			0, buf, 0, WDR_TIMEOUT);

	/* Termios defaults are set by usb_serial_init. We don't change
	   port->tty->termios - this would loose speed settings, etc.
	   This is same behaviour as serial.c/rs_open() - Kuba */

	/* ftdi_set_termios  will send usb control messages */
	ftdi_set_termios(port, &tmp_termios);

	/* Turn on RTS and DTR since we are not flow controlling by default */
	if (set_dtr(serial->dev, usb_sndctrlpipe(serial->dev, 0),HIGH) < 0) {
		err("%s Error from DTR HIGH urb", __FUNCTION__);
	}
	if (set_rts(serial->dev, usb_sndctrlpipe(serial->dev, 0),HIGH) < 0){
		err("%s Error from RTS HIGH urb", __FUNCTION__);
	}

	/* Start reading from the device */
	FILL_BULK_URB(port->read_urb, serial->dev, 
		      usb_rcvbulkpipe(serial->dev, port->bulk_in_endpointAddress),
		      port->read_urb->transfer_buffer, port->read_urb->transfer_buffer_length,
		      ftdi_read_bulk_callback, port);
	result = usb_submit_urb(port->read_urb);
	if (result)
		err("%s - failed submitting read urb, error %d", __FUNCTION__, result);

	return result;
} /* ftdi_open */


static void ftdi_close (struct usb_serial_port *port, struct file *filp)
{ /* ftdi_close */
	struct usb_serial *serial = port->serial; /* Checked in usbserial.c */
	unsigned int c_cflag = port->tty->termios->c_cflag;
	char buf[1];

	dbg("%s", __FUNCTION__);

	if (serial->dev) {
		if (c_cflag & HUPCL){
			/* Disable flow control */
			if (usb_control_msg(serial->dev, 
					    usb_sndctrlpipe(serial->dev, 0),
					    FTDI_SIO_SET_FLOW_CTRL_REQUEST,
					    FTDI_SIO_SET_FLOW_CTRL_REQUEST_TYPE,
					    0, 0, buf, 0, WDR_TIMEOUT) < 0) {
				err("error from flowcontrol urb");
			}	    

			/* drop DTR */
			if (set_dtr(serial->dev, usb_sndctrlpipe(serial->dev, 0), LOW) < 0){
				err("Error from DTR LOW urb");
			}
			/* drop RTS */
			if (set_rts(serial->dev, usb_sndctrlpipe(serial->dev, 0),LOW) < 0) {
				err("Error from RTS LOW urb");
			}	
		} /* Note change no line is hupcl is off */

		/* shutdown our bulk reads and writes */
		/* ***CHECK*** behaviour when there is nothing queued */
		usb_unlink_urb (port->write_urb);
		usb_unlink_urb (port->read_urb);
	}
} /* ftdi_close */


  
/* The ftdi_sio requires the first byte to have:
 *  B0 1
 *  B1 0
 *  B2..7 length of message excluding byte 0
 */
static int ftdi_write (struct usb_serial_port *port, int from_user,
			   const unsigned char *buf, int count)
{ /* ftdi_write */
	struct usb_serial *serial = port->serial;
	struct ftdi_private *priv = (struct ftdi_private *)port->private;
	unsigned char *first_byte = port->write_urb->transfer_buffer;
	int data_offset ;
	int result;
	
	dbg("%s port %d, %d bytes", __FUNCTION__, port->number, count);

	if (count == 0) {
		err("write request of 0 bytes");
		return 0;
	}
	
	data_offset = priv->write_offset;
        dbg("data_offset set to %d",data_offset);

	if (port->write_urb->status == -EINPROGRESS) {
		dbg("%s - already writing", __FUNCTION__);
		return (0);
	}		

	count += data_offset;
	count = (count > port->bulk_out_size) ? port->bulk_out_size : count;

	/* Copy in the data to send */
	if (from_user) {
		if (copy_from_user(port->write_urb->transfer_buffer + data_offset,
				   buf, count - data_offset )){
			return -EFAULT;
		}
	} else {
		memcpy(port->write_urb->transfer_buffer + data_offset,
		       buf, count - data_offset );
	}  

	first_byte = port->write_urb->transfer_buffer;
	if (data_offset > 0){
		/* Write the control byte at the front of the packet*/
		*first_byte = 1 | ((count-data_offset) << 2) ; 
	}

	dbg("%s Bytes: %d, First Byte: 0x%02x", __FUNCTION__,count, first_byte[0]);
	usb_serial_debug_data (__FILE__, __FUNCTION__, count, first_byte);
		
	/* send the data out the bulk port */
	FILL_BULK_URB(port->write_urb, serial->dev, 
		      usb_sndbulkpipe(serial->dev, port->bulk_out_endpointAddress),
		      port->write_urb->transfer_buffer, count,
		      ftdi_write_bulk_callback, port);
		
	result = usb_submit_urb(port->write_urb);
	if (result) {
		err("%s - failed submitting write urb, error %d", __FUNCTION__, result);
		return 0;
	}

	dbg("%s write returning: %d", __FUNCTION__, count - data_offset);
	return (count - data_offset);
} /* ftdi_write */


static void ftdi_write_bulk_callback (struct urb *urb)
{
	struct usb_serial_port *port = (struct usb_serial_port *)urb->context;
	struct usb_serial *serial;

	dbg("%s", __FUNCTION__);

	if (port_paranoia_check (port, "ftdi_write_bulk_callback")) {
		return;
	}
	
	serial = port->serial;
	if (serial_paranoia_check (serial, "ftdi_write_bulk_callback")) {
		return;
	}
	
	if (urb->status) {
		dbg("nonzero write bulk status received: %d", urb->status);
		return;
	}
	queue_task(&port->tqueue, &tq_immediate);
	mark_bh(IMMEDIATE_BH);

	return;
} /* ftdi_write_bulk_callback */


static int ftdi_write_room( struct usb_serial_port *port )
{
	struct ftdi_private *priv = (struct ftdi_private *)port->private;
	int room;

	if ( port->write_urb->status == -EINPROGRESS) {
		/* There is a race here with the _write routines but it won't hurt */
		room = 0;
	} else { 
		room = port->bulk_out_size - priv->write_offset;
	}
	return(room);
} /* ftdi_write_room */


static void ftdi_read_bulk_callback (struct urb *urb)
{ /* ftdi_read_bulk_callback */
	struct usb_serial_port *port = (struct usb_serial_port *)urb->context;
	struct usb_serial *serial;
       	struct tty_struct *tty = port->tty ;
	struct ftdi_private *priv = (struct ftdi_private *) port->private;
	char error_flag;
       	unsigned char *data = urb->transfer_buffer;

	const int data_offset = 2;
	int i;
	int result;

	dbg("%s - port %d", __FUNCTION__, port->number);

	if (port_paranoia_check (port, "ftdi_sio_read_bulk_callback")) {
		return;
	}

	serial = port->serial;
	if (serial_paranoia_check (serial, "ftdi_sio_read_bulk_callback")) {
		return;
	}

	if (urb->status) {
		/* This will happen at close every time so it is a dbg not an err */
		dbg("nonzero read bulk status received: %d", urb->status);
		return;
	}

	if (urb->actual_length > 2) {
		usb_serial_debug_data (__FILE__, __FUNCTION__, urb->actual_length, data);
	} else {
                dbg("Just status 0o%03o0o%03o",data[0],data[1]);
        }


	/* TO DO -- check for hung up line and handle appropriately: */
	/*   send hangup  */
	/* See acm.c - you do a tty_hangup  - eg tty_hangup(tty) */
	/* if CD is dropped and the line is not CLOCAL then we should hangup */

	/* Compare new line status to the old one, signal if different */
	if (priv != NULL) {
		char new_status = data[0] & FTDI_STATUS_B0_MASK;
		if (new_status != priv->prev_status) {
			priv->diff_status |= new_status ^ priv->prev_status;
			wake_up_interruptible(&priv->delta_msr_wait);
			priv->prev_status = new_status;
		}
	}

	/* Handle errors and break */
	error_flag = TTY_NORMAL;
        /* Although the device uses a bitmask and hence can have multiple */
        /* errors on a packet - the order here sets the priority the */
        /* error is returned to the tty layer  */
	
	if ( data[1] & FTDI_RS_OE ) { 
		error_flag = TTY_OVERRUN;
                dbg("OVERRRUN error");
	}
	if ( data[1] & FTDI_RS_BI ) { 
		error_flag = TTY_BREAK;
                dbg("BREAK received");
	}
	if ( data[1] & FTDI_RS_PE ) { 
		error_flag = TTY_PARITY;
                dbg("PARITY error");
	}
	if ( data[1] & FTDI_RS_FE ) { 
		error_flag = TTY_FRAME;
                dbg("FRAMING error");
	}
	if (urb->actual_length > data_offset) {

		for (i = data_offset ; i < urb->actual_length ; ++i) {
			/* have to make sure we don't overflow the buffer
			  with tty_insert_flip_char's */
			if(tty->flip.count >= TTY_FLIPBUF_SIZE) {
				tty_flip_buffer_push(tty);
			}
			/* Note that the error flag is duplicated for 
			   every character received since we don't know
			   which character it applied to */
			tty_insert_flip_char(tty, data[i], error_flag);
		}
	  	tty_flip_buffer_push(tty);


	} 

#ifdef NOT_CORRECT_BUT_KEEPING_IT_FOR_NOW
	/* if a parity error is detected you get status packets forever
	   until a character is sent without a parity error.
	   This doesn't work well since the application receives a never
	   ending stream of bad data - even though new data hasn't been sent.
	   Therefore I (bill) have taken this out.
	   However - this might make sense for framing errors and so on 
	   so I am leaving the code in for now.
	*/
      else {
		if (error_flag != TTY_NORMAL){
			dbg("error_flag is not normal");
				/* In this case it is just status - if that is an error send a bad character */
				if(tty->flip.count >= TTY_FLIPBUF_SIZE) {
					tty_flip_buffer_push(tty);
				}
				tty_insert_flip_char(tty, 0xff, error_flag);
				tty_flip_buffer_push(tty);
		}
	}
#endif

	/* Continue trying to always read  */
	FILL_BULK_URB(port->read_urb, serial->dev, 
		      usb_rcvbulkpipe(serial->dev, port->bulk_in_endpointAddress),
		      port->read_urb->transfer_buffer, port->read_urb->transfer_buffer_length,
		      ftdi_read_bulk_callback, port);

	result = usb_submit_urb(port->read_urb);
	if (result)
		err("%s - failed resubmitting read urb, error %d", __FUNCTION__, result);

	return;
} /* ftdi_read_bulk_callback */


static void ftdi_break_ctl( struct usb_serial_port *port, int break_state )
{
	struct usb_serial *serial = port->serial;
	struct ftdi_private *priv = (struct ftdi_private *)port->private;
	__u16 urb_value = 0; 
	char buf[1];
	
	/* break_state = -1 to turn on break, and 0 to turn off break */
	/* see drivers/char/tty_io.c to see it used */
	/* last_set_data_urb_value NEVER has the break bit set in it */

	if (break_state) {
		urb_value = priv->last_set_data_urb_value | FTDI_SIO_SET_BREAK;
	} else {
		urb_value = priv->last_set_data_urb_value; 
	}

	
	if (usb_control_msg(serial->dev, usb_sndctrlpipe(serial->dev, 0),
			    FTDI_SIO_SET_DATA_REQUEST, 
			    FTDI_SIO_SET_DATA_REQUEST_TYPE,
			    urb_value , 0,
			    buf, 0, WDR_TIMEOUT) < 0) {
		err("%s FAILED to enable/disable break state (state was %d)", __FUNCTION__,break_state);
	}	   

	dbg("%s break state is %d - urb is %d", __FUNCTION__,break_state, urb_value);
	
}


/* old_termios contains the original termios settings and tty->termios contains
 * the new setting to be used
 * WARNING: set_termios calls this with old_termios in kernel space
 */

static void ftdi_set_termios (struct usb_serial_port *port, struct termios *old_termios)
{ /* ftdi_termios */
	struct usb_serial *serial = port->serial;
	unsigned int cflag = port->tty->termios->c_cflag;
	struct ftdi_private *priv = (struct ftdi_private *)port->private;	
	__u16 urb_value; /* will hold the new flags */
	char buf[1]; /* Perhaps I should dynamically alloc this? */
	
	
	dbg("%s", __FUNCTION__);


	/* NOTE These routines can get interrupted by 
	   ftdi_sio_read_bulk_callback  - need to examine what this 
           means - don't see any problems yet */
	
	/* Set number of data bits, parity, stop bits */
	
	urb_value = 0;
	urb_value |= (cflag & CSTOPB ? FTDI_SIO_SET_DATA_STOP_BITS_2 :
		      FTDI_SIO_SET_DATA_STOP_BITS_1);
	urb_value |= (cflag & PARENB ? 
		      (cflag & PARODD ? FTDI_SIO_SET_DATA_PARITY_ODD : 
		       FTDI_SIO_SET_DATA_PARITY_EVEN) :
		      FTDI_SIO_SET_DATA_PARITY_NONE);
	if (cflag & CSIZE) {
		switch (cflag & CSIZE) {
		case CS5: urb_value |= 5; dbg("Setting CS5"); break;
		case CS6: urb_value |= 6; dbg("Setting CS6"); break;
		case CS7: urb_value |= 7; dbg("Setting CS7"); break;
		case CS8: urb_value |= 8; dbg("Setting CS8"); break;
		default:
			err("CSIZE was set but not CS5-CS8");
		}
	}

	/* This is needed by the break command since it uses the same command - but is
	 *  or'ed with this value  */
	priv->last_set_data_urb_value = urb_value;
	
	if (usb_control_msg(serial->dev, usb_sndctrlpipe(serial->dev, 0),
			    FTDI_SIO_SET_DATA_REQUEST, 
			    FTDI_SIO_SET_DATA_REQUEST_TYPE,
			    urb_value , 0,
			    buf, 0, 100) < 0) {
		err("%s FAILED to set databits/stopbits/parity", __FUNCTION__);
	}	   

	/* Now do the baudrate */
	if ((cflag & CBAUD) == B0 ) {
		/* Disable flow control */
		if (usb_control_msg(serial->dev, usb_sndctrlpipe(serial->dev, 0),
				    FTDI_SIO_SET_FLOW_CTRL_REQUEST, 
				    FTDI_SIO_SET_FLOW_CTRL_REQUEST_TYPE,
				    0, 0, 
				    buf, 0, WDR_TIMEOUT) < 0) {
			err("%s error from disable flowcontrol urb", __FUNCTION__);
		}	    
		/* Drop RTS and DTR */
		if (set_dtr(serial->dev, usb_sndctrlpipe(serial->dev, 0),LOW) < 0){
			err("%s Error from DTR LOW urb", __FUNCTION__);
		}
		if (set_rts(serial->dev, usb_sndctrlpipe(serial->dev, 0),LOW) < 0){
			err("%s Error from RTS LOW urb", __FUNCTION__);
		}	
		
	} else {
		/* set the baudrate determined before */
		if (change_speed(port)) {
			err("%s urb failed to set baurdrate", __FUNCTION__);
		}
	}

	/* Set flow control */
	/* Note device also supports DTR/CD (ugh) and Xon/Xoff in hardware */
	if (cflag & CRTSCTS) {
		dbg("%s Setting to CRTSCTS flow control", __FUNCTION__);
		if (usb_control_msg(serial->dev, 
				    usb_sndctrlpipe(serial->dev, 0),
				    FTDI_SIO_SET_FLOW_CTRL_REQUEST, 
				    FTDI_SIO_SET_FLOW_CTRL_REQUEST_TYPE,
				    0 , FTDI_SIO_RTS_CTS_HS,
				    buf, 0, WDR_TIMEOUT) < 0) {
			err("urb failed to set to rts/cts flow control");
		}		
		
	} else { 
		/* CHECKME Assuming XON/XOFF handled by tty stack - not by device */
		dbg("%s Turning off hardware flow control", __FUNCTION__);
		if (usb_control_msg(serial->dev, 
				    usb_sndctrlpipe(serial->dev, 0),
				    FTDI_SIO_SET_FLOW_CTRL_REQUEST, 
				    FTDI_SIO_SET_FLOW_CTRL_REQUEST_TYPE,
				    0, 0, 
				    buf, 0, WDR_TIMEOUT) < 0) {
			err("urb failed to clear flow control");
		}				
		
	}
	return;
} /* ftdi_termios */


static int ftdi_ioctl (struct usb_serial_port *port, struct file * file, unsigned int cmd, unsigned long arg)
{
	struct usb_serial *serial = port->serial;
	struct ftdi_private *priv = (struct ftdi_private *)port->private;

	__u16 urb_value=0; /* Will hold the new flags */
	char buf[2];
	int  ret, mask;
	
	dbg("%s cmd 0x%04x", __FUNCTION__, cmd);

	/* Based on code from acm.c and others */
	switch (cmd) {

	case TIOCMGET:
		dbg("%s TIOCMGET", __FUNCTION__);
		switch (priv->chip_type) {
		case SIO:
			/* Request the status from the device */
			if ((ret = usb_control_msg(serial->dev, 
						   usb_rcvctrlpipe(serial->dev, 0),
						   FTDI_SIO_GET_MODEM_STATUS_REQUEST, 
						   FTDI_SIO_GET_MODEM_STATUS_REQUEST_TYPE,
						   0, 0, 
						   buf, 1, WDR_TIMEOUT)) < 0 ) {
				err("%s Could not get modem status of device - err: %d", __FUNCTION__,
				    ret);
				return(ret);
			}
			break;
		case FT8U232AM:
			/* the 8U232AM returns a two byte value (the sio is a 1 byte value) - in the same
			   format as the data returned from the in point */
			if ((ret = usb_control_msg(serial->dev, 
						   usb_rcvctrlpipe(serial->dev, 0),
						   FTDI_SIO_GET_MODEM_STATUS_REQUEST, 
						   FTDI_SIO_GET_MODEM_STATUS_REQUEST_TYPE,
						   0, 0, 
						   buf, 2, WDR_TIMEOUT)) < 0 ) {
				err("%s Could not get modem status of device - err: %d", __FUNCTION__,
				    ret);
				return(ret);
			}
			break;
		default:
			return -EFAULT;
			break;
		}

		return put_user((buf[0] & FTDI_SIO_DSR_MASK ? TIOCM_DSR : 0) |
				(buf[0] & FTDI_SIO_CTS_MASK ? TIOCM_CTS : 0) |
				(buf[0]  & FTDI_SIO_RI_MASK  ? TIOCM_RI  : 0) |
				(buf[0]  & FTDI_SIO_RLSD_MASK ? TIOCM_CD  : 0),
				(unsigned long *) arg);
		break;

	case TIOCMSET: /* Turns on and off the lines as specified by the mask */
		dbg("%s TIOCMSET", __FUNCTION__);
		if (get_user(mask, (unsigned long *) arg))
			return -EFAULT;
		urb_value = ((mask & TIOCM_DTR) ? HIGH : LOW);
		if (set_dtr(serial->dev, usb_sndctrlpipe(serial->dev, 0),urb_value) < 0){
			err("Error from DTR set urb (TIOCMSET)");
		}
		urb_value = ((mask & TIOCM_RTS) ? HIGH : LOW);
		if (set_rts(serial->dev, usb_sndctrlpipe(serial->dev, 0),urb_value) < 0){
			err("Error from RTS set urb (TIOCMSET)");
		}	
		break;
					
	case TIOCMBIS: /* turns on (Sets) the lines as specified by the mask */
		dbg("%s TIOCMBIS", __FUNCTION__);
 	        if (get_user(mask, (unsigned long *) arg))
			return -EFAULT;
  	        if (mask & TIOCM_DTR){
			if ((ret = set_dtr(serial->dev, 
					   usb_sndctrlpipe(serial->dev, 0),
					   HIGH)) < 0) {
				err("Urb to set DTR failed");
				return(ret);
			}
		}
		if (mask & TIOCM_RTS) {
			if ((ret = set_rts(serial->dev, 
					   usb_sndctrlpipe(serial->dev, 0),
					   HIGH)) < 0){
				err("Urb to set RTS failed");
				return(ret);
			}
		}
					break;

	case TIOCMBIC: /* turns off (Clears) the lines as specified by the mask */
		dbg("%s TIOCMBIC", __FUNCTION__);
 	        if (get_user(mask, (unsigned long *) arg))
			return -EFAULT;
  	        if (mask & TIOCM_DTR){
			if ((ret = set_dtr(serial->dev, 
					   usb_sndctrlpipe(serial->dev, 0),
					   LOW)) < 0){
				err("Urb to unset DTR failed");
				return(ret);
			}
		}	
		if (mask & TIOCM_RTS) {
			if ((ret = set_rts(serial->dev, 
					   usb_sndctrlpipe(serial->dev, 0),
					   LOW)) < 0){
				err("Urb to unset RTS failed");
				return(ret);
			}
		}
		break;

		/*
		 * I had originally implemented TCSET{A,S}{,F,W} and
		 * TCGET{A,S} here separately, however when testing I
		 * found that the higher layers actually do the termios
		 * conversions themselves and pass the call onto
		 * ftdi_sio_set_termios. 
		 *
		 */

	case TIOCGSERIAL: /* gets serial port data */
		return get_serial_info(port, (struct serial_struct *) arg);

	case TIOCSSERIAL: /* sets serial port data */
		return set_serial_info(port, (struct serial_struct *) arg);

	/*
	 * Wait for any of the 4 modem inputs (DCD,RI,DSR,CTS) to change
	 * - mask passed in arg for lines of interest
	 *   (use |'ed TIOCM_RNG/DSR/CD/CTS for masking)
	 * Caller should use TIOCGICOUNT to see which one it was.
	 *
	 * This code is borrowed from linux/drivers/char/serial.c
	 */
	case TIOCMIWAIT:
		while (priv != NULL) {
			interruptible_sleep_on(&priv->delta_msr_wait);
			/* see if a signal did it */
			if (signal_pending(current))
				return -ERESTARTSYS;
			else {
				char diff = priv->diff_status;

				if (diff == 0) {
					return -EIO; /* no change => error */
				}

				/* Consume all events */
				priv->diff_status = 0;

				/* Return 0 if caller wanted to know about these bits */
				if ( ((arg & TIOCM_RNG) && (diff & FTDI_RS0_RI)) ||
				     ((arg & TIOCM_DSR) && (diff & FTDI_RS0_DSR)) ||
				     ((arg & TIOCM_CD)  && (diff & FTDI_RS0_RLSD)) ||
				     ((arg & TIOCM_CTS) && (diff & FTDI_RS0_CTS)) ) {
					return 0;
				}
				/*
				 * Otherwise caller can't care less about what happened,
				 * and so we continue to wait for more events.
				 */
			}
		}
		/* NOTREACHED */

	default:
	  /* This is not an error - turns out the higher layers will do 
	   *  some ioctls itself (see comment above)
 	   */
		dbg("%s arg not supported - it was 0x%04x", __FUNCTION__,cmd);
		return(-ENOIOCTLCMD);
		break;
	}
	return 0;
} /* ftdi_ioctl */


static int __init ftdi_init (void)
{
	dbg("%s", __FUNCTION__);
	usb_serial_register (&ftdi_SIO_device);
	usb_serial_register (&ftdi_8U232AM_device);
	info(DRIVER_VERSION ":" DRIVER_DESC);
	return 0;
}


static void __exit ftdi_exit (void)
{
	dbg("%s", __FUNCTION__);
	usb_serial_deregister (&ftdi_SIO_device);
	usb_serial_deregister (&ftdi_8U232AM_device);
}


module_init(ftdi_init);
module_exit(ftdi_exit);

MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");

MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug, "Debug enabled or not");

