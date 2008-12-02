/* flashdrv.h - FLASH memory interface header file */

/* Copyright Galileo Technology. */

#ifndef __INCflashdrvh
#define __INCflashdrvh

/* includes */

#include "core.h"

/* defines */

/* Supported Flash Manufactures */

#define AMD_FLASH       0x01
#define ST_FLASH        0x20
#define INTEL_FLASH     0x89
#define MICRON_FLASH    0x89

/* Supported Flash Devices */

/* AMD Devices */
#define AM29F400BT      0x2223
#define AM29F400BB      0x22AB
#define AM29LV800BT     0x22DA
#define AM29LV400BT     0x22B9
#define AM29LV400BB     0x22BA
#define AM29LV040B      0x4f
/* ST Devices */
#define M29W040         0xE3
/* INTEL Devices - We have added I before the name defintion.*/
#define I28F320J3A      0x16
#define I28F640J3A      0x17
#define I28F128J3A      0x18
#define I28F320B3_B     0x8897
#define I28F320B3_T     0x8896
#define I28F160B3_B     0x8891
#define I28F160B3_T     0x8890

#define POINTER_TO_FLASH           flashParametrs[0]
#define FLASH_BASE_ADDRESS         flashParametrs[1]
#define FLASH_WIDTH                flashParametrs[2] /* In Bytes */
#define FLASH_MODE                 flashParametrs[3] /* In bits  */
#define MANUFACTOR_ID              POINTER_TO_FLASH + 0
#define VENDOR_ID                  POINTER_TO_FLASH + 1
#define NUMBER_OF_SECTORS          POINTER_TO_FLASH + 2
#define FIRST_SECTOR_SIZE          POINTER_TO_FLASH + 3
#define NUM_OF_DEVICES             FLASH_WIDTH / (FLASH_MODE / 8)

/* typedefs */

typedef enum _FlashMode {PURE8,X8 = 8,X16 = 16} FLASHmode;
/* PURE8 - when using a flash device whice can be configurated only as
            8 bit device. */
/* X8    - when using a flash device which is 16 bit wide but configured to
           operate in 8 bit mode.*/
/* X16   - when using a flash device which is 16 bit wide */

bool    flashErase(void);
bool    flashEraseSector(unsigned int sectorNumber);
bool    flashWriteWord(unsigned int offset,unsigned int data);
bool    flashWriteShort(unsigned int offset,unsigned short sdata);
bool    flashWriteChar(unsigned int offset,unsigned char cdata);
void    flashReset(void);
unsigned int    flashInWhichSector(unsigned int offset);
unsigned int    flashGetSectorSize(unsigned int sectorNumber);
unsigned int    flashInit(unsigned int baseAddress,unsigned int flashWidth,
                          FLASHmode FlashMode);
unsigned int    flashGetNumOfSectors(void);
unsigned int    flashGetSize(void);
unsigned int    flashGetSectorOffset(unsigned int sectorNum);
unsigned int    flashWriteBlock(unsigned int offset,unsigned int numOfByte,
                        unsigned char * blockAddress);
unsigned int    flashReadWord(unsigned int offset);
unsigned char   flashReadChar(unsigned int offset);
unsigned short  flashReadShort(unsigned int offset);
unsigned int    flashReadBlock(unsigned int offset,unsigned int numOfByte,
                               unsigned char * blockAddress);
#endif /* __INCflashdrvh */

