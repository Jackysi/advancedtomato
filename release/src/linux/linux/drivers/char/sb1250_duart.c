/*
 * Copyright (C) 2000, 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


/* 
 * Driver support for the on-chip sb1250 dual-channel serial port,
 * running in asynchronous mode.  Also, support for doing a serial console
 * on one of those ports 
 *
 * The non-console part of this code is based heavily on the serial_21285.c
 * driver also in this directory.  See tty_driver.h for a description of some
 * of the driver functions, though it (like most of the inline code
 * documentation :) is a bit out of date.  
 */
#include <linux/config.h>
#include <linux/types.h>
#include <linux/serial.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/termios.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/errno.h>
#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/tty_flip.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/sibyte/swarm.h>
#include <asm/sibyte/sb1250_regs.h>
#include <asm/sibyte/sb1250_uart.h>
#include <asm/sibyte/sb1250_int.h>
#include <asm/sibyte/sb1250.h>
#include <asm/sibyte/64bit.h>

/* Toggle spewing of debugging output */
#undef DUART_SPEW

#define DEFAULT_CFLAGS          (CS8 | B115200)

#define SB1250_DUART_MINOR_BASE	64

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

/*
 * Still not sure what the termios structures set up here are for, 
 *  but we have to supply pointers to them to register the tty driver
 */
static struct tty_driver sb1250_duart_driver, sb1250_duart_callout_driver;
static int ref_count;
static struct tty_struct *duart_table[2];
static struct termios    *duart_termios[2];
static struct termios    *duart_termios_locked[2];

/*
 * tmp_buf is used as a temporary buffer by serial_write.  We need to
 * lock it in case the copy_from_user blocks while swapping in a page,
 * and some other program tries to do a serial write at the same time.
 * Since the lock will only come under contention when the system is
 * swapping and available memory is low, it makes sense to share one
 * buffer across all the serial ports, since it significantly saves
 * memory if large numbers of serial ports are open.
 */
static unsigned char *tmp_buf = 0;
DECLARE_MUTEX(tmp_buf_sem);

/*
 * This lock protects both the open flags for all the uart states as 
 * well as the reference count for the module
 */
static spinlock_t          open_lock = SPIN_LOCK_UNLOCKED;

/* Bit fields of flags in the flags field below */

typedef struct { 
	struct tty_struct   *tty;
	unsigned char       outp_buf[CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE];
	unsigned int        outp_head;
	unsigned int        outp_tail;
	unsigned int        outp_count;
	spinlock_t          outp_lock;
	unsigned int        outp_stopped;
	unsigned int        open;
	unsigned long       flags;
	unsigned int        last_cflags;
} uart_state_t;

static uart_state_t uart_states[2] = { [0 ... 1] = {
	tty:                0,
	outp_head:          0,
	outp_tail:          0,
	outp_lock:          SPIN_LOCK_UNLOCKED,
	outp_count:         0,
	open:               0,
	flags:              0,
	last_cflags:        0,
}};

/*
 * Inline functions local to this module 
 */


/*
 * Mask out the passed interrupt lines at the duart level.  This should be
 * called while holding the associated outp_lock.
 */
static inline void duart_mask_ints(unsigned int line, unsigned int mask)
{
	u64 tmp;
	tmp = in64(IO_SPACE_BASE | A_DUART_IMRREG(line));
	tmp &= ~mask;
	out64(tmp, IO_SPACE_BASE | A_DUART_IMRREG(line));
}

	
/* Unmask the passed interrupt lines at the duart level */
static inline void duart_unmask_ints(unsigned int line, unsigned int mask)
{
	u64 tmp;
	tmp = in64(IO_SPACE_BASE | A_DUART_IMRREG(line));
	tmp |= mask;
	out64(tmp, IO_SPACE_BASE | A_DUART_IMRREG(line));
}

static inline unsigned long get_status_reg(unsigned int line)
{
	uint64_t status;

#ifdef CONFIG_SB1_PASS_2_WORKAROUNDS
	in64(IO_SPACE_BASE | A_DUART_CHANREG(line, R_DUART_CMD));
#endif
	status = in64(IO_SPACE_BASE | A_DUART_CHANREG(line, R_DUART_STATUS));
#ifdef CONFIG_SB1_PASS_2_WORKAROUNDS
	in64(IO_SPACE_BASE | A_DUART_CHANREG(line, R_DUART_CMD));
#endif
	return status;
}

