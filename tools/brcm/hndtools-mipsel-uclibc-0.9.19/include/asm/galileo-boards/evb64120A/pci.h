/* PCI.h - PCI functions header file */

/* Copyright - Galileo technology. */

#ifndef  __INCpcih
#define  __INCpcih

/* includes */

#include"core.h"

/* defines */

#define PCI0_MASTER_ENABLE(deviceNumber) pci0WriteConfigReg(                  \
          PCI_0STATUS_AND_COMMAND,deviceNumber,MASTER_ENABLE |                \
          pci0ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber) )

#define PCI0_MASTER_DISABLE(deviceNumber) pci0WriteConfigReg(                 \
          PCI_0STATUS_AND_COMMAND,deviceNumber,~MASTER_ENABLE &               \
          pci0ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber) )

#define PCI1_MASTER_ENABLE(deviceNumber) pci1WriteConfigReg(                  \
          PCI_0STATUS_AND_COMMAND,deviceNumber,MASTER_ENABLE |                \
          pci1ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber) )

#define PCI1_MASTER_DISABLE(deviceNumber) pci1WriteConfigReg(                 \
          PCI_0STATUS_AND_COMMAND,deviceNumber,~MASTER_ENABLE &               \
          pci1ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber) )

#define PCI0_MEMORY_ENABLE(deviceNumber) pci0WriteConfigReg(                  \
          PCI_0STATUS_AND_COMMAND,deviceNumber,MEMORY_ENABLE |                \
          pci0ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber) )

#define PCI1_MEMORY_ENABLE(deviceNumber) pci1WriteConfigReg(                  \
          PCI_0STATUS_AND_COMMAND,deviceNumber,MEMORY_ENABLE |                \
          pci1ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber) )

#define PCI0_IO_ENABLE(deviceNumber) pci0WriteConfigReg(                      \
          PCI_0STATUS_AND_COMMAND,deviceNumber,I_O_ENABLE |                   \
          pci0ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber) )

#define PCI1_IO_ENABLE(deviceNumber) pci1WriteConfigReg(                      \
          PCI_0STATUS_AND_COMMAND,deviceNumber,I_O_ENABLE |                   \
          pci1ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber) )

#define PCI0_SLAVE_ENABLE(deviceNumber) pci0WriteConfigReg(                   \
          PCI_0STATUS_AND_COMMAND,deviceNumber,MEMORY_ENABLE | I_O_ENABLE |   \
          pci0ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber) )

#define PCI1_SLAVE_ENABLE(deviceNumber) pci1WriteConfigReg(                   \
          PCI_0STATUS_AND_COMMAND,deviceNumber,MEMORY_ENABLE | I_O_ENABLE |   \
          pci1ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber) )

#define PCI0_DISABLE(deviceNumber) pci0WriteConfigReg(                        \
          PCI_0STATUS_AND_COMMAND,deviceNumber,0xfffffff8  &                  \
          pci0ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber))

#define PCI1_DISABLE(deviceNumber) pci1WriteConfigReg(                        \
          PCI_0STATUS_AND_COMMAND,deviceNumber,0xfffffff8  &                  \
          pci1ReadConfigReg(PCI_0STATUS_AND_COMMAND,deviceNumber))

#define 	MASTER_ENABLE			BIT2
#define		MEMORY_ENABLE			BIT1
#define		I_O_ENABLE  			BIT0
#define     SELF                    0
/* Agent on the PCI bus may have up to 6 BARS. */
#define     BAR0                    0x10
#define     BAR1                    0x14
#define     BAR2                    0x18
#define     BAR3                    0x1c
#define     BAR4                    0x20
#define     BAR5                    0x24


/* typedefs */

