/*
 * Copyright (C) 2013, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * Functions to read/write GPIO pins
 *
 * $Id: bcmgpio.c 241182 2011-02-17 21:50:03Z $
 */

#include <typedefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <sys/ioctl.h>

#include <bcmnvram.h>
#include <bcmgpio.h>

#if defined(__ECOS)
/* directly access the gpio registers */
#include <bcmutils.h>
#include <siutils.h>
static void *gpio_sih;
#else
#include <linux_gpio.h>
#endif

#ifdef BCMDBG
#define GPIO_ERROR(fmt, args...) printf("%s: " fmt "\n" , __FUNCTION__ , ## args)
#else
#define GPIO_ERROR(fmt, args...)
#endif

/* GPIO registers */
#define BCMGPIO_REG_IN		0
#define BCMGPIO_REG_OUT		1
#define BCMGPIO_REG_OUTEN	2
#define BCMGPIO_REG_RESERVE 	3
#define BCMGPIO_REG_RELEASE 	4

#define BCMGPIO_MAX_FD		4

#define BCMGPIO_STRB_MS_TIME	50	/* 50 ms */
#define BCMGPIO_STRB_NS_TIME 	(BCMGPIO_STRB_MS_TIME * 1000 * 1000)	/* in ns units */

/* GPIO information */
typedef struct {
	int connected;				/* is gpio being used? */
	bcmgpio_dirn_t dirn;			/* direction: IN or OUT */
	unsigned long on_time;			/* in 10 ms units */
	unsigned long strobe_period;		/* in 10 ms units */
	unsigned long timer_count;		/* 10 ms tick counter */
	unsigned long strobe_count;		/* strobe counter */
	unsigned long tot_strobes;		/* total number of strobes */
	unsigned long orig_state;		/* original state of GPIO before blinking */
	int strobing;				/* is gpio strobing? */
	int *strobe_done;			/* pointer to memory which is used to signal strobe completion */
	bcm_timer_id timer_id;			/* id of the strobe timer */
} bcmgpio_info_t;

/* Bitmask of connected pins */
static unsigned long connect_mask;

/* Global containing the file descriptor of the GPIO kernel drivers */
static int bcmgpio_fd;

/* Global containing information about each GPIO pin */
static bcmgpio_info_t bcmgpio_info[BCMGPIO_MAXPINS];

/* Static function prototypes */

/* Generic functions to read/write Chip Common core's GPIO registers on the AP */
static int bcmgpio_drvinit(void);
static void bcmgpio_drvcleanup(void);

static void bcmgpio_toggle (unsigned long gpio_mask);
static void bcmgpio_timercb (bcm_timer_id tid, int gpio_pin);

/**********************************************************************************************
 *  Functions visible to this file only
******************************************************************************************** */
static int 
bcmgpio_drvinit ()
{
#if defined(__ECOS)
	if (!(gpio_sih = (void *)si_kattach(SI_OSH)))
		return -1;
	bcmgpio_fd = 1;
	si_gpiosetcore(gpio_sih);
	return 0;
#else
	bcmgpio_fd = open("/dev/gpio", O_RDWR);
	if (bcmgpio_fd == -1) {
		GPIO_ERROR ("Failed to open /dev/gpio\n");
		return -1;
	}
	return 0;
#endif
}

static void 
bcmgpio_drvcleanup ()
{
#if defined(__ECOS)
#else	
	if (bcmgpio_fd!= -1) {
		close (bcmgpio_fd);
		bcmgpio_fd = -1;
	}
#endif
}

