/* Memory.c - Memory mappings and remapping functions */

/* Copyright - Galileo technology. */

/*
DESCRIPTION
This file contains function which gives the user the ability to remap the
SDRAM memory and devices windows, please pay attention to overlapping windows
since the function do not take care of that for you.
When remapping the SDRAM or devices memory space pay attention to the PCI
mappings and make sure to coordinate between the two interfaces!!!
*/

/* includes */

#ifdef __linux__
#include <asm/galileo-boards/evb64120A/core.h>
#include <asm/galileo-boards/evb64120A/memory.h>
#else
#include "Core.h"
#include "Memory.h"
#endif

/********************************************************************
* getMemoryBankBaseAddress - Extract the base address of a memory bank
*      - If the memory bank size is 0 then this base address has no meaning !!!
*
* INPUTS:  MEMORY_BANK bank - SDRAM Bank number.
* OUTPUT:  N/A
* RETURNS: Memory bank base address.
*********************************************************************/
unsigned int getMemoryBankBaseAddress(MEMORY_BANK bank)
{
	unsigned int base, regBase;
	GT_REG_READ((SCS_1_0_LOW_DECODE_ADDRESS + (bank / 2) * 0x10),
		    &base);
	base = base << 21;
	GT_REG_READ((SCS_0_LOW_DECODE_ADDRESS + bank * 8), &regBase);
	base = base | (regBase << 20);
	return base;
}

/********************************************************************
* getDeviceBaseAddress - Extract the base address of a device.
*           - If the device size is 0 then this base address has no meaning!!!
*
* INPUT:   DEVICE device - Bank number.
* OUTPUT:  N/A
* RETURNS: Device base address.
*********************************************************************/
unsigned int getDeviceBaseAddress(DEVICE device)
{
	unsigned int base, regBase;
	GT_REG_READ((CS_2_0_LOW_DECODE_ADDRESS + (device / 3) * 0x10),
		    &base);
	base = base << 21;
	GT_REG_READ((CS_0_LOW_DECODE_ADDRESS + device * 0x8), &regBase);
	base = base | (regBase << 20);
	return base;
}

/********************************************************************
* getMemoryBankSize - Extract the size of a memory bank.
*
* INPUT:   MEMORY_BANK bank - Bank number
* OUTPUT:  N/A
* RETURNS: Memory bank size.
*********************************************************************/
unsigned int getMemoryBankSize(MEMORY_BANK bank)
{
	unsigned int size, base, value;
	base = getMemoryBankBaseAddress(bank);
	GT_REG_READ((SCS_0_HIGH_DECODE_ADDRESS + bank * 8), &size);
	size = ((size + 1) << 20) - (base & 0x0fffffff);
	GT_REG_READ((SCS_0_HIGH_DECODE_ADDRESS + bank * 8), &value);
	if (value == 0)
		return 0;
	else
		return size;
}

/********************************************************************
* getDeviceSize - Extract the size of a device memory space
*
* INPUT:    DEVICE device - Device number
* OUTPUT:   N/A
* RETURNS:  Size of a device memory space.
*********************************************************************/
unsigned int getDeviceSize(DEVICE device)
{
	unsigned int size, base, value;
	base = getDeviceBaseAddress(device);
	GT_REG_READ((CS_0_HIGH_DECODE_ADDRESS + device * 8), &size);
	size = ((size + 1) << 20) - (base & 0x0fffffff);
	GT_REG_READ((CS_0_HIGH_DECODE_ADDRESS + device * 8), &value);
	if ((value + 1) == 0)
		return 0;
	else
		return size;
}

/********************************************************************
* getDeviceWidth - A device can be with: 1,2,4 or 8 Bytes data width.
*                  The width is determine in registers: 'Device Parameters'
*                  registers (0x45c, 0x460, 0x464, 0x468, 0x46c - for each device.
*                  at bits: [21:20].
*
* INPUT:    DEVICE device - Device number
* OUTPUT:   N/A
* RETURNS:  Device width in Bytes (1,2,4, or 8), 0 if error had occurred.
*********************************************************************/
unsigned int getDeviceWidth(DEVICE device)
{
	unsigned int width;
	unsigned int regValue;

	GT_REG_READ(DEVICE_BANK0PARAMETERS + device * 4, &regValue);
	width = (regValue & 0x00300000) >> 20;
	switch (width) {
	case 0:
		return 1;
	case 1:
		return 2;
	case 2:
		return 4;
	case 3:
		return 8;
	default:
		return 0;
	}
}

