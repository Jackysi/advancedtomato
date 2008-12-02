/*
 * lasat_board.c
 *
 * Thomas Horsten <thh@lasat.com>
 * Copyright (C) 2000 LASAT Networks A/S.
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * Routines specific to the LASAT boards
 */
#include <asm/lasat/lasat.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <asm/bootinfo.h>
#include <asm/lasat/lasat_mtd.h>
#include <asm/addrspace.h>
#include "at93c.h"
/* New model description table */
#include "lasat_models.h"
struct lasat_info lasat_board_info;

extern unsigned long crc32(unsigned long, unsigned char *, int);


int EEPROMRead(unsigned int pos, unsigned char *data, int len)
{
	int i;

	for (i=0; i<len; i++)
		*data++ = at93c_read(pos++);

	return 0;
}
int EEPROMWrite(unsigned int pos, unsigned char *data, int len)
{
	int i;

	for (i=0; i<len; i++)
		at93c_write(pos++, *data++);

	return 0;
}

static int upgrade_eeprom_info(struct lasat_eeprom_struct * ser_data)
{
	struct lasat_eeprom_struct_pre7 old;

	memcpy(&old, ser_data, sizeof(struct lasat_eeprom_struct_pre7));

	switch (ser_data->version) {
	case 1:
	case 2:
	case 3:
		/* These have old serial numbers that we can't convert. */
		return -1;

	case 4:
		/* This used flags for obscure purposes. */
		old.version = 5;

	case 5:
		/* Writecount didn't exist. */
		old.writecount = 1;
		old.version = 6;

	case 6:
		/* The length of the part numbers have changed. */
		/* So the print_serial, prod_partno, etc have moved. */
		/* Also, now the dash is part of the part number (in
		 * accordance with our philosophy that the part numbers
		 * are to be considered "random data"). */

		memset(ser_data, 0, 128);

		ser_data->cfg[0] = 0;
		ser_data->cfg[1] = 0;
		ser_data->cfg[2] = 0;

		memcpy(ser_data->hwaddr, old.hwaddr0, 6);

		memcpy(ser_data->print_partno, old.print_partno, 6);
		ser_data->print_partno[6] = '-';
		memcpy(ser_data->print_partno+7, old.print_partno+6, 3);

		memcpy(ser_data->print_serial, old.print_serial, 14);

		memcpy(ser_data->prod_partno, old.prod_partno, 6);
		ser_data->prod_partno[6] = '-';
		memcpy(ser_data->prod_partno+7, old.prod_partno+6, 3);

		memcpy(ser_data->prod_serial, old.prod_serial, 14);

		memcpy(ser_data->passwd_hash, old.passwd_hash, 16);

		ser_data->vendid = old.vendor;
		ser_data->ts_ref = old.ts_ref;
		ser_data->ts_signoff = old.ts_signoff;
		ser_data->serviceflag = old.writecount;

		ser_data->ipaddr = old.ipaddr;
		ser_data->netmask = old.netmask;

		/* make up something */
		ser_data->cfg[0] = 0x01132001;
		ser_data->cfg[1] = 0x00010061;

		ser_data->prid = ((ser_data->cfg[0] >> 4) & 0x0f);
		ser_data->version = 7;


	case 7:
		/* Up to date */
		return 0;

	default:
		/* What, an unknown version? */
		return -1;
	}
}