static int
bcmgpio_ioctl(int gpioreg, unsigned int mask , unsigned int val)
{
#if defined(__ECOS)
	int value;
	switch (gpioreg) {
		case BCMGPIO_REG_IN:
			value = si_gpioin(gpio_sih);
			break;
		case BCMGPIO_REG_OUT:
			value = si_gpioout(gpio_sih, mask, val,GPIO_APP_PRIORITY);
			break;
		case BCMGPIO_REG_OUTEN:
			value = si_gpioouten(gpio_sih, mask, val,GPIO_APP_PRIORITY);
			break;
		case BCMGPIO_REG_RESERVE:
			value = si_gpioreserve(gpio_sih, mask, GPIO_APP_PRIORITY);
			break;
		case BCMGPIO_REG_RELEASE:
			/* 
 			* releasing the gpio doesn't change the current 
			* value on the GPIO last write value 
 			* persists till some one overwrites it
			*/
			value = si_gpiorelease(gpio_sih, mask, GPIO_APP_PRIORITY);
			break;
		default:
			GPIO_ERROR ("invalid gpioreg %d\n", gpioreg);
			value = -1;
			break;
	}
	return value;
#else
	struct gpio_ioctl gpio;
	int type;

	gpio.val = val;
	gpio.mask = mask;

	switch (gpioreg) {
		case BCMGPIO_REG_IN:
			type = GPIO_IOC_IN; 
			break;
		case BCMGPIO_REG_OUT:
			type = GPIO_IOC_OUT; 
			break;
		case BCMGPIO_REG_OUTEN:
			type = GPIO_IOC_OUTEN; 
			break;
		case BCMGPIO_REG_RESERVE:
			type = GPIO_IOC_RESERVE; 
			break;
		case BCMGPIO_REG_RELEASE:
			type = GPIO_IOC_RELEASE; 
			break;
		default:
			GPIO_ERROR ("invalid gpioreg %d\n", gpioreg);
			return -1;
	}
	if (ioctl(bcmgpio_fd, type, &gpio) < 0) {
		GPIO_ERROR ("invalid gpioreg %d\n", gpioreg);
		return -1;
	}
	return (gpio.val);
#endif
}

static void 
bcmgpio_toggle (unsigned long gpio_mask)
{
	unsigned long regval;

	regval = bcmgpio_ioctl(BCMGPIO_REG_OUT,0,0);
	if (regval & gpio_mask)
		regval &= ~gpio_mask;
	else
		regval |= gpio_mask;
	bcmgpio_ioctl(BCMGPIO_REG_OUT,gpio_mask,regval);
}


static void 
bcmgpio_timercb (bcm_timer_id tid, int gpio_pin)
{
	unsigned long bitmask;

	if (bcmgpio_info[gpio_pin].strobing) {
		bitmask = (unsigned long) 1 << gpio_pin;

		bcmgpio_info[gpio_pin].timer_count++;

		if (bcmgpio_info[gpio_pin].timer_count == bcmgpio_info[gpio_pin].on_time) {
			bcmgpio_toggle (bitmask);
		}
		else if (bcmgpio_info[gpio_pin].timer_count > bcmgpio_info[gpio_pin].on_time) {
			if (bcmgpio_info[gpio_pin].timer_count == bcmgpio_info[gpio_pin].strobe_period) {
				bcmgpio_info[gpio_pin].timer_count = 0;

				if (bcmgpio_info[gpio_pin].tot_strobes > 0) {
					bcmgpio_info[gpio_pin].strobe_count++;

					if (bcmgpio_info[gpio_pin].strobe_count == bcmgpio_info[gpio_pin].tot_strobes) {
						bcmgpio_strobe_stop (gpio_pin);
						bcmgpio_out (bitmask, bcmgpio_info[gpio_pin].orig_state);
						if (bcmgpio_info[gpio_pin].strobe_done != NULL)
							*(bcmgpio_info[gpio_pin].strobe_done) = 1;
						return;
					}
				}

				bcmgpio_toggle (bitmask);
			}
		}
	}
}