/********************************************************************
* mapMemoryBanks0and1 - Sets new bases and boundaries for memory banks 0 and 1
*                     - Pay attention to the PCI mappings and make sure to
*                       coordinate between the two interfaces!!!
*                     - It is the programmer`s responsibility to make sure
*                       there are no conflicts with other memory spaces!!!
*                     - If a bank needs to be closed , give it a 0 length
*
*
* INPUTS: unsigned int bank0Base - required bank 0 base address.
*         unsigned int bank0Length - required bank 0 size.
*         unsigned int bank1Base - required bank 1 base address.
*         unsigned int bank1Length - required bank 1 size.
* RETURNS: true on success, false on failure or if one of the parameters is
*          erroneous.
*********************************************************************/
bool mapMemoryBanks0and1(unsigned int bank0Base, unsigned int bank0Length,
			 unsigned int bank1Base, unsigned int bank1Length)
{
	unsigned int mainBank0Top = bank0Base + bank0Length;
	unsigned int mainBank1Top = bank1Base + bank1Length;
	unsigned int memBank0Base, bank0Top;
	unsigned int memBank1Base, bank1Top;

	if (bank0Base <= bank1Base) {
		if ((bank0Base + bank0Length) > bank1Base)
			return false;
	} else {
		if ((bank1Base + bank1Length) > bank0Base)
			return false;
	}

	if (bank0Length == 0)
		mainBank0Top++;
	if (bank1Length == 0)
		mainBank1Top++;

	memBank0Base = ((unsigned int) (bank0Base & 0x0fffffff)) >> 20;
	bank0Top = ((unsigned int) (mainBank0Top & 0x0fffffff)) >> 20;
	memBank1Base = ((unsigned int) (bank1Base & 0x0fffffff)) >> 20;
	bank1Top = ((unsigned int) (mainBank1Top & 0x0fffffff)) >> 20;

	if (mainBank1Top > mainBank0Top) {
		bank0Base >>= 21;
		mainBank0Top =
		    ((unsigned int) (mainBank1Top & 0x0fffffff)) >> 21;
	} else {
		bank0Base = bank1Base >> 21;
		mainBank0Top =
		    ((unsigned int) (mainBank0Top & 0x0fffffff)) >> 21;
	}
	GT_REG_WRITE(SCS_1_0_LOW_DECODE_ADDRESS, bank0Base);
	if ((bank0Length + bank1Length) != 0) {
		GT_REG_WRITE(SCS_1_0_HIGH_DECODE_ADDRESS,
			     mainBank0Top - 1);
	} else {
		GT_REG_WRITE(SCS_1_0_HIGH_DECODE_ADDRESS, 0x0);
	}
	if (bank1Length != 0) {
		GT_REG_WRITE(SCS_1_HIGH_DECODE_ADDRESS, bank1Top - 1);
	} else {
		GT_REG_WRITE(SCS_1_HIGH_DECODE_ADDRESS, 0x0);
	}
	GT_REG_WRITE(SCS_1_LOW_DECODE_ADDRESS, memBank1Base);
	if (bank0Length != 0) {
		GT_REG_WRITE(SCS_0_HIGH_DECODE_ADDRESS, bank0Top - 1);
	} else {
		GT_REG_WRITE(SCS_0_HIGH_DECODE_ADDRESS, 0x0);
	}
	GT_REG_WRITE(SCS_0_LOW_DECODE_ADDRESS, memBank0Base);
	return true;
}

