/* hello.c 
 * Copyright (C) 1998 by Ori Pomerantz
 * 
 * "Hello, world" - the kernel module version. 
 */

/* The necessary header files */

/* Standard in kernel modules */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/tqueue.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/sysctl.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>

#include <typedefs.h>
#include "bcm4710.h"
#include <sbextif.h>
#include <bcmnvram.h>

#ifdef RESET_BUTTOM_ENABLE
    #define BCM47XX_SOFTWARE_RESET  0x40
    #define RESET_WAIT	4	// seconds
#endif


/* value for /proc/sys/diag */
#define BIT_DMZ		0x01
#define BIT_SESSION	0x02
#define BIT_DIAG	0x04
#define BIT_DIAG_STARLIKE	0x10

/* Address offset */
#define LED_WIRELESS2	0x10
#define LED_WIRELESS1	0x11
#define LED_DMZ		0x12
#define LED_DIAG	0x13
#define LED_SESSION	0x14

static unsigned int last = BIT_DIAG;
static unsigned int diag = BIT_DIAG;

static int led = 1;
static int starlike = 0;

#ifdef RESET_BUTTOM_ENABLE
static unsigned int sec = 0;
static unsigned int lastsec = 0;
#endif

static struct ctl_table_header *diag_sysctl_header;
struct timer_list timer;
struct timer_list diagtimer;

static ctl_table mytable[] = {
         { 2000, "diag", 
	   &diag, sizeof(diag), 
	   0644, NULL, 
	   proc_dointvec },
         { 0 }
};


#ifdef RESET_BUTTOM_ENABLE
static void system_reboot(void){

	/* Reset the LED */
	*(volatile u8*)(KSEG1ADDR(BCM4710_EUART)+LED_DMZ);
	*(volatile u8*)(KSEG1ADDR(BCM4710_EUART)+LED_SESSION);
	*(volatile u8*)(KSEG1ADDR(BCM4710_EUART)+LED_DIAG)=0xFF;

	machine_restart(NULL);
}
#endif


static void diag_startlike(ulong data)
{
	
	if( led == 1 )
		*(volatile u8*)(KSEG1ADDR(BCM4710_EUART)+LED_DIAG)=0xFF;
	else
		*(volatile u8*)(KSEG1ADDR(BCM4710_EUART)+LED_DIAG);

	/* 0, 1 hopping */
	led ^= 1 ;

	diagtimer.function = diag_startlike;
	diagtimer.expires = jiffies + 10;
	add_timer(&diagtimer);
}


static void diag_loop(ulong data)
{
#ifdef RESET_BUTTOM_ENABLE
	extifregs_t *eir;
	unsigned long s;
#endif
	
//	printk("diag_loop: diag=%d\n", diag );

	if( diag != last ){
		printk("diag_loop: Reset LED.\n");
		
		if( diag & BIT_DMZ )
			*(volatile u8*)(KSEG1ADDR(BCM4710_EUART)+LED_DMZ)=0xFF;
		else
			*(volatile u8*)(KSEG1ADDR(BCM4710_EUART)+LED_DMZ);

		if( diag & BIT_SESSION )
			*(volatile u8*)(KSEG1ADDR(BCM4710_EUART)+LED_SESSION)=0xFF;
		else
			*(volatile u8*)(KSEG1ADDR(BCM4710_EUART)+LED_SESSION);

		if( diag & BIT_DIAG )
			*(volatile u8*)(KSEG1ADDR(BCM4710_EUART)+LED_DIAG)=0xFF;
		else
			*(volatile u8*)(KSEG1ADDR(BCM4710_EUART)+LED_DIAG);

		if( diag & BIT_DIAG_STARLIKE ){
			if (starlike == 0){
				starlike = 1;
				diagtimer.function = diag_startlike;
				diagtimer.expires = jiffies + 10;
				add_timer(&diagtimer);
			}
		}
		else{
			if (starlike == 1){
				starlike = 0;
				del_timer(&diagtimer);
			}
		}

//	*(volatile u8*)(KSEG1ADDR(BCM4710_EUART) + LED_WIRELESS2) = 0xff;
//	*(volatile u8*)(KSEG1ADDR(BCM4710_EUART) + LED_WIRELESS1)= 0xff;
//	save_and_cli(s);
		last = diag;
		
	}


#ifdef RESET_BUTTOM_ENABLE
	eir = (extifregs_t *) ioremap_nocache(BCM4710_REG_EXTIF, sizeof(extifregs_t));
	save_and_cli(s);

	if (!(readl(&eir->gpioin) & BCM47XX_SOFTWARE_RESET))
		sec++;

	restore_flags(s);
	if( sec ){
		if( sec >= RESET_WAIT ){
			printk("Back to initial setting.\n");
			// ..........

			printk("software reset\n");
			system_reboot();
		}
		if( sec == lastsec ){
			printk("software reset\n");
			system_reboot();
		}
		lastsec=sec;
	}
#endif

	timer.function = diag_loop;
	timer.expires = jiffies + HZ;
	add_timer(&timer);
}


static void diag_show(void)
{
//	printk("diag_show:\n");
	diag_sysctl_header = register_sysctl_table(mytable, 0);

	timer.function = diag_loop;
	timer.expires = jiffies + HZ;
	add_timer(&timer);

}

int __init diag_init()
{
	diag_show();
	return 0;
}
/* Cleanup - undid whatever init_module did */
static void __exit diag_exit()
{
//	printk("diag_exit:\n");
	unregister_sysctl_table(diag_sysctl_header);
	del_timer(&timer);
}


module_init(diag_init);
module_exit(diag_exit);


