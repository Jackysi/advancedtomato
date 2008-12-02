/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/init301.h,v 1.4 2000/12/02 01:16:17 dawes Exp $ */
#ifndef  _INIT301_
#define  _INIT301_

#include "osdef.h"
#include "initdef.h"
#include "vgatypes.h"
#include "vstruct.h"

#ifdef TC
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#endif

#ifdef LINUX_XF86
#include "xf86.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "sis.h"
#include "sis_regs.h"
#endif

#ifdef LINUX_KERNEL
#include <asm/io.h>
#include <linux/types.h>
#include <linux/sisfb.h>
#endif

#ifdef WIN2000
#include <stdio.h>
#include <string.h>
#include <miniport.h>
#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"
#include "ntddvdeo.h"
#include "video.h"
#include "sisv.h"
#endif


extern   BOOLEAN  SiS_SearchVBModeID(SiS_Private *SiS_Pr, UCHAR *RomAddr, USHORT *);

BOOLEAN  SiS_Is301B(SiS_Private *SiS_Pr, USHORT BaseAddr);
BOOLEAN  SiS_IsM650or651(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
BOOLEAN  SiS_IsDisableCRT2(SiS_Private *SiS_Pr, USHORT BaseAddr);
BOOLEAN  SiS_IsVAMode(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
BOOLEAN  SiS_IsDualEdge(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
BOOLEAN  SiS_CRT2IsLCD(SiS_Private *SiS_Pr, USHORT BaseAddr);
void     SiS_SetDefCRT2ExtRegs(SiS_Private *SiS_Pr, USHORT BaseAddr);
USHORT   SiS_GetRatePtrCRT2(SiS_Private *SiS_Pr, UCHAR *ROMAddr, USHORT ModeNo,USHORT ModeIdIndex,
                            PSIS_HW_DEVICE_INFO HwDeviceExtension);
BOOLEAN  SiS_AdjustCRT2Rate(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT MODEIdIndex,
                            USHORT RefreshRateTableIndex,USHORT *i,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SaveCRT2Info(SiS_Private *SiS_Pr, USHORT ModeNo);
void     SiS_GetCRT2Data(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
		         USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_GetCRT2DataLVDS(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                             USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_GetCRT2PtrA(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                         USHORT RefreshRateTableIndex,USHORT *CRT2Index,USHORT *ResIndex);
void     SiS_GetCRT2Part2Ptr(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
		             USHORT RefreshRateTableIndex,USHORT *CRT2Index, USHORT *ResIndex);
void     SiS_GetCRT2Data301(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                            USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
USHORT   SiS_GetResInfo(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
void     SiS_GetCRT2ResInfo(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                            PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_GetRAMDAC2DATA(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                            USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_GetCRT2Ptr(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                        USHORT RefreshRateTableIndex,USHORT *CRT2Index,USHORT *ResIndex,
			PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SetCRT2ModeRegs(SiS_Private *SiS_Pr, USHORT BaseAddr,USHORT ModeNo,USHORT ModeIdIndex,
                             PSIS_HW_DEVICE_INFO );
void     SiS_SetHiVision(SiS_Private *SiS_Pr, USHORT BaseAddr,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_GetLVDSDesData(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
			    USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SetCRT2Offset(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                           USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
USHORT   SiS_GetOffset(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                       USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
USHORT   SiS_GetColorDepth(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
USHORT   SiS_GetMCLK(SiS_Private *SiS_Pr, UCHAR *ROMAddr, PSIS_HW_DEVICE_INFO HwDeviceExtension);
USHORT   SiS_CalcDelayVB(SiS_Private *SiS_Pr);
USHORT   SiS_GetVCLK2Ptr(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                         USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SetCRT2Sync(SiS_Private *SiS_Pr, USHORT BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,
                         USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SetRegANDOR(USHORT Port,USHORT Index,USHORT DataAND,USHORT DataOR);
void     SiS_SetRegOR(USHORT Port,USHORT Index,USHORT DataOR);
void     SiS_SetRegAND(USHORT Port,USHORT Index,USHORT DataAND);
USHORT   SiS_GetVGAHT2(SiS_Private *SiS_Pr);
void     SiS_SetGroup2(SiS_Private *SiS_Pr, USHORT BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                       USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SetGroup3(SiS_Private *SiS_Pr, USHORT BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                       PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SetGroup4(SiS_Private *SiS_Pr, USHORT  BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                       USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SetGroup5(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO, USHORT  BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,
                       USHORT ModeIdIndex);
void     SiS_FinalizeLCD(SiS_Private *SiS_Pr, USHORT BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                         PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SetCRT2VCLK(SiS_Private *SiS_Pr, USHORT BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                         USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_EnableCRT2(SiS_Private *SiS_Pr);
void     SiS_GetVBInfo(SiS_Private *SiS_Pr, USHORT BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                       PSIS_HW_DEVICE_INFO HwDeviceExtension);
BOOLEAN  SiS_BridgeIsOn(SiS_Private *SiS_Pr, USHORT BaseAddr,PSIS_HW_DEVICE_INFO);
BOOLEAN  SiS_BridgeIsEnable(SiS_Private *SiS_Pr, USHORT BaseAddr,PSIS_HW_DEVICE_INFO);
BOOLEAN  SiS_BridgeInSlave(SiS_Private *SiS_Pr);
void     SiS_PresetScratchregister(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SetTVSystem(SiS_Private *SiS_Pr);
void     SiS_LongWait(SiS_Private *SiS_Pr);
USHORT   SiS_GetQueueConfig(SiS_Private *SiS_Pr);
void     SiS_VBLongWait(SiS_Private *SiS_Pr);
USHORT   SiS_GetVCLKLen(SiS_Private *SiS_Pr, UCHAR *ROMAddr);
void     SiS_WaitVBRetrace(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_WaitRetrace1(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_WaitRetrace2(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_WaitRetraceDDC(SiS_Private *SiS_Pr);
void     SiS_SetCRT2ECLK(SiS_Private *SiS_Pr, UCHAR *ROMAddr, USHORT ModeNo,USHORT ModeIdIndex,
                         USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_GetLVDSDesPtr(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                           USHORT RefreshRateTableIndex,USHORT *PanelIndex,USHORT *ResIndex,
			   PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_GetLVDSDesPtrA(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                            USHORT RefreshRateTableIndex,USHORT *PanelIndex,USHORT *ResIndex);
void     SiS_SetTPData(SiS_Private *SiS_Pr);
void     SiS_ModCRT1CRTC(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                         USHORT RefreshRateTableIndex,PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SetCHTVReg(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                        USHORT RefreshRateTableIndex);
void     SiS_GetCHTVRegPtr(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                           USHORT RefreshRateTableIndex);
void     SiS_SetCH700x(SiS_Private *SiS_Pr, USHORT tempax);
USHORT   SiS_GetCH700x(SiS_Private *SiS_Pr, USHORT tempax);
void     SiS_SetCH701x(SiS_Private *SiS_Pr, USHORT tempax);
USHORT   SiS_GetCH701x(SiS_Private *SiS_Pr, USHORT tempax);
void     SiS_SetCH70xx(SiS_Private *SiS_Pr, USHORT tempax);
USHORT   SiS_GetCH70xx(SiS_Private *SiS_Pr, USHORT tempax);
#ifdef LINUX_XF86
USHORT   SiS_I2C_GetByte(SiS_Private *SiS_Pr);
Bool     SiS_I2C_PutByte(SiS_Private *SiS_Pr, USHORT data);
Bool     SiS_I2C_Address(SiS_Private *SiS_Pr, USHORT addr);
void     SiS_I2C_Stop(SiS_Private *SiS_Pr);
#endif
void     SiS_SetCH70xxANDOR(SiS_Private *SiS_Pr, USHORT tempax,USHORT tempbh);
void     SiS_SetSwitchDDC2(SiS_Private *SiS_Pr);
USHORT   SiS_SetStart(SiS_Private *SiS_Pr);
USHORT   SiS_SetStop(SiS_Private *SiS_Pr);
void     SiS_DDC2Delay(SiS_Private *SiS_Pr, USHORT delaytime);
USHORT   SiS_SetSCLKLow(SiS_Private *SiS_Pr);
USHORT   SiS_SetSCLKHigh(SiS_Private *SiS_Pr);
USHORT   SiS_ReadDDC2Data(SiS_Private *SiS_Pr, USHORT tempax);
USHORT   SiS_WriteDDC2Data(SiS_Private *SiS_Pr, USHORT tempax);
USHORT   SiS_CheckACK(SiS_Private *SiS_Pr);
#ifdef SIS315H
void     SiS_OEM310Setting(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
                           UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
void     SiS_OEMLCD(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
                    UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
USHORT 	 GetLCDPtrIndex(SiS_Private *SiS_Pr);
USHORT   GetTVPtrIndex(SiS_Private *SiS_Pr);
void     SetDelayComp(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
             	     UCHAR *ROMAddr,USHORT ModeNo);
void     SetAntiFlicker(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
               	       UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
void     SetEdgeEnhance(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
                       UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
void     SetYFilter(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
           	   UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
void	 SetPhaseIncr(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
             	     UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
#endif
#ifdef SIS300
void     SiS_OEM300Setting(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
                           UCHAR *ROMAddr,USHORT ModeNo);
USHORT 	 GetOEMLCDPtr(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, int Flag);
USHORT 	 GetOEMTVPtr(SiS_Private *SiS_Pr);
void	 SetOEMTVDelay(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
              	      UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
void	 SetOEMLCDDelay(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
               	       UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
void	 SetOEMAntiFlicker(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,
                          USHORT BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
void  	 SetOEMPhaseIncr(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
                         UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
void	 SetOEMYFilter(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT BaseAddr,
               	       UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex);
#endif
USHORT   GetRevisionID(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension);
BOOLEAN  SiS_LowModeStuff(SiS_Private *SiS_Pr, USHORT ModeNo,PSIS_HW_DEVICE_INFO HwDeviceExtension);

BOOLEAN  SiS_GetLCDResInfo(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo, USHORT ModeIdIndex,
                           PSIS_HW_DEVICE_INFO HwDeviceExtension);
/* void    SiS_CHACRT1CRTC(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                        USHORT RefreshRateTableIndex); */

BOOLEAN  SiS_SetCRT2Group301(SiS_Private *SiS_Pr, USHORT BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,
                             PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SetGroup1(SiS_Private *SiS_Pr, USHORT BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                       PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT RefreshRateTableIndex);
void     SiS_SetGroup1_LVDS(SiS_Private *SiS_Pr, USHORT BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                            PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT RefreshRateTableIndex);
void     SiS_SetGroup1_LCDA(SiS_Private *SiS_Pr, USHORT  BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                            PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT RefreshRateTableIndex);
void     SiS_SetGroup1_301(SiS_Private *SiS_Pr, USHORT BaseAddr,UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                           PSIS_HW_DEVICE_INFO HwDeviceExtension,USHORT RefreshRateTableIndex);
#ifdef SIS300
void     SiS_SetCRT2FIFO_300(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,
                             PSIS_HW_DEVICE_INFO HwDeviceExtension);
#endif
#ifdef SIS315H
void     SiS_SetCRT2FIFO_310(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,
                             PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_CRT2AutoThreshold(SiS_Private *SiS_Pr, USHORT  BaseAddr);
#endif
BOOLEAN  SiS_GetLCDDDCInfo(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_UnLockCRT2(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO,USHORT BaseAddr);
void     SiS_LockCRT2(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO,USHORT BaseAddr);
void     SiS_DisableBridge(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO,USHORT  BaseAddr);
void     SiS_EnableBridge(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO,USHORT BaseAddr);
void     SiS_SetPanelDelay(SiS_Private *SiS_Pr, UCHAR* ROMAddr,PSIS_HW_DEVICE_INFO,USHORT DelayTime);
void     SiS_ShortDelay(SiS_Private *SiS_Pr, USHORT delay);
void     SiS_LongDelay(SiS_Private *SiS_Pr, USHORT delay);
void     SiS_GenericDelay(SiS_Private *SiS_Pr, USHORT delay);
void     SiS_VBWait(SiS_Private *SiS_Pr);

void     SiS_SiS30xBLOn(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension);
void     SiS_SiS30xBLOff(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension);

/* TW: New functions (with mostly temporary names) */
void     SiS_Chrontel701xBLOn(SiS_Private *SiS_Pr);
void     SiS_Chrontel701xOn(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension,
                            USHORT BaseAddr);
void     SiS_Chrontel701xBLOff(SiS_Private *SiS_Pr);
void     SiS_Chrontel701xOff(SiS_Private *SiS_Pr);
void     SiS_ChrontelResetDB(SiS_Private *SiS_Pr);
void     SiS_ChrontelDoSomething4(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
void     SiS_ChrontelDoSomething3(SiS_Private *SiS_Pr, USHORT ModeNo, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
void     SiS_ChrontelDoSomething2(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
void     SiS_ChrontelDoSomething1(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
BOOLEAN  SiS_WeHaveBacklightCtrl(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
BOOLEAN  SiS_IsYPbPr(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
BOOLEAN  SiS_IsTVOrYPbPr(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
BOOLEAN  SiS_IsLCDOrLCDA(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
void     SiS_SetCH701xForLCD(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension, USHORT BaseAddr);
void     SiS_Chrontel19f2(SiS_Private *SiS_Pr);
BOOLEAN  SiS_CR36BIOSWord23b(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension);
BOOLEAN  SiS_CR36BIOSWord23d(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension);
BOOLEAN  SiS_IsSR13_CR30(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO HwDeviceExtension);
/* TW end */

extern   void     SiS_SetReg1(USHORT, USHORT, USHORT);
extern   void     SiS_SetReg3(USHORT, USHORT);
extern   UCHAR    SiS_GetReg1(USHORT, USHORT);
extern   UCHAR    SiS_GetReg2(USHORT);
extern   BOOLEAN  SiS_SearchModeID(SiS_Private *SiS_Pr, UCHAR *ROMAddr, USHORT *ModeNo,USHORT *ModeIdIndex);
extern   BOOLEAN  SiS_GetRatePtr(SiS_Private *SiS_Pr, ULONG, USHORT);
extern   void     SiS_SetReg4(USHORT, ULONG);
extern   ULONG    SiS_GetReg3(USHORT);
extern   void     SiS_DisplayOff(SiS_Private *SiS_Pr);
extern   void     SiS_DisplayOn(SiS_Private *SiS_Pr);
extern   UCHAR    SiS_GetModePtr(SiS_Private *SiS_Pr, UCHAR *ROMAddr, USHORT ModeNo,USHORT ModeIdIndex);
extern   BOOLEAN  SiS_GetLCDACRT1Ptr(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
		                     USHORT RefreshRateTableIndex,USHORT *ResInfo,USHORT *DisplayType);
extern   BOOLEAN  SiS_GetLVDSCRT1Ptr(SiS_Private *SiS_Pr, UCHAR *ROMAddr,USHORT ModeNo,USHORT ModeIdIndex,
                                     USHORT RefreshRateTableIndex,USHORT *ResInfo,USHORT *DisplayType);
extern   void     SiS_LoadDAC(SiS_Private *SiS_Pr, PSIS_HW_DEVICE_INFO, UCHAR *ROMAddr,USHORT ModeNo,
                              USHORT ModeIdIndex);
#ifdef SIS315H
extern   UCHAR    SiS_Get310DRAMType(SiS_Private *SiS_Pr, UCHAR *ROMAddr,PSIS_HW_DEVICE_INFO HwDeviceExtension);
#endif

#ifdef LINUX_XF86
/* DDC functions */
USHORT   SiS_InitDDCRegs(SiS_Private *SiS_Pr, SISPtr pSiS, USHORT adaptnum, USHORT DDCdatatype);
USHORT   SiS_WriteDABDDC(SiS_Private *SiS_Pr);
USHORT   SiS_PrepareReadDDC(SiS_Private *SiS_Pr);
USHORT   SiS_PrepareDDC(SiS_Private *SiS_Pr);
void     SiS_SendACK(SiS_Private *SiS_Pr, USHORT yesno);
USHORT   SiS_DoProbeDDC(SiS_Private *SiS_Pr);
USHORT   SiS_ProbeDDC(SiS_Private *SiS_Pr);
USHORT   SiS_ReadDDC(SiS_Private *SiS_Pr, SISPtr pSiS, USHORT DDCdatatype, unsigned char *buffer);
USHORT   SiS_HandleDDC(SiS_Private *SiS_Pr, SISPtr pSiS, USHORT adaptnum,
                       USHORT DDCdatatype, unsigned char *buffer);
#endif

#endif