/********************************************************************
* mapMemoryBanks2and3 - Sets new bases and boundaries for memory banks 2 and 3
*                     - Pay attention to the PCI mappings and make sure to
*                       coordinate between the two interfaces!!!
*                     - It`s the programmer`s responsibility to make sure there
*                       are no conflicts with other memory spaces!!!
*                     - If a bank needs to be closed , give it a 0 length.
*
*
* INPUTS: unsigned int bank2Base - required bank 2 base address.
*         unsigned int bank2Length - required bank 2 size.
*         unsigned int bank3Base - required bank 3 base address.
*         unsigned int bank3Length - required bank 3 size.
* RETURNS: true on success, false on failure or if one of the parameters is
*          erroneous.
*********************************************************************/
bool mapMemoryBanks2and3(unsigned int bank2Base, unsigned int bank2Length,
			 unsigned int bank3Base, unsigned int bank3Length)
{
	unsigned int mainBank2Top =
	    (unsigned int) (bank2Base + bank2Length);
	unsigned int mainBank3Top =
	    (unsigned int) (bank3Base + bank3Length);
	unsigned int memBank2Base, bank2Top;
	unsigned int memBank3Base, bank3Top;

	if (bank2Base <= bank3Base) {
		if ((bank2Base + bank2Length) > bank3Base)
			return false;
	} else {
		if ((bank3Base + bank3Length) > bank2Base)
			return false;
	}
	if (bank2Length == 0)
		mainBank2Top++;
	if (bank3Length == 0)
		mainBank3Top++;

	memBank2Base = ((unsigned int) (bank2Base & 0x0fffffff)) >> 20;
	bank2Top = ((unsigned int) (mainBank2Top & 0x0fffffff)) >> 20;
	memBank3Base = ((unsigned int) (bank3Base & 0x0fffffff)) >> 20;
	bank3Top = ((unsigned int) (mainBank3Top & 0x0fffffff)) >> 20;

	if (mainBank3Top > mainBank2Top) {
		bank2Base >>= 21;
		mainBank2Top =
		    ((unsigned int) (mainBank3Top & 0x0fffffff)) >> 21;
	} else {
		bank2Base = bank3Base >> 21;
		mainBank2Top =
		    ((unsigned int) (mainBank2Top & 0x0fffffff)) >> 21;
	}
	GT_REG_WRITE(SCS_3_2_LOW_DECODE_ADDRESS, bank2Base);
	if ((bank2Length + bank3Length) != 0) {
		GT_REG_WRITE(SCS_3_2_HIGH_DECODE_ADDRESS,
			     mainBank2Top - 1);
	} else {
		GT_REG_WRITE(SCS_3_2_HIGH_DECODE_ADDRESS, 0x0);
	}
	if (bank3Length != 0) {
		GT_REG_WRITE(SCS_3_HIGH_DECODE_ADDRESS, bank3Top - 1);
	} else {
		GT_REG_WRITE(SCS_3_HIGH_DECODE_ADDRESS, 0x0);
	}
	GT_REG_WRITE(SCS_3_LOW_DECODE_ADDRESS, memBank3Base);
	if (bank2Length != 0) {
		GT_REG_WRITE(SCS_2_HIGH_DECODE_ADDRESS, bank2Top - 1);
	} else {
		GT_REG_WRITE(SCS_2_HIGH_DECODE_ADDRESS, 0x0);
	}
	GT_REG_WRITE(SCS_2_LOW_DECODE_ADDRESS, memBank2Base);
	return true;
}

