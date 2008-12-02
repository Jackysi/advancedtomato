/*
 *  arch/mips/galileo/compressed/evb64120A_memSetup.c
 *
 *  By RidgeRun Inc, (Leveraged from Galileo's sbd.c)
 *
 *  Xfer an image from flash to ram.
 *  For use with Galileo EVB64120A MIPS eval board.
 */
#include "ns16550.h"
#include <linux/serial_reg.h>
#include <asm/galileo-boards/evb64120A/pci.h>
#include <asm/galileo-boards/evb64120A/core.h>

void XferToRam(void);
bool mapMemoryBanks0and1(unsigned int bank0Base, unsigned int bank0Length,
			 unsigned int bank1Base, unsigned int bank1Length);
bool mapMemoryBanks2and3(unsigned int bank2Base, unsigned int bank2Length,
			 unsigned int bank3Base, unsigned int bank3Length);
bool mapDevices0_1and2MemorySpace(unsigned int device0Base,
				  unsigned int device0Length,
				  unsigned int device1Base,
				  unsigned int device1Length,
				  unsigned int device2Base,
				  unsigned int device2Length);

#define RUNNINGFROMFLASH
#include "./xfer.c"

/******************************
 Routine:
 Description:
 ******************************/
unsigned int readWord(unsigned int addr)
{
	unsigned int tmp;
	tmp = *(unsigned int *) (addr | NONE_CACHEABLE);
	return WORDSWAP(tmp);
}

/******************************
 Routine:
 Description:
 ******************************/
void writeWord(unsigned int addr, unsigned int data)
{
	*((unsigned int *) (addr | NONE_CACHEABLE)) = WORDSWAP(data);
}

/******************************
 Routine:
 Description:
 ******************************/
unsigned int GetExtendedMemorySize(void)
{
	unsigned int address, data = 0x11223344, type;
	unsigned int bank1_ef = false, bank2_ef = false, bank3_ef = false;
	unsigned int bank0_size, bank2_size, bank3_size, total_size = 0;

	mapMemoryBanks0and1(0, 0x800000, 0x800000, 0x800000);
	mapMemoryBanks2and3(0x1000000, 0x800000, 0x1800000, 0x800000);
	type = readWord(0x14000810);
	switch (type) {
	case 16:
		bank0_size = 0x1000000;
		break;
	case 64:
		bank0_size = 0x4000000;
		break;
	case 128:
		bank0_size = 0x8000000;
		break;
	case 256:
		bank0_size = 0x10000000;
		break;
	default:
		bank0_size = 0x1000000;
		break;
	}

	type = readWord(0x14000814);
	switch (type) {
	case 16:
		bank2_size = 0x1000000;
		bank3_size = 0x1000000;
		break;
	case 64:
		bank2_size = 0x4000000;
		bank3_size = 0x4000000;
		break;
	case 128:
		bank2_size = 0x8000000;
		bank3_size = 0x8000000;
		break;
	case 256:
		bank2_size = 0x10000000;
		bank3_size = 0x10000000;
		break;
	default:
		bank2_size = 0x1000000;
		bank3_size = 0x1000000;
		break;
	}

	/* Check which banks exist */
	/* Bank 1 */
	for (address = 0xffff00; address < 0x1000000; address += 4)
		writeWord(address, data);
	for (address = 0xffff00; address < 0x1000000; address += 4) {
		if (readWord(address) != data)
			break;
	}
	if (address == 0x1000000)
		bank1_ef = true;
	// Bank 2
	for (address = 0x17fff00; address < 0x1800000; address += 4)
		writeWord(address, data);
	for (address = 0x17fff00; address < 0x1800000; address += 4) {
		if (readWord(address) != data)
			break;
	}
	if (address == 0x1800000)
		bank2_ef = true;
	else
		bank2_size = 0x0;
	// Bank 3
	for (address = 0x1ffff00; address < 0x2000000; address += 4)
		writeWord(address, data);
	for (address = 0x1ffff00; address < 0x2000000; address += 4) {
		if (readWord(address) != data)
			break;
	}
	if (address == 0x2000000)
		bank3_ef = true;

	// Reconfig the system with the new bank0 (and maybe bank1) size.
	if (bank0_size == 0x10000000)
		bank1_ef = false;
	if (bank1_ef == true) {
		mapMemoryBanks0and1(0, bank0_size, bank0_size, bank0_size);
		// Fix the PCI bars
		pci0MapMemoryBanks0_1(0, bank0_size * 2);
		pci1MapMemoryBanks0_1(0, bank0_size * 2);
		total_size += bank0_size * 2;
	} else {
		mapMemoryBanks0and1(0, bank0_size, bank0_size, 0);
		// Fix the PCI bars
		pci0MapMemoryBanks0_1(0, bank0_size);
		pci1MapMemoryBanks0_1(0, bank0_size);
		total_size += bank0_size;
	}
	if (total_size == 0x10000000) {
		bank2_ef = false;
		bank3_ef = false;
	} else {
		if ((total_size + bank2_size) > 0x10000000) {
			bank2_size = 0x10000000 - total_size;
			bank3_ef = false;
		} else {
			if (bank3_size + total_size + bank2_size >
			    0x10000000) {
				bank3_size =
				    0x10000000 - (total_size + bank2_size);
			}
		}
	}
	if (bank2_ef == true) {
		if (bank3_ef == true) {
			mapMemoryBanks2and3(total_size, bank2_size,
					    total_size + bank2_size,
					    bank3_size);
			// Fix the PCI bars
			pci0MapMemoryBanks2_3(total_size,
					      bank2_size + bank3_size);
			pci1MapMemoryBanks2_3(total_size,
					      bank2_size + bank3_size);
			total_size += (bank2_size + bank3_size);
		} else {
			mapMemoryBanks2and3(total_size, bank2_size,
					    total_size + bank2_size, 0);
			// Fix the PCI bars
			pci0MapMemoryBanks2_3(total_size, bank2_size);
			pci1MapMemoryBanks2_3(total_size, bank2_size);
			total_size += bank2_size;
		}
	} else {
		mapMemoryBanks2and3(total_size, 0, total_size, 0);
		pci0MapMemoryBanks2_3(total_size, 0);
		pci1MapMemoryBanks2_3(total_size, 0);
	}
	/* Reorganize the devices memory map */
	mapDevices0_1and2MemorySpace(0x1c000000, 0x800000, 0x1a000000,
				     0xc00000, 0x1d000000, 0x800000);

	XferToRam();
	return 0;		// Not that we'll ever get to this line of code, but
	// it does satisfy a compiler warning.
}
