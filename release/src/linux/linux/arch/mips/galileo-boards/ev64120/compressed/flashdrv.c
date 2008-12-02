/* flashdrv.c - FLASH memory functions and definitions*/

/* Copyright Galileo Technology. */

/*
DESCRIPTION
This flash driver gives the user a convenient interface to FLASH memory located
on the user`s board, it supports various layout configurations such as:
1. One pure 8 bit device (Such as AMD`s AM29LV040B).
2. 1,2,4 or 8 devices 16 bit wide configured to operate in 8 bit mode.
3. 1,2 or 4 devices each 16 bit wide.
Before using the driver you must call the initialization function at least once
or when ever you are changing the FLASH base address.
The list bellow contains the supported FLASH memory devices, new devices can be
added easily in the future.
*/

/*includes*/
#ifdef __linux__
#include <asm/galileo-boards/evb64120A/flashdrv.h>
#else
#include "flashdrv.h"
#endif
/* locals */

#ifdef __MIPSEB__		    // skranz, add
#define BE			// skranz, add
#endif				// skranz, add

/******************************************************************************
* Those two tables contain the supported flash devices information needed by
* the driver:
* The first table "flashParametrs" starts with 10 shared fields
*  (currently 6 are reserved):
*   index 0 => Pointer to an entry in the second table list
*   index 1 => baseAddress - Flash memory device base address.
*   index 2 => width - 1, 2, 4 or 8 Bytes.
*   index 3 => mode - PURE8, X8 or X16 flash configuration (for X16 devices only)
* The second table (flashTypes) contains:
* Entry`s structure:
*   Manufacture ID,Device ID,number of sectors,list of sector`s sizes
*   (in Kbytes starting with sector number 0).
* The end of the list is pointed with a zero.
******************************************************************************/
unsigned int flashParametrs[10];	/* 0  Entry pointer */
				 /* 0  Base address  */
				 /* 0  Width         */
				 /* 0  Mode          */
				 /* 0,0,0,0,0,0, spare entries. */
unsigned int flashTypes[] = {

	/* 0 */ AMD_FLASH, AM29F400BB, 11, 16, 8, 8, 32, 64, 64, 64, 64,
	    64, 64, 64,
	/* 1 */ AMD_FLASH, AM29F400BT, 11, 64, 64, 64, 64, 64, 64, 64, 32,
	    8, 8, 16,
	/* 2 */ ST_FLASH, M29W040, 8, 64, 64, 64, 64, 64, 64, 64, 64,
	/* 3 */ AMD_FLASH, AM29LV040B, 8, 64, 64, 64, 64, 64, 64, 64, 64,
	/* 4 */ AMD_FLASH, AM29LV800BT, 19, 64, 64, 64, 64, 64, 64, 64, 64,
	    64, 64,
	64, 64, 64, 64, 64, 32, 8, 8, 16,
	/* 5 */ INTEL_FLASH, I28F320J3A, 32, 128, 128, 128, 128, 128, 128,
	    128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128,
	/* 6 */ INTEL_FLASH, I28F640J3A, 64, 128, 128, 128, 128, 128, 128,
	    128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128,
	/* 7 */ INTEL_FLASH, I28F128J3A, 128, 128, 128, 128, 128, 128, 128,
	    128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
	/* 8 */ AMD_FLASH, AM29LV400BB, 11, 16, 8, 8, 32, 64, 64, 64, 64,
	    64, 64, 64,
	/* 9 */ AMD_FLASH, AM29LV400BT, 11, 64, 64, 64, 64, 64, 64, 64, 32,
	    8, 8, 16,
	/* 10 */ INTEL_FLASH, I28F320B3_T, 71, 64, 64, 64, 64, 64, 64, 64,
	    64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 8, 8, 8, 8, 8, 8, 8, 8,
	/* 11 */ INTEL_FLASH, I28F320B3_B, 71, 8, 8, 8, 8, 8, 8, 8, 8, 64,
	    64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	/* 12 */ INTEL_FLASH, I28F160B3_B, 39, 8, 8, 8, 8, 8, 8, 8, 8, 64,
	    64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64,
	/* 13 */ INTEL_FLASH, I28F160B3_T, 39, 64, 64, 64, 64, 64, 64, 64,
	    64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 8, 8, 8, 8, 8, 8, 8, 8,

	0			/* End of list indicator */
};

/********************************************************************
* flashInit - Initializes the FLASH memory driver`s parameters, this function
*             must be called at least once before using the FLASH memory.
*             If you are changing the FLASH base address call this function
*             again.
*
* INPUTS:     unsigned int flashBaseAddress - The flash base Address.
*             unsigned int flashWidth - Flash bus width in Bytes: 1,2,4 or 8.
*             flashMode - PURE8, X8 or X16.
* RETURNS:    Flash Size, zero when operation (flashInit) failed.
*********************************************************************/
unsigned int flashInit(unsigned int flashBaseAddress,
		       unsigned int flashWidth, FLASHmode flashMode)
{
	unsigned short mfrId = 0;
	unsigned short devId = 0xffff;
	unsigned int FirstAddr, SecondAddr, ThirdAddr;
	unsigned int pArray = 0;
	unsigned int counter;
	unsigned int flashSize = 0;

	/* update the list with relevant parametrs */
	flashParametrs[0] = 0;	/* Default initialization */
	flashParametrs[1] = flashBaseAddress;
	flashParametrs[2] = flashWidth;
	flashParametrs[3] = flashMode;
	/* Get the FLASH`s ID */
	switch (FLASH_WIDTH) {
	case 1:
		/* AMD or ST ?? * */
		if (flashMode == PURE8) {	/* Boot Flash */
			FirstAddr = 0x5555;
			SecondAddr = 0x2aaa;
			ThirdAddr = 0x5555;
		} else {	/* X16 device configured to 8bit Mode */

			FirstAddr = 0xaaaa;
			SecondAddr = 0x5555;
			ThirdAddr = 0xaaaa;
		}
		flashReset();
		WRITE_CHAR(FLASH_BASE_ADDRESS + FirstAddr, 0xAA);
		WRITE_CHAR(FLASH_BASE_ADDRESS + SecondAddr, 0x55);
		WRITE_CHAR(FLASH_BASE_ADDRESS + ThirdAddr, 0x90);
		READ_CHAR(FLASH_BASE_ADDRESS + 0x0, &mfrId);
		if (mfrId == AMD_FLASH || mfrId == ST_FLASH) {
			flashReset();
			WRITE_CHAR(FLASH_BASE_ADDRESS + FirstAddr, 0xAA);
			WRITE_CHAR(FLASH_BASE_ADDRESS + SecondAddr, 0x55);
			WRITE_CHAR(FLASH_BASE_ADDRESS + ThirdAddr, 0x90);
			READ_CHAR(FLASH_BASE_ADDRESS + 0x1, &devId);
			break;
		}
		/* Micron or Intel ?? * */
		WRITE_CHAR(FLASH_BASE_ADDRESS, 0xff);	/* Read Array */
		/* Flash reset for Intel/Micron */
		WRITE_CHAR(FLASH_BASE_ADDRESS, 0x90);	/* IDENTIFY Device */
		READ_CHAR(FLASH_BASE_ADDRESS + 0x0, &mfrId);	/*Address for ManufactureID */
		if (mfrId == INTEL_FLASH || mfrId == MICRON_FLASH) {
			WRITE_CHAR(FLASH_BASE_ADDRESS, 0xff);	/* Read Array */
			/*Flash reset for Intel/Micron */
			WRITE_CHAR(FLASH_BASE_ADDRESS, 0x90);	/* IDENTIFY Device */
			READ_CHAR(FLASH_BASE_ADDRESS + 0x1, &devId);	/*Address for DeviceID */
		}
		break;
	case 2:
	case 4:
	case 8:
		/* AMD or ST ??? */
		flashReset();
		WRITE_SHORT(FLASH_BASE_ADDRESS + 0x5555 * FLASH_WIDTH,
			    0xaa);
		WRITE_SHORT(FLASH_BASE_ADDRESS + 0x2aaa * FLASH_WIDTH,
			    0x55);
		WRITE_SHORT(FLASH_BASE_ADDRESS + 0x5555 * FLASH_WIDTH,
			    0x90);
		READ_SHORT(FLASH_BASE_ADDRESS, &mfrId);
		flashReset();
		/* Read the device ID */
		if (mfrId == AMD_FLASH || mfrId == ST_FLASH) {
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    0x5555 * FLASH_WIDTH, 0xaa);
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    0x2aaa * FLASH_WIDTH, 0x55);
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    0x5555 * FLASH_WIDTH, 0x90);
			READ_SHORT(FLASH_BASE_ADDRESS + 0x1 * FLASH_WIDTH,
				   &devId);
			break;
		}
		/* Micron or Intel ?? * */
		WRITE_WORD(FLASH_BASE_ADDRESS, 0x00ff00ff);
		WRITE_WORD(FLASH_BASE_ADDRESS, 0x00900090);
		if ((FLASH_WIDTH == 4) || (FLASH_WIDTH == 8)) {	/* 32 or 64 bit */
			READ_SHORT(FLASH_BASE_ADDRESS, &mfrId);
		} else {	/* FLASH_WIDTH = 2 */

			READ_SHORT(FLASH_BASE_ADDRESS, &mfrId);
		}
		if ((mfrId == INTEL_FLASH) || (mfrId == MICRON_FLASH)) {
			/* Flash reset for Intel/Micron */
			flashReset();
			WRITE_WORD(FLASH_BASE_ADDRESS, 0x00ff00ff);
			WRITE_WORD(FLASH_BASE_ADDRESS, 0x00900090);
			READ_SHORT(FLASH_BASE_ADDRESS + 0x1 * FLASH_WIDTH,
				   &devId);
		}
		break;

	}
	/* Try to locate the device in the supported flashes list (FLASH_TYPE).
	   according to the keys:
	   1) mfrId - manufactor ID.
	   2) devId - device ID.
	 */

	while (true) {
		if (flashTypes[pArray] == 0) {
			flashReset();
			return 0;	/* Device not in the list */
		}
		if ((flashTypes[pArray] == mfrId) &&
		    (flashTypes[pArray + 1] == devId)) {
			POINTER_TO_FLASH = pArray;
			for (counter = 0;
			     counter < flashTypes[NUMBER_OF_SECTORS];
			     counter++) {
				flashSize =
				    flashSize +
				    flashTypes[FIRST_SECTOR_SIZE +
					       counter];
			}
			if (FLASH_MODE != PURE8) {
				flashReset();
				return (flashSize * _1K *
					(FLASH_WIDTH / (FLASH_MODE / 8)));
			} else {
				flashReset();
				return (flashSize * _1K * FLASH_WIDTH);
			}
		}
		pArray += (3 + flashTypes[pArray + 2]);	/* Move to next entry */
	}
}

