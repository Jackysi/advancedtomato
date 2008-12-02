/*
 * ip22-hpc.c: Routines for generic manipulation of the HPC controllers.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1998 Ralf Baechle
 */
#include <linux/init.h>
#include <linux/types.h>

#include <asm/addrspace.h>
#include <asm/sgi/sgihpc.h>
#include <asm/sgi/sgint23.h>
#include <asm/sgialib.h>
#include <asm/bootinfo.h>

#define HPC_DEBUG(args...)

struct hpc3_regs *hpc3c0, *hpc3c1;
struct hpc3_miscregs *hpc3mregs;

/* We need software copies of these because they are write only. */
u32 sgi_hpc_write1, sgi_hpc_write2;

/* Machine specific identifier knobs. */
int sgi_has_ioc2 = 0;
int sgi_guiness = 0;
int sgi_boardid;

extern char *system_type;

void __init sgihpc_init(void)
{
	unsigned int sid, crev, brev;

	hpc3c0 = (struct hpc3_regs *) (KSEG1 + HPC3_CHIP0_PBASE);
	hpc3c1 = (struct hpc3_regs *) (KSEG1 + HPC3_CHIP1_PBASE);
	hpc3mregs = (struct hpc3_miscregs *) (KSEG1 + HPC3_MREGS_PBASE);
	sid = hpc3mregs->sysid;

	sid &= 0xff;
	crev = (sid & 0xe0) >> 5;
	brev = (sid & 0x1e) >> 1;

	HPC_DEBUG("sgihpc_init: crev<%2x> brev<%2x>\n", crev, brev);
	HPC_DEBUG("sgihpc_init: ");

	/* This test works now thanks to William J. Earl */
	if ((sid & 1) == 0 ) {
		HPC_DEBUG("GUINESS ");
		sgi_guiness = 1;
		system_type = "SGI Indy";
	} else {
		HPC_DEBUG("FULLHOUSE ");
		sgi_guiness = 0;
		system_type = "SGI Indigo2";
	}
	sgi_boardid = brev;

	HPC_DEBUG("sgi_boardid<%d> ", sgi_boardid);

	if(crev == 1) {
		if((sid & 1) || (brev >= 2)) {
			HPC_DEBUG("IOC2 ");
			sgi_has_ioc2 = 1;
		} else {
			HPC_DEBUG("IOC1 revision 1 ");
		}
	} else {
		HPC_DEBUG("IOC1 revision 0 ");
	}
	HPC_DEBUG("\n");

	sgi_hpc_write1 = (HPC3_WRITE1_PRESET | HPC3_WRITE1_KMRESET |
			  HPC3_WRITE1_ERESET | HPC3_WRITE1_LC0OFF);

	sgi_hpc_write2 = (HPC3_WRITE2_EASEL   | HPC3_WRITE2_NTHRESH |
			  HPC3_WRITE2_TPSPEED | HPC3_WRITE2_EPSEL   |
			  HPC3_WRITE2_U0AMODE | HPC3_WRITE2_U1AMODE);

	if(!sgi_guiness)
		sgi_hpc_write1 |= HPC3_WRITE1_GRESET;
	hpc3mregs->write1 = sgi_hpc_write1;
	hpc3mregs->write2 = sgi_hpc_write2;

	hpc3c0->pbus_piocfgs[0][6] |= HPC3_PIOPCFG_HW;
}
