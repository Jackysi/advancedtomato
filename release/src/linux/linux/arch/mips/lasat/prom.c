/*
 * PROM interface routines.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/ioport.h>
#include <asm/bootinfo.h>
#include <asm/lasat/lasat.h>
#include <asm/cpu.h>

#include "at93c.h"
#include <asm/lasat/eeprom.h>

char arcs_cmdline[CL_SIZE];

static struct at93c_defs at93c_defs[N_MACHTYPES] = {
	{(void *)AT93C_REG_100, (void *)AT93C_RDATA_REG_100, AT93C_RDATA_SHIFT_100,
	AT93C_WDATA_SHIFT_100, AT93C_CS_M_100, AT93C_CLK_M_100},
	{(void *)AT93C_REG_200, (void *)AT93C_RDATA_REG_200, AT93C_RDATA_SHIFT_200,
	AT93C_WDATA_SHIFT_200, AT93C_CS_M_200, AT93C_CLK_M_200},
};

void __init prom_init(int argc, char **argv, char **envp, int *prom_vec)
{
	if (mips_cpu.cputype == CPU_R5000)
		mips_machtype = MACH_LASAT_200;
	else
		mips_machtype = MACH_LASAT_100;

	at93c = &at93c_defs[mips_machtype];

	lasat_init_board_info();		/* Read info from EEPROM */

	mips_machgroup = MACH_GROUP_LASAT;

	/* Get the command line */
	if (argc>0) {
		strncpy(arcs_cmdline, argv[0], CL_SIZE-1);
		arcs_cmdline[CL_SIZE-1] = '\0';
	}

	/* Set the I/O base address */
	set_io_port_base(KSEG1);

	/* Set memory regions */
	ioport_resource.start = 0;		/* Should be KSEGx ???	*/
	ioport_resource.end = 0xffffffff;	/* Should be ???	*/
}

void prom_free_prom_memory(void)
{
}

void prom_printf(const char * fmt, ...)
{
	return;
}

const char *get_system_type(void)
{
	return lasat_board_info.li_bmstr;
}