/********************************************************************
* flashReset - Resets the Flash memory (FLASH`s internal protocol reset).
*
* INTPUTS:  N/A
* OUTPUT:   N/A
*********************************************************************/
void flashReset()
{
	unsigned char ucData;
	unsigned short usData;
	unsigned int uiData;

	if ((flashTypes[POINTER_TO_FLASH] == AMD_FLASH) ||
	    (flashTypes[POINTER_TO_FLASH]) == ST_FLASH) {
		if (FLASH_MODE == X16) {
			ucData = 0xf0;
			usData = 0xf0;
			uiData = 0x00f000f0;
		} else {	/* case of PURE8 or X8 */

			ucData = 0xf0;
			usData = 0xf0f0;
			uiData = 0xf0f0f0f0;
		}
	} else {
		if (FLASH_MODE == X16) {
			ucData = 0xff;
			usData = 0xff;
			uiData = 0x00ff00ff;
		} else {	/* case of PURE8 or X8 */

			ucData = 0xff;
			usData = 0xffff;
			uiData = 0xffffffff;
		}
	}
	switch (FLASH_WIDTH) {
	case 1:
		WRITE_CHAR(FLASH_BASE_ADDRESS, ucData);
		break;
	case 2:
		WRITE_SHORT(FLASH_BASE_ADDRESS, usData);
		break;
	case 4:
		WRITE_WORD(FLASH_BASE_ADDRESS, uiData);
		break;
	case 8:
		WRITE_WORD(FLASH_BASE_ADDRESS, uiData);
		WRITE_WORD(FLASH_BASE_ADDRESS + 0x4, uiData);
		break;
	}
}

/********************************************************************
* flashErase - The function erases the WHOLE flash memory.
*
*
* RETURNS: true on success,false on failure
*********************************************************************/
bool flashErase()
{
	unsigned int totalFlashSize;
	unsigned int address;
	unsigned int readData;
	unsigned int nextSector;

	flashReset();
	totalFlashSize = flashGetSize();
	/* scan all flash memory space. */
	address = 0;
	while (address < totalFlashSize) {
		readData = flashReadWord(address);
		if (readData != 0xffffffff) {	/* offset with dirty data. */
			flashEraseSector(flashInWhichSector(address));
			nextSector = flashInWhichSector(address) + 1;
			if (nextSector < flashTypes[NUMBER_OF_SECTORS])
				/* jump to next sector. */
				address = flashGetSectorOffset(nextSector);
			else
				/* end of erasing. */
				address = totalFlashSize;
		} else
			address += 4;
	}
	return true;
}