/********************************************************************
* mapDevices0_1and2MemorySpace - Sets new bases and boundaries for devices 0,1
*                                and 2
*                     - Pay attention to the PCI mappings and make sure to
*                        coordinate between the two interfaces!!!
*                     - It`s the programmer`s responsibility to make sure there
*                       are no conflicts with other memory spaces!!!
*                     - If a device needs to be closed , give it a 0 length
*
*
* INPUTS: unsigned int device0Base - required cs_0 base address.
*         unsigned int device0Length - required cs_0 size.
*         unsigned int device1Base - required cs_1 base address.
*         unsigned int device1Length - required cs_0 size.
*         unsigned int device2Base - required cs_2 base address.
*         unsigned int device2Length - required cs_2 size.
* RETURNS: true on success, false on failure or if one of the parameters is
*          erroneous.
*********************************************************************/
bool mapDevices0_1and2MemorySpace(unsigned int device0Base,
				  unsigned int device0Length,
				  unsigned int device1Base,
				  unsigned int device1Length,
				  unsigned int device2Base,
				  unsigned int device2Length)
{
	unsigned int deviceBank0Top =
	    (unsigned int) (device0Base + device0Length);
	unsigned int deviceBank1Top =
	    (unsigned int) (device1Base + device1Length);
	unsigned int deviceBank2Top =
	    (unsigned int) (device2Base + device2Length);
	unsigned int device0BaseTemp = 0, device0TopTemp = 0;
	unsigned int bank0Base, bank0Top;
	unsigned int bank1Base, bank1Top;
	unsigned int bank2Base, bank2Top;
	bank0Base = ((unsigned int) (device0Base & 0x0fffffff)) >> 20;
	bank0Top = ((unsigned int) (deviceBank0Top & 0x0fffffff)) >> 20;
	bank1Base = ((unsigned int) (device1Base & 0x0fffffff)) >> 20;
	bank1Top = ((unsigned int) (deviceBank1Top & 0x0fffffff)) >> 20;
	bank2Base = ((unsigned int) (device2Base & 0x0fffffff)) >> 20;
	bank2Top = ((unsigned int) (deviceBank2Top & 0x0fffffff)) >> 20;

	if (device0Length == 0)
		deviceBank0Top++;
	if (device1Length == 0)
		deviceBank1Top++;
	if (device2Length == 0)
		deviceBank2Top++;

	if (device0Base <= device1Base && device0Base <= device2Base) {
		if ((device0Base + device0Length) > device1Base || \
		    (device0Base + device0Length) > device2Base)
			return false;
		if (device1Base <= device2Base) {
			if ((device1Base + device1Length) > device2Base)
				return false;
		} else {
			if ((device2Base + device2Length) > device1Base)
				return false;
		}
	}

	if (device1Base <= device0Base && device1Base <= device2Base) {
		if ((device1Base + device1Length) > device0Base ||
		    (device1Base + device1Length) > device2Base)
			return false;
		if (device0Base <= device2Base) {
			if ((device0Base + device0Length) > device2Base)
				return false;
		} else {
			if ((device2Base + device2Length) > device0Base)
				return false;
		}
	}

	if (device2Base <= device1Base && device2Base <= device0Base) {
		if ((device2Base + device2Length) > device1Base ||
		    (device2Base + device2Length) > device0Base)
			return false;
		if (device0Base <= device1Base) {
			if ((device0Base + device0Length) > device1Base)
				return false;
		} else {
			if ((device1Base + device1Length) > device0Base)
				return false;
		}
	}

	if ((deviceBank2Top > deviceBank1Top) && (deviceBank1Top >
						  deviceBank0Top)) {
		device0BaseTemp = device0Base >> 21;
		device0TopTemp =
		    ((unsigned int) (deviceBank2Top & 0x0fffffff)) >> 21;
	}
	if ((deviceBank2Top > deviceBank0Top)
	    && (deviceBank0Top > deviceBank1Top)) {
		device0BaseTemp = device1Base >> 21;
		device0TopTemp =
		    ((unsigned int) (deviceBank2Top & 0x0fffffff)) >> 21;
	}
	if ((deviceBank1Top > deviceBank2Top)
	    && (deviceBank2Top > deviceBank0Top)) {
		device0BaseTemp = device0Base >> 21;
		device0TopTemp =
		    ((unsigned int) (deviceBank1Top & 0x0fffffff)) >> 21;
	}
	if ((deviceBank1Top > deviceBank0Top)
	    && (deviceBank0Top > deviceBank2Top)) {
		device0BaseTemp = device2Base >> 21;
		device0TopTemp =
		    ((unsigned int) (deviceBank1Top & 0x0fffffff)) >> 21;
	}
	if ((deviceBank0Top > deviceBank2Top)
	    && (deviceBank2Top > deviceBank1Top)) {
		device0BaseTemp = device1Base >> 21;
		device0TopTemp =
		    ((unsigned int) (deviceBank0Top & 0x0fffffff)) >> 21;
	}
	if ((deviceBank0Top > deviceBank1Top)
	    && (deviceBank1Top > deviceBank2Top)) {
		device0BaseTemp = device2Base >> 21;
		device0TopTemp =
		    ((unsigned int) (deviceBank0Top & 0x0fffffff)) >> 21;
	}
	GT_REG_WRITE(CS_2_0_LOW_DECODE_ADDRESS, device0BaseTemp);
	if ((device0Length + device1Length + device2Length) != 0) {
		GT_REG_WRITE(CS_2_0_HIGH_DECODE_ADDRESS,
			     device0TopTemp - 1);
	} else {
		GT_REG_WRITE(CS_2_0_HIGH_DECODE_ADDRESS, 0x0);
	}
	GT_REG_WRITE(CS_0_LOW_DECODE_ADDRESS, bank0Base);
	if (device0Length != 0) {
		GT_REG_WRITE(CS_0_HIGH_DECODE_ADDRESS, bank0Top - 1);
	} else {
		GT_REG_WRITE(CS_0_HIGH_DECODE_ADDRESS, 0x0);
	}
	GT_REG_WRITE(CS_1_LOW_DECODE_ADDRESS, bank1Base);
	if (device1Length != 0) {
		GT_REG_WRITE(CS_1_HIGH_DECODE_ADDRESS, bank1Top - 1);
	} else {
		GT_REG_WRITE(CS_1_HIGH_DECODE_ADDRESS, 0x0);
	}
	GT_REG_WRITE(CS_2_LOW_DECODE_ADDRESS, bank2Base);
	if (device2Length != 0) {
		GT_REG_WRITE(CS_2_HIGH_DECODE_ADDRESS, bank2Top - 1);
	} else {
		GT_REG_WRITE(CS_2_HIGH_DECODE_ADDRESS, 0x0);
	}
	return true;
}

