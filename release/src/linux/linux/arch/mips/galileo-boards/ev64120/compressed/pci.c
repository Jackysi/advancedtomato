/* PCI.c - PCI functions */

/* Copyright - Galileo technology. */

#ifdef __linux__
#include <asm/galileo-boards/evb64120A/core.h>
#include <asm/galileo-boards/evb64120A/pci.h>
#ifndef PROM
#include <linux/kernel.h>
#endif

#undef PCI_DEBUG

#ifdef PCI_DEBUG
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif

#else
#include "core.h"
#include "pci.h"
#include <string.h>
#endif

/********************************************************************
* pci0ScanDevices   - This function scan PCI0 bus, if found any device on
*                     this bus it interrogate the Device for the information
*                     it can discover.
*                     The fields with all information are the following:
*    char            type[20];
*    unsigned int    deviceNum;
*    unsigned int    venID;
*    unsigned int    deviceID;
*    unsigned int    bar0Base;
*    unsigned int    bar0Size;
*    unsigned int    bar1Base;
*    unsigned int    bar1Size;
*    unsigned int    bar2Base;
*    unsigned int    bar2Size;
*    unsigned int    bar3Base;
*    unsigned int    bar3Size;
*    unsigned int    bar4Base;
*    unsigned int    bar4Size;
*    unsigned int    bar5Base;
*    unsigned int    bar5Size;
*
* Inputs:   PCI0_DEVICE* pci0Detect - Pointer to an array of STRUCT PCI0_DEVICE.
*           unsigned int numberOfElment - The PCI0_DEVICE Array length.
* Output:   None.
*********************************************************************/

void pci0ScanDevices(PCI_DEVICE * pci0Detect, unsigned int numberOfElment)
{
	PCI_DEVICE *pci0ArrayPointer = pci0Detect;
	unsigned int id;	/* PCI Configuration register 0x0. */
	unsigned int device;	/* device`s Counter. */
	unsigned int classCode;	/* PCI Configuration register 0x8 */
	unsigned int arrayCounter = 0;
	unsigned int memBaseAddress;
	unsigned int memSize;
	unsigned int c18RegValue;

	PCI0_MASTER_ENABLE(SELF);
	/* According to PCI REV 2.1 MAX agents on the bus are -21- */
	for (device = 6; device < 8; device++) {
		id = pci0ReadConfigReg(PCI_0DEVICE_AND_VENDOR_ID, device);
		GT_REG_READ(INTERRUPT_CAUSE_REGISTER, &c18RegValue);
		/* Clearing bit 18 of in the Cause Register 0xc18 by writting 0. */
		GT_REG_WRITE(INTERRUPT_CAUSE_REGISTER,
			     c18RegValue & 0xfffbffff);
		if ((id != 0xffffffff) && !(c18RegValue & 0x40000)) {
			classCode =
			    pci0ReadConfigReg
			    (PCI_0CLASS_CODE_AND_REVISION_ID, device);
			pci0ArrayPointer->deviceNum = device;
			pci0ArrayPointer->venID = (id & 0xffff);
			pci0ArrayPointer->deviceID =
			    ((id & 0xffff0000) >> 16);
			DBG("\nrr: venID %x devID %x\n",
			    pci0ArrayPointer->venID,
			    pci0ArrayPointer->deviceID);
			DBG("rr: device found %x\n",
			    pci0ArrayPointer->deviceNum);
			/* BAR0 parameters */
			memBaseAddress = pci0ReadConfigReg(BAR0, device);
			pci0ArrayPointer->bar0Type = memBaseAddress & 1;
			pci0ArrayPointer->bar0Base =
			    memBaseAddress & 0xfffff000;
			pci0WriteConfigReg(BAR0, device, 0xffffffff);
			memSize = pci0ReadConfigReg(BAR0, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci0ArrayPointer->bar0Size = 0;
			} else {
				if (pci0ArrayPointer->bar0Type == 0)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci0ArrayPointer->bar0Size = memSize;
			}
			DBG("rr: device BAR0 size %x\n", memSize);
			DBG("rr: device BAR0 address %x\n",
			    memBaseAddress);
			pci0WriteConfigReg(BAR0, device, memBaseAddress);
			/* BAR1 parameters */
			memBaseAddress = pci0ReadConfigReg(BAR1, device);
			pci0ArrayPointer->bar1Type = memBaseAddress & 1;
			pci0ArrayPointer->bar1Base =
			    memBaseAddress & 0xfffff000;
			pci0WriteConfigReg(BAR1, device, 0xffffffff);
			memSize = pci0ReadConfigReg(BAR1, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci0ArrayPointer->bar1Size = 0;
			} else {
				if (pci0ArrayPointer->bar1Type == 0)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci0ArrayPointer->bar1Size = memSize;
			}
			DBG("rr: device BAR1 size %x\n", memSize);
			DBG("rr: device BAR1 address %x\n",
			    memBaseAddress);
			pci0WriteConfigReg(BAR1, device, memBaseAddress);
			/* BAR2 parameters */
			memBaseAddress = pci0ReadConfigReg(BAR2, device);
			pci0ArrayPointer->bar2Type = memBaseAddress & 1;
			pci0ArrayPointer->bar2Base =
			    memBaseAddress & 0xfffff000;
			pci0WriteConfigReg(BAR2, device, 0xffffffff);
			memSize = pci0ReadConfigReg(BAR2, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci0ArrayPointer->bar2Size = 0;
			} else {
				if (pci0ArrayPointer->bar2Type == 0)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci0ArrayPointer->bar2Size = memSize;
			}
			DBG("rr: device BAR2 size %x\n", memSize);
			DBG("rr: device BAR2 address %x\n",
			    memBaseAddress);
			pci0WriteConfigReg(BAR2, device, memBaseAddress);
			/* BAR3 parameters */
			memBaseAddress = pci0ReadConfigReg(BAR3, device);
			pci0ArrayPointer->bar3Type = memBaseAddress & 1;
			pci0ArrayPointer->bar3Base =
			    memBaseAddress & 0xfffff000;
			pci0WriteConfigReg(BAR3, device, 0xffffffff);
			memSize = pci0ReadConfigReg(BAR3, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci0ArrayPointer->bar3Size = 0;
			} else {
				if (pci0ArrayPointer->bar3Type == 0)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci0ArrayPointer->bar3Size = memSize;
			}
			DBG("rr: device BAR3 size %x\n", memSize);
			DBG("rr: device BAR3 address %x\n",
			    memBaseAddress);
			pci0WriteConfigReg(BAR3, device, memBaseAddress);
			/* BAR4 parameters */
			memBaseAddress = pci0ReadConfigReg(BAR4, device);
			pci0ArrayPointer->bar4Type = memBaseAddress & 1;
			pci0ArrayPointer->bar4Base =
			    memBaseAddress & 0xfffff000;
			pci0WriteConfigReg(BAR4, device, 0xffffffff);
			memSize = pci0ReadConfigReg(BAR4, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci0ArrayPointer->bar4Size = 0;
			} else {
				if (pci0ArrayPointer->bar4Type == 0)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci0ArrayPointer->bar4Size = memSize;
			}
			DBG("rr: device BAR4 size %x\n", memSize);
			DBG("rr: device BAR4 address %x\n",
			    memBaseAddress);
			pci0WriteConfigReg(BAR4, device, memBaseAddress);
			/* BAR5 parameters */
			memBaseAddress = pci0ReadConfigReg(BAR5, device);
			pci0ArrayPointer->bar5Type = memBaseAddress & 1;
			pci0ArrayPointer->bar5Base =
			    memBaseAddress & 0xfffff000;
			pci0WriteConfigReg(BAR5, device, 0xffffffff);
			memSize = pci0ReadConfigReg(BAR5, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci0ArrayPointer->bar5Size = 0;
			} else {
				if (pci0ArrayPointer->bar5Type == 1)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci0ArrayPointer->bar5Size = memSize;
			}
			DBG("rr: device BAR5 size %x\n", memSize);
			DBG("rr: device BAR5 address %x\n",
			    memBaseAddress);
			pci0WriteConfigReg(BAR5, device, memBaseAddress);
			/* End of BARs Detection. */

			classCode = ((classCode & 0xff000000) >> 24);
			switch (classCode) {
			case 0x0:
				strcpy(pci0ArrayPointer->type,
				       "Old generation device");
				break;
			case 0x1:
				strcpy(pci0ArrayPointer->type,
				       "Mass storage controller");
				break;
			case 0x2:
				strcpy(pci0ArrayPointer->type,
				       "Network controller");
				break;
			case 0x3:
				strcpy(pci0ArrayPointer->type,
				       "Display controller");
				break;
			case 0x4:
				strcpy(pci0ArrayPointer->type,
				       "Multimedia device");
				break;
			case 0x5:
				strcpy(pci0ArrayPointer->type,
				       "Memory controller");
				break;
			case 0x6:
				strcpy(pci0ArrayPointer->type,
				       "Bridge Device");
				break;
			case 0x7:
				strcpy(pci0ArrayPointer->type,
				       "Simple Communication controllers");
				break;
			case 0x8:
				strcpy(pci0ArrayPointer->type,
				       "Base system peripherals");
				break;
			case 0x9:
				strcpy(pci0ArrayPointer->type,
				       "Input Devices");
				break;
			case 0xa:
				strcpy(pci0ArrayPointer->type,
				       "Docking stations");
				break;
			case 0xb:
				strcpy(pci0ArrayPointer->type,
				       "Processors");
				break;
			case 0xc:
				strcpy(pci0ArrayPointer->type,
				       "Serial bus controllers");
				break;
			case 0xd:
				strcpy(pci0ArrayPointer->type,
				       "Wireless controllers");
				break;
			case 0xe:
				strcpy(pci0ArrayPointer->type,
				       "Intelligent I/O controllers");
				break;
			case 0xf:
				strcpy(pci0ArrayPointer->type,
				       "Satellite communication controllers");
				break;
			case 0x10:
				strcpy(pci0ArrayPointer->type,
				       "Encryption/Decryption controllers");
				break;
			case 0x11:
				strcpy(pci0ArrayPointer->type,
				       "Data acquisition and signal processing controllers");
				break;
			default:
				break;

			}
			arrayCounter++;	/* point to the next element in the Array. */
			if (arrayCounter == numberOfElment)
				return;	/* When the Array is fully used, return. */
			/* Else, points to next free Element. */
			pci0ArrayPointer = &pci0Detect[arrayCounter];
		}
	}
	pci0ArrayPointer->deviceNum = 0;	/* 0 => End of List */
}