/********************************************************************
* flashEraseSector - The function erases a specific sector in the flash memory.
*
* INPUTS:  Sector number.
* RETURNS: true on success,false on failure.
*********************************************************************/
bool flashEraseSector(unsigned int sectorNumber)
{
	volatile unsigned int spin;
	unsigned int regValue;
	unsigned int sectorBaseAddress = 0;
	unsigned int i;
	unsigned int data20, dataD0, data70;
	unsigned int dataPoll;
	unsigned int FirstAddr, SecondAddr, ThirdAddr, FourthAddr,
	    FifthAddr;
	unsigned int FirstData, SecondData, ThirdData;
	unsigned int FourthData, FifthData, SixthData;

	/* calculate the sector base Address according to the following parametrs:
	   1: FLASH_WIDTH
	   2: the size of each sector which it detailed in the table */

	/* checking the if the sectorNumber is legal. */
	if (sectorNumber > flashTypes[NUMBER_OF_SECTORS] - 1)
		return false;
	/* now the calculation begining of the sector Address */
	for (i = 0; i < sectorNumber; i++) {
		sectorBaseAddress =
		    sectorBaseAddress + flashTypes[FIRST_SECTOR_SIZE + i];
	}
	/* In case of X8 wide the address should be */
	if (FLASH_MODE == PURE8)
		sectorBaseAddress = _1K * sectorBaseAddress;
	if (FLASH_MODE == X8)
		sectorBaseAddress = _1K * sectorBaseAddress;
	/* In case of X16 wide the address should be */
	if (FLASH_MODE == X16)
		sectorBaseAddress = _1K * sectorBaseAddress / 2;
	flashReset();
	if ((flashTypes[POINTER_TO_FLASH] == AMD_FLASH) || \
	    (flashTypes[POINTER_TO_FLASH] == ST_FLASH)) {
		switch (FLASH_WIDTH) {
		case 1:
			if (FLASH_MODE == PURE8) {	/* Boot Flash PURE8 */
				FirstAddr = 0x5555;
				SecondAddr = 0x2aaa;
				ThirdAddr = 0x5555;
				FourthAddr = 0x5555;
				FifthAddr = 0x2aaa;
			} else {
				FirstAddr = 0xaaaa;
				SecondAddr = 0x5555;
				ThirdAddr = 0xaaaa;
				FourthAddr = 0xaaaa;
				FifthAddr = 0x5555;
			}
			WRITE_CHAR(FLASH_BASE_ADDRESS + FirstAddr, 0xAA);
			WRITE_CHAR(FLASH_BASE_ADDRESS + SecondAddr, 0x55);
			WRITE_CHAR(FLASH_BASE_ADDRESS + ThirdAddr, 0x80);
			WRITE_CHAR(FLASH_BASE_ADDRESS + FourthAddr, 0xAA);
			WRITE_CHAR(FLASH_BASE_ADDRESS + FifthAddr, 0x55);
			WRITE_CHAR(
				   (FLASH_BASE_ADDRESS +
				    (sectorBaseAddress & 0xffffff00)),
				   0x30);
			/* Poll on the flash */
			do {
				READ_CHAR(FLASH_BASE_ADDRESS +
					  sectorBaseAddress, &regValue);
			} while ((regValue & 0x80) != 0x80);

			break;
		case 2:
			if (FLASH_MODE == X16) {
				FirstData = 0xaa;	/* Data for the First  Cycle */
				SecondData = 0x55;	/* Data for the Second Cycle */
				ThirdData = 0x80;	/* Data for the Third  Cycle */
				FourthData = 0xaa;	/* Data for the Fourth Cycle */
				FifthData = 0x55;	/* Data for the Fifth  Cycle */
				SixthData = 0x30;	/* Data for the Sixth  Cycle */
				FirstAddr = 0x5555;	/* Address for the First  Cycle */
				SecondAddr = 0x2aaa;	/* Address for the Second Cycle */
				ThirdAddr = 0x5555;	/* Address for the Third  Cycle */
				FourthAddr = 0x5555;	/* Address for the Fourth Cycle */
				FifthAddr = 0x2aaa;	/* Address for the Fifth  Cycle */
			} else {	/* (FLASH_MODE = 8) */

				FirstData = 0xaaaa;	/* Data for the First  Cycle */
				SecondData = 0x5555;	/* Data for the Second Cycle */
				ThirdData = 0x8080;	/* Data for the Third  Cycle */
				FourthData = 0xaaaa;	/* Data for the Fourth Cycle */
				FifthData = 0x5555;	/* Data for the Fifth  Cycle */
				SixthData = 0x3030;	/* Data for the Sixth  Cycle */
				FirstAddr = 0xaaaa;	/* Address for the First  Cycle */
				SecondAddr = 0x5555;	/* Address for the Second Cycle */
				ThirdAddr = 0xaaaa;	/* Address for the Third  Cycle */
				FourthAddr = 0xaaaa;	/* Address for the Fourth Cycle */
				FifthAddr = 0x5555;	/* Address for the Fifth  Cycle */
			}
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    FirstAddr * FLASH_WIDTH, FirstData);
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    SecondAddr * FLASH_WIDTH, SecondData);
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    ThirdAddr * FLASH_WIDTH, ThirdData);
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    FourthAddr * FLASH_WIDTH, FourthData);
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    FifthAddr * FLASH_WIDTH, FifthData);
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    (sectorBaseAddress & 0xffffff00) *
				    FLASH_WIDTH, SixthData);
			/* Poll on the flash */
			if (FLASH_MODE == X16) {	/* 1 device of 16 bit */
				dataPoll = 0x0080;
			} else {	/* (FLASH_MODE = 8) ==> 2 devices , 8 bit each => 16bit */

				dataPoll = 0x8080;
			}
			do {
				READ_SHORT(FLASH_BASE_ADDRESS +
					   sectorBaseAddress * FLASH_WIDTH,
					   &regValue);
				for (spin = 0; spin < 100; spin++) {
				}	// skranz, added spin loop.
			} while ((regValue & dataPoll) != dataPoll);
			break;
		case 4:
			if (FLASH_MODE == X16) {
				FirstData = 0x00aa00aa;	/* Data for the First  Cycle */
				SecondData = 0x00550055;	/* Data for the Second Cycle */
				ThirdData = 0x00800080;	/* Data for the Third  Cycle */
				FourthData = 0x00aa00aa;	/* Data for the Fourth Cycle */
				FifthData = 0x00550055;	/* Data for the Fifth  Cycle */
				SixthData = 0x00300030;	/* Data for the Sixth  Cycle */
				FirstAddr = 0x5555;	/* Address for the First  Cycle */
				SecondAddr = 0x2aaa;	/* Address for the Second Cycle */
				ThirdAddr = 0x5555;	/* Address for the Third  Cycle */
				FourthAddr = 0x5555;	/* Address for the Fourth Cycle */
				FifthAddr = 0x2aaa;	/* Address for the Fifth  Cycle */
			} else {	/* if (FLASH_MODE == 8) */

				FirstData = 0xaaaaaaaa;	/* Data for the First  Cycle */
				SecondData = 0x55555555;	/* Data for the Second Cycle */
				ThirdData = 0x80808080;	/* Data for the Third  Cycle */
				FourthData = 0xAAAAAAAA;	/* Data for the Fourth Cycle */
				FifthData = 0x55555555;	/* Data for the Fifth  Cycle */
				SixthData = 0x30303030;	/* Data for the Sixth  Cycle */
				FirstAddr = 0xaaaa;	/* Address for the First  Cycle */
				SecondAddr = 0x5555;	/* Address for the Second Cycle */
				ThirdAddr = 0xaaaa;	/* Address for the Third  Cycle */
				FourthAddr = 0xaaaa;	/* Address for the Fourth Cycle */
				FifthAddr = 0x5555;	/* Address for the Fifth  Cycle */
			}
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   FirstAddr * FLASH_WIDTH, FirstData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   SecondAddr * FLASH_WIDTH, SecondData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   ThirdAddr * FLASH_WIDTH, ThirdData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   FourthAddr * FLASH_WIDTH, FourthData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   FifthAddr * FLASH_WIDTH, FifthData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   (sectorBaseAddress & 0xffffff00) *
				   FLASH_WIDTH, SixthData);
			/* Poll on the flash */
			if (FLASH_MODE == X16) {	/* 4 devices , 16 bit each => 64bit */
				dataPoll = 0x00800080;
			} else {	/* (FLASH_MODE = 8) ==> 8 devices , 8 bit each => 64bit */

				dataPoll = 0x80808080;
			}
			do {
				READ_WORD(FLASH_BASE_ADDRESS +
					  sectorBaseAddress * FLASH_WIDTH,
					  &regValue);
			} while ((regValue & dataPoll) != dataPoll);
			break;
		case 8:	/* In case of 64bit width the transformation is 1->8 */
			if (FLASH_MODE == X16) {
				FirstData = 0x00aa00aa;	/* Data for the First  Cycle */
				SecondData = 0x00550055;	/* Data for the Second Cycle */
				ThirdData = 0x00800080;	/* Data for the Third  Cycle */
				FourthData = 0x00aa00aa;	/* Data for the Fourth Cycle */
				FifthData = 0x00550055;	/* Data for the Fifth  Cycle */
				SixthData = 0x00300030;	/* Data for the Sixth  Cycle */
				FirstAddr = 0x5555;	/* Address for the First  Cycle */
				SecondAddr = 0x2aaa;	/* Address for the Second Cycle */
				ThirdAddr = 0x5555;	/* Address for the Third  Cycle */
				FourthAddr = 0x5555;	/* Address for the Fourth Cycle */
				FifthAddr = 0x2aaa;	/* Address for the Fifth  Cycle */
			} else {	/* (FLASH_MODE = 8 */

				FirstData = 0xaaaaaaaa;	/* Data for the First  Cycle */
				SecondData = 0x55555555;	/* Data for the Second Cycle */
				ThirdData = 0x80808080;	/* Data for the Third  Cycle */
				FourthData = 0xAAAAAAAA;	/* Data for the Fourth Cycle */
				FifthData = 0x55555555;	/* Data for the Fifth  Cycle */
				SixthData = 0x30303030;	/* Data for the Sixth  Cycle */
				FirstAddr = 0xaaaa;	/* Address for the First  Cycle */
				SecondAddr = 0x5555;	/* Address for the Second Cycle */
				ThirdAddr = 0xaaaa;	/* Address for the Third  Cycle */
				FourthAddr = 0xaaaa;	/* Address for the Fourth Cycle */
				FifthAddr = 0x5555;	/* Address for the Fifth  Cycle */
			}
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   FirstAddr * FLASH_WIDTH, FirstData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   SecondAddr * FLASH_WIDTH, SecondData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   ThirdAddr * FLASH_WIDTH, ThirdData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   FourthAddr * FLASH_WIDTH, FourthData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   FifthAddr * FLASH_WIDTH, FifthData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   (sectorBaseAddress & 0xffffff00) *
				   FLASH_WIDTH, SixthData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   FirstAddr * FLASH_WIDTH + 4, FirstData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   SecondAddr * FLASH_WIDTH + 4,
				   SecondData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   ThirdAddr * FLASH_WIDTH + 4, ThirdData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   FourthAddr * FLASH_WIDTH + 4,
				   FourthData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   FifthAddr * FLASH_WIDTH + 4, FifthData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   (sectorBaseAddress & 0xffffff00)
				   * FLASH_WIDTH + 4, SixthData);
			/* Poll on the flash */
			if (FLASH_MODE == X16) {	/* 4 devices , 16 bit each => 64bit */
				dataPoll = 0x00800080;
			} else {	/* (FLASH_MODE = 8) ==> 8 devices , 8 bit each => 64bit */

				dataPoll = 0x80808080;
			}
			do {
				READ_WORD(FLASH_BASE_ADDRESS +
					  sectorBaseAddress * FLASH_WIDTH,
					  &regValue);
			} while ((regValue & dataPoll) != dataPoll);
			do {
				READ_WORD(FLASH_BASE_ADDRESS +
					  sectorBaseAddress * FLASH_WIDTH +
					  4, &regValue);
			} while ((regValue & dataPoll) != dataPoll);
			break;
		default:
			return false;
		}
	} /* End of 'flash erase sector' for AMD/ST */
	else {			/* Intel/Micron */

		switch (FLASH_WIDTH) {
		case 1:
			WRITE_CHAR(FLASH_BASE_ADDRESS, 0x20);
			WRITE_CHAR(
				   (FLASH_BASE_ADDRESS +
				    (sectorBaseAddress & 0xffffff00)),
				   0xd0);
			/* Poll on the flash */
			while (true) {
				WRITE_CHAR(FLASH_BASE_ADDRESS, 0x70);
				READ_CHAR(FLASH_BASE_ADDRESS, &regValue);
				if ((regValue & 0x80) == 0x80)
					break;
			}
			break;
		case 2:
			if (FLASH_MODE == X16) {	/* 1 device 16 bit.  */
				data20 = 0x0020;;
				dataD0 = 0x00d0;;
			} else {	/* (FLASH_MODE = 8) ==> 2 devices , 8 bit each => 16bit */

				data20 = 0x2020;
				dataD0 = 0xd0d0;
			}
			WRITE_SHORT(FLASH_BASE_ADDRESS, data20);
			WRITE_SHORT(
				    (FLASH_BASE_ADDRESS +
				     ((sectorBaseAddress * 2) &
				      0xffffff00)), dataD0);
			/* Poll on the flash */
			if (FLASH_MODE == X16) {
				dataPoll = 0x0080;
				data70 = 0x0070;
			} else {	/* (FLASH_MODE = 8) */

				dataPoll = 0x8080;
				data70 = 0x7070;
			}
			while (true) {
				WRITE_SHORT(FLASH_BASE_ADDRESS +
					    sectorBaseAddress * 2, data70);
				READ_SHORT(FLASH_BASE_ADDRESS +
					   sectorBaseAddress * 2,
					   &regValue);
				if ((regValue & 0x0080) == 0x0080)
					break;
			}
			break;
		case 4:
			if (FLASH_MODE == X16) {	/* 2 devices , 16 bit each => 32bit */
				data20 = 0x00200020;
				dataD0 = 0x00d000d0;
			} else {	/* (FLASH_MODE = 8) ==> 4 devices , 8 bit each => 32bit */

				data20 = 0x20202020;
				dataD0 = 0xd0d0d0d0;
			}
			WRITE_WORD(FLASH_BASE_ADDRESS, data20);
			WRITE_WORD(
				   (FLASH_BASE_ADDRESS +
				    ((sectorBaseAddress * 4) &
				     0xffffff00)), dataD0);
			/* Poll on the flash */
			if (FLASH_MODE == X16) {
				dataPoll = 0x0080;
				data70 = 0x0070;
			} else {	/* (FLASH_MODE = 8) */

				dataPoll = 0x8080;
				data70 = 0x7070;
			}
			while (true) {
				WRITE_SHORT(FLASH_BASE_ADDRESS, data70);
				READ_SHORT(FLASH_BASE_ADDRESS, &regValue);
				if ((regValue & dataPoll) == dataPoll)
					break;
			}
			while (true) {
				WRITE_SHORT(FLASH_BASE_ADDRESS + 2,
					    data70);
				READ_SHORT(FLASH_BASE_ADDRESS + 2,
					   &regValue);
				if ((regValue & dataPoll) == dataPoll)
					break;
			}
			break;
		case 8:
			if (FLASH_MODE == X16) {	/* 4 devices , 16 bit each => 64bit */
				data20 = 0x00200020;
				dataD0 = 0x00d000d0;
			} else {	/* (FLASH_MODE = 8) ==> 8 devices , 8 bit each => 64bit */

				data20 = 0x20202020;
				dataD0 = 0xd0d0d0d0;
			}
			WRITE_WORD(FLASH_BASE_ADDRESS, data20);
			WRITE_WORD(
				   (FLASH_BASE_ADDRESS +
				    ((sectorBaseAddress * 8) &
				     0xffffff00)), dataD0);
			WRITE_WORD(FLASH_BASE_ADDRESS + 4, data20);
			WRITE_WORD(
				   (FLASH_BASE_ADDRESS +
				    ((sectorBaseAddress * 8) & 0xffffff00 +
				     4)), dataD0);
			/* Poll on the flash */
			if (FLASH_MODE == X16) {
				dataPoll = 0x0080;
				data70 = 0x0070;
			} else {	/* (FLASH_MODE = 8) */

				dataPoll = 0x8080;
				data70 = 0x7070;
			}
			while (true) {
				WRITE_SHORT(FLASH_BASE_ADDRESS +
					    sectorBaseAddress * 8, data70);
				READ_SHORT(FLASH_BASE_ADDRESS +
					   sectorBaseAddress * 8,
					   &regValue);
				if ((regValue & dataPoll) == dataPoll)
					break;
			}
			while (true) {
				WRITE_SHORT(FLASH_BASE_ADDRESS + 2,
					    data70);
				READ_SHORT(FLASH_BASE_ADDRESS + 2,
					   &regValue);
				if ((regValue & dataPoll) == dataPoll)
					break;
			}
			while (true) {
				WRITE_SHORT(FLASH_BASE_ADDRESS + 4,
					    data70);
				READ_SHORT(FLASH_BASE_ADDRESS + 4,
					   &regValue);
				if ((regValue & dataPoll) == dataPoll)
					break;
			}
			while (true) {
				WRITE_SHORT(FLASH_BASE_ADDRESS + 6,
					    data70);
				READ_SHORT(FLASH_BASE_ADDRESS + 6,
					   &regValue);
				if ((regValue & dataPoll) == dataPoll)
					break;
			}
			break;
		default:
			return false;
		}
	}
	flashReset();
	return true;
}

