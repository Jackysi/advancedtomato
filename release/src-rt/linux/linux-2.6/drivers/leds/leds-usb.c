/*

	GPIO-controlled USB LED Driver
	Copyright (C) 2010 Fedor Kozhevnikov

	Licensed under GNU GPL v2.

*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/ctype.h>
#include "leds.h"

#include <typedefs.h>
#include <bcmutils.h>
#include <siutils.h>

static si_t *sih;
static unsigned int gpio_pin = 255;
static unsigned int reverse = 0;

static ssize_t gpio_pin_show(struct class_device *dev, char *buf)
{
	int pin = reverse ? gpio_pin : (gpio_pin ? -gpio_pin : -99);

	sprintf(buf, "%d\n", pin);
	return (strlen(buf) + 1);
}

static ssize_t gpio_pin_store(struct class_device *dev, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = class_get_devdata(dev);
	int ret = -EINVAL;
	char *after;
	int value = simple_strtol(buf, &after, 0);
	size_t count = after - buf;

	if (*after && isspace(*after))
		count++;

	if (count == size) {
		write_lock(&led_cdev->trigger_lock);
		reverse = (value >= 0);
		gpio_pin = reverse ? value : (value == -99 ? 0 : -value);
		write_unlock(&led_cdev->trigger_lock);
		ret = count;
	}

	return ret;
}

static CLASS_DEVICE_ATTR(gpio_pin, 0644, gpio_pin_show,
			gpio_pin_store);

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

static struct led_classdev usb_led = {
       .name = "usb-led",
       .brightness_set = usb_led_set,
       .default_trigger = "usbdev",
};

static int __init init(void)
{
	int rc;

	if (!(sih = si_kattach(SI_OSH)))
		return -ENODEV;

	rc = led_classdev_register(NULL, &usb_led);
	if (rc) return rc;

	rc = class_device_create_file(usb_led.class_dev,
				      &class_device_attr_gpio_pin);
	if (rc) led_classdev_unregister(&usb_led);

	return rc;
}

static void __exit fini(void)
{
	class_device_remove_file(usb_led.class_dev,
				&class_device_attr_gpio_pin);
	led_classdev_unregister(&usb_led);
}

module_init(init);
module_exit(fini);

MODULE_LICENSE("GPL");
