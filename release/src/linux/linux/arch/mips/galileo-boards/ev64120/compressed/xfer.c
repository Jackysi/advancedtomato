/*
 *  arch/mips/galileo/compressed/xfer.c
 *
 *  By RidgeRun Inc,
 *
 *  Xfer an image from flash to ram.
 *  For use with Galileo EVB64120A MIPS eval board.
 */

#include "linux/serial_reg.h"

#define port 0xbd000000
#define inb(addr) (*(volatile unsigned char *) ((unsigned long)(addr)))
#define outb(b,addr) (*(volatile unsigned char *) ((unsigned long)(addr)) = (b))

#ifdef RUNNINGFROMFLASH
// This is where our image of interest is mapped to
// when the jumbers are set for booting out of flash.
// (flash part starts at address 0xbfC00000)
#define srcAddr 0xbfC20000
#else
// This is where our image of interest is mapped to
// when the jumbers are set for booting out of eprom.
// (flash part starts at address 0xbf000000)
#define srcAddr 0xbf020000
#endif

static int PortAddress(unsigned int channel, unsigned char reg);
static void inline cons_hook(void);
static void print_message(const char *string);

/******************************
 Routine:
 Description:
 ******************************/
void XferToRam(void)
{
	unsigned int temp;
	void (*entry_point) (void);

	cons_hook();

	print_message("Copying image from Flash to Ram.\n");
	for (temp = 0; temp < (0x100000 - 0x20000); temp = temp + 4) {
		*(volatile unsigned int *) (temp + 0xa0400000) =
		    *(volatile unsigned int *) (temp + srcAddr);
		if (*(volatile unsigned int *) (temp + 0xa0400000) !=
		    *(volatile unsigned int *) (temp + srcAddr)) {
			print_message
			    ("Error!: copy verification failed.\n");
			break;
		}
	}

	print_message("Now jumping to the code just xferred to ram.\n");
	entry_point = (void *) 0x80400000;
	entry_point();
}

/******************************
 Routine:
 Description:
 ******************************/
static int PortAddress(unsigned int channel, unsigned char reg)
{
	unsigned int channelOffset = 0x20;
	unsigned int regDelta = 4;
	return (port + (channel * channelOffset) + (reg * regDelta));
}

/******************************
 Routine:
 Description:
 ******************************/
static void cons_hook(void)
{
	register int comstat;
	unsigned temp;
	unsigned int channel = 1;	// Channel 1 is the main serial
	// connector of the EVB64120A. Channel 0
	// is the secondary serial port (typically
	// the unsoldered connector of the board).

	temp = *(unsigned int *) 0xb4000464;
	*(unsigned int *) 0xb4000464 = 0xffff4f14;

	// Set Baud Rate, baud=115K
	outb(0x83, PortAddress(channel, UART_LCR));
	outb(0x00, PortAddress(channel, UART_DLM));
	outb(0x02, PortAddress(channel, UART_DLL));
	outb(0x03, PortAddress(channel, UART_LCR));

	comstat = inb(PortAddress(channel, UART_LSR));
	comstat = inb(PortAddress(channel, UART_RX));
	outb(0x00, PortAddress(channel, UART_IER));
}

/******************************
 Routine:
 Description:
 ******************************/
static void print_message(const char *string)
{
	register int count, loop;
	/* Display Opening Message */
	for (count = 0; string[count]; count++) {
		if (string[count] == '\n') {
			*(char *) 0xbd000020 = '\r';
		}
		*(char *) 0xbd000020 = string[count];
		for (loop = 0; loop < 2000; loop++) {
		}
	}
}