/* Derive which uart a call is for from the passed tty line.  */
static inline unsigned int get_line(struct tty_struct *tty) 
{
	unsigned int line = MINOR(tty->device) - tty->driver.minor_start;
	if (line > 1)
		printk(KERN_CRIT "Invalid line\n");

	return line;
}


/* 
 * Generic interrupt handler for both channels.  dev_id is a pointer
 * to the proper uart_states structure, so from that we can derive 
 * which port interrupted 
 */

static void duart_int(int irq, void *dev_id, struct pt_regs *regs)
{
	uart_state_t *us = (uart_state_t *)dev_id;
	unsigned int line = us - uart_states;
	struct tty_struct *tty = us->tty;

#ifdef DUART_SPEW
	printk("DUART INT\n");
#endif
	/*
	 * We could query the ISR to figure out why we are here, but since
	 * we are here, we may as well just take care of both rx and tx
	 */
	spin_lock(&us->outp_lock);
	if (get_status_reg(line) & M_DUART_RX_RDY) {
		do {
			unsigned int status = get_status_reg(line);
			unsigned int ch = in64(IO_SPACE_BASE |
				A_DUART_CHANREG(line, R_DUART_RX_HOLD));
			unsigned int flag = 0;
			if (status & 0x10)
				tty_insert_flip_char(tty, 0, TTY_OVERRUN);
			if (status & 0x20) {
				printk("Parity error!\n");
				flag = TTY_PARITY;
			} else if (status & 0x40) {
				printk("Frame error!\n");
				flag = TTY_FRAME;
			}
			tty_insert_flip_char(tty, ch, flag);
		} while (get_status_reg(line) & M_DUART_RX_RDY);
		tty_flip_buffer_push(tty);
	} 
	if ((get_status_reg(line) & M_DUART_TX_RDY) && us->outp_count) {
		do {
			out64(us->outp_buf[us->outp_head], IO_SPACE_BASE | A_DUART_CHANREG(line, R_DUART_TX_HOLD));
			us->outp_head = (us->outp_head + 1) & (CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE-1);
			us->outp_count--;
		} while ((get_status_reg(line) & M_DUART_TX_RDY) &&
		         us->outp_count);

		if (us->open &&
		    (us->outp_count < (CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE/2))) {
			/*
			 * We told the discipline at one point that we had no
			 * space, so it went to sleep.  Wake it up when we hit
			 * half empty
			 */
			if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
			     tty->ldisc.write_wakeup)
				tty->ldisc.write_wakeup(tty);
			wake_up_interruptible(&tty->write_wait);
		}
		if (!us->outp_count)
			duart_mask_ints(line, M_DUART_IMR_TX);
	}
	spin_unlock(&us->outp_lock);
}

/*
 *  Actual driver functions
 */

/* Return the number of characters we can accomodate in a write at this instant */
static int duart_write_room(struct tty_struct *tty)
{
	uart_state_t *us = (uart_state_t *) tty->driver_data;
	int retval;

	retval = CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE - us->outp_count;

#ifdef DUART_SPEW
	printk("duart_write_room called, returning %i\n", retval);
#endif

	return retval;
}

/* memcpy the data from src to destination, but take extra care if the
   data is coming from user space */
static inline int copy_buf(char *dest, const char *src, int size, int from_user) 
{
	if (from_user) {
		(void) copy_from_user(dest, src, size); 
	} else {
		memcpy(dest, src, size);
	}
	return size;
}

/*
 * Buffer up to count characters from buf to be written.  If we don't have
 * other characters buffered, enable the tx interrupt to start sending
 */