/********************************************************************
* flashWriteWord - Write 32Bit to the FLASH memory at the given offset from the
*                  FLASH base address.
*   			   address 0 = 0x00000000 !!
*				   Attention!!! data "0" cannot be programed back to
*                  "1" (only by first performing an earase operation).
*                  The function takes care of Big/Little endian conversion
*
* INPUTS:  offset - The offset from the flash`s base address.
*          data   - The data that should be written.
* RETURNS: true on success,false on failure
*********************************************************************/
bool flashWriteWord(unsigned int offset, unsigned int data)
{
	unsigned char c, rc;
	unsigned short s, rs;
	register unsigned int rw;
	register unsigned int regValue;
	register unsigned int FirstAddr, SecondAddr, ThirdAddr;
	register unsigned int FirstData, SecondData, ThirdData;
	register unsigned int data10, data20, data70, data80;

	if ((flashTypes[POINTER_TO_FLASH] == AMD_FLASH) || \
	    (flashTypes[POINTER_TO_FLASH] == ST_FLASH)) {
		switch (FLASH_WIDTH) {
		case 1:	/* Split the 32 bit write into four 8bit Writings */
			if (FLASH_MODE == PURE8) {	/* Boot Flash */
				FirstAddr = 0x5555;
				SecondAddr = 0x2aaa;
				ThirdAddr = 0x5555;
			} else {
				FirstAddr = 0xaaaa;
				SecondAddr = 0x5555;
				ThirdAddr = 0xaaaa;
			}
			WRITE_CHAR(FLASH_BASE_ADDRESS + FirstAddr, 0xaa);
			WRITE_CHAR(FLASH_BASE_ADDRESS + SecondAddr, 0x55);
			WRITE_CHAR(FLASH_BASE_ADDRESS + ThirdAddr, 0xa0);
#ifdef BE
			c = (data >> 24);
#else
			c = data;
#endif
			WRITE_CHAR(FLASH_BASE_ADDRESS + offset, c);
			/* Writing first Byte */
			while (true) {
				READ_CHAR(FLASH_BASE_ADDRESS + offset,
					  &rc);
				if ((rc & 0x80) == (c & 0x80))	/* DQ7 =? DATA */
					break;	/* DQ7 =  DATA */
				if ((rc & 0x20) == 0x20) {	/* DQ5 =? '1'  */
					READ_CHAR(FLASH_BASE_ADDRESS +
						  offset, &rc);
					if ((rc & 0x80) == (c & 0x80))
						break;	/* DQ7 = DATA  */
					else
						return false;	/* DQ7 != DATA */
				}
			}
			WRITE_CHAR(FLASH_BASE_ADDRESS + FirstAddr, 0xaa);
			WRITE_CHAR(FLASH_BASE_ADDRESS + SecondAddr, 0x55);
			WRITE_CHAR(FLASH_BASE_ADDRESS + ThirdAddr, 0xa0);
#ifdef BE
			c = (data >> 16);
#else
			c = (data >> 8);
#endif
			WRITE_CHAR(FLASH_BASE_ADDRESS + offset + 1, c);
			/* Writing second Byte */
			while (true) {
				READ_CHAR(FLASH_BASE_ADDRESS + offset + 1,
					  &rc);
				if ((rc & 0x80) == (c & 0x80))	/* DQ7 =? DATA */
					break;	/* DQ7 = DATA  */
				if ((rc & 0x20) == 0x20) {	/* DQ5 =? '1'  */
					READ_CHAR(FLASH_BASE_ADDRESS +
						  offset + 1, &rc);
					if ((rc & 0x80) == (c & 0x80))
						break;	/* DQ7 = DATA  */
					else
						return false;	/* DQ7 != DATA */
				}
			}
			WRITE_CHAR(FLASH_BASE_ADDRESS + FirstAddr, 0xaa);
			WRITE_CHAR(FLASH_BASE_ADDRESS + SecondAddr, 0x55);
			WRITE_CHAR(FLASH_BASE_ADDRESS + ThirdAddr, 0xa0);
#ifdef BE
			c = (data >> 8);
#else
			c = (data >> 16);
#endif
			WRITE_CHAR(FLASH_BASE_ADDRESS + offset + 2, c);
			/* Writing third Byte */
			while (true) {
				READ_CHAR(FLASH_BASE_ADDRESS + offset + 2,
					  &rc);
				if ((rc & 0x80) == (c & 0x80))	/* DQ7 =? DATA */
					break;	/* DQ7 = DATA  */
				if ((rc & 0x20) == 0x20) {	/* DQ5 =? '1'  */
					READ_CHAR(FLASH_BASE_ADDRESS +
						  offset + 2, &rc);
					if ((rc & 0x80) == (c & 0x80))
						break;	/* DQ7 = DATA  */
					else
						return false;	/* DQ7 != DATA */
				}
			}
			WRITE_CHAR(FLASH_BASE_ADDRESS + FirstAddr, 0xaa);
			WRITE_CHAR(FLASH_BASE_ADDRESS + SecondAddr, 0x55);
			WRITE_CHAR(FLASH_BASE_ADDRESS + ThirdAddr, 0xa0);
#ifdef BE
			c = data;
#else
			c = (data >> 24);
#endif
			WRITE_CHAR(FLASH_BASE_ADDRESS + offset + 3, c);
			/* Writing fourth Byte */
			while (true) {
				READ_CHAR(FLASH_BASE_ADDRESS + offset + 3,
					  &rc);
				if ((rc & 0x80) == (c & 0x80))	/* DQ7 =? DATA */
					break;	/* DQ7 = DATA  */
				if ((rc & 0x20) == 0x20) {	/* DQ5 =? '1'  */
					READ_CHAR(FLASH_BASE_ADDRESS +
						  offset + 3, &rc);
					if ((rc & 0x80) == (c & 0x80))
						break;	/* DQ7 = DATA  */
					else
						return false;	/* DQ7 != DATA */
				}
			}
			break;
		case 2:	/* Split the 32 bit write into two 8/16 bit Writings
				   (16bit width). */
			if (FLASH_MODE == X16) {
				FirstData = 0xaa;	/* Data for the First  Cycle    */
				SecondData = 0x55;	/* Data for the Second Cycle    */
				ThirdData = 0xa0;	/* Data for the Third  Cycle    */
				FirstAddr = 0x5555;	/* Address for the First  Cycle */
				SecondAddr = 0x2aaa;	/* Address for the Second Cycle */
				ThirdAddr = 0x5555;	/* Address for the Third  Cycle */
			} else {	/* if (FLASH_MODE == 8) */

				FirstData = 0xaaaa;	/* Data for the First  Cycle    */
				SecondData = 0x5555;	/* Data for the Second Cycle    */
				ThirdData = 0xa0a0;	/* Data for the Third  Cycle    */
				FirstAddr = 0xaaaa;	/* Address for the First  Cycle */
				SecondAddr = 0x5555;	/* Address for the Second Cycle */
				ThirdAddr = 0xaaaa;	/* Address for the Third  Cycle */
			}
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    FirstAddr * FLASH_WIDTH, FirstData);
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    SecondAddr * FLASH_WIDTH, SecondData);
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    ThirdAddr * FLASH_WIDTH, ThirdData);
#ifdef BE
			s = (data >> 16);