/**********************************************************************************************
 *  GPIO functions 
******************************************************************************************** */
int 
bcmgpio_connect (int gpio_pin, bcmgpio_dirn_t gpio_dirn)
{
	unsigned long bitmask;

	assert ((gpio_pin >= 0) && (gpio_pin <= BCMGPIO_MAXINDEX));	
	assert ((gpio_dirn == BCMGPIO_DIRN_IN) || (gpio_dirn == BCMGPIO_DIRN_OUT));

	if (connect_mask == 0) {
		if (bcmgpio_drvinit () != 0) 
			return -1;
	}
	if (bcmgpio_info[gpio_pin].connected)
		return -1;

	bitmask = ((unsigned long) 1 << gpio_pin);

	bcmgpio_info[gpio_pin].connected = 1;
	bcmgpio_info[gpio_pin].dirn = gpio_dirn;

	/* reserve the pin*/
	bcmgpio_ioctl(BCMGPIO_REG_RESERVE, bitmask, bitmask);

	if (gpio_dirn == BCMGPIO_DIRN_IN)
		bcmgpio_ioctl(BCMGPIO_REG_OUTEN, bitmask, 0);
	else
		bcmgpio_ioctl(BCMGPIO_REG_OUTEN, bitmask, bitmask);

	connect_mask |= bitmask;

	return 0;
}

/* 
 * releasing the gpio doesn't change the current value on the GPIO last write value 
 * persists till some one overwrites it
*/
int 
bcmgpio_disconnect (int gpio_pin)
{
	unsigned long bitmask;

	assert ((gpio_pin >= 0) && (gpio_pin <= BCMGPIO_MAXINDEX));	

	if (! bcmgpio_info[gpio_pin].connected)
		return -1;

	bitmask = ((unsigned long) 1 << gpio_pin);

	if (bcmgpio_info[gpio_pin].strobing)
		bcmgpio_strobe_stop (gpio_pin);

	bcmgpio_info[gpio_pin].connected = 0;

	/* release the pin*/
	bcmgpio_ioctl(BCMGPIO_REG_RELEASE, bitmask, 0);

	connect_mask &= ~bitmask;

	if (connect_mask == 0)
		bcmgpio_drvcleanup ();

	return 0;
}

int 
bcmgpio_in (unsigned long gpio_mask, unsigned long *value)
{
	unsigned long regin;

	if ((gpio_mask & connect_mask) != gpio_mask)
		return -1;

	regin = bcmgpio_ioctl (BCMGPIO_REG_IN, 0, 0);

	*value = regin & gpio_mask;

	return 0;
}


int 
bcmgpio_out (unsigned long gpio_mask, unsigned long value)
{

	if ((gpio_mask & connect_mask) != gpio_mask)
		return -1;

	bcmgpio_ioctl (BCMGPIO_REG_OUT, gpio_mask, value);

	return 0;
}