static int duart_write(struct tty_struct * tty, int from_user,
		      const unsigned char *buf, int count)
{
	uart_state_t *us = (uart_state_t *) tty->driver_data;
	unsigned int line = get_line(tty);
	int c, total = 0;
	unsigned long flags;

	if (from_user && verify_area(VERIFY_READ, buf, count)) {
		return -EINVAL;
	}
#ifdef DUART_SPEW
	printk("duart_write called for %i chars by %i (%s)\n", count, current->pid, current->comm);
#endif
	if (!count ||
	    (us->outp_count == CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE)) {
		return 0;
	}
	if (from_user) {
		down(&tmp_buf_sem);
		while (1) {
			c = MIN(count, MIN(PAGE_SIZE - us->outp_count - 1,
					   PAGE_SIZE - us->outp_tail));
			if (c <= 0)
				break;
			
			c -= copy_from_user(tmp_buf, buf, c);
			if (!c) {
				if (!total)
					total = -EFAULT;
				break;
			}
			
			spin_lock_irqsave(&us->outp_lock, flags);
			c = MIN(c, MIN(CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE - us->outp_count - 1,
				       CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE - us->outp_tail));
			memcpy(us->outp_buf + us->outp_tail, tmp_buf, c);
			us->outp_tail = (us->outp_tail + c) & (CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE-1);
			us->outp_count += c;
			spin_unlock_irqrestore(&us->outp_lock, flags);
			
			buf += c;
			count -= c;
			total += c;
		}
		up(&tmp_buf_sem);
	} else {
	    while (1) {
		    spin_lock_irqsave(&us->outp_lock, flags);
		    c = MIN(count, MIN(CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE - us->outp_count - 1,
				       CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE - us->outp_tail));
		    if (c <= 0) {
			    spin_unlock_irqrestore(&us->outp_lock, flags);
			    break;
		    }

		    memcpy(us->outp_buf + us->outp_tail, buf, c);
		    us->outp_tail = (us->outp_tail + c) & (CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE-1);
		    us->outp_count += c;
		    spin_unlock_irqrestore(&us->outp_lock, flags);

		    buf += c;
		    count -= c;
		    total += c;
	    }
	}
	/* Excessive... */
	if (us->outp_count && !us->outp_stopped) {
		spin_lock_irqsave(&us->outp_lock, flags);
		duart_unmask_ints(line, M_DUART_IMR_TX);
		spin_unlock_irqrestore(&us->outp_lock, flags);
	}

	return total;
}


/* Buffer one character to be written.  If there's not room for it, just drop
   it on the floor.  This is used for echo, among other things */
static void duart_put_char(struct tty_struct *tty, u_char ch)
{
	uart_state_t *us = (uart_state_t *) tty->driver_data;
	unsigned int line = get_line(tty);
	unsigned long flags;

#ifdef DUART_SPEW
	printk("duart_put_char called.  Char is %x (%c)\n", (int)ch, ch);
#endif
	spin_lock_irqsave(&us->outp_lock, flags);
	if (us->outp_count != CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE) {
		us->outp_buf[us->outp_tail] = ch;
		us->outp_tail = (us->outp_tail + 1) &(CONFIG_SB1250_DUART_OUTPUT_BUF_SIZE-1);
		if (!(us->outp_count || us->outp_stopped)) {
			duart_unmask_ints(line, M_DUART_IMR_TX);
		}
		us->outp_count++;
	}
	spin_unlock_irqrestore(&us->outp_lock, flags);
}

/* Return the number of characters in the output buffer that have yet to be 
   written */
static int duart_chars_in_buffer(struct tty_struct *tty)
{
	uart_state_t *us = (uart_state_t *) tty->driver_data;
	int retval;

	retval = us->outp_count;

#ifdef DUART_SPEW
	printk("duart_chars_in_buffer returning %i\n", retval);
#endif
	return retval;
}

/* Kill everything we haven't yet shoved into the FIFO.  Turn off the
   transmit interrupt since we've nothing more to transmit */
static void duart_flush_buffer(struct tty_struct *tty)
{
	uart_state_t *us = (uart_state_t *) tty->driver_data;
	unsigned int line = get_line(tty);
	unsigned long flags;

#ifdef DUART_SPEW
	printk("duart_flush_buffer called\n");
#endif
	duart_mask_ints(line, M_DUART_IMR_TX);
	spin_lock_irqsave(&us->outp_lock, flags);
	us->outp_head = us->outp_tail = us->outp_count = 0;
	spin_unlock_irqrestore(&us->outp_lock, flags);

	wake_up_interruptible(&us->tty->write_wait);

	if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
	    tty->ldisc.write_wakeup)
		tty->ldisc.write_wakeup(tty);
}