int lasat_init_board_info(void)
{
	int c;
	unsigned long crc;
	unsigned long cfg0, cfg1;
	const vendor_info_t    *pvi;
	const product_info_t   *ppi;
	int i_n_base_models = N_BASE_MODELS;
	const char * const * i_txt_base_models = txt_base_models;
	int i_n_vendors = N_VENDORS;
	vendor_info_t const *i_vendor_info_table = vendor_info_table;
	int i_n_prids = N_PRIDS;

	memset(&lasat_board_info, 0, sizeof(lasat_board_info));

	/* Assume EEPROM struct is LASAT_EEPROM_VERSION */
	lasat_board_info.li_eeprom_upgrade_version = 1;
	
	/* First read the EEPROM info */
	EEPROMRead(0, (unsigned char *)&lasat_board_info.li_eeprom_info, 
		   sizeof(struct lasat_eeprom_struct));

	/* Check the CRC */
	crc = crc32(0x0, (unsigned char *)(&lasat_board_info.li_eeprom_info),
		    sizeof(struct lasat_eeprom_struct) - 4);

	if (crc != lasat_board_info.li_eeprom_info.crc32) {
		return -1;
	}

	if (lasat_board_info.li_eeprom_info.version != LASAT_EEPROM_VERSION)
	{
		if (0 > upgrade_eeprom_info(&(lasat_board_info.li_eeprom_info))) {
			printk("Upgrading EEPROM information from version %d to version %d failed!\n", 
			       (unsigned int)lasat_board_info.li_eeprom_info.version,
			       LASAT_EEPROM_VERSION);
			return -1;
		}
		lasat_write_eeprom_info();

		/* OK, the EEPROM struct is not LASAT_EEPROM_VERSION */
		lasat_board_info.li_eeprom_upgrade_version = 0;
	}

	/*
	 * Part and serial no.
	 */
	memcpy(lasat_board_info.li_partno,
	       lasat_board_info.li_eeprom_info.prod_partno, 12);
	lasat_board_info.li_partno[12] = '\0';

	memcpy(lasat_board_info.li_serial,
	       lasat_board_info.li_eeprom_info.prod_serial,
	       14);
	lasat_board_info.li_serial[14] = '\0';

	/*
	 * If configuration field is present, use that
	 */

	cfg0 = lasat_board_info.li_eeprom_info.cfg[0];
	cfg1 = lasat_board_info.li_eeprom_info.cfg[1];

	if ( LASAT_W0_DSCTYPE(cfg0) != 1) {
		return -1;
	}
	/* We have a valid configuration */

	switch (LASAT_W0_SDRAMBANKSZ(cfg0)) {
	case 0:
		lasat_board_info.li_memsize = 0x0800000;
		break;
	case 1:
		lasat_board_info.li_memsize = 0x1000000;
		break;
	case 2:
		lasat_board_info.li_memsize = 0x2000000;
		break;
	case 3:
		lasat_board_info.li_memsize = 0x4000000;
		break;
	case 4:
		lasat_board_info.li_memsize = 0x8000000;
		break;
	default:
		lasat_board_info.li_memsize = 0;
	}

	switch (LASAT_W0_SDRAMBANKS(cfg0)) {
	case 0:
		break;
	case 1:
		lasat_board_info.li_memsize *= 2;
		break;
	default:
		break;
	}

	switch (LASAT_W0_BUSSPEED(cfg0)) {
	case 0x0:
		lasat_board_info.li_bus_hz = 60000000;
		break;
	case 0x1:
		lasat_board_info.li_bus_hz = 66000000;
		break;
	case 0x2:
		lasat_board_info.li_bus_hz = 66666667;
		break;
	case 0x3:
		lasat_board_info.li_bus_hz = 80000000;
		break;
	case 0x4:
		lasat_board_info.li_bus_hz = 83333333;
		break;
	case 0x5:
		lasat_board_info.li_bus_hz = 100000000;
		break;
	}

	switch (LASAT_W0_CPUCLK(cfg0)) {
	case 0x0:
		lasat_board_info.li_cpu_hz =
			lasat_board_info.li_bus_hz;
		break;
	case 0x1:
		lasat_board_info.li_cpu_hz =
			lasat_board_info.li_bus_hz +
			(lasat_board_info.li_bus_hz >> 1);	
		break;
	case 0x2:
		lasat_board_info.li_cpu_hz =
			lasat_board_info.li_bus_hz +
			lasat_board_info.li_bus_hz;
		break;
	case 0x3:
		lasat_board_info.li_cpu_hz =
			lasat_board_info.li_bus_hz +
			lasat_board_info.li_bus_hz +
			(lasat_board_info.li_bus_hz >> 1);
		break;
	case 0x4:
		lasat_board_info.li_cpu_hz =
			lasat_board_info.li_bus_hz +
			lasat_board_info.li_bus_hz +
			lasat_board_info.li_bus_hz;
		break;
	}

	switch (LASAT_W1_EDHAC(cfg1)) {
	case 0x0:
		lasat_board_info.li_edhac = 0;
		lasat_board_info.li_eadi  = 0;
		break;
	case 0x1:
		lasat_board_info.li_edhac = 0;
		lasat_board_info.li_eadi  = 1;
		break;
	case 0x2:
		lasat_board_info.li_edhac = 1;
		lasat_board_info.li_eadi  = 1;
		break;
	case 0x3:
		lasat_board_info.li_edhac = 2;
		lasat_board_info.li_eadi  = 1;
		break;
	}
	/* The 200 board always has EADI */
	if (LASAT_W0_CPUTYPE(cfg0) == 1) {
		lasat_board_info.li_eadi = 1;
	}

	lasat_board_info.li_hifn = LASAT_W1_HIFN(cfg1);
	lasat_board_info.li_isdn = LASAT_W1_ISDN(cfg1);
	lasat_board_info.li_ide  = LASAT_W1_IDE(cfg1);
	lasat_board_info.li_hdlc = LASAT_W1_HDLC(cfg1);
	lasat_board_info.li_usversion = LASAT_W1_USVERSION(cfg1);

	/* Flash size */
	switch (LASAT_W1_FLASHSIZE(cfg1)) {
	case 0:
		lasat_board_info.li_flash_size = 0x200000;
		break;
	case 1:
		lasat_board_info.li_flash_size = 0x400000;
		break;
	case 2:
		lasat_board_info.li_flash_size = 0x800000;
		break;
	case 3:
		lasat_board_info.li_flash_size = 0x1000000;
		break;
	case 4:
		lasat_board_info.li_flash_size = 0x2000000;
		break;
	}

	/* Flash base addresses */
	if (mips_machtype == MACH_LASAT_100) {
		lasat_board_info.li_flash_base = KSEG1ADDR(0x1e000000);
		lasat_board_info.li_flash_service_base = KSEG1ADDR(0x1e400000);
		lasat_board_info.li_flash_service_size = 0x100000;
		lasat_board_info.li_flash_normal_base = KSEG1ADDR(0x1e500000);
		lasat_board_info.li_flash_normal_size = 0x100000;
		if (lasat_board_info.li_flash_size > 0x200000) {
			lasat_board_info.li_flash_cfg_base = KSEG1ADDR(0x1e600000);
			lasat_board_info.li_flash_cfg_size = 0x100000;
			lasat_board_info.li_flash_fs_base = KSEG1ADDR(0x1e700000);
			lasat_board_info.li_flash_fs_size = 0x500000;
		}
	} else {
		lasat_board_info.li_flash_base = KSEG1ADDR(0x10000000);
		if (lasat_board_info.li_flash_size < 0x1000000) {
			lasat_board_info.li_flash_service_base = KSEG1ADDR(0x10000000);
			lasat_board_info.li_flash_service_size = 0x100000;
			lasat_board_info.li_flash_cfg_base = KSEG1ADDR(0x10200000);
			lasat_board_info.li_flash_cfg_size = 0x100000;
			lasat_board_info.li_flash_normal_base = KSEG1ADDR(0x10100000);
			lasat_board_info.li_flash_normal_size = 0x100000;
			if (lasat_board_info.li_flash_size >= 0x400000) {
				lasat_board_info.li_flash_fs_base = KSEG1ADDR(0x10300000);
				lasat_board_info.li_flash_fs_size = 
					lasat_board_info.li_flash_size - 0x300000;
			}
		} else {
			lasat_board_info.li_flash_service_base = KSEG1ADDR(0x10400000);
			lasat_board_info.li_flash_service_size = 0x100000;
			lasat_board_info.li_flash_cfg_base = KSEG1ADDR(0x10000000);
			lasat_board_info.li_flash_cfg_size = 0x200000;
			lasat_board_info.li_flash_normal_base = KSEG1ADDR(0x10200000);
			lasat_board_info.li_flash_normal_size = 0x100000;
			lasat_board_info.li_flash_fs_base = KSEG1ADDR(0x10500000);
			lasat_board_info.li_flash_fs_size = 0xa00000;
		}
	}

	lasat_board_info.li_bmid = LASAT_W0_BMID(cfg0);
	lasat_board_info.li_prid = lasat_board_info.li_eeprom_info.prid;
	if (lasat_board_info.li_prid == 0xffff || lasat_board_info.li_prid == 0)
		lasat_board_info.li_prid = lasat_board_info.li_bmid;
	lasat_board_info.li_vendid = lasat_board_info.li_eeprom_info.vendid;

	/* Base model stuff */
	if (lasat_board_info.li_bmid > i_n_base_models)
		lasat_board_info.li_bmid = i_n_base_models;
	strcpy(lasat_board_info.li_bmstr, i_txt_base_models[lasat_board_info.li_bmid]);

	/* Vendor stuff */
	if (lasat_board_info.li_vendid >= i_n_vendors)
		lasat_board_info.li_vendid = 0;
	pvi = &i_vendor_info_table[lasat_board_info.li_vendid];
	strcpy(lasat_board_info.li_vendstr, pvi->vi_name);

	/* Product ID dependent values */
	c = lasat_board_info.li_prid;
	if (c >= i_n_prids) {
		strcpy(lasat_board_info.li_namestr, "Unknown Model");
		strcpy(lasat_board_info.li_typestr, "Unknown Type");
		lasat_board_info.li_vpn_kbps = 0;
		lasat_board_info.li_vpn_tunnels = 0;
		lasat_board_info.li_vpn_clients = 0;
	} else {
		/* Product ID names (also depending on vendor ID) */
		ppi = &pvi->vi_product_info[c];
		strcpy(lasat_board_info.li_namestr, ppi->pi_name);
		if (ppi->pi_type)
			strcpy(lasat_board_info.li_typestr, ppi->pi_type);
		else
			sprintf(lasat_board_info.li_typestr, "%d",10*c);
	}

	lasat_board_info.li_debugaccess = lasat_board_info.li_eeprom_info.debugaccess;

	return 0;
}

void lasat_write_eeprom_info(void)
{
	unsigned long crc;

	/* Generate the CRC */
	crc = crc32(0x0, (unsigned char *)(&lasat_board_info.li_eeprom_info),
		    sizeof(struct lasat_eeprom_struct) - 4);
	lasat_board_info.li_eeprom_info.crc32 = crc;

	/* Write the EEPROM info */
	EEPROMWrite(0, (unsigned char *)&lasat_board_info.li_eeprom_info, 
		    sizeof(struct lasat_eeprom_struct));
}

char *get_firmware_version(void)
{
	char *fw;
	fw = (unsigned char *)(lasat_board_info.li_flash_normal_base);

	if ( (((unsigned long *)fw)[0] != 0xfedeabba) ||
			(((unsigned long *)fw)[1] != 0x00bedead))
		return "NONE";

	fw += 0x50;
	if ((*fw == 0) || (strlen(fw) > 175)) {
		return "UNKNOWN";
	}

	return fw;
}