#else
			s = data;
#endif
			WRITE_SHORT(FLASH_BASE_ADDRESS + offset, s);
			/* Writing Two Bytes */
			if (FLASH_MODE == X16) {
				data80 = 0x80;;
				data20 = 0x20;;
			} else {	/* if (FLASH_MODE == 8) */

				data80 = 0x8080;
				data20 = 0x2020;
			}
			while (true) {
				READ_SHORT(FLASH_BASE_ADDRESS + offset,
					   &rs);
				if ((rs & data80) == (s & data80))	/* DQ7 =? DATA */
					break;	/* DQ7 =  DATA */
				if ((rs & data20) == data20) {	/* DQ5 =? DATA */
					READ_SHORT(FLASH_BASE_ADDRESS +
						   offset, &rs);
					if ((rs & data80) == (s & data80))
						break;	/* DQ7 = DATA  */
					else {
						flashReset();
						return false;	/* DQ7 != DATA */
					}
				}
			}
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    FirstAddr * FLASH_WIDTH, FirstData);
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    SecondAddr * FLASH_WIDTH, SecondData);
			WRITE_SHORT(FLASH_BASE_ADDRESS +
				    ThirdAddr * FLASH_WIDTH, ThirdData);
#ifdef BE
			s = data;
#else
			s = (data >> 16);
