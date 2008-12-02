/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */
#include <linux/config.h>
#include <asm/galileo-boards/evb64120A/pci.h>
#include <asm/byteorder.h>

#include "etherboot.h"
#include "pci_etherboot.h"
#include "galileo_port.h"

#define MAX_PCI_DEVS 10
PCI_DEVICE pci0_devices[MAX_PCI_DEVS];
PCI_DEVICE pci1_devices[MAX_PCI_DEVS];

static int pci_range_ck(unsigned char bus, unsigned char dev)
{
	if ((bus == 0) && (dev >= 0) && (dev < 30))
		return 0;	// Bus/Device Number OK

	return -1;		// Bus/Device Number not OK
}

/********************************************************************
 * pcibios_(read/write)_config_(byte/word/dword)
 *
 *Inputs :
 *bus - bus number
 *dev - device number
 *offset - register offset in the configuration space
 *val - value to be written / read
 *
 *Outputs :
 *Sucees/Failure
 *********************************************************************/
#define PCI_CFG_DATA    ((volatile unsigned long *)0xb4000cfc)
#define PCI_CFG_CTRL    ((volatile unsigned long *)0xb4000cf8)

#define PCI_CFG_SET(dev,fun,off) \
        ((*PCI_CFG_CTRL) = cpu_to_le32((0x80000000 | ((dev)<<11) | ((fun)<<8) | (off))))

