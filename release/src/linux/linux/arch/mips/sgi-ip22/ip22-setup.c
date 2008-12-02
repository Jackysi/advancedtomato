/*
 * ip22-setup.c: SGI specific setup, including init of the feature struct.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997, 1998 Ralf Baechle (ralf@gnu.org)
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kbd_ll.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/console.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/pc_keyb.h>

#include <asm/addrspace.h>
#include <asm/bcache.h>
#include <asm/bootinfo.h>
#include <asm/keyboard.h>
#include <asm/irq.h>
#include <asm/reboot.h>
#include <asm/ds1286.h>
#include <asm/sgialib.h>
#include <asm/sgi/sgimc.h>
#include <asm/sgi/sgihpc.h>
#include <asm/sgi/sgint23.h>
#include <asm/time.h>
#include <asm/gdb-stub.h>
#include <asm/io.h>
#include <asm/traps.h>

#ifdef CONFIG_REMOTE_DEBUG
extern void rs_kgdb_hook(int);
extern void breakpoint(void);
static int remote_debug = 0;
#endif

#if defined(CONFIG_SERIAL_CONSOLE) || defined(CONFIG_ARC_CONSOLE)
extern void console_setup(char *);
#endif

extern void sgitime_init(void);

extern struct hpc3_miscregs *hpc3mregs;
extern struct rtc_ops indy_rtc_ops;
extern void indy_reboot_setup(void);
extern void sgi_volume_set(unsigned char);

#define sgi_kh ((struct hpc_keyb *) &(hpc3mregs->kbdmouse0))

#define KBD_STAT_IBF		0x02	/* Keyboard input buffer full */

unsigned long sgi_gfxaddr;

static void sgi_request_region(void)
{
	/* No I/O ports are being used on the Indy.  */
}

static int sgi_request_irq(void (*handler)(int, void *, struct pt_regs *))
{
	/* Dirty hack, this get's called as a callback from the keyboard
	 * driver.  We piggyback the initialization of the front panel
	 * button handling on it even though they're technically not
	 * related with the keyboard driver in any way.  Doing it from
	 * ip22_setup wouldn't work since kmalloc isn't initialized yet.
	 */
	indy_reboot_setup();

	return request_irq(SGI_KEYBD_IRQ, handler, 0, "keyboard", NULL);
}

static int sgi_aux_request_irq(void (*handler)(int, void *, struct pt_regs *))
{
	/* Nothing to do, interrupt is shared with the keyboard hw  */
	return 0;
}

static void sgi_aux_free_irq(void)
{
	/* Nothing to do, interrupt is shared with the keyboard hw  */
}

static unsigned char sgi_read_input(void)
{
	return sgi_kh->data;
}

static void sgi_write_output(unsigned char val)
{
	int status;

	do {
		status = sgi_kh->command;
	} while (status & KBD_STAT_IBF);
	sgi_kh->data = val;
}

static void sgi_write_command(unsigned char val)
{
	int status;

	do {
		status = sgi_kh->command;
	} while (status & KBD_STAT_IBF);
	sgi_kh->command = val;
}

static unsigned char sgi_read_status(void)
{
	return sgi_kh->command;
}

struct kbd_ops sgi_kbd_ops = {
	sgi_request_region,
	sgi_request_irq,

	sgi_aux_request_irq,
	sgi_aux_free_irq,

	sgi_read_input,
	sgi_write_output,
	sgi_write_command,
	sgi_read_status
};

void __init ip22_setup(void)
{
	char *ctype;
#ifdef CONFIG_REMOTE_DEBUG
	char *kgdb_ttyd;
#endif
	sgitime_init();
	/* Init the INDY HPC I/O controller.  Need to call this before
	 * fucking with the memory controller because it needs to know the
	 * boardID and whether this is a Guiness or a FullHouse machine.
	 */
	sgihpc_init();

	/* Init INDY memory controller. */
	sgimc_init();

#ifdef CONFIG_BOARD_SCACHE
	/* Now enable boardcaches, if any. */
	indy_sc_init();
#endif
	conswitchp = NULL;

	/* Set the IO space to some sane value */
	set_io_port_base (KSEG1ADDR (0x00080000));

	/* ARCS console environment variable is set to "g?" for
	 * graphics console, it is set to "d" for the first serial
	 * line and "d2" for the second serial line.
	 */
	ctype = ArcGetEnvironmentVariable("console");
	if (ctype && *ctype == 'd') {
#ifdef CONFIG_SERIAL_CONSOLE
		if(*(ctype + 1) == '2')
			console_setup("ttyS1");
		else
			console_setup("ttyS0");
#endif
	}
#ifdef CONFIG_ARC_CONSOLE
	else if (!ctype || *ctype != 'g') {
		/* Use ARC if we don't want serial ('d') or
		 * Newport ('g'). */
		prom_flags |= PROM_FLAG_USE_AS_CONSOLE;
		console_setup("arc");
	}
#endif

#ifdef CONFIG_REMOTE_DEBUG
	kgdb_ttyd = prom_getcmdline();
	if ((kgdb_ttyd = strstr(kgdb_ttyd, "kgdb=ttyd")) != NULL) {
		int line;
		kgdb_ttyd += strlen("kgdb=ttyd");
		if (*kgdb_ttyd != '1' && *kgdb_ttyd != '2')
			printk("KGDB: Uknown serial line /dev/ttyd%c, "
			       "falling back to /dev/ttyd1\n", *kgdb_ttyd);
		line = *kgdb_ttyd == '2' ? 0 : 1;
		printk("KGDB: Using serial line /dev/ttyd%d for session\n",
		       line ? 1 : 2);
		rs_kgdb_hook(line);

		printk("KGDB: Using serial line /dev/ttyd%d for session, "
			    "please connect your debugger\n", line ? 1 : 2);

		remote_debug = 1;
		/* Breakpoints and stuff are in sgi_irq_setup() */
	}
#endif

	sgi_volume_set(simple_strtoul(ArcGetEnvironmentVariable("volume"), NULL, 10));

#ifdef CONFIG_VT
#ifdef CONFIG_SGI_NEWPORT_CONSOLE
	if (ctype && *ctype == 'g'){
		unsigned long *gfxinfo;
		long (*__vec)(void) = (void *) *(long *)((PROMBLOCK)->pvector + 0x20);

		gfxinfo = (unsigned long *)__vec();
		sgi_gfxaddr = ((gfxinfo[1] >= 0xa0000000
			       && gfxinfo[1] <= 0xc0000000)
			       ? gfxinfo[1] - 0xa0000000 : 0);

		/* newport addresses? */
		if (sgi_gfxaddr == 0x1f0f0000 || sgi_gfxaddr == 0x1f4f0000) {
			conswitchp = &newport_con;

			screen_info = (struct screen_info) {
				0, 0,		/* orig-x, orig-y */
				0,		/* unused */
				0,		/* orig_video_page */
				0,		/* orig_video_mode */
				160,		/* orig_video_cols */
				0, 0, 0,	/* unused, ega_bx, unused */
				64,		/* orig_video_lines */
				0,		/* orig_video_isVGA */
				16		/* orig_video_points */
			};
		}
	}
#endif
#ifdef CONFIG_DUMMY_CONSOLE
	/* Either if newport console wasn't used or failed to initialize. */
	if(conswitchp != &newport_con)
		conswitchp = &dummy_con;
#endif
#endif

	rtc_ops = &indy_rtc_ops;
	kbd_ops = &sgi_kbd_ops;
#ifdef CONFIG_PSMOUSE
	aux_device_present = 0xaa;
#endif
}