/* See sb1250 user manual for details on these registers */
static inline void duart_set_cflag(unsigned int line, unsigned int cflag)
{
	unsigned int mode_reg1 = 0, mode_reg2 = 0;
	unsigned int clk_divisor;

	switch (cflag & CSIZE) {
	case CS7:
		mode_reg1 |= V_DUART_BITS_PER_CHAR_7;
		
	default:
		/* We don't handle CS5 or CS6...is there a way we're supposed to flag this? 
		   right now we just force them to CS8 */
		mode_reg1 |= 0x0;
		break;
	}
	if (cflag & CSTOPB) {
	        mode_reg2 |= M_DUART_STOP_BIT_LEN_2;	
	}
	if (!(cflag & PARENB)) {
	        mode_reg1 |= V_DUART_PARITY_MODE_NONE;	
	}
	if (cflag & PARODD) {
		mode_reg1 |= M_DUART_PARITY_TYPE_ODD;
	}
	
	/* Formula for this is (5000000/baud)-1, but we saturate
	   at 12 bits, which means we can't actually do anything less
	   that 1200 baud */
	switch (cflag & CBAUD) {
	case B200:	
	case B300:	
	case B1200:	clk_divisor = 4095;		break;
	case B1800:	clk_divisor = 2776;		break;
	case B2400:	clk_divisor = 2082;		break;
	case B4800:	clk_divisor = 1040;		break;
	default:
	case B9600:	clk_divisor = 519;		break;
	case B19200:	clk_divisor = 259;		break;
	case B38400:	clk_divisor = 129;		break;
	case B57600:	clk_divisor = 85;		break;
	case B115200:	clk_divisor = 42;		break;
	}
	out64(mode_reg1, IO_SPACE_BASE | A_DUART_CHANREG(line, R_DUART_MODE_REG_1));
	out64(mode_reg2, IO_SPACE_BASE | A_DUART_CHANREG(line, R_DUART_MODE_REG_2));
	out64(clk_divisor, IO_SPACE_BASE | A_DUART_CHANREG(line, R_DUART_CLK_SEL));
	uart_states[line].last_cflags = cflag;
}


/* Handle notification of a termios change.  */
static void duart_set_termios(struct tty_struct *tty, struct termios *old)
{
#ifdef DUART_SPEW 
	printk("duart_set_termios called by %i (%s)\n", current->pid, current->comm);
#endif
	if (old && tty->termios->c_cflag == old->c_cflag)
		return;
	duart_set_cflag(get_line(tty), tty->termios->c_cflag);
}

/* Stop pushing stuff into the fifo, now.  Do the mask under the 
   outp_lock to avoid races involving turning the interrupt line on/off */
static void duart_stop(struct tty_struct *tty)
{
	uart_state_t *us = (uart_state_t *) tty->driver_data;
	unsigned long flags;

#ifdef DUART_SPEW
	printk("duart_stop called\n");
#endif
	spin_lock_irqsave(&us->outp_lock, flags);
	duart_mask_ints(get_line(tty), M_DUART_IMR_TX);
	us->outp_stopped = 1;
	spin_unlock_irqrestore(&us->outp_lock, flags);
}