/********************************************************************
* pci1ScanDevices   - This function scan PCI1 bus, if found any device on
*                     this bus it interrogate the Device for the information
*                     it can discover.
*                     The fields with all information are the following:
*    char            type[20];
*    unsigned int    deviceNum;
*    unsigned int    venID;
*    unsigned int    deviceID;
*    unsigned int    bar0Base;
*    unsigned int    bar0Size;
*    unsigned int    bar1Base;
*    unsigned int    bar1Size;
*    unsigned int    bar2Base;
*    unsigned int    bar2Size;
*    unsigned int    bar3Base;
*    unsigned int    bar3Size;
*    unsigned int    bar4Base;
*    unsigned int    bar4Size;
*    unsigned int    bar5Base;
*    unsigned int    bar5Size;
*
* Inputs:   Pointer to an array of STRUCT PCI1_DEVICE.
* Output:   None.
*********************************************************************/

void pci1ScanDevices(PCI_DEVICE * pci1Detect, unsigned int numberOfElment)
{
	PCI_DEVICE *pci1ArrayPointer = pci1Detect;
	unsigned int id;	/* PCI Configuration register 0x0. */
	unsigned int device;	/* device`s Counter. */
	unsigned int classCode;	/* PCI Configuration register 0x8 */
	unsigned int arrayCounter = 0;
	unsigned int memBaseAddress;
	unsigned int memSize;
	unsigned int c98RegValue;

	PCI1_MASTER_ENABLE(SELF);
	/* According to PCI REV 2.1 MAX agents on the bus are -21- */
	for (device = 1; device < 22; device++) {
		id = pci1ReadConfigReg(PCI_0DEVICE_AND_VENDOR_ID, device);
		GT_REG_READ(HIGH_INTERRUPT_CAUSE_REGISTER, &c98RegValue);
		/* Clearing bit 18 of in the High Cause Register 0xc98 */
		GT_REG_WRITE(HIGH_INTERRUPT_CAUSE_REGISTER,
			     c98RegValue & 0xfffbffff);
		if ((id != 0xffffffff) && !(c98RegValue & 0x40000)) {
			classCode =
			    pci1ReadConfigReg
			    (PCI_0CLASS_CODE_AND_REVISION_ID, device);
			pci1ArrayPointer->deviceNum = device;
			pci1ArrayPointer->venID = (id & 0xffff);
			pci1ArrayPointer->deviceID =
			    ((id & 0xffff0000) >> 16);

			/* BAR0 parameters */
			memBaseAddress = pci1ReadConfigReg(BAR0, device);
			pci1ArrayPointer->bar0Type = memBaseAddress & 1;
			pci1ArrayPointer->bar0Base =
			    memBaseAddress & 0xfffff000;
			pci1WriteConfigReg(BAR0, device, 0xffffffff);
			memSize = pci1ReadConfigReg(BAR0, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci1ArrayPointer->bar0Size = 0;
			} else {
				if (pci1ArrayPointer->bar0Type == 0)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci1ArrayPointer->bar0Size = memSize;
			}
			pci1WriteConfigReg(BAR0, device, memBaseAddress);
			/* BAR1 parameters */
			memBaseAddress = pci1ReadConfigReg(BAR1, device);
			pci1ArrayPointer->bar1Type = memBaseAddress & 1;
			pci1ArrayPointer->bar1Base =
			    memBaseAddress & 0xfffff000;
			pci1WriteConfigReg(BAR1, device, 0xffffffff);
			memSize = pci1ReadConfigReg(BAR1, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci1ArrayPointer->bar1Size = 0;
			} else {
				if (pci1ArrayPointer->bar1Type == 0)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci1ArrayPointer->bar1Size = memSize;
			}
			pci1WriteConfigReg(BAR1, device, memBaseAddress);
			/* BAR2 parameters */
			memBaseAddress = pci1ReadConfigReg(BAR2, device);
			pci1ArrayPointer->bar2Type = memBaseAddress & 1;
			pci1ArrayPointer->bar2Base =
			    memBaseAddress & 0xfffff000;
			pci1WriteConfigReg(BAR2, device, 0xffffffff);
			memSize = pci1ReadConfigReg(BAR2, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci1ArrayPointer->bar2Size = 0;
			} else {
				if (pci1ArrayPointer->bar2Type == 0)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci1ArrayPointer->bar2Size = memSize;
			}
			pci1WriteConfigReg(BAR2, device, memBaseAddress);
			/* BAR3 parameters */
			memBaseAddress = pci1ReadConfigReg(BAR3, device);
			pci1ArrayPointer->bar3Type = memBaseAddress & 1;
			pci1ArrayPointer->bar3Base =
			    memBaseAddress & 0xfffff000;
			pci1WriteConfigReg(BAR3, device, 0xffffffff);
			memSize = pci1ReadConfigReg(BAR3, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci1ArrayPointer->bar3Size = 0;
			} else {
				if (pci1ArrayPointer->bar3Type == 0)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci1ArrayPointer->bar3Size = memSize;
			}
			pci1WriteConfigReg(BAR3, device, memBaseAddress);
			/* BAR4 parameters */
			memBaseAddress = pci1ReadConfigReg(BAR4, device);
			pci1ArrayPointer->bar4Type = memBaseAddress & 1;
			pci1ArrayPointer->bar4Base =
			    memBaseAddress & 0xfffff000;
			pci1WriteConfigReg(BAR4, device, 0xffffffff);
			memSize = pci1ReadConfigReg(BAR4, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci1ArrayPointer->bar4Size = 0;
			} else {
				if (pci1ArrayPointer->bar4Type == 0)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci1ArrayPointer->bar4Size = memSize;
			}
			pci1WriteConfigReg(BAR4, device, memBaseAddress);
			/* BAR5 parameters */
			memBaseAddress = pci1ReadConfigReg(BAR5, device);
			pci1ArrayPointer->bar5Type = memBaseAddress & 1;
			pci1ArrayPointer->bar5Base =
			    memBaseAddress & 0xfffff000;
			pci1WriteConfigReg(BAR5, device, 0xffffffff);
			memSize = pci1ReadConfigReg(BAR5, device);
			if (memSize == 0) {	/* case of an empty BAR */
				pci1ArrayPointer->bar5Size = 0;
			} else {
				if (pci1ArrayPointer->bar5Type == 0)	/* memory space */
					memSize =
					    ~(memSize & 0xfffffff0) + 1;
				else	/* IO space */
					memSize =
					    ~(memSize & 0xfffffffc) + 1;
				pci1ArrayPointer->bar5Size = memSize;
			}
			pci1WriteConfigReg(BAR5, device, memBaseAddress);
			/* End of BARs Detection. */

			classCode = ((classCode & 0xff000000) >> 24);
			switch (classCode) {
			case 0x0:
				strcpy(pci1ArrayPointer->type,
				       "Old generation device");
				break;
			case 0x1:
				strcpy(pci1ArrayPointer->type,
				       "Mass storage controller");
				break;
			case 0x2:
				strcpy(pci1ArrayPointer->type,
				       "Network controller");
				break;
			case 0x3:
				strcpy(pci1ArrayPointer->type,
				       "Display controller");
				break;
			case 0x4:
				strcpy(pci1ArrayPointer->type,
				       "Multimedia device");
				break;
			case 0x5:
				strcpy(pci1ArrayPointer->type,
				       "Memory controller");
				break;
			case 0x6:
				strcpy(pci1ArrayPointer->type,
				       "Bridge Device");
				break;
			case 0x7:
				strcpy(pci1ArrayPointer->type,
				       "Simple Communication controllers");
				break;
			case 0x8:
				strcpy(pci1ArrayPointer->type,
				       "Base system peripherals");
				break;
			case 0x9:
				strcpy(pci1ArrayPointer->type,
				       "Input Devices");
				break;
			case 0xa:
				strcpy(pci1ArrayPointer->type,
				       "Docking stations");
				break;
			case 0xb:
				strcpy(pci1ArrayPointer->type,
				       "Processors");
				break;
			case 0xc:
				strcpy(pci1ArrayPointer->type,
				       "Serial bus controllers");
				break;
			case 0xd:
				strcpy(pci1ArrayPointer->type,
				       "Wireless controllers");
				break;
			case 0xe:
				strcpy(pci1ArrayPointer->type,
				       "Intelligent I/O controllers");
				break;
			case 0xf:
				strcpy(pci1ArrayPointer->type,
				       "Satellite communication controllers");
				break;
			case 0x10:
				strcpy(pci1ArrayPointer->type,
				       "Encryption/Decryption controllers");
				break;
			case 0x11:
				strcpy(pci1ArrayPointer->type,
				       "Data acquisition and signal processing controllers");
				break;
			}
			arrayCounter++;	/* point to the next element in the Array. */
			if (arrayCounter == numberOfElment)
				return;	/* When the Array is fully used, return. */
			/* Else, points to next free Element. */
			pci1ArrayPointer = &pci1Detect[arrayCounter];
		}
	}
	pci1ArrayPointer->deviceNum = 0;	/* 0 => End of List */
}

