/*
 * ip22-system.c: Probe the system type using ARCS prom interface library.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>

#include <asm/cpu.h>
#include <asm/sgi/sgi.h>
#include <asm/sgialib.h>

enum sgi_mach sgimach;

struct smatch {
	char *name;
	int type;
};

static struct smatch sgi_cputable[] = {{ 
	.name	= "MIPS-R2000",
	.type	= CPU_R2000,
},{
	.name	= "MIPS-R3000",
	.type	= CPU_R3000,
},{
	.name	= "MIPS-R3000A",
	.type	= CPU_R3000A,
},{
	.name	= "MIPS-4000",
	.type	= CPU_R4000SC,
},{
	.name	= "MIPS-R4400",
	.type	= CPU_R4400SC,
},{
	.name	= "MIPS-R4600",
	.type	= CPU_R4600,
},{
	.name	= "MIPS-R8000",
	.type	= CPU_R8000,
},{
	.name	= "MIPS-R5000",
	.type	= CPU_R5000,
},{
	.name	= "MIPS-R5000A",
	.type	= CPU_R5000A,
},{
	.name	= "MIPS-R10000",
	.type	= CPU_R10000,
}};

static int __init string_to_cpu(char *s)
{
	ULONG cnt;
	char c;
	int i;

	for(i = 0; i < (sizeof(sgi_cputable) / sizeof(struct smatch)); i++) {
		if(!strcmp(s, sgi_cputable[i].name))
			return sgi_cputable[i].type;
	}
	prom_printf("\nYeee, could not determine MIPS cpu type <%s>\n", s);
	prom_printf("press a key to reboot\n");
	ArcRead(0, &c, 1, &cnt);
	ArcEnterInteractiveMode();
	return 0;
}

/*
 * We' call this early before loadmmu().  If we do the other way around
 * the firmware will crash and burn.
 */
void __init sgi_sysinit(void)
{
	pcomponent *p, *toplev, *cpup = 0;
	int cputype = -1;
	ULONG cnt;
	char c;


	/* The root component tells us what machine architecture we
	 * have here.
	 */
	p = ArcGetChild(PROM_NULL_COMPONENT);

	/* Now scan for cpu(s). */
	printk(KERN_INFO);
	toplev = p = ArcGetChild(p);
	while(p) {
		int ncpus = 0;

		if(p->type == Cpu) {
			if(++ncpus > 1) {
				prom_printf("\nYeee, SGI MP not ready yet\n");
				prom_printf("press a key to reboot\n");
				ArcRead(0, &c, 1, &cnt);
				ArcEnterInteractiveMode();
			}
			printk("CPU: %s ", (char *)p->iname);
			cpup = p;
			cputype = string_to_cpu((char *)cpup->iname);
		}
		p = ArcGetPeer(p);
	}
	if (cputype == -1) {
		prom_printf("\nYeee, could not find cpu ARCS component\n");
		prom_printf("press a key to reboot\n");
		ArcRead(0, &c, 1, &cnt);
		ArcEnterInteractiveMode();
	}
	p = ArcGetChild(cpup);
	while(p) {
		switch(p->class) {
		case processor:
			switch(p->type) {
			case Fpu:
				printk("FPU<%s> ", (char *)p->iname);
				break;

			default:
				break;
			};
			break;

		case cache:
			switch(p->type) {
			case picache:
				printk("ICACHE ");
				break;

			case pdcache:
				printk("DCACHE ");
				break;

			case sccache:
				printk("SCACHE ");
				break;

			default:
				break;

			};
			break;

		default:
			break;
		};
		p = ArcGetPeer(p);
	}
	printk("\n");
}