static int duart_ioctl(struct tty_struct *tty, struct file * file,
		       unsigned int cmd, unsigned long arg)
{
/*	if (serial_paranoia_check(info, tty->device, "rs_ioctl"))
	return -ENODEV;*/
	switch (cmd) {
	case TIOCMGET:
		printk("Ignoring TIOCMGET\n");
		break;
	case TIOCMBIS:
		printk("Ignoring TIOCMBIS\n");
		break;
	case TIOCMBIC:
		printk("Ignoring TIOCMBIC\n");
		break;
	case TIOCMSET:
		printk("Ignoring TIOCMSET\n");
		break;
	case TIOCGSERIAL:
		printk("Ignoring TIOCGSERIAL\n");
		break;
	case TIOCSSERIAL:
		printk("Ignoring TIOCSSERIAL\n");
		break;
	case TIOCSERCONFIG:
		printk("Ignoring TIOCSERCONFIG\n");
		break;
	case TIOCSERGETLSR: /* Get line status register */
		printk("Ignoring TIOCSERGETLSR\n");
		break;
	case TIOCSERGSTRUCT:
		printk("Ignoring TIOCSERGSTRUCT\n");
		break;
	case TIOCMIWAIT:
		printk("Ignoring TIOCMIWAIT\n");
		break;
	case TIOCGICOUNT:
		printk("Ignoring TIOCGICOUNT\n");
		break;
	case TIOCSERGWILD:
		printk("Ignoring TIOCSERGWILD\n");
		break;
	case TIOCSERSWILD:
		printk("Ignoring TIOCSERSWILD\n");
		break;
	default:
		break;
	}
//	printk("Ignoring IOCTL %x from pid %i (%s)\n", cmd, current->pid, current->comm);
	return -ENOIOCTLCMD;
	return 0;
}

/* Stop pushing stuff into the fifo, now.  Do the mask under the 
   outp_lock to avoid races involving turning the interrupt line on/off */
static void duart_start(struct tty_struct *tty)
{
	uart_state_t *us = (uart_state_t *) tty->driver_data;
	unsigned long flags;

#ifdef DUART_SPEW
	printk("duart_start called\n");
#endif
	spin_lock_irqsave(&us->outp_lock, flags);
	if (us->outp_count) {
		duart_unmask_ints(get_line(tty), M_DUART_IMR_TX);
	}
	us->outp_stopped = 0;
	spin_unlock_irqrestore(&us->outp_lock, flags);
}

/* Not sure on the semantics of this; are we supposed to wait until the stuff
   already in the hardware FIFO drains, or are we supposed to wait until 
   we've drained the output buffer, too?  I'm assuming the former, 'cause thats
   what the other drivers seem to assume 
*/

static void duart_wait_until_sent(struct tty_struct *tty, int timeout)
{
	unsigned int line = get_line(tty);
	unsigned long orig_jiffies;

	orig_jiffies = jiffies;
#ifdef DUART_SPEW
	printk("duart_wait_until_sent(%d)+\n", timeout);
#endif
	while (!(get_status_reg(line) & M_DUART_TX_EMT)) {
		set_current_state(TASK_INTERRUPTIBLE);
	 	schedule_timeout(1);
		if (signal_pending(current))
			break;
		if (timeout && time_after(jiffies, orig_jiffies + timeout))
			break;
	}
#ifdef DUART_SPEW
	printk("duart_wait_until_sent()-\n");
#endif
}

/*
 * duart_hangup() --- called by tty_hangup() when a hangup is signaled.
 */
static void duart_hangup(struct tty_struct *tty)
{
	//uart_state_t *info = (uart_state_t *) tty->driver_data;

	//info->tty = 0;
	//wake_up_interruptible(&info->open_wait);
}

/*
 * Open a tty line.  Note that this can be called multiple times, so ->open can
 * be >1.  Only set up the tty struct if this is a "new" open, e.g. ->open was
 * zero
 */
static int duart_open(struct tty_struct *tty, struct file *filp)
{
	uart_state_t *us;
	unsigned long flags;
	unsigned int line;

	MOD_INC_USE_COUNT;
#ifndef CONFIG_SIBYTE_SB1250_DUART_NO_PORT_1
	if (get_line(tty) > 1)
#else
	if (get_line(tty) > 0)
#endif
	                      {
		MOD_DEC_USE_COUNT;
		return -ENODEV;
	}

#ifdef DUART_SPEW
	printk("duart_open called by %i (%s), tty is %p, rw is %p, ww is %p\n",
	       current->pid, current->comm, tty, tty->read_wait,
	       tty->write_wait);
#endif

	line = get_line(tty);
	us = uart_states + line;
	tty->driver_data = us;

	if (!tmp_buf) {
		tmp_buf = (unsigned char *) get_free_page(GFP_KERNEL);
		if (!tmp_buf){
			return -ENOMEM;
		}
	}

	spin_lock_irqsave(&open_lock, flags);
	if (!us->open) {
		us->tty = tty;
		us->tty->termios->c_cflag = us->last_cflags;
	}
	us->open++;
	duart_unmask_ints(line, M_DUART_IMR_RX);
	spin_unlock_irqrestore(&open_lock, flags);

	return 0;
}