typedef struct pciDevice
{
    char            type[20];
    unsigned int    deviceNum;
    unsigned int    venID;
    unsigned int    deviceID;
    unsigned int    bar0Base;
    unsigned int    bar0Size;
    unsigned int    bar0Type;
    unsigned int    bar1Base;
    unsigned int    bar1Size;
    unsigned int    bar1Type;
    unsigned int    bar2Base;
    unsigned int    bar2Size;
    unsigned int    bar2Type;
    unsigned int    bar3Base;
    unsigned int    bar3Size;
    unsigned int    bar3Type;
    unsigned int    bar4Base;
    unsigned int    bar4Size;
    unsigned int    bar4Type;
    unsigned int    bar5Base;
    unsigned int    bar5Size;
    unsigned int    bar5Type;
} PCI_DEVICE;

void    pci0WriteConfigReg(unsigned int regOffset,unsigned int pciDevNum,
                           unsigned int data);
void    pci1WriteConfigReg(unsigned int regOffset,unsigned int pciDevNum,
                           unsigned int data);
void    pci0ScanDevices(PCI_DEVICE *pci0Detect,unsigned int numberOfElment);
void    pci1ScanDevices(PCI_DEVICE *pci1Detect,unsigned int numberOfElment);
unsigned int    pci0ReadConfigReg (unsigned int regOffset,
                                   unsigned int pciDevNum);
unsigned int    pci1ReadConfigReg (unsigned int regOffset,
                                   unsigned int pciDevNum);

/*      Master`s memory space   */

void    pci0MapIOspace(unsigned int pci0IoBase,unsigned int pci0IoLength);
void    pci0MapMemory0space(unsigned int pci0Mem0Base,
                            unsigned int pci0Mem0Length);
void    pci0MapMemory1space(unsigned int pci0Mem1Base,
                            unsigned int pci0Mem1Length);

void    pci1MapIOspace(unsigned int pci1IoBase,unsigned int pci1IoLength);
void    pci1MapMemory0space(unsigned int pci1Mem0Base,
                            unsigned int pci1Mem0Length);
void    pci1MapMemory1space(unsigned int pci1Mem1Base,
                            unsigned int pci1Mem1Length);

unsigned int    pci0GetIOspaceBase(void);
unsigned int    pci0GetIOspaceSize(void);
unsigned int    pci0GetMemory0Base(void);
unsigned int    pci0GetMemory0Size(void);
unsigned int    pci0GetMemory1Base(void);
unsigned int    pci0GetMemory1Size(void);

unsigned int    pci1GetIOspaceBase(void);
unsigned int    pci1GetIOspaceSize(void);
unsigned int    pci1GetMemory0Base(void);
unsigned int    pci1GetMemory0Size(void);
unsigned int    pci1GetMemory1Base(void);
unsigned int    pci1GetMemory1Size(void);

/*      Slave`s memory space   */
void    pci0MapInternalRegSpace(unsigned int pci0InternalBase);
void    pci0MapInternalRegIOSpace(unsigned int pci0InternalBase);
void    pci0MapMemoryBanks0_1(unsigned int pci0Dram0_1Base,
                              unsigned int pci0Dram0_1Size);
void    pci0MapMemoryBanks2_3(unsigned int pci0Dram2_3Base,
                              unsigned int pci0Dram2_3Size);
void    pci0MapDevices0_1and2MemorySpace(unsigned int pci0Dev012Base,
                                         unsigned int pci0Dev012Length);
void    pci0MapDevices3andBootMemorySpace(unsigned int pci0Dev3andBootBase,
                                          unsigned int pci0Dev3andBootLength);

void    pci1MapInternalRegSpace(unsigned int pci1InternalBase);
void    pci1MapInternalRegIOSpace(unsigned int pci1InternalBase);
void    pci1MapMemoryBanks0_1(unsigned int pci1Dram0_1Base,
                              unsigned int pci1Dram0_1Size);
void    pci1MapMemoryBanks2_3(unsigned int pci1Dram2_3Base,
                              unsigned int pci1Dram2_3Size);
void    pci1MapDevices0_1and2MemorySpace(unsigned int pci1Dev012Base,
                                         unsigned int pci1Dev012Length);
void    pci1MapDevices3andBootMemorySpace(unsigned int pci1Dev3andBootBase,
                                          unsigned int pci1Dev3andBootLength);

#endif  /* __INCpcih */