/********************************************************************
* pci0WriteConfigReg - Write to a PCI configuration register
*                    - Make sure the GT is configured as a master before
*                      writingto another device on the PCI.
*                    - The function takes care of Big/Little endian conversion.
* Inputs:   unsigned int regOffset: The register offset as it apears in the GT spec
*                   (or any other PCI device spec)
*           pciDevNum: The device number needs to be addressed.
*
*  Configuration Address 0xCF8:
*
*       31 30    24 23  16 15  11 10     8 7      2  0     <=bit Number
*  |congif|Reserved|  Bus |Device|Function|Register|00|
*  |Enable|        |Number|Number| Number | Number |  |    <=field Name
*
*********************************************************************/

void pci0WriteConfigReg(unsigned int regOffset, unsigned int pciDevNum,
			unsigned int data)
{
	unsigned int DataForRegCf8;
	unsigned int functionNum;

	functionNum = regOffset & 0x00000700;
	pciDevNum = pciDevNum << 11;
	regOffset = regOffset & 0x0fffffff;
	DataForRegCf8 = (regOffset | pciDevNum | functionNum) | BIT31;
	GT_REG_WRITE(PCI_0CONFIGURATION_ADDRESS, DataForRegCf8);
	if (pciDevNum == SELF) {	/* This board */
		GT_REG_WRITE(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER,
			     data);
	} else {		/* configuration Transaction over the pci. */

		/* The PCI is working in LE Mode So it swap the Data. */
		GT_REG_WRITE(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER,
			     WORDSWAP(data));
	}
}