#endif
			WRITE_SHORT(FLASH_BASE_ADDRESS + offset + 2, s);
			/* Writing Two Bytes */
			while (true) {
				READ_SHORT(FLASH_BASE_ADDRESS + offset + 2,
					   &rs);
				if ((rs & data80) == (s & data80))	/* DQ7 =? DATA */
					break;	/* DQ7 =  DATA */
				if ((rs & data20) == data20) {	/* DQ5 =? '1'  */
					READ_SHORT(FLASH_BASE_ADDRESS +
						   offset + 2, &rs);
					if ((rs & data80) == (s & data80))
						break;	/* DQ7 = DATA  */
					else {
						flashReset();
						return false;	/* DQ7 != DATA */
					}
				}
			}
			return true;
		case 4:
		case 8:
			if (FLASH_MODE == X16) {
				FirstData = 0x00aa00aa;
				SecondData = 0x00550055;
				ThirdData = 0x00a000a0;
				FirstAddr = 0x5555;
				SecondAddr = 0x2aaa;
				ThirdAddr = 0x5555;
			} else {	/* (FLASH_MODE == 8) */

				FirstData = 0xaaaaaaaa;	/* Data for the First  Cycle    */
				SecondData = 0x55555555;	/* Data for the Second Cycle    */
				ThirdData = 0xa0a0a0a0;	/* Data for the Third  Cycle    */
				FirstAddr = 0xaaaaaaaa;	/* Address for the First  Cycle */
				SecondAddr = 0x55555555;	/* Address for the Second Cycle */
				ThirdAddr = 0xaaaaaaaa;	/* Address for the Third  Cycle */
			}
			WRITE_WORD(FLASH_BASE_ADDRESS + FirstAddr *
				   FLASH_WIDTH + offset % FLASH_WIDTH,
				   FirstData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   SecondAddr * FLASH_WIDTH +
				   offset % FLASH_WIDTH, SecondData);
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   ThirdAddr * FLASH_WIDTH +
				   offset % FLASH_WIDTH, ThirdData);
			/* writting the word. */
			WRITE_WORD(FLASH_BASE_ADDRESS + offset, data);
			/* preparing the polling patterns. */
			if (FLASH_MODE == X16) {
				data80 = 0x00800080;
				data20 = 0x00200020;
			} else {	/* (FLASH_MODE == 8) */

				data80 = 0x80808080;
				data20 = 0x20202020;
			}
			while (true) {	/* polling loop. */
				rw = READWORD(FLASH_BASE_ADDRESS + offset);
				/* DQ7 =? DATA */
				if ((rw & data80) == (data & data80))
					break;	/* DQ7 =  DATA */
				if ((rw & data20) == data20) {	/* DQ5 =? '1'  */
					rw =
					    READWORD(FLASH_BASE_ADDRESS +
						     offset);
					if ((rw & data80) ==
					    (data & data80)) break;	/* DQ7 = DATA  */
					else
						return false;	/* DQ7 != DATA */
				}
			}
			return true;
		default:
			return false;	/* case of invalid flash Width. */
		}
	} else {		/* Intel/Micron */

		switch (FLASH_WIDTH) {
		case 1:
			/* Writing First Byte */
			WRITE_CHAR(FLASH_BASE_ADDRESS, 0x10);
#ifdef BE
			c = (data >> 24);
#else
			c = data;
#endif
			WRITE_CHAR(FLASH_BASE_ADDRESS + offset, c);
			while (true) {
				/* Reading STATUS Register */
				WRITE_CHAR(FLASH_BASE_ADDRESS, 0x70);
				regValue = READCHAR(FLASH_BASE_ADDRESS);
				if ((regValue & 0x80) == 0x80)
					break;	/* Case of Write-Operation had Ended */
			}
			/* Reading STATUS Register for Writing Verification */
			WRITE_CHAR(FLASH_BASE_ADDRESS, 0x70);
			regValue = READCHAR(FLASH_BASE_ADDRESS);
			if ((regValue & 0x10) == 0x10)
				return false;	/* Write failure */

			/* Writing Second Byte */
			WRITE_CHAR(FLASH_BASE_ADDRESS + 1, 0x10);
#ifdef BE
			c = (data >> 16);
#else
			c = (data >> 8);
#endif
			WRITE_CHAR(FLASH_BASE_ADDRESS + offset + 1, c);
			while (true) {
				/* Reading STATUS Register */
				WRITE_CHAR(FLASH_BASE_ADDRESS + 1, 0x70);
				regValue =
				    READCHAR(FLASH_BASE_ADDRESS + 1);
				if ((regValue & 0x80) == 0x80)
					break;	/* Write operation ended */
			}
			/* Reading STATUS Register for Writing verification */
			WRITE_CHAR(FLASH_BASE_ADDRESS + 1, 0x70);
			regValue = READCHAR(FLASH_BASE_ADDRESS + 1);
			if ((regValue & 0x10) == 0x10)
				return false;	/* Write failure */

			/* Writing Third Byte */
			WRITE_CHAR(FLASH_BASE_ADDRESS + 2, 0x10);
#ifdef BE
			c = (data >> 8);
#else
			c = (data >> 16);
#endif
			WRITE_CHAR(FLASH_BASE_ADDRESS + offset + 2, c);
			while (true) {
				/* Reading STATUS Register */
				WRITE_CHAR(FLASH_BASE_ADDRESS + 2, 0x70);
				regValue =
				    READCHAR(FLASH_BASE_ADDRESS + 2);
				if ((regValue & 0x80) == 0x80)
					break;	/* Write operation ended */
			}
			/* Reading STATUS Register for Writing Verification */
			WRITE_CHAR(FLASH_BASE_ADDRESS + 2, 0x70);
			regValue = READCHAR(FLASH_BASE_ADDRESS + 2);
			if ((regValue & 0x10) == 0x10)
				return false;	/* Write failure */

			/* Writing Fourth Byte */
			WRITE_CHAR(FLASH_BASE_ADDRESS + 3, 0x10);
#ifdef BE
			c = data;
#else
			c = (data >> 24);
#endif
			WRITE_CHAR(FLASH_BASE_ADDRESS + offset + 3, c);
			while (true) {
				/* Reading STATUS Register */
				WRITE_CHAR(FLASH_BASE_ADDRESS + 3, 0x70);
				regValue =
				    READCHAR(FLASH_BASE_ADDRESS + 3);
				if ((regValue & 0x80) == 0x80)
					break;	/* Write operation ended */
			}
			/* Reading STATUS Register for Writing Verification */
			WRITE_CHAR(FLASH_BASE_ADDRESS + 3, 0x70);
			regValue = READCHAR(FLASH_BASE_ADDRESS + 3);
			if ((regValue & 0x10) == 0x10)
				return false;	/* Write failure */
			flashReset();
			return true;
		case 2:
			if (FLASH_MODE == X16) {	/* Case of one X16 bit device */
				FirstData = 0x0010;	/* Data for the First  Cycle  */
			} else {	/* if (FLASH_MODE == 8) ==> Case of two X8 bit devices */

				FirstData = 0x1010;	/* Data for the First  Cycle  */
			}
			/* Writing First two Bytes */
			WRITE_SHORT(FLASH_BASE_ADDRESS, FirstData);
#ifdef BE
			s = (data >> 16);
#else
			s = data;
#endif
			WRITE_SHORT(FLASH_BASE_ADDRESS + offset, s);
			if (FLASH_MODE == X16) {
				data70 = 0x0070;
				data80 = 0x0080;
				data10 = 0x0010;
			} else {	/* case of (FLASH_MODE == X8) */

				data70 = 0x7070;
				data80 = 0x8080;
				data10 = 0x1010;
			}
			/* polling on writing action => when done break. */
			while (true) {
				WRITE_SHORT(FLASH_BASE_ADDRESS, data70);
				regValue = READSHORT(FLASH_BASE_ADDRESS);
				if ((regValue & data80) == data80)
					break;
			}
			/* Reading STATUS Register for Writing Verification */
			WRITE_CHAR(FLASH_BASE_ADDRESS, data70);
			regValue = READCHAR(FLASH_BASE_ADDRESS);
			if ((regValue & data10) == data10)
				return false;	/* Write failure */
			/* Writing Last two Bytes */
			WRITE_SHORT(FLASH_BASE_ADDRESS + 2, FirstData);
#ifdef BE
			s = data;
#else
			s = (data >> 16);
#endif
			WRITE_SHORT(FLASH_BASE_ADDRESS + offset + 2, s);
			/* polling on writing action => when done break. */
			while (true) {
				WRITE_SHORT(FLASH_BASE_ADDRESS + 2,
					    data70);
				regValue =
				    READSHORT(FLASH_BASE_ADDRESS + 2);
				if ((regValue & data80) == data80)
					break;
			}
			/* Reading STATUS Register for Writing Verification */
			WRITE_CHAR(FLASH_BASE_ADDRESS, data70);
			regValue = READCHAR(FLASH_BASE_ADDRESS);
			if ((regValue & data10) == data10)
				return false;	/* Write failure */
			flashReset();
			return true;
		case 4:
		case 8:
			if (FLASH_MODE == X16) {	/* Case of one X16 bit device */
				FirstData = 0x00100010;	/* Data for the First  Cycle  */
			} else {	/* (FLASH_MODE == 8) ==> Case of two X8 bit devices */

				FirstData = 0x10101010;	/* Data for the First  Cycle  */
			}
			/* Writing First two Bytes */
			WRITE_WORD(FLASH_BASE_ADDRESS +
				   offset % FLASH_WIDTH, FirstData);
#ifdef BE
			s = (data >> 16);
#else
			s = data;
#endif
			/* writing the 32-bit data to flash. */
			WRITE_WORD(FLASH_BASE_ADDRESS + offset, data);
			if (FLASH_MODE == X16) {
				data70 = 0x0070;
				data80 = 0x0080;
				data10 = 0x0010;
			} else {	/* (FLASH_MODE == 8) */

				data70 = 0x7070;
				data80 = 0x8080;
				data10 = 0x1010;
			}
			while (true) {
				WRITE_SHORT(FLASH_BASE_ADDRESS +
					    offset % FLASH_WIDTH, data70);
				regValue = READSHORT(FLASH_BASE_ADDRESS);
				if ((regValue & data80) == data80)
					break;
			}
			/* Reading STATUS Register for Writing Verification */
			WRITE_CHAR(FLASH_BASE_ADDRESS, data70);
			regValue = READCHAR(FLASH_BASE_ADDRESS);
			if ((regValue & data10) == data10)
				return false;	/* Write failure */

			/* Writing Last two Bytes */
#ifdef BE
			s = data;
#else
			s = (data >> 16);
#endif
			while (true) {
				WRITE_SHORT(FLASH_BASE_ADDRESS +
					    offset % FLASH_WIDTH + 2,
					    data70);
				regValue =
				    READSHORT(FLASH_BASE_ADDRESS +
					      offset % FLASH_WIDTH + 2);
				if ((regValue & data80) == data80)
					break;
			}
			/* Reading STATUS Register for Writing Verification */
			WRITE_CHAR(FLASH_BASE_ADDRESS, data70);
			regValue = READCHAR(FLASH_BASE_ADDRESS);
			if ((regValue & data10) == data10)
				return false;	/* Write failure */

			flashReset();
			return true;
		default:
			flashReset();
			return false;
		}
	}
	flashReset();
	return true;
}

