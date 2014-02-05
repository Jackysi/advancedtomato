/*

	GPIO-controlled USB LED Driver
	Copyright (C) 2010-2011 Fedor Kozhevnikov

	Licensed under GNU GPL v2.

*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include "leds.h"

#include <typedefs.h>
#include <bcmutils.h>
#include <siutils.h>

#ifndef CONFIG_PROC_FS
#error PROC_FS must be configured!
#endif

#define DEV_BUS_ID_SIZE	32

static si_t *sih;
static unsigned int gpio_pin = 255;
static unsigned int reverse = 0;

static void gpio_led_ctl(int on_off)
{
	si_gpioreserve(sih, 1 << gpio_pin, GPIO_APP_PRIORITY);
	si_gpioouten(sih, 1 << gpio_pin, 1 << gpio_pin, GPIO_APP_PRIORITY);
	si_gpioout(sih, 1 << gpio_pin, on_off << gpio_pin, GPIO_APP_PRIORITY);
}

static void usb_led_set(struct led_classdev *led_cdev, enum led_brightness brightness)
{
	if (gpio_pin < 16) {
		if (reverse) brightness = !brightness;
		gpio_led_ctl(brightness ? 1 : 0);
	}
}

static DEFINE_RWLOCK(usbdev_lock);
static LIST_HEAD(usbdev_list);

struct led_usbdev {
	struct list_head list;
	struct led_classdev usb_led;
	char dev_name[DEV_BUS_ID_SIZE];
};

#define LIST_FIND(head, cmpfn, type, args...)		\
({							\
	const struct list_head *__i, *__j = NULL;	\
							\
	read_lock_bh(&usbdev_lock);			\
	list_for_each(__i, (head))			\
		if (cmpfn((const type)__i , ## args)) {	\
			__j = __i;			\
			break;				\
		}					\
	read_unlock_bh(&usbdev_lock);			\
	(type)__j;					\
})

static void del_usbdev(struct led_usbdev *dev)
{
	led_classdev_unregister(&dev->usb_led);
	list_del(&dev->list);
	kfree(dev);
}

static void usbdev_flush(void)
{
	struct list_head *cur_item, *tmp_item;

	write_lock_bh(&usbdev_lock);
	list_for_each_safe(cur_item, tmp_item, &usbdev_list) {
		struct led_usbdev *dev = (void *)cur_item;
		del_usbdev(dev);
	}
	write_unlock_bh(&usbdev_lock);
}

static inline int usbdev_matched(const struct led_usbdev *i, const char *name)
{
    return strcmp(name, i->dev_name) == 0;
}

extern ssize_t usbdev_trig_set_name(struct led_classdev *led_cdev,
			     const char *buf,
			     size_t size);

static int add_usbdev(const char *name)
{
	struct led_usbdev *dev;

	/* Check if the device already exists */
	dev = LIST_FIND(&usbdev_list,
		usbdev_matched,
		struct led_usbdev *,
		name);

	if (dev == NULL) {
		/* create new device */
		dev = (struct led_usbdev *)kzalloc(sizeof(struct led_usbdev), GFP_ATOMIC);
		if (dev == NULL) return -1;

		INIT_LIST_HEAD(&dev->list);
		memcpy(dev->dev_name, name, strlen(name));
		dev->usb_led.name = dev->dev_name;
		dev->usb_led.brightness_set = usb_led_set;
		dev->usb_led.default_trigger = "usbdev";

		/* add to global table and register */
		write_lock_bh(&usbdev_lock);
		list_add(&dev->list, &usbdev_list);
		write_unlock_bh(&usbdev_lock);

		if (led_classdev_register(NULL, &dev->usb_led) ||
		    usbdev_trig_set_name(&dev->usb_led, name, strlen(name)) < 0) {
			del_usbdev(dev);
			return -1;
		}
	}

	return 0;
}

static void remove_usbdev(const char *name)
{
	struct led_usbdev *dev;

	dev = LIST_FIND(&usbdev_list,
		usbdev_matched,
		struct led_usbdev *,
		name);

	if (dev) del_usbdev(dev);
}

static struct proc_dir_entry *usbled_dir = NULL;

static int add_device_write(struct file *file, const char *buffer, unsigned long length, void *data)
{
	char dev_name[DEV_BUS_ID_SIZE];

	if ((length > 0) && (length < DEV_BUS_ID_SIZE)) {
		memcpy(dev_name, buffer, length);
		dev_name[length] = 0;
		return (add_usbdev(dev_name) == 0) ? length : 0;
	}
	return 0;
}

static int remove_device_write(struct file *file, const char *buffer, unsigned long length, void *data)
{
	char dev_name[DEV_BUS_ID_SIZE];

	if ((length > 0) && (length < DEV_BUS_ID_SIZE)) {
		memcpy(dev_name, buffer, length);
		dev_name[length] = 0;
		remove_usbdev(dev_name);
		return length;
	}
	return 0;
}

static int gpio_pin_write(struct file *file, const char *buffer, unsigned long length, void *data)
{
	char s[11];
	int value;

	if ((length > 0) && (length < 11)) {
		memcpy(s, buffer, length);
		s[length] = 0;

		value = simple_strtol(s, NULL, 0);
		write_lock(&usbdev_lock);
		reverse = (value >= 0);
		gpio_pin = reverse ? value : (value == -99 ? 0 : -value);
		write_unlock(&usbdev_lock);

		return length;
	}
	return 0;
}

static int gpio_pin_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int pin = reverse ? gpio_pin : (gpio_pin ? -gpio_pin : -99);
	return sprintf(page, "%d\n", pin);
}

static int __init init(void)
{
	struct proc_dir_entry *p;

	if (!(sih = si_kattach(SI_OSH)))
		return -ENODEV;

	usbled_dir = proc_mkdir("leds-usb", NULL);

	if (usbled_dir) {
		usbled_dir->owner = THIS_MODULE;

		p = create_proc_entry("add", 0200, usbled_dir);
		if (p) {
			p->owner = THIS_MODULE;
			p->write_proc = add_device_write;
		}

		p = create_proc_entry("remove", 0200, usbled_dir);
		if (p) {
			p->owner = THIS_MODULE;
			p->write_proc = remove_device_write;
		}

		p = create_proc_entry("gpio_pin", 0200, usbled_dir);
		if (p) {
			p->owner = THIS_MODULE;
			p->write_proc = gpio_pin_write;
			p->read_proc = gpio_pin_read;
		}
	}

	return (usbled_dir == NULL);
}

static void __exit fini(void)
{
	usbdev_flush();

	if (usbled_dir) {
		remove_proc_entry("gpio_pin", usbled_dir);
		remove_proc_entry("remove", usbled_dir);
		remove_proc_entry("add", usbled_dir);

		remove_proc_entry("leds-usb", NULL);
		usbled_dir = NULL;
	}
}

module_init(init);
module_exit(fini);

MODULE_LICENSE("GPL");
