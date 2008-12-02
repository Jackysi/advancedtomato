/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1997, 1998, 2001 by Ralf Baechle
 */
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/notifier.h>
#include <linux/timer.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/reboot.h>
#include <asm/ds1286.h>
#include <asm/sgialib.h>
#include <asm/sgi/sgihpc.h>
#include <asm/sgi/sgint23.h>

/*
 * Just powerdown if init hasn't done after POWERDOWN_TIMEOUT seconds.
 * I'm not sure if this feature is a good idea, for now it's here just to
 * make the power button make behave just like under IRIX.
 */
#define POWERDOWN_TIMEOUT	120

/*
 * Blink frequency during reboot grace period and when paniced.
 */
#define POWERDOWN_FREQ		(HZ / 4)
#define PANIC_FREQ		(HZ / 8)

static unsigned char sgi_volume;

static struct timer_list power_timer, blink_timer, debounce_timer, volume_timer;
static int shuting_down = 0, has_paniced = 0, setup_done = 0;

static void sgi_machine_restart(char *command) __attribute__((noreturn));
static void sgi_machine_halt(void) __attribute__((noreturn));
static void sgi_machine_power_off(void) __attribute__((noreturn));

static void sgi_machine_restart(char *command)
{
	if (shuting_down)
		sgi_machine_power_off();
	ArcReboot();
}

static void sgi_machine_halt(void)
{
	if (shuting_down)
		sgi_machine_power_off();
	ArcEnterInteractiveMode();
}

static void sgi_machine_power_off(void)
{
	unsigned char val;

	cli();

	/* Disable watchdog */
	val = CMOS_READ(RTC_CMD);
	CMOS_WRITE(val|RTC_WAM, RTC_CMD);
	CMOS_WRITE(0, RTC_WSEC);
	CMOS_WRITE(0, RTC_WHSEC);

	while(1) {
		hpc3mregs->panel=0xfe;
		/* Good bye cruel world ...  */

		/* If we're still running, we probably got sent an alarm
		   interrupt.  Read the flag to clear it.  */
		val = CMOS_READ(RTC_HOURS_ALARM);
	}
}

static void power_timeout(unsigned long data)
{
	sgi_machine_power_off();
}

static void blink_timeout(unsigned long data)
{
	sgi_hpc_write1 ^= (HPC3_WRITE1_LC0OFF|HPC3_WRITE1_LC1OFF);
	hpc3mregs->write1 = sgi_hpc_write1;

	mod_timer(&blink_timer, jiffies+data);
}

static void debounce(unsigned long data)
{
	del_timer(&debounce_timer);
	if (ioc_icontrol->istat1 & 2) { /* Interrupt still being sent.  */
		debounce_timer.expires = jiffies + 5; /* 0.05s  */
		add_timer(&debounce_timer);

		hpc3mregs->panel = 0xf3;

		return;
	}

	if (has_paniced)
		ArcReboot();

	enable_irq(SGI_PANEL_IRQ);
}

static inline void power_button(void)
{
	if (has_paniced)
		return;

	if (shuting_down || kill_proc(1, SIGINT, 1)) {
		/* No init process or button pressed twice.  */
		sgi_machine_power_off();
	}

	shuting_down = 1;
	blink_timer.data = POWERDOWN_FREQ;
	blink_timeout(POWERDOWN_FREQ);

	init_timer(&power_timer);
	power_timer.function = power_timeout;
	power_timer.expires = jiffies + POWERDOWN_TIMEOUT * HZ;
	add_timer(&power_timer);
}

void inline sgi_volume_set(unsigned char volume)
{
	sgi_volume = volume;

	hpc3c0->pbus_extregs[2][0] = sgi_volume;
	hpc3c0->pbus_extregs[2][1] = sgi_volume;
}

void inline sgi_volume_get(unsigned char *volume)
{
	*volume = sgi_volume;
}

static inline void volume_up_button(unsigned long data)
{
	del_timer(&volume_timer);

	if (sgi_volume < 0xff)
		sgi_volume++;

	hpc3c0->pbus_extregs[2][0] = sgi_volume;
	hpc3c0->pbus_extregs[2][1] = sgi_volume;

	if (ioc_icontrol->istat1 & 2) {
		volume_timer.expires = jiffies + 1;
		add_timer(&volume_timer);
	}

}

static inline void volume_down_button(unsigned long data)
{
	del_timer(&volume_timer);

	if (sgi_volume > 0)
		sgi_volume--;

	hpc3c0->pbus_extregs[2][0] = sgi_volume;
	hpc3c0->pbus_extregs[2][1] = sgi_volume;

	if (ioc_icontrol->istat1 & 2) {
		volume_timer.expires = jiffies + 1;
		add_timer(&volume_timer);
	}
}

static void panel_int(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int buttons;

	buttons = hpc3mregs->panel;
	hpc3mregs->panel = 0x03;	/* power_interrupt | power_supply_on */

	if (ioc_icontrol->istat1 & 2) { /* Wait until interrupt goes away */
		disable_irq(SGI_PANEL_IRQ);
		init_timer(&debounce_timer);
		debounce_timer.function = debounce;
		debounce_timer.expires = jiffies + 5;
		add_timer(&debounce_timer);
	}

	if (!(buttons & 0x02))		/* Power button was pressed */
		power_button();
	if (!(buttons & 0x40)) {	/* Volume up button was pressed */
		init_timer(&volume_timer);
		volume_timer.function = volume_up_button;
		volume_timer.expires = jiffies + 1;
		add_timer(&volume_timer);
	}
	if (!(buttons & 0x10)) {	/* Volume down button was pressed */
		init_timer(&volume_timer);
		volume_timer.function = volume_down_button;
		volume_timer.expires = jiffies + 1;
		add_timer(&volume_timer);
	}
}

static int panic_event(struct notifier_block *this, unsigned long event,
                      void *ptr)
{
	if (has_paniced)
		return NOTIFY_DONE;
	has_paniced = 1;

	blink_timer.data = PANIC_FREQ;
	blink_timeout(PANIC_FREQ);

	return NOTIFY_DONE;
}

static struct notifier_block panic_block = {
	.notifier_call	= panic_event,
};

void indy_reboot_setup(void)
{
	if (setup_done)
		return;
	setup_done = 1;

	_machine_restart = sgi_machine_restart;
	_machine_halt = sgi_machine_halt;
	_machine_power_off = sgi_machine_power_off;

	request_irq(SGI_PANEL_IRQ, panel_int, 0, "Front Panel", NULL);
	init_timer(&blink_timer);
	blink_timer.function = blink_timeout;
	notifier_chain_register(&panic_notifier_list, &panic_block);
}