/*
 * Close a reference count out.  If reference count hits zero, null the
 * tty, kill the interrupts.  The tty_io driver is responsible for making
 * sure we've cleared out our internal buffers before calling close()
 */
static void duart_close(struct tty_struct *tty, struct file *filp)
{
	uart_state_t *us = (uart_state_t *) tty->driver_data;
	unsigned long flags;

#ifdef DUART_SPEW
	printk("duart_close called by %i (%s)\n", current->pid, current->comm);
#endif

	if (!us || !us->open)
		return;

	spin_lock_irqsave(&open_lock, flags);
	us->open--;
	ref_count--;
	spin_unlock_irqrestore(&open_lock, flags);
	MOD_DEC_USE_COUNT;
}


/* Set up the driver and register it, register the 2 1250 UART interrupts.  This
   is called from tty_init, or as a part of the module init */
static int __init sb1250_duart_init(void) 
{
	sb1250_duart_driver.magic            = TTY_DRIVER_MAGIC;
	sb1250_duart_driver.driver_name      = "serial";
#ifdef CONFIG_DEVFS_FS
	sb1250_duart_driver.name             = "tts/%d";
#else
	sb1250_duart_driver.name             = "ttyS";
#endif
	sb1250_duart_driver.major            = TTY_MAJOR;
	sb1250_duart_driver.minor_start      = SB1250_DUART_MINOR_BASE;
	sb1250_duart_driver.num              = 2;
	sb1250_duart_driver.type             = TTY_DRIVER_TYPE_SERIAL;
	sb1250_duart_driver.subtype          = SERIAL_TYPE_NORMAL;
	sb1250_duart_driver.init_termios     = tty_std_termios;
	sb1250_duart_driver.flags            = TTY_DRIVER_REAL_RAW;
	sb1250_duart_driver.refcount         = &ref_count;
	sb1250_duart_driver.table            = duart_table;
	sb1250_duart_driver.termios          = duart_termios;
	sb1250_duart_driver.termios_locked   = duart_termios_locked;

	sb1250_duart_driver.open             = duart_open;
	sb1250_duart_driver.close            = duart_close;
	sb1250_duart_driver.write            = duart_write;
	sb1250_duart_driver.put_char         = duart_put_char;
	sb1250_duart_driver.write_room       = duart_write_room;
	sb1250_duart_driver.chars_in_buffer  = duart_chars_in_buffer;
	sb1250_duart_driver.flush_buffer     = duart_flush_buffer;
	sb1250_duart_driver.ioctl            = duart_ioctl;
	sb1250_duart_driver.set_termios      = duart_set_termios;
	sb1250_duart_driver.stop             = duart_stop;
	sb1250_duart_driver.start            = duart_start;
	sb1250_duart_driver.hangup           = duart_hangup;
	sb1250_duart_driver.wait_until_sent  = duart_wait_until_sent;

	sb1250_duart_callout_driver          = sb1250_duart_driver;
#ifdef CONFIG_DEVFS_FS
	sb1250_duart_callout_driver.name     = "cua/%d";
#else
	sb1250_duart_callout_driver.name     = "cua";
#endif
	sb1250_duart_callout_driver.major    = TTYAUX_MAJOR;
	sb1250_duart_callout_driver.subtype  = SERIAL_TYPE_CALLOUT;

	duart_mask_ints(0, 0xf);
	if (request_irq(K_INT_UART_0, duart_int, 0, "uart0", &uart_states[0])) {
		panic("Couldn't get uart0 interrupt line");
	}
	out64(M_DUART_RX_EN|M_DUART_TX_EN, IO_SPACE_BASE | A_DUART_CHANREG(0, R_DUART_CMD));
#ifndef CONFIG_SIBYTE_SB1250_DUART_NO_PORT_1
	duart_mask_ints(1, 0xf);
	if (request_irq(K_INT_UART_1, duart_int, 0, "uart1", &uart_states[1])) {
		panic("Couldn't get uart1 interrupt line");
	}
	out64(M_DUART_RX_EN|M_DUART_TX_EN, IO_SPACE_BASE | A_DUART_CHANREG(1, R_DUART_CMD));
#endif	

	/* Interrupts are now active, our ISR can be called. */

	if (tty_register_driver(&sb1250_duart_driver)) {
		printk(KERN_ERR "Couldn't register sb1250 duart serial driver\n");
	}
	if (tty_register_driver(&sb1250_duart_callout_driver)) {
		printk(KERN_ERR "Couldn't register sb1250 duart callout driver\n");
	}
	duart_set_cflag(0, DEFAULT_CFLAGS);
#ifndef CONFIG_SIBYTE_SB1250_DUART_NO_PORT_1
	duart_set_cflag(1, DEFAULT_CFLAGS);
#endif
	return 0;
}