int pcibios_read_config_dword(unsigned char bus, unsigned char dev,
			      unsigned char offset, unsigned int *val)
{

	if (offset & 0x3) {
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	if (pci_range_ck(bus, dev)) {
		*val = 0xFFFFFFFF;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}
	PCI_CFG_SET(dev, 0, offset);
	if (dev != 0) {
		*val = *PCI_CFG_DATA;
	} else {
		*val = cpu_to_le32(*PCI_CFG_DATA);
	}
	return PCIBIOS_SUCCESSFUL;
}

int pcibios_read_config_word(unsigned char bus, unsigned char dev,
			     unsigned char offset, unsigned short *val)
{
	if (offset & 0x1)
		return PCIBIOS_BAD_REGISTER_NUMBER;
	if (pci_range_ck(bus, dev)) {
		*val = 0xffff;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}
	PCI_CFG_SET(dev, 0, (offset & ~0x3));
	if (dev != 0) {
		*val = *PCI_CFG_DATA >> ((offset & 3) * 8);
	} else {
		*val = cpu_to_le32(*PCI_CFG_DATA) >> ((offset & 3) * 8);
	}
	return PCIBIOS_SUCCESSFUL;
}

int pcibios_read_config_byte(unsigned char bus, unsigned char dev,
			     unsigned char offset, unsigned char *val)
{
	if (pci_range_ck(bus, dev)) {
		*val = 0xff;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}
	PCI_CFG_SET(dev, 0, (offset & ~0x3));
	if (dev != 0) {
		*val = *PCI_CFG_DATA >> ((offset & 3) * 8);
	} else {
		*val = cpu_to_le32(*PCI_CFG_DATA >> ((offset & 3) * 8));
	}
	return PCIBIOS_SUCCESSFUL;
}

int pcibios_write_config_dword(unsigned char bus, unsigned char dev,
			       unsigned char offset, unsigned int val)
{
	if (offset & 0x3)
		return PCIBIOS_BAD_REGISTER_NUMBER;
	if (pci_range_ck(bus, dev))
		return PCIBIOS_DEVICE_NOT_FOUND;
	PCI_CFG_SET(dev, 0, offset);
	if (dev != 0) {
		*PCI_CFG_DATA = val;
	} else {
		*PCI_CFG_DATA = cpu_to_le32(val);
	}
	return PCIBIOS_SUCCESSFUL;
}

int pcibios_write_config_word(unsigned char bus, unsigned char dev,
			      unsigned char offset, unsigned short val)
{
	unsigned long tmp;

	if (offset & 0x1)
		return PCIBIOS_BAD_REGISTER_NUMBER;
	if (pci_range_ck(bus, dev))
		return PCIBIOS_DEVICE_NOT_FOUND;
	PCI_CFG_SET(dev, 0, (offset & ~0x3));
	if (dev != 0) {
		tmp = *PCI_CFG_DATA;
	} else {
		tmp = cpu_to_le32(*PCI_CFG_DATA);
	}
	tmp &= ~(0xffff << ((offset & 0x3) * 8));
	tmp |= (val << ((offset & 0x3) * 8));
	if (dev != 0) {
		*PCI_CFG_DATA = tmp;
	} else {
		*PCI_CFG_DATA = cpu_to_le32(tmp);
	}
	return PCIBIOS_SUCCESSFUL;
}

int pcibios_write_config_byte(unsigned char bus, unsigned char dev,
			      unsigned char offset, unsigned char val)
{
	unsigned long tmp;

	if (pci_range_ck(bus, dev))
		return PCIBIOS_DEVICE_NOT_FOUND;
	PCI_CFG_SET(dev, 0, (offset & ~0x3));
	if (dev != 0) {
		tmp = *PCI_CFG_DATA;
	} else {
		tmp = cpu_to_le32(*PCI_CFG_DATA);
	}
	tmp &= ~(0xff << ((offset & 0x3) * 8));
	tmp |= (val << ((offset & 0x3) * 8));
	*PCI_CFG_DATA = cpu_to_le32(tmp);
	return PCIBIOS_SUCCESSFUL;
}


/********************************************************************
 * eth_pci_init - Fill pci_devi
 *
 *Inputs :
 *bus - bus number
 *dev - device number
 *offset - register offset in the configuration space
 *val - value to be written / read
 *
 *Outputs :
 *Sucees/Failure
 *********************************************************************/
void eth_pci_init(struct pci_device *pcidev)
{
	int i, count;
	pcibios_write_config_word(0, 0, 4, 0x7);
	pcibios_write_config_dword(0, 8, BAR0, 0x12000000);
	pcibios_write_config_dword(0, 8, BAR1, 0x10000001);
	pcibios_write_config_dword(0, 8, BAR2, 0x12100000);
	strcpy(pci0_devices[0].type, "Network Controller");
	pci0_devices[0].deviceNum = 8;
	pci0_devices[0].venID = 0x8086;
	pci0_devices[0].deviceID = 0x1229;

	pci0_devices[0].bar0Base = 0x12000000;
	pci0_devices[0].bar0Size = 0x00001000;
	pci0_devices[0].bar0Type = 0;
	pci0_devices[0].bar1Base = 0x10000000;
	pci0_devices[0].bar1Size = 0x40;
	pci0_devices[0].bar1Type = 1;
	pci0_devices[0].bar2Base = 0x12100000;
	pci0_devices[0].bar2Size = 0x00100000;
	pci0_devices[0].bar2Type = 0;
	for (i = 0; pcidev[i].vendor != 0; i++) {
		/* Look for device in PCI0 first */
		for (count = 0; count < MAX_PCI_DEVS; count++) {
			if ((pci0_devices[count].type[0] != 0)
			    && ((unsigned short) pci0_devices[count].
				venID == (unsigned short) pcidev[i].vendor)
			    && ((unsigned short) pci0_devices[count].
				deviceID ==
				(unsigned short) pcidev[i].dev_id)) {
				if ((pci0_devices[count].bar0Type == 1)
				    && (pci0_devices[count].bar0Size != 0))
					pcidev[i].ioaddr =
					    pci0_devices[count].bar0Base;
				if ((pci0_devices[count].bar1Type == 1)
				    && (pci0_devices[count].bar1Size != 0))
					pcidev[i].ioaddr =
					    pci0_devices[count].bar1Base;
				if ((pci0_devices[count].bar2Type == 1)
				    && (pci0_devices[count].bar2Size != 0))
					pcidev[i].ioaddr =
					    pci0_devices[count].bar2Base;
				if ((pci0_devices[count].bar3Type == 1)
				    && (pci0_devices[count].bar3Size != 0))
					pcidev[i].ioaddr =
					    pci0_devices[count].bar3Base;
				if ((pci0_devices[count].bar4Type == 1)
				    && (pci0_devices[count].bar4Size != 0))
					pcidev[i].ioaddr =
					    pci0_devices[count].bar4Base;
				if ((pci0_devices[count].bar5Type == 1)
				    && (pci0_devices[count].bar5Size != 0))
					pcidev[i].ioaddr =
					    pci0_devices[count].bar5Base;

				if ((pci0_devices[count].bar0Type == 0)
				    && (pci0_devices[count].bar0Size != 0))
					pcidev[i].membase =
					    pci0_devices[count].bar0Base;
				if ((pci0_devices[count].bar1Type == 0)
				    && (pci0_devices[count].bar1Size != 0))
					pcidev[i].membase =
					    pci0_devices[count].bar1Base;
				if ((pci0_devices[count].bar2Type == 0)
				    && (pci0_devices[count].bar2Size != 0))
					pcidev[i].membase =
					    pci0_devices[count].bar2Base;
				if ((pci0_devices[count].bar3Type == 0)
				    && (pci0_devices[count].bar3Size != 0))
					pcidev[i].membase =
					    pci0_devices[count].bar3Base;
				if ((pci0_devices[count].bar4Type == 0)
				    && (pci0_devices[count].bar4Size != 0))
					pcidev[i].membase =
					    pci0_devices[count].bar4Base;
				if ((pci0_devices[count].bar5Type == 0)
				    && (pci0_devices[count].bar5Size != 0))
					pcidev[i].membase =
					    pci0_devices[count].bar5Base;
				pcidev[i].bus = 0;
				pcidev[i].devfn =
				    pci0_devices[count].deviceNum;
			}
#ifdef CONFIG_EVB_PCI1
			if ((pci1_devices[count].type[0] != 0)
			    && (pci1_devices[count].venID ==
				pcidev[i].vendor)
			    && (pci1_devices[count].deviceID ==
				pcidev[i].dev_id)) {
				if ((pci1_devices[count].bar0Type == 1)
				    && (pci1_devices[count].bar0Size != 0))
					pcidev[i].ioaddr =
					    pci1_devices[count].bar0Base;
				if ((pci1_devices[count].bar1Type == 1)
				    && (pci1_devices[count].bar1Size != 0))
					pcidev[i].ioaddr =
					    pci1_devices[count].bar1Base;
				if ((pci1_devices[count].bar2Type == 1)
				    && (pci1_devices[count].bar2Size != 0))
					pcidev[i].ioaddr =
					    pci1_devices[count].bar2Base;
				if ((pci1_devices[count].bar3Type == 1)
				    && (pci1_devices[count].bar3Size != 0))
					pcidev[i].ioaddr =
					    pci1_devices[count].bar3Base;
				if ((pci1_devices[count].bar4Type == 1)
				    && (pci1_devices[count].bar4Size != 0))
					pcidev[i].ioaddr =
					    pci1_devices[count].bar4Base;
				if ((pci1_devices[count].bar5Type == 1)
				    && (pci1_devices[count].bar5Size != 0))
					pcidev[i].ioaddr =
					    pci1_devices[count].bar5Base;

				if ((pci1_devices[count].bar0Type == 0)
				    && (pci1_devices[count].bar0Size != 0))
					pcidev[i].membase =
					    pci1_devices[count].bar0Base;
				if ((pci1_devices[count].bar1Type == 0)
				    && (pci1_devices[count].bar1Size != 0))
					pcidev[i].membase =
					    pci1_devices[count].bar1Base;
				if ((pci1_devices[count].bar2Type == 0)
				    && (pci1_devices[count].bar2Size != 0))
					pcidev[i].membase =
					    pci1_devices[count].bar2Base;
				if ((pci1_devices[count].bar3Type == 0)
				    && (pci1_devices[count].bar3Size != 0))
					pcidev[i].membase =
					    pci1_devices[count].bar3Base;
				if ((pci1_devices[count].bar4Type == 0)
				    && (pci1_devices[count].bar4Size != 0))
					pcidev[i].membase =
					    pci1_devices[count].bar4Base;
				if ((pci1_devices[count].bar5Type == 0)
				    && (pci1_devices[count].bar5Size != 0))
					pcidev[i].membase =
					    pci1_devices[count].bar5Base;

				pcidev[i].bus = 1;
				pcidev[i].devfn =
				    pci1_devices[count].deviceNum;
			}
#endif
		}
	}
}