/********************************************************************
* pci1WriteConfigReg - Write to a PCI configuration register
*                   - Make sure the GT is configured as a master before writing
*                     to another device on the PCI.
*                   - The function takes care of Big/Little endian conversion.
* Inputs:   unsigned int regOffset: The register offset as it apears in the
*           GT spec (or any other PCI device spec)
*           pciDevNum: The device number needs to be addressed.
*
*  Configuration Address 0xCF8:
*
*       31 30    24 23  16 15  11 10     8 7      2  0     <=bit Number
*  |congif|Reserved|  Bus |Device|Function|Register|00|
*  |Enable|        |Number|Number| Number | Number |  |    <=field Name
*
*********************************************************************/

void pci1WriteConfigReg(unsigned int regOffset, unsigned int pciDevNum,
			unsigned int data)
{
	unsigned int DataForRegCf8;
	unsigned int functionNum;

	functionNum = regOffset & 0x00000700;
	pciDevNum = pciDevNum << 11;
	regOffset = regOffset & 0x0fffffff;
	if (pciDevNum == SELF) {	/* This board */
		/* when configurating our own PCI 1 L-unit the access is through
		   the PCI 0 interface with reg number = reg number + 0x80 */
		DataForRegCf8 =
		    (regOffset | pciDevNum | functionNum | 0x80) | BIT31;
		GT_REG_WRITE(PCI_0CONFIGURATION_ADDRESS, DataForRegCf8);
	} else {
		DataForRegCf8 =
		    (regOffset | pciDevNum | functionNum) | BIT31;
		GT_REG_WRITE(PCI_1CONFIGURATION_ADDRESS, DataForRegCf8);
	}
	if (pciDevNum == SELF) {	/* This board */
		GT_REG_WRITE(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER,
			     data);
	} else {
		GT_REG_WRITE(PCI_1CONFIGURATION_DATA_VIRTUAL_REGISTER,
			     WORDSWAP(data));
	}
}

/********************************************************************
* pci0ReadConfigReg  - Read from a PCI0 configuration register
*                    - Make sure the GT is configured as a master before
*                      reading from another device on the PCI.
*                   - The function takes care of Big/Little endian conversion.
* INPUTS:   regOffset: The register offset as it apears in the GT spec (or PCI
*                        spec)
*           pciDevNum: The device number needs to be addressed.
* RETURNS: data , if the data == 0xffffffff check the master abort bit in the
*                 cause register to make sure the data is valid
*
*  Configuration Address 0xCF8:
*
*       31 30    24 23  16 15  11 10     8 7      2  0     <=bit Number
*  |congif|Reserved|  Bus |Device|Function|Register|00|
*  |Enable|        |Number|Number| Number | Number |  |    <=field Name
*
*********************************************************************/

unsigned int pci0ReadConfigReg(unsigned int regOffset,
			       unsigned int pciDevNum)
{
	unsigned int DataForRegCf8;
	unsigned int data;
	unsigned int functionNum;

	functionNum = regOffset & 0x00000700;
	pciDevNum = pciDevNum << 11;
	regOffset = regOffset & 0x0fffffff;
	DataForRegCf8 = (regOffset | pciDevNum | functionNum) | BIT31;
	GT_REG_WRITE(PCI_0CONFIGURATION_ADDRESS, DataForRegCf8);
	if (pciDevNum == SELF) {	/* This board */
		GT_REG_READ(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER,
			    &data);
		return data;
	} else {		/* The PCI is working in LE Mode So it swap the Data. */

		GT_REG_READ(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER,
			    &data);
		return WORDSWAP(data);
	}
}

/********************************************************************
* pci1ReadConfigReg  - Read from a PCI1 configuration register
*                    - Make sure the GT is configured as a master before
*                      reading from another device on the PCI.
*                   - The function takes care of Big/Little endian conversion.
* INPUTS:   regOffset: The register offset as it apears in the GT spec (or PCI
*                        spec)
*           pciDevNum: The device number needs to be addressed.
* RETURNS: data , if the data == 0xffffffff check the master abort bit in the
*                 cause register to make sure the data is valid
*
*  Configuration Address 0xCF8:
*
*       31 30    24 23  16 15  11 10     8 7      2  0     <=bit Number
*  |congif|Reserved|  Bus |Device|Function|Register|00|
*  |Enable|        |Number|Number| Number | Number |  |    <=field Name
*
*********************************************************************/