/********************************************************************
* mapDevices3andBootMemorySpace - Sets new bases and boundaries for devices:
*                                 3 and boot
*                     - Pay attention to the PCI mappings and make sure to
*                       coordinate between the two interfaces!!!
*                     - It is the programmer`s responsibility to make sure
*                       there are no conflicts with other memory spaces!!!
*                     - If a device needs to be closed , give it a 0 length.
*
* INPUTS: base and length of device 3and boot
* RETURNS: true on success, false on failure
*********************************************************************/
bool mapDevices3andBootMemorySpace(unsigned int device3Base,
				   unsigned int device3Length,
				   unsigned int bootDeviceBase,
				   unsigned int bootDeviceLength)
{
	unsigned int deviceBank3Top =
	    (unsigned int) (device3Base + device3Length);
	unsigned int deviceBankBootTop =
	    (unsigned int) (bootDeviceBase + bootDeviceLength);
	unsigned int bank3Base, bank3Top;
	unsigned int bank4Base, bank4Top;
	unsigned int Device1Base, Device1Top;

	bank3Top = ((unsigned int) (deviceBank3Top & 0x0fffffff)) >> 20;
	bank4Top = ((unsigned int) (deviceBankBootTop & 0x0fffffff)) >> 20;
	bank3Base = ((unsigned int) (device3Base & 0x0fffffff)) >> 20;
	bank4Base = ((unsigned int) (bootDeviceBase & 0x0fffffff)) >> 20;

	if (device3Base <= bootDeviceBase) {
		if (deviceBank3Top > bootDeviceBase)
			return false;
	} else {
		if (deviceBankBootTop > device3Base)
			return false;
	}

	if (deviceBankBootTop > deviceBank3Top) {
		Device1Base = device3Base >> 21;
		Device1Top =
		    ((unsigned int) (deviceBankBootTop & 0x0fffffff)) >>
		    21;
	} else {
		Device1Base = bootDeviceBase >> 21;
		Device1Top =
		    ((unsigned int) (deviceBank3Top & 0x0fffffff)) >> 21;
	}
	GT_REG_WRITE(CS_3_BOOTCS_LOW_DECODE_ADDRESS, Device1Base);
	if ((device3Length + bootDeviceLength) != 0) {
		GT_REG_WRITE(CS_3_BOOTCS_HIGH_DECODE_ADDRESS,
			     Device1Top - 1);
	} else {
		GT_REG_WRITE(CS_3_BOOTCS_HIGH_DECODE_ADDRESS, 0x0);
	}
	GT_REG_WRITE(CS_3_LOW_DECODE_ADDRESS, bank3Base);
	if (device3Length != 0) {
		GT_REG_WRITE(CS_3_HIGH_DECODE_ADDRESS, bank3Top - 1);
	} else {
		GT_REG_WRITE(CS_3_HIGH_DECODE_ADDRESS, 0x0);
	}
	GT_REG_WRITE(BOOTCS_LOW_DECODE_ADDRESS, bank4Base);
	if (bootDeviceLength != 0) {
		GT_REG_WRITE(BOOTCS_HIGH_DECODE_ADDRESS, bank4Top - 1);
	} else {
		GT_REG_WRITE(BOOTCS_HIGH_DECODE_ADDRESS, 0x0);
	}
	return true;
}

