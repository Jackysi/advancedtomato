/*
 * RNG driver for AMD Geode RNGs
 *
 * Copyright 2008 Willy Tarreau <w@1wt.eu>
 *
 * Inspired by drivers/char/hw_random/geode-rng.c from kernel 2.6.25.
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <asm/io.h>

/* We read the random generator every 50 ms */
#define RNG_INTERVAL (HZ/20+1)

#define GEODE_RNG_DATA_REG   0x50
#define GEODE_RNG_STATUS_REG 0x54

static struct timer_list timer;
static void __iomem *rng_addr;
static u32 rng_data[2];
static int nb_data;

static u32 geode_rng_data_read(void)
{
	return readl(rng_addr + GEODE_RNG_DATA_REG);
}

static int geode_rng_data_present(void)
{
	return !!(readl(rng_addr + GEODE_RNG_STATUS_REG));
}

static void geode_rng_timer(unsigned long data)
{
	if (!geode_rng_data_present())
		goto out;

	rng_data[nb_data] = geode_rng_data_read();
	nb_data++;
	if (nb_data > 1) {
		nb_data = 0;
		/* We have collected 64 bits. Maybe we should reduce the
		 * announced entropy ? At least, check for changing data
		 * and refuse to feed consts.
		 */
		if (rng_data[0] != rng_data[1])
			batch_entropy_store(rng_data[0], rng_data[1], 64);
	}
 out:
	timer.expires = jiffies + RNG_INTERVAL;
	add_timer(&timer);
}

static int __init geode_rng_init(void)
{
	struct pci_dev *pdev = NULL;
	unsigned long rng_base;
	int err = -ENODEV;

	pdev = pci_find_device(PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_LX_AES, NULL);

	if (pdev == NULL) {
		printk(KERN_ERR "geode-rng: AMD Geode RNG device not found\n");
		goto out;
	}

	if ((err = pci_enable_device(pdev)))
		goto out;

	if ((err = pci_enable_device_bars(pdev, 1)))
		goto out;

	rng_base = pci_resource_start(pdev, 0);
	if (rng_base == 0)
		goto out;

	err = -ENOMEM;
	rng_addr = ioremap(rng_base, 0x58);
	if (!rng_addr)
		goto out;

	printk(KERN_INFO "AMD Geode RNG detected and enabled\n");

	init_timer(&timer);
	timer.function = geode_rng_timer;
	timer.data = 0;
	timer.expires = jiffies + RNG_INTERVAL;
	add_timer(&timer);
	err = 0;
out:
	return err;
}

static void __exit geode_rng_exit(void)
{
	del_timer_sync(&timer);
	iounmap(rng_addr);
}

module_init(geode_rng_init);
module_exit(geode_rng_exit);

MODULE_AUTHOR("Willy Tarreau");
MODULE_LICENSE("GPL");