unsigned int pci1ReadConfigReg(unsigned int regOffset,
			       unsigned int pciDevNum)
{
	unsigned int DataForRegCf8;
	unsigned int data;
	unsigned int functionNum;

	functionNum = regOffset & 0x00000700;
	pciDevNum = pciDevNum << 11;
	regOffset = regOffset & 0x0fffffff;
	if (pciDevNum == SELF) {	/* This board */
		/* when configurating our own PCI 1 L-unit the access is through
		   the PCI 0 interface with reg number = reg number + 0x80 */
		DataForRegCf8 =
		    (regOffset | pciDevNum | functionNum | 0x80) | BIT31;
		GT_REG_WRITE(PCI_0CONFIGURATION_ADDRESS, DataForRegCf8);
	} else {
		DataForRegCf8 =
		    (regOffset | pciDevNum | functionNum) | BIT31;
		GT_REG_WRITE(PCI_1CONFIGURATION_ADDRESS, DataForRegCf8);
	}
	if (pciDevNum == SELF) {	/* This board */
		GT_REG_READ(PCI_0CONFIGURATION_DATA_VIRTUAL_REGISTER,
			    &data);
		return data;
	} else {
		GT_REG_READ(PCI_1CONFIGURATION_DATA_VIRTUAL_REGISTER,
			    &data);
		return WORDSWAP(data);
	}
}

/********************************************************************
* pci0MapIOspace - Maps PCI0 IO space for the master.
* Inputs: base and length of pci0Io
*********************************************************************/

void pci0MapIOspace(unsigned int pci0IoBase, unsigned int pci0IoLength)
{
	unsigned int pci0IoTop =
	    (unsigned int) (pci0IoBase + pci0IoLength);

	if (pci0IoLength == 0)
		pci0IoTop++;

	pci0IoBase = (unsigned int) (pci0IoBase >> 21);
	pci0IoTop = (unsigned int) (((pci0IoTop - 1) & 0x0fffffff) >> 21);
	GT_REG_WRITE(PCI_0I_O_LOW_DECODE_ADDRESS, pci0IoBase);
	GT_REG_WRITE(PCI_0I_O_HIGH_DECODE_ADDRESS, pci0IoTop);
}

/********************************************************************
* pci1MapIOspace - Maps PCI1 IO space for the master.
* Inputs: base and length of pci1Io
*********************************************************************/

void pci1MapIOspace(unsigned int pci1IoBase, unsigned int pci1IoLength)
{
	unsigned int pci1IoTop =
	    (unsigned int) (pci1IoBase + pci1IoLength);

	if (pci1IoLength == 0)
		pci1IoTop++;

	pci1IoBase = (unsigned int) (pci1IoBase >> 21);
	pci1IoTop = (unsigned int) (((pci1IoTop - 1) & 0x0fffffff) >> 21);
	GT_REG_WRITE(PCI_1I_O_LOW_DECODE_ADDRESS, pci1IoBase);
	GT_REG_WRITE(PCI_1I_O_HIGH_DECODE_ADDRESS, pci1IoTop);
}

/********************************************************************
* pci0MapMemory0space - Maps PCI0 memory0 space for the master.
* Inputs: base and length of pci0Mem0
*********************************************************************/


void pci0MapMemory0space(unsigned int pci0Mem0Base,
			 unsigned int pci0Mem0Length)
{
	unsigned int pci0Mem0Top = pci0Mem0Base + pci0Mem0Length;

	if (pci0Mem0Length == 0)
		pci0Mem0Top++;

	pci0Mem0Base = pci0Mem0Base >> 21;
	pci0Mem0Top = ((pci0Mem0Top - 1) & 0x0fffffff) >> 21;
	GT_REG_WRITE(PCI_0MEMORY0_LOW_DECODE_ADDRESS, pci0Mem0Base);
	GT_REG_WRITE(PCI_0MEMORY0_HIGH_DECODE_ADDRESS, pci0Mem0Top);
}

/********************************************************************
* pci1MapMemory0space - Maps PCI1 memory0 space for the master.
* Inputs: base and length of pci1Mem0
*********************************************************************/

void pci1MapMemory0space(unsigned int pci1Mem0Base,
			 unsigned int pci1Mem0Length)
{
	unsigned int pci1Mem0Top = pci1Mem0Base + pci1Mem0Length;

	if (pci1Mem0Length == 0)
		pci1Mem0Top++;

	pci1Mem0Base = pci1Mem0Base >> 21;
	pci1Mem0Top = ((pci1Mem0Top - 1) & 0x0fffffff) >> 21;
	GT_REG_WRITE(PCI_1MEMORY0_LOW_DECODE_ADDRESS, pci1Mem0Base);
	GT_REG_WRITE(PCI_1MEMORY0_HIGH_DECODE_ADDRESS, pci1Mem0Top);
}

/********************************************************************
* pci0MapMemory1space - Maps PCI0 memory1 space for the master.
* Inputs: base and length of pci0Mem1
*********************************************************************/

void pci0MapMemory1space(unsigned int pci0Mem1Base,
			 unsigned int pci0Mem1Length)
{
	unsigned int pci0Mem1Top = pci0Mem1Base + pci0Mem1Length;

	if (pci0Mem1Length == 0)
		pci0Mem1Top++;

	pci0Mem1Base = pci0Mem1Base >> 21;
	pci0Mem1Top = ((pci0Mem1Top - 1) & 0x0fffffff) >> 21;
	GT_REG_WRITE(PCI_0MEMORY1_LOW_DECODE_ADDRESS, pci0Mem1Base);
	GT_REG_WRITE(PCI_0MEMORY1_HIGH_DECODE_ADDRESS, pci0Mem1Top);
#ifndef PROM
	DBG(KERN_INFO "pci0Mem1Base %x\n", pci0Mem1Base);
	DBG(KERN_INFO "pci0Mem1Top %x\n", pci0Mem1Top);
	GT_REG_READ(PCI_0MEMORY0_ADDRESS_REMAP, &pci0Mem1Base);
	DBG(KERN_INFO "Mem 0/0 remap %x\n", pci0Mem1Base);
	GT_REG_READ(PCI_0MEMORY1_ADDRESS_REMAP, &pci0Mem1Base);
	DBG(KERN_INFO "Mem 0/1 remap %x\n", pci0Mem1Base);
	GT_REG_WRITE(PCI_0MEMORY1_ADDRESS_REMAP, 0x500);
	GT_REG_READ(PCI_0MEMORY1_ADDRESS_REMAP, &pci0Mem1Base);
	DBG(KERN_INFO "Mem 0/1 remapped %x\n", pci0Mem1Base);
#endif
}

/********************************************************************
* pci1MapMemory1space - Maps PCI1 memory1 space for the master.
* Inputs: base and length of pci1Mem1
*********************************************************************/

