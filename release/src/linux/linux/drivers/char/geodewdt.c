/* Watchdog timer for the Geode GX/LX
 *
 * Copyright (C) 2006, Advanced Micro Devices, Inc.
 * Backported to 2.4 by Willy Tarreau <w@1wt.eu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */


#include <linux/module.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/fs.h>
#include <linux/notifier.h>
#include <linux/reboot.h>

#include <asm/uaccess.h>
#include <asm/geode-mfgpt.h>

#define GEODEWDT_HZ 500
#define GEODEWDT_SCALE 6
#define GEODEWDT_MAX_SECONDS 131

#define WDT_FLAGS_OPEN 1
#define WDT_FLAGS_ORPHAN 2

/* The defaults for the other timers are 60, so we'll use that too */

static int cur_interval = 60;
MODULE_PARM(cur_interval, "i");
MODULE_PARM_DESC(cur_interval, "Watchdog interval in seconds. 1<= interval <=131, default=60.");

static int wdt_timer;
static unsigned long wdt_flags;
static int safe_close;

static void geodewdt_ping(void)
{
	//printk("PING\n");
	geode_mfgpt_write(wdt_timer, MFGPT_REG_SETUP, 0);
	geode_mfgpt_write(wdt_timer, MFGPT_REG_COUNTER, 0);
	geode_mfgpt_write(wdt_timer, MFGPT_REG_SETUP, MFGPT_SETUP_CNTEN);
}

static void geodewdt_stop(void)
{
	//printk("STOP\n");
	geode_mfgpt_write(wdt_timer, MFGPT_REG_SETUP, 0);
	geode_mfgpt_write(wdt_timer, MFGPT_REG_COUNTER, 0);
}

static int geodewdt_set_heartbeat(int val)
{
	if (val < 1 || val > GEODEWDT_MAX_SECONDS)
		return -EINVAL;

	geode_mfgpt_write(wdt_timer, MFGPT_REG_SETUP, 0);
	geode_mfgpt_write(wdt_timer, MFGPT_REG_CMP2, val * GEODEWDT_HZ);
	geode_mfgpt_write(wdt_timer, MFGPT_REG_COUNTER, 0);
	geode_mfgpt_write(wdt_timer, MFGPT_REG_SETUP, MFGPT_SETUP_CNTEN);

	//printk("HEARTBEAT %d\n", val);
	cur_interval = val;
	return 0;
}

static int
geodewdt_open(struct inode *inode, struct file *file)
{
	if (MINOR(inode->i_rdev) != WATCHDOG_MINOR)
		return -ENODEV;

        if (test_and_set_bit(WDT_FLAGS_OPEN, &wdt_flags))
                return -EBUSY;

        if (!test_and_clear_bit(WDT_FLAGS_ORPHAN, &wdt_flags))
                MOD_INC_USE_COUNT;

	geodewdt_ping();
        return 0;
}

static int
geodewdt_release(struct inode *inode, struct file *file)
{
	if (MINOR(inode->i_rdev) != WATCHDOG_MINOR)
		return 0;

	if (safe_close) {
		geodewdt_stop();
		MOD_DEC_USE_COUNT;
	}
	else {
		printk(KERN_CRIT "Unexpected close - watchdog is not stopping.\n");
		geodewdt_ping();

		set_bit(WDT_FLAGS_ORPHAN, &wdt_flags);
	}

	clear_bit(WDT_FLAGS_OPEN, &wdt_flags);
	safe_close = 0;
	return 0;
}

static ssize_t
geodewdt_write(struct file *file, const char __user *data, size_t len,
	       loff_t *ppos)
{
        if(len) {
		size_t i;
                safe_close = 0;

                for (i = 0; i != len; i++) {
			char c;

			if (get_user(c, data + i))
				return -EFAULT;

			if (c == 'V')
				safe_close = 1;
		}
	}

	geodewdt_ping();
	return len;
}

static int
geodewdt_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	       unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	int interval;

	static struct watchdog_info ident = {
		.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING
		| WDIOF_MAGICCLOSE,
		.firmware_version =     0,
		.identity =             "Geode Watchdog",
        };

	switch(cmd) {
	case WDIOC_GETSUPPORT:
		return copy_to_user(argp, &ident,
				    sizeof(ident)) ? -EFAULT : 0;
		break;

	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
		return put_user(0, p);

	case WDIOC_KEEPALIVE:
		geodewdt_ping();
		return 0;

	case WDIOC_SETTIMEOUT:
		if (get_user(interval, p))
			return -EFAULT;

		if (geodewdt_set_heartbeat(interval))
			return -EINVAL;

/* Fall through */

	case WDIOC_GETTIMEOUT:
		return put_user(cur_interval, p);
	}

	return -ENOTTY;
}

static int geodewdt_notify_sys(struct notifier_block *this,
			       unsigned long code, void *unused)
{
        if(code==SYS_DOWN || code==SYS_HALT)
		geodewdt_stop();

        return NOTIFY_DONE;
}

static struct file_operations geodewdt_fops = {
        .owner          = THIS_MODULE,
        .llseek         = no_llseek,
        .write          = geodewdt_write,
        .ioctl          = geodewdt_ioctl,
        .open           = geodewdt_open,
        .release        = geodewdt_release,
};

static struct miscdevice geodewdt_miscdev = {
	.minor = WATCHDOG_MINOR,
	.name = "watchdog",
	.fops = &geodewdt_fops
};

static struct notifier_block geodewdt_notifier = {
	.notifier_call = geodewdt_notify_sys
};

static int __init geodewdt_init(void)
{
	int ret, timer;

	timer = geode_mfgpt_alloc_timer(MFGPT_TIMER_ANY, MFGPT_DOMAIN_ANY);

	if (timer == -1) {
		printk(KERN_ERR "geodewdt:  No timers were available\n");
		return -ENODEV;
	}

	wdt_timer = timer;

	/* Set up the timer */

	geode_mfgpt_write(wdt_timer, MFGPT_REG_SETUP,
			  GEODEWDT_SCALE | (3 << 8));

	/* Set up comparator 2 to reset when the event fires */
	geode_mfgpt_set_event(wdt_timer, MFGPT_CMP2, MFGPT_EVENT_RESET);

	/* Set up the initial timeout */

	geode_mfgpt_write(wdt_timer, MFGPT_REG_CMP2,
		cur_interval * GEODEWDT_HZ);

	ret = misc_register(&geodewdt_miscdev);
	if (ret)
		return ret;

	ret = register_reboot_notifier(&geodewdt_notifier);

	if (ret)
		misc_deregister(&geodewdt_miscdev);

	return ret;
}

static void __exit
geodewdt_exit(void)
{
	misc_deregister(&geodewdt_miscdev);
	unregister_reboot_notifier(&geodewdt_notifier);
}

module_init(geodewdt_init);
module_exit(geodewdt_exit);

MODULE_AUTHOR("Advanced Micro Devices, Inc");
MODULE_DESCRIPTION("Geode GX/LX Watchdog Driver");
MODULE_LICENSE("GPL");