/********************************************************************
* flashReadWord - Read 32Bit from the FLASH memory at a given offset
*                 from the FLASH base address.
* 				  address 0 = 0x00000000 !!
*                 The function takes care of Big/Little endian conversion
* INPUTS:  offset,the offset from the flash`s base address
* RETURNS: data
*********************************************************************/
unsigned int flashReadWord(unsigned int offset)
{
	unsigned int regValue;
	flashReset();
	READ_WORD(FLASH_BASE_ADDRESS + offset, &regValue);
	return regValue;
}

/********************************************************************
* flashInWhichSector - Returns the sector`s number at which offset is at.
*
* INPUTS:  Offset
* RETURNS: Sector number,or 0xffffffff in case the address is out of range or
*          flash wasn't initialize.
*********************************************************************/
unsigned int flashInWhichSector(unsigned int offset)
{
	unsigned int sectorNumber, numberOfDevices;
	unsigned int accMemory = 0;

	if ((FLASH_MODE == PURE8) || (FLASH_MODE == X8)) {
		numberOfDevices = FLASH_WIDTH;
	} else {		/* X16 mode */

		numberOfDevices = FLASH_WIDTH / 2;
	}
	for (sectorNumber = 0;
	     sectorNumber < flashTypes[NUMBER_OF_SECTORS]; sectorNumber++) {
		accMemory =
		    accMemory + flashTypes[FIRST_SECTOR_SIZE +
					   sectorNumber];
		if (offset < accMemory * numberOfDevices * 1024)
			return sectorNumber;
	}
	return 0xffffffff;
}