void pci1MapMemory1space(unsigned int pci1Mem1Base,
			 unsigned int pci1Mem1Length)
{
	unsigned int pci1Mem1Top = pci1Mem1Base + pci1Mem1Length;

	if (pci1Mem1Length == 0)
		pci1Mem1Top++;

	pci1Mem1Base = pci1Mem1Base >> 21;
	pci1Mem1Top = ((pci1Mem1Top - 1) & 0x0fffffff) >> 21;
	GT_REG_WRITE(PCI_1MEMORY1_LOW_DECODE_ADDRESS, pci1Mem1Base);
	GT_REG_WRITE(PCI_1MEMORY1_HIGH_DECODE_ADDRESS, pci1Mem1Top);
}

/********************************************************************
* pci0GetIOspaceBase - Return PCI0 IO Base Address.
* Inputs: N/A
* Returns: PCI0 IO Base Address.
*********************************************************************/

unsigned int pci0GetIOspaceBase()
{
	unsigned int base;
	GT_REG_READ(PCI_0I_O_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	return base;
}

/********************************************************************
* pci0GetIOspaceSize - Return PCI0 IO Bar Size.
* Inputs: N/A
* Returns: PCI0 IO Bar Size.
*********************************************************************/

unsigned int pci0GetIOspaceSize()
{
	unsigned int top, base, size;
	GT_REG_READ(PCI_0I_O_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	GT_REG_READ(PCI_0I_O_HIGH_DECODE_ADDRESS, &top);
	top = (top << 21);
	size = ((top - base) & 0xfffffff);
	size = size | 0x1fffff;
	return (size + 1);
}

/********************************************************************
* pci0GetMemory0Base - Return PCI0 Memory 0 Base Address.
* Inputs: N/A
* Returns: PCI0 Memory 0 Base Address.
*********************************************************************/

unsigned int pci0GetMemory0Base()
{
	unsigned int base;
	GT_REG_READ(PCI_0MEMORY0_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	return base;
}

/********************************************************************
* pci0GetMemory0Size - Return PCI0 Memory 0 Bar Size.
* Inputs: N/A
* Returns: PCI0 Memory 0 Bar Size.
*********************************************************************/

unsigned int pci0GetMemory0Size()
{
	unsigned int top, base, size;
	GT_REG_READ(PCI_0MEMORY0_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	GT_REG_READ(PCI_0MEMORY0_HIGH_DECODE_ADDRESS, &top);
	top = (top << 21);
	size = ((top - base) & 0xfffffff);
	size = size | 0x1fffff;
	return (size + 1);
}

/********************************************************************
* pci0GetMemory1Base - Return PCI0 Memory 1 Base Address.
* Inputs: N/A
* Returns: PCI0 Memory 1 Base Address.
*********************************************************************/

unsigned int pci0GetMemory1Base()
{
	unsigned int base;
	GT_REG_READ(PCI_0MEMORY1_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	return base;
}

/********************************************************************
* pci0GetMemory1Size - Return PCI0 Memory 1 Bar Size.
* Inputs: N/A
* Returns: PCI0 Memory 1 Bar Size.
*********************************************************************/

unsigned int pci0GetMemory1Size()
{
	unsigned int top, base, size;
	GT_REG_READ(PCI_0MEMORY1_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	GT_REG_READ(PCI_0MEMORY1_HIGH_DECODE_ADDRESS, &top);
	top = (top << 21);
	size = ((top - base) & 0xfffffff);
	size = size | 0x1fffff;
	return (size + 1);
}

/********************************************************************
* pci1GetIOspaceBase - Return PCI1 IO Base Address.
* Inputs: N/A
* Returns: PCI1 IO Base Address.
*********************************************************************/

unsigned int pci1GetIOspaceBase()
{
	unsigned int base;
	GT_REG_READ(PCI_1I_O_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	return base;
}

/********************************************************************
* pci1GetIOspaceSize - Return PCI1 IO Bar Size.
* Inputs: N/A
* Returns: PCI1 IO Bar Size.
*********************************************************************/

unsigned int pci1GetIOspaceSize()
{
	unsigned int top, base, size;
	GT_REG_READ(PCI_1I_O_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	GT_REG_READ(PCI_1I_O_HIGH_DECODE_ADDRESS, &top);
	top = (top << 21);
	size = ((top - base) & 0xfffffff);
	size = size | 0x1fffff;
	return (size + 1);
}

/********************************************************************
* pci1GetMemory0Base - Return PCI1 Memory 0 Base Address.
* Inputs: N/A
* Returns: PCI1 Memory 0 Base Address.
*********************************************************************/

unsigned int pci1GetMemory0Base()
{
	unsigned int base;
	GT_REG_READ(PCI_1MEMORY0_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	return base;
}

/********************************************************************
* pci1GetMemory0Size - Return PCI1 Memory 0 Bar Size.
* Inputs: N/A
* Returns: PCI1 Memory 0 Bar Size.
*********************************************************************/

unsigned int pci1GetMemory0Size()
{
	unsigned int top, base, size;
	GT_REG_READ(PCI_1MEMORY1_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	GT_REG_READ(PCI_1MEMORY1_HIGH_DECODE_ADDRESS, &top);
	top = (top << 21);
	size = ((top - base) & 0xfffffff);
	size = size | 0x1fffff;
	return (size + 1);
}

/********************************************************************
* pci1GetMemory1Base - Return PCI1 Memory 1 Base Address.
* Inputs: N/A
* Returns: PCI1 Memory 1 Base Address.
*********************************************************************/

unsigned int pci1GetMemory1Base()
{
	unsigned int base;
	GT_REG_READ(PCI_1MEMORY1_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	return base;
}

/********************************************************************
* pci1GetMemory1Size - Return PCI1 Memory 1 Bar Size.
* Inputs: N/A
* Returns: PCI1 Memory 1 Bar Size.
*********************************************************************/

unsigned int pci1GetMemory1Size()
{
	unsigned int top, base, size;
	GT_REG_READ(PCI_1MEMORY1_LOW_DECODE_ADDRESS, &base);
	base = base << 21;
	GT_REG_READ(PCI_1MEMORY1_HIGH_DECODE_ADDRESS, &top);
	top = (top << 21);
	size = ((top - base) & 0xfffffff);
	size = size | 0x1fffff;
	return (size + 1);
}

/********************************************************************
* pci0MapInternalRegSpace - Maps the internal registers memory space for the
*                           slave.
*                           Stays the same for all GT devices Disco include
* Inputs: base of pci0 internal register
*********************************************************************/

void pci0MapInternalRegSpace(unsigned int pci0InternalBase)
{
	pci0InternalBase = pci0InternalBase & 0xfffff000;
	pci0InternalBase =
	    pci0InternalBase |
	    (pci0ReadConfigReg
	     (PCI_0INTERNAL_REGISTERS_MEMORY_MAPPED_BASE_ADDRESS,
	      SELF) & 0x00000fff);
	pci0WriteConfigReg
	    (PCI_0INTERNAL_REGISTERS_MEMORY_MAPPED_BASE_ADDRESS, SELF,
	     pci0InternalBase);
}

/********************************************************************
* pci1MapInternalRegSpace - Maps the internal registers memory space for the
*                           slave.
*                           Stays the same for all GT devices Disco include
* Inputs: base of pci1 internal register
*********************************************************************/

void pci1MapInternalRegSpace(unsigned int pci1InternalBase)
{
	pci1InternalBase = pci1InternalBase & 0xfffff000;
	pci1InternalBase =
	    pci1InternalBase |
	    (pci1ReadConfigReg
	     (PCI_0INTERNAL_REGISTERS_MEMORY_MAPPED_BASE_ADDRESS,
	      SELF) & 0x00000fff);
	pci1WriteConfigReg
	    (PCI_0INTERNAL_REGISTERS_MEMORY_MAPPED_BASE_ADDRESS, SELF,
	     pci1InternalBase);
}

/********************************************************************
* pci0MapInternalRegIOSpace - Maps the internal registers IO space for the
*                             slave.
*                             Stays the same for all GT devices Disco include
* Inputs: base of pci0 internal io register
*********************************************************************/

void pci0MapInternalRegIOSpace(unsigned int pci0InternalBase)
{
	pci0InternalBase = pci0InternalBase & 0xfffff000;
	pci0InternalBase =
	    pci0InternalBase |
	    (pci0ReadConfigReg
	     (PCI_0INTERNAL_REGISTERS_I_OMAPPED_BASE_ADDRESS,
	      0) & 0x00000fff);
	pci0WriteConfigReg(PCI_0INTERNAL_REGISTERS_I_OMAPPED_BASE_ADDRESS,
			   SELF, pci0InternalBase);
}

/********************************************************************
* pci0MapInternalRegIOSpace - Maps the internal registers IO space for the
*                             slave.
*                             Stays the same for all GT devices Disco include
* Inputs: base of pci1 internal io register
*********************************************************************/

void pci1MapInternalRegIOSpace(unsigned int pci1InternalBase)
{
	pci1InternalBase = pci1InternalBase & 0xfffff000;
	pci1InternalBase =
	    pci1InternalBase |
	    (pci1ReadConfigReg
	     (PCI_0INTERNAL_REGISTERS_I_OMAPPED_BASE_ADDRESS,
	      SELF) & 0x00000fff);
	pci1WriteConfigReg(PCI_0INTERNAL_REGISTERS_I_OMAPPED_BASE_ADDRESS,
			   SELF, pci1InternalBase);
}

/********************************************************************
* pci0MapMemoryBanks0_1 - Maps PCI0 memory banks 0 and 1 for the slave.
*                         for Discovery we need two function: SCS0 & SCS1
*                         (instead of SCS[1:0])
* Inputs: base and size of pci0 dram
*********************************************************************/


void pci0MapMemoryBanks0_1(unsigned int pci0Dram0_1Base,
			   unsigned int pci0Dram0_1Size)
{
	pci0Dram0_1Base = pci0Dram0_1Base & 0xfffff000;
	pci0Dram0_1Base =
	    pci0Dram0_1Base |
	    (pci0ReadConfigReg(PCI_0SCS_1_0_BASE_ADDRESS, SELF) &
	     0x00000fff);
	pci0WriteConfigReg(PCI_0SCS_1_0_BASE_ADDRESS, SELF,
			   pci0Dram0_1Base);
	/* swapped Bar */
	pci0WriteConfigReg(PCI_0SWAPPED_SCS_1_0_BASE_ADDRESS, SELF,
			   pci0Dram0_1Base);
	if (pci0Dram0_1Size == 0)
		pci0Dram0_1Size++;
	GT_REG_WRITE(PCI_0SCS_1_0_BANK_SIZE, pci0Dram0_1Size - 1);
}

/********************************************************************
* pci1MapMemoryBanks0_1 - Maps PCI1 memory banks 0 and 1 for the slave.
*                         for Discovery we need two function: SCS0 & SCS1
*                         (instead of SCS[1:0])
* Inputs: base and size of pci1 dram
*********************************************************************/

void pci1MapMemoryBanks0_1(unsigned int pci1Dram0_1Base,
			   unsigned int pci1Dram0_1Size)
{
	pci1Dram0_1Base = pci1Dram0_1Base & 0xfffff000;
	pci1Dram0_1Base =
	    pci1Dram0_1Base |
	    (pci1ReadConfigReg(PCI_0SCS_1_0_BASE_ADDRESS, SELF) &
	     0x00000fff);
	pci1WriteConfigReg(PCI_0SCS_1_0_BASE_ADDRESS, SELF,
			   pci1Dram0_1Base);
	/* swapped Bar */
	pci1WriteConfigReg(PCI_0SWAPPED_SCS_1_0_BASE_ADDRESS, SELF,
			   pci1Dram0_1Base);
	if (pci1Dram0_1Size == 0)
		pci1Dram0_1Size++;
	GT_REG_WRITE(PCI_1SCS_1_0_BANK_SIZE, pci1Dram0_1Size - 1);
}

/********************************************************************
* pci0MapMemoryBanks2_3 - Maps PCI0 memory banks 2 and 3 for the slave.
*                         for Discovery we need two function: SCS2 & SCS3
*                         (instead of SCS[3:2])
* Inputs: base and size of pci0 dram
*********************************************************************/

void pci0MapMemoryBanks2_3(unsigned int pci0Dram2_3Base,
			   unsigned int pci0Dram2_3Size)
{
	pci0Dram2_3Base = pci0Dram2_3Base & 0xfffff000;
	pci0Dram2_3Base =
	    pci0Dram2_3Base |
	    (pci0ReadConfigReg(PCI_0SCS_3_2_BASE_ADDRESS, SELF) &
	     0x00000fff);
	pci0WriteConfigReg(PCI_0SCS_3_2_BASE_ADDRESS, SELF,
			   pci0Dram2_3Base);
	/* swapped Bar */
	pci0WriteConfigReg(PCI_0SWAPPED_SCS_3_2_BASE_ADDRESS, SELF,
			   pci0Dram2_3Base);
	if (pci0Dram2_3Size == 0)
		pci0Dram2_3Size++;
	GT_REG_WRITE(PCI_0SCS_3_2_BANK_SIZE, pci0Dram2_3Size - 1);
}

/********************************************************************
* pci1MapMemoryBanks2_3 - Maps PCI1 memory banks 2 and 3 for the slave.
*                         for Discovery we need two function: SCS2 & SCS3
*                         (instead of SCS[3:2])
* Inputs: base and size of pci1 dram
*********************************************************************/

void pci1MapMemoryBanks2_3(unsigned int pci1Dram2_3Base,
			   unsigned int pci1Dram2_3Size)
{
	pci1Dram2_3Base = pci1Dram2_3Base & 0xfffff000;
	pci1Dram2_3Base =
	    pci1Dram2_3Base |
	    (pci1ReadConfigReg(PCI_0SCS_3_2_BASE_ADDRESS, SELF) &
	     0x00000fff);
	pci1WriteConfigReg(PCI_0SCS_3_2_BASE_ADDRESS, SELF,
			   pci1Dram2_3Base);
	/* swapped Bar */
	pci1WriteConfigReg(PCI_0SWAPPED_SCS_3_2_BASE_ADDRESS, SELF,
			   pci1Dram2_3Base);
	if (pci1Dram2_3Size == 0)
		pci1Dram2_3Size++;
	GT_REG_WRITE(PCI_1SCS_3_2_BANK_SIZE, pci1Dram2_3Size - 1);
}

/********************************************************************
* pci0MapDevices0_1and2MemorySpace - Maps PCI0 devices 0,1 and 2 memory spaces
*                                    for the slave.
*                                    For the Discovery there are 3 separate
*                                    fucnction's
* Inputs: base and lengthof pci0 devises012
*********************************************************************/


void pci0MapDevices0_1and2MemorySpace(unsigned int pci0Dev012Base,
				      unsigned int pci0Dev012Length)
{
	pci0Dev012Base = pci0Dev012Base & 0xfffff000;
	pci0Dev012Base =
	    pci0Dev012Base |
	    (pci0ReadConfigReg(PCI_0CS_2_0_BASE_ADDRESS, SELF) &
	     0x00000fff);
	pci0WriteConfigReg(PCI_0CS_2_0_BASE_ADDRESS, SELF, pci0Dev012Base);
	if (pci0Dev012Length == 0)
		pci0Dev012Length++;
	GT_REG_WRITE(PCI_0CS_2_0_BANK_SIZE, pci0Dev012Length - 1);
}

/********************************************************************
* pci1MapDevices0_1and2MemorySpace - Maps PCI1 devices 0,1 and 2 memory spaces
*                                    for the slave.
*                                    For the Discovery there are 3 separate
*                                    fucnction's
* Inputs: base and lengthof pci1 devises012
*********************************************************************/

void pci1MapDevices0_1and2MemorySpace(unsigned int pci1Dev012Base,
				      unsigned int pci1Dev012Length)
{
	pci1Dev012Base = pci1Dev012Base & 0xfffff000;
	pci1Dev012Base =
	    pci1Dev012Base |
	    (pci1ReadConfigReg(PCI_0CS_2_0_BASE_ADDRESS, SELF) &
	     0x00000fff);
	pci1WriteConfigReg(PCI_0CS_2_0_BASE_ADDRESS, SELF, pci1Dev012Base);
	if (pci1Dev012Length == 0)
		pci1Dev012Length++;
	GT_REG_WRITE(PCI_1CS_2_0_BANK_SIZE, pci1Dev012Length - 1);
}

/********************************************************************
* pci0MapDevices3andBootMemorySpace - Maps PCI0 devices 3 and boot memory
*                                     spaces for the slave.
*                                     For the Discovery there are 2 separate
*                                     fucnction's
* Inputs: base and length of pci0 device3/ boot
*********************************************************************/

void pci0MapDevices3andBootMemorySpace(unsigned int pci0Dev3andBootBase,
				       unsigned int pci0Dev3andBootLength)
{
	pci0Dev3andBootBase = pci0Dev3andBootBase & 0xfffff000;
	pci0Dev3andBootBase =
	    pci0Dev3andBootBase |
	    (pci0ReadConfigReg(PCI_0CS_3_BOOTCS_BASE_ADDRESS, SELF) &
	     0x00000fff);
	pci0WriteConfigReg(PCI_0CS_3_BOOTCS_BASE_ADDRESS, SELF,
			   pci0Dev3andBootBase);
	/* swapped Bar */
	pci0WriteConfigReg(PCI_0SWAPPED_CS_3_BOOTCS_BASE_ADDRESS, SELF,
			   pci0Dev3andBootBase);
	if (pci0Dev3andBootLength == 0)
		pci0Dev3andBootLength++;
	GT_REG_WRITE(PCI_0CS_3_BOOTCS_BANK_SIZE,
		     pci0Dev3andBootLength - 1);
}

/********************************************************************
* pci1MapDevices3andBootMemorySpace - Maps PCI1 devices 3 and boot memory
*                                     spaces for the slave.
*                                     For the Discovery there are 2 separate
*                                     fucnction's
* Inputs: base and length of pci1 device3/ boot
*********************************************************************/

void pci1MapDevices3andBootMemorySpace(unsigned int pci1Dev3andBootBase,
				       unsigned int pci1Dev3andBootLength)
{
	pci1Dev3andBootBase = pci1Dev3andBootBase & 0xfffff000;
	pci1Dev3andBootBase =
	    pci1Dev3andBootBase |
	    (pci1ReadConfigReg(PCI_0CS_3_BOOTCS_BASE_ADDRESS, SELF) &
	     0x00000fff);
	pci1WriteConfigReg(PCI_0CS_3_BOOTCS_BASE_ADDRESS, SELF,
			   pci1Dev3andBootBase);
	/* swapped Bar */
	pci1WriteConfigReg(PCI_0SWAPPED_CS_3_BOOTCS_BASE_ADDRESS, SELF,
			   pci1Dev3andBootBase);
	if (pci1Dev3andBootLength == 0)
		pci1Dev3andBootLength++;
	GT_REG_WRITE(PCI_1CS_3_BOOTCS_BANK_SIZE,
		     pci1Dev3andBootLength - 1);
}