/********************************************************************
* modifyDeviceParameters - This function can be used to modify a device`s
*                          parameters.
*                        - Be advised to check the spec before modifying them.
* Inputs:
* Returns: false if one of the parameters is erroneous,true otherwise.
*********************************************************************/
bool modifyDeviceParameters(DEVICE device, unsigned int turnOff,
			    unsigned int accToFirst,
			    unsigned int accToNext, unsigned int aleToWr,
			    unsigned int wrActive, unsigned int wrHigh,
			    unsigned int width, bool paritySupport)
{
	unsigned int data, oldValue;

	if ((turnOff > 0x7 && turnOff != DONT_MODIFY)
	    || (accToFirst > 0xf && accToFirst != DONT_MODIFY)
	    || (accToNext > 0xf && accToNext != DONT_MODIFY)
	    || (aleToWr > 0x7 && aleToWr != DONT_MODIFY)
	    || (wrActive > 0x7 && wrActive != DONT_MODIFY)
	    || (wrHigh > 0x7 && wrHigh != DONT_MODIFY)) {
		return false;
	}

	GT_REG_READ((DEVICE_BANK0PARAMETERS + device * 4), &oldValue);
	if (turnOff == DONT_MODIFY)
		turnOff = oldValue & 0x00000007;
	else
		turnOff = turnOff;

	if (accToFirst == DONT_MODIFY)
		accToFirst = oldValue & 0x00000078;
	else
		accToFirst = accToFirst << 3;

	if (accToNext == DONT_MODIFY)
		accToNext = oldValue & 0x00000780;
	else
		accToNext = accToNext << 7;

	if (aleToWr == DONT_MODIFY)
		aleToWr = oldValue & 0x00003800;
	else
		aleToWr = aleToWr << 11;

	if (wrActive == DONT_MODIFY)
		wrActive = oldValue & 0x0001c000;
	else
		wrActive = wrActive << 14;

	if (wrHigh == DONT_MODIFY)
		wrHigh = oldValue & 0x000e0000;
	else
		wrHigh = wrHigh << 17;

	data =
	    turnOff | accToFirst | accToNext | aleToWr | wrActive | wrHigh;
	switch (width) {
	case _8BIT:
		break;
	case _16BIT:
		data = data | _16BIT;
		break;
	case _32BIT:
		data = data | _32BIT;
		break;
	case _64BIT:
		data = data | _64BIT;
		break;
	default:
		return false;
	}
	if (paritySupport == true)
		data = data | PARITY_SUPPORT;
	GT_REG_WRITE(DEVICE_BANK0PARAMETERS + device * 4, data);
	return true;
}

/********************************************************************
* remapAddress - This fubction used for address remapping
* Inputs:      - regOffset: remap register
*                remapHeader : remapped address
* Returns: false if one of the parameters is erroneous,true otherwise.
*********************************************************************/
bool remapAddress(unsigned int remapReg, unsigned int remapValue)
{
	unsigned int valueForReg;
	valueForReg = (remapValue & 0xffe00000) >> 21;
	GT_REG_WRITE(remapReg, valueForReg);
	return true;
}