/********************************************************************
* flashGetSectorSize - When given a Valid sector Number returns its Size.
*
* INPUTS:  unsigned int sectorNumber.
* RETURNS: Sector size. (if Sector number isn't valid or flash wasn't
*          initialize return 0.)
*********************************************************************/
unsigned int flashGetSectorSize(unsigned int sectorNumber)
{
	if (sectorNumber >= flashTypes[NUMBER_OF_SECTORS])
		return 0;
	else {
		if (FLASH_MODE != PURE8)
			return (flashTypes
				[FIRST_SECTOR_SIZE +
				 sectorNumber] * _1K * (FLASH_WIDTH * 8 /
							FLASH_MODE));
		else		/* in case of PUR8 */
			return (flashTypes
				[FIRST_SECTOR_SIZE +
				 sectorNumber] * _1K * FLASH_WIDTH);
	}
}

/********************************************************************
* getFlashSize - Return Total flash size.
*
* INPUTS:  N/A.
* RETURNS: Flash size. (If flash wasn't initialize return 0)
*********************************************************************/
unsigned int flashGetSize()
{
	unsigned int sectorNum;
	unsigned int totalSize = 0;

	if (POINTER_TO_FLASH == 0)
		return 0;	/* case of flash not initialize */
	for (sectorNum = 0; sectorNum < flashTypes[NUMBER_OF_SECTORS];
	     sectorNum++) {
		totalSize += flashGetSectorSize(sectorNum);
	}
	return (totalSize);

}

/********************************************************************
* flashGetSectorOffset - Returns sector base address.
*
* INPUTS:  unsigned int sectorNum.
* RETURNS: Sector Base Address.
*********************************************************************/
unsigned int flashGetSectorOffset(unsigned int sectorNum)
{
	unsigned int i;
	unsigned int sectorBaseAddress = 0;
	unsigned int numOfDevices;

	if (sectorNum > (flashParametrs[NUMBER_OF_SECTORS] - 1))
		return 0xffffffff;
	for (i = 0; i < sectorNum; i++) {
		sectorBaseAddress =
		    sectorBaseAddress + flashTypes[FIRST_SECTOR_SIZE + i];
	}
	if (FLASH_MODE == X16)
		numOfDevices = FLASH_WIDTH * 8 / FLASH_MODE;
	else
		numOfDevices = FLASH_WIDTH;
	return (_1K * sectorBaseAddress * numOfDevices);

}

/********************************************************************
* flashWriteBlock - Write block of chars to flash.
*
* INPUTS:  unsigned int offset - flash destination address.
*          unsigned int numOfByte - block size.
*          unsigned char * blockAddress - block source address.
* RETURNS: Number of Bytes written.
*********************************************************************/
unsigned int flashWriteBlock(unsigned int offset, unsigned int numOfByte,
			     unsigned char *blockAddress)
{
	register unsigned int flashWrite;
	register unsigned int align;
	register unsigned int num;
	register unsigned int i;

	if ((offset + numOfByte) > flashGetSize())
		numOfByte = flashGetSize() - offset;	/* getting to flash boundary. */
	num = numOfByte;
	align = offset % 4;	/* alignment toward flash.    */
	/* writes chars until the offset toward flash will be align. */
	for (i = align; (i < 4) && (numOfByte > 0) && (align != 0); i++) {
		flashWriteChar(offset, blockAddress[0]);
		numOfByte--;
		offset++;
		blockAddress++;
	}
	while (numOfByte > 3) {
#ifdef LE
		flashWrite = blockAddress[0] | (blockAddress[1] << 8) |
		    (blockAddress[2] << 16) | (blockAddress[3] << 24);
#else
		flashWrite = blockAddress[3] | (blockAddress[2] << 8) |
		    (blockAddress[1] << 16) | (blockAddress[0] << 24);
#endif
		if (flashWrite != 0xffffffff)	/* for optimization. */
			flashWriteWord(offset, flashWrite);
		numOfByte -= 4;
		blockAddress += 4;
		offset += 4;
	}
	while (numOfByte > 0) {
		flashWriteChar(offset, blockAddress[0]);
		numOfByte--;
		blockAddress++;
		offset++;
	}
	return num;
}

/********************************************************************
* flashReadBlock - Read block of chars from flash.
*
* INPUTS:  unsigned int offset - flash source address.
*          unsigned int numOfByte - block size.
*          unsigned char * blockAddress - block destination address.
* RETURNS: Number of Bytes written.
*********************************************************************/
unsigned int flashReadBlock(unsigned int offset, unsigned int numOfByte,
			    unsigned char *blockAddress)
{
	unsigned int i;
	for (i = 0; i < numOfByte; i++) {
		blockAddress[i] = flashReadChar(offset + i);
	}
	return numOfByte;
}

/********************************************************************
* flashReadChar - read one charecter form given flash offset.
*
* INPUTS:  unsigned int offset - required offset to be read from.
* RETURNS: read charecter.
*********************************************************************/
unsigned char flashReadChar(unsigned int offset)
{
	unsigned char regValue;

	flashReset();
	READ_CHAR(FLASH_BASE_ADDRESS + offset, &regValue);
	return regValue;
}

/********************************************************************
* flashReadShort - read 16bit form given flash offset.
*
* INPUTS:  unsigned int offset - required offset to be read from.
* RETURNS: 16bit data.
*********************************************************************/
unsigned short flashReadShort(unsigned int offset)
{
	unsigned short regValue;

	flashReset();
	READ_SHORT(FLASH_BASE_ADDRESS + offset, &regValue);
	return regValue;
}

/********************************************************************
* flashWriteShort - write 16bit data to a given flash offset.
*                  It reads the whole word 32bit wide, modify the  short
*                  and write back the word.
*
* INPUTS:  unsigned int offset - required offset to be write to.
*          unsigned short sdata - data to be written.
* RETURNS: true if writting successesed false otherwise.
*********************************************************************/
bool flashWriteShort(unsigned int offset, unsigned short sdata)
{
	unsigned int align;
	unsigned int flashWrite;
	unsigned int flashRead;

	align = offset % 4;
	if ((align == 1) || (align == 3))
		return false;	/* offset misaligned. */
	flashRead = flashReadWord(offset - align);
	if (align == 0)
#ifdef BE
		flashWrite = (flashRead & 0x0000ffff) | (sdata << 16);
#else
		flashWrite = (flashRead & 0xffff0000) | sdata;
#endif
	else			/* (align == 2) */
#ifdef BE
		flashWrite = (flashRead & 0xffff0000) | sdata;
#else
		flashWrite = (flashRead & 0x0000ffff) | (sdata << 16);
#endif
	flashWriteWord(offset - align, flashWrite);
	return true;

}

/********************************************************************
* flashWriteChar - write one charecter (8 bit) to a given flash offset.
*                  It reads the whole word 32bit wide, modify the charecter
*                  and write back the word.
*
* INPUTS:  unsigned int offset - required offset to be write to.
*          unsigned short sdata - data to be written.
* RETURNS: true if writting successed.
*********************************************************************/
bool flashWriteChar(unsigned int offset, unsigned char cdata)
{
	unsigned int align;
	unsigned int flashWrite;
	unsigned int flashRead;

	align = offset % 4;
	flashRead = flashReadWord(offset - align);
#ifdef BE
	flashWrite = (flashRead & ~(0xff000000 >> (8 * align))) |
	    (cdata << (8 * (3 - align)));
#else
	flashWrite = (flashRead & ~(0xff000000 << (8 * align))) |
	    (cdata << (8 * align));
#endif
	flashWriteWord(offset - align, flashWrite);
	return true;
}

/********************************************************************
* flashGetNumOfSectors - write one charecter (8 bit) to a given flash offset.
*                        It reads the whole word 32bit wide, modify the
*                        charecter and write back the word.
*
* INPUTS:  N/A.
* RETURNS: Number of sectors.
*********************************************************************/
unsigned int flashGetNumOfSectors(void)
{
	return (flashTypes[NUMBER_OF_SECTORS]);
}