/* Unload the driver.  Unregister stuff, get ready to go away */
static void __exit sb1250_duart_fini(void)
{
	unsigned long flags;
	int ret;

	save_and_cli(flags);
	ret = tty_unregister_driver(&sb1250_duart_callout_driver);
	if (ret) {
		printk(KERN_ERR "Unable to unregister sb1250 duart callout driver (%d)\n", ret);
	}
	ret = tty_unregister_driver(&sb1250_duart_driver);
	if (ret) {
		printk(KERN_ERR "Unable to unregister sb1250 duart serial driver (%d)\n", ret);
	}
	free_irq(K_INT_UART_0, &uart_states[0]);
	disable_irq(K_INT_UART_0);
#ifndef CONFIG_SIBYTE_SB1250_DUART_NO_PORT_1
	free_irq(K_INT_UART_1, &uart_states[1]);
	disable_irq(K_INT_UART_1);
#endif
	restore_flags(flags);
}

module_init(sb1250_duart_init);
module_exit(sb1250_duart_fini);
MODULE_DESCRIPTION("SB1250 Duart serial driver");
MODULE_AUTHOR("Justin Carlson <carlson@sibyte.com>");

#ifdef CONFIG_SIBYTE_SB1250_DUART_CONSOLE

/*
 * Serial console stuff.  Very basic, polling driver for doing serial
 * console output.  The console_sem is held by the caller, so we
 * shouldn't be interrupted for more console activity.
 * XXXKW What about getting interrupted by uart driver activity?
 */

static void ser_console_write(struct console *cons, const char *str,
                              unsigned int count)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
                if (str[i] == '\n') {
                        /* Expand LF -> CRLF */
                        while (!(get_status_reg(0) & M_DUART_TX_RDY)) {
                                /* Spin, doing nothing.  */
                        }
			out64('\r', IO_SPACE_BASE | A_DUART_CHANREG(0, R_DUART_TX_HOLD));
                }
		while (!(get_status_reg(0) & M_DUART_TX_RDY)) {
			/* Spin, doing nothing.  */
		}
		out64(str[i], IO_SPACE_BASE | A_DUART_CHANREG(0, R_DUART_TX_HOLD));
	}
	/*
	 * Make sure we leave room, in case the higher-level uart
         * driver expects it
	 */
	while (!(get_status_reg(0) & M_DUART_TX_RDY)) {
		/* Spin, doing nothing.  */
	}
}

static kdev_t ser_console_device(struct console *c)
{
	return MKDEV(TTY_MAJOR, SB1250_DUART_MINOR_BASE + c->index);
}

static int ser_console_setup(struct console *cons, char *str)
{
	/* Initialize the transmitter */
	
	duart_set_cflag(0, DEFAULT_CFLAGS);
	return 0;
}

static struct console sb1250_ser_cons = {
	name:		"ttyS",
	write:		ser_console_write,
	device:		ser_console_device,
	setup:		ser_console_setup,
	flags:		CON_PRINTBUFFER,
	index:		-1,
};

void __init sb1250_serial_console_init(void)
{
	register_console(&sb1250_ser_cons);
}

#endif /* CONFIG_SIBYTE_SB1250_DUART_CONSOLE */
