/*  NS16550 UART registers  */

#ifndef NS16550H
#define NS16550H

#define UART_BASE		0xbd000000
#define NS16550_CHANA   PHYS_TO_K1(UART_BASE + 0x20)
#define NS16550_CHANB   PHYS_TO_K1(UART_BASE)

#ifndef NS16550_HZ
#define NS16550_HZ      3686400
#endif

#ifdef __ASSEMBLY__

#ifndef NSREG
#define NSREG(x) ((x)*4)
#endif

#define DATA	NSREG(0)	/* data register (R/W) */
#define IER	    NSREG(1)	/* interrupt enable (W) */
#define IIR	    NSREG(2)	/* interrupt identification (R) */
#define	FIFO	IIR		/* 16550 fifo control (W) */
#define CFCR	NSREG(3)	/* line control register (R/W) */
#define MCR	    NSREG(4)	/* modem control register (R/W) */
#define LSR	    NSREG(5)	/* line status register (R/W) */
#define MSR	    NSREG(6)	/* modem status register (R/W) */
#define SCR	    NSREG(7)	/* scratch register (R/W) */

#else

#ifndef nsreg
#if #endian(little)
#define nsreg(x)	unsigned :24; unsigned char x;
#else
/*#define nsreg(x)	unsigned char x; unsigned :24;*/
#define nsreg(x) unsigned int x:8;unsigned int :24;
#endif
#endif

typedef struct {
	nsreg(data);		/* data register (R/W) */
	nsreg(ier);		/* interrupt enable (W) */
	nsreg(iir);		/* interrupt identification (R) */
#define	fifo	iir		/* 16550 fifo control (W) */
	nsreg(cfcr);		/* line control register (R/W) */
	nsreg(mcr);		/* modem control register (R/W) */
	nsreg(lsr);		/* line status register (R/W) */
	nsreg(msr);		/* modem status register (R/W) */
	nsreg(scr);		/* scratch register (R/W) */
} ns16550dev;
#endif


/* 16 bit baud rate divisor (lower byte in dca_data, upper in dca_ier) */
#define BRTC(x)         (NS16550_HZ / (16*(x)))

/* interrupt enable register */
#define IER_ERXRDY      0x1	/* int on rx ready */
#define IER_ETXRDY      0x2	/* int on tx ready */
#define IER_ERLS        0x4	/* int on line status change */
#define IER_EMSC        0x8	/* int on modem status change */

/* interrupt identification register */
#define IIR_IMASK       0xf	/* mask */
#define IIR_RXTOUT      0xc	/* receive timeout */
#define IIR_RLS         0x6	/* receive line status */
#define IIR_RXRDY       0x4	/* receive ready */
#define IIR_TXRDY       0x2	/* transmit ready */
#define IIR_NOPEND      0x1	/* nothing */
#define IIR_MLSC        0x0	/* modem status */
#define IIR_FIFO_MASK   0xc0	/* set if FIFOs are enabled */

/* fifo control register */
#define FIFO_ENABLE     0x01	/* enable fifo */
#define FIFO_RCV_RST    0x02	/* reset receive fifo */
#define FIFO_XMT_RST    0x04	/* reset transmit fifo */
#define FIFO_DMA_MODE   0x08	/* enable dma mode */
#define FIFO_TRIGGER_1  0x00	/* trigger at 1 char */
#define FIFO_TRIGGER_4  0x40	/* trigger at 4 chars */
#define FIFO_TRIGGER_8  0x80	/* trigger at 8 chars */
#define FIFO_TRIGGER_14 0xc0	/* trigger at 14 chars */

/* character format control register */
#define CFCR_DLAB       0x80	/* divisor latch */
#define CFCR_SBREAK     0x40	/* send break */
#define CFCR_PZERO      0x30	/* zero parity */
#define CFCR_PONE       0x20	/* one parity */
#define CFCR_PEVEN      0x10	/* even parity */
#define CFCR_PODD       0x00	/* odd parity */
#define CFCR_PENAB      0x08	/* parity enable */
#define CFCR_STOPB      0x04	/* 2 stop bits */
#define CFCR_8BITS      0x03	/* 8 data bits */
#define CFCR_7BITS      0x02	/* 7 data bits */
#define CFCR_6BITS      0x01	/* 6 data bits */
#define CFCR_5BITS      0x00	/* 5 data bits */

/* modem control register */
#define MCR_LOOPBACK    0x10	/* loopback */
#define MCR_IENABLE     0x08	/* output 2 = int enable */
#define MCR_DRS         0x04	
#define MCR_RTS         0x02	/* enable RTS */
#define MCR_DTR         0x01	/* enable DTR */

/* line status register */
#define LSR_RCV_FIFO    0x80	/* error in receive fifo */
#define LSR_TSRE        0x40	/* transmitter empty */
#define LSR_TXRDY       0x20	/* transmitter ready */
#define LSR_BI          0x10	/* break detected */
#define LSR_FE          0x08	/* framing error */
#define LSR_PE          0x04	/* parity error */
#define LSR_OE          0x02	/* overrun error */
#define LSR_RXRDY       0x01	/* receiver ready */
#define LSR_RCV_MASK    0x1f

/* modem status register */
#define MSR_DCD         0x80	/* DCD active */
#define MSR_RI          0x40	/* RI  active */
#define MSR_DSR         0x20	/* DSR active */
#define MSR_CTS         0x10	/* CTS active */
#define MSR_DDCD        0x08	/* DCD changed */
#define MSR_TERI        0x04	/* RI  changed */
#define MSR_DDSR        0x02	/* DSR changed */
#define MSR_DCTS        0x01	/* CTS changed */

#endif