int 
bcmgpio_strobe_start (int gpio_pin, bcmgpio_strobe_t *strobe_info)
{
	unsigned long regout;
	int status;
	struct itimerspec its;

	assert ((gpio_pin >= 0) && (gpio_pin <= BCMGPIO_MAXINDEX));	

	if (! strobe_info->timer_module) {
		GPIO_ERROR ("bcmgpio_strobe: Invalid timer module ID\n");
		return -1;
	}

	if (! bcmgpio_info[gpio_pin].connected)
		return -1;

	if (bcmgpio_info[gpio_pin].dirn == BCMGPIO_DIRN_IN)
		return -1;

	if (bcmgpio_info[gpio_pin].strobing)
		return 0;

	if ((status = bcm_timer_create (strobe_info->timer_module, &bcmgpio_info[gpio_pin].timer_id)) != 0) {
		bcmgpio_info[gpio_pin].timer_id = 0;
		GPIO_ERROR ("bcmgpio_strobe: Timer creation failed with error %d\n", status);
		return -1;
	}

	if ((status = bcm_timer_connect (bcmgpio_info[gpio_pin].timer_id, (bcm_timer_cb) bcmgpio_timercb, (int) gpio_pin)) != 0) {
		bcm_timer_delete (bcmgpio_info[gpio_pin].timer_id);
		bcmgpio_info[gpio_pin].timer_id = 0;
		GPIO_ERROR ("bcmgpio_strobe: Timer connect failed with error %d\n", status);
		return -1;
	}

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = BCMGPIO_STRB_NS_TIME;
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = BCMGPIO_STRB_NS_TIME;
		
	if ((status = bcm_timer_settime (bcmgpio_info[gpio_pin].timer_id, &its)) != 0) {
		bcm_timer_delete (bcmgpio_info[gpio_pin].timer_id);
		bcmgpio_info[gpio_pin].timer_id = 0;
		GPIO_ERROR ("bcmgpio_strobe: Timer set failed with error %d\n", status);
		return -1;
	}

	regout = bcmgpio_ioctl (BCMGPIO_REG_OUT, 0, 0);

	bcmgpio_info[gpio_pin].orig_state = regout & ((unsigned long) 1 << gpio_pin);
			
	bcmgpio_info[gpio_pin].strobe_period = strobe_info->strobe_period_in_ms / BCMGPIO_STRB_MS_TIME;
	bcmgpio_info[gpio_pin].on_time = 
		(strobe_info->duty_percent * bcmgpio_info[gpio_pin].strobe_period) / 100;
	bcmgpio_info[gpio_pin].tot_strobes = strobe_info->num_strobes;
	bcmgpio_info[gpio_pin].strobe_count = 0;
	bcmgpio_info[gpio_pin].timer_count = 0;

	bcmgpio_info[gpio_pin].strobing = 1;

	bcmgpio_info[gpio_pin].strobe_done = strobe_info->strobe_done;
	if (bcmgpio_info[gpio_pin].strobe_done != NULL)
		*(bcmgpio_info[gpio_pin].strobe_done) = 0;

	return 0;
}


int
bcmgpio_strobe_stop (int gpio_pin)
{
	assert ((gpio_pin >= 0) && (gpio_pin <= BCMGPIO_MAXINDEX));	

	if (! bcmgpio_info[gpio_pin].connected)
		return -1;

	if (bcmgpio_info[gpio_pin].strobing) {
		bcmgpio_info[gpio_pin].strobing = 0;

		if (bcmgpio_info[gpio_pin].timer_id != 0) {
			bcm_timer_delete (bcmgpio_info[gpio_pin].timer_id);
			bcmgpio_info[gpio_pin].timer_id = 0;
		}
	}

	return 0;
}

/* Search for token in comma separated token-string */
static inline int
findmatch(char *string, char *name)
{
	uint len;
	char *c;

	len = strlen(name);
	while ((c = strchr(string, ',')) != NULL) {
		if (len == (uint)(c - string) && !strncmp(string, name, len))
			return 1;
		string = c + 1;
	}

	return (!strcmp(string, name));
}


/* Return gpio pin number assigned to the named pin */
/*
* Variable should be in format:
*
*	gpio<N>=pin_name,pin_name
*
* This format allows multiple features to share the gpio with
* mutual understanding.
*
* 'def_pin' is returned if a specific gpio is not defined for the requested functionality 
* and if def_pin is not used by others.
*/
int
bcmgpio_getpin(char *pin_name, uint def_pin)
{
	char name[] = "gpioXXXX";
	char *val;
	uint pin;

	/* Go thru all possibilities till a match in pin name */
	for (pin = 0; pin < BCMGPIO_MAXPINS; pin ++) {
		sprintf(name, "gpio%d", pin);
		val = nvram_get(name);
		if (val && findmatch(val, pin_name))
			return pin;
	}

	if (def_pin != BCMGPIO_UNDEFINED) {
		/* make sure the default pin is not used by someone else */
		sprintf(name, "gpio%d", def_pin);
		if (nvram_get(name))
		{
			def_pin =  BCMGPIO_UNDEFINED;
		}
	}

	return def_pin;
}
