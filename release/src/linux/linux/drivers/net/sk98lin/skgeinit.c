/******************************************************************************
 *
 * Name:	skgeinit.c
 * Project:	GEnesis, PCI Gigabit Ethernet Adapter
 * Version:	$Revision: 1.1.1.4 $
 * Date:	$Date: 2003/10/14 08:08:27 $
 * Purpose:	Contains functions to initialize the GE HW
 *
 ******************************************************************************/

/******************************************************************************
 *
 *	(C)Copyright 1998-2001 SysKonnect GmbH.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	The information in this file is provided "AS IS" without warranty.
 *
 ******************************************************************************/


#include "h/skdrv1st.h"
#include "h/xmac_ii.h"
#include "h/skdrv2nd.h"

/* defines ********************************************************************/

/* defines for SkGeXmitLed() */
#define XMIT_LED_INI	0
#define XMIT_LED_CNT	(RX_LED_VAL - RX_LED_INI)
#define XMIT_LED_CTRL	(RX_LED_CTRL- RX_LED_INI)
#define XMIT_LED_TST	(RX_LED_TST - RX_LED_INI)

/* Queue Size units */
#define QZ_UNITS	0x7

/* Types of RAM Buffer Queues */
#define SK_RX_SRAM_Q	1	/* small receive queue */
#define SK_RX_BRAM_Q	2	/* big receive queue */
#define SK_TX_RAM_Q	3	/* small or big transmit queue */

/* typedefs *******************************************************************/
/* global variables ***********************************************************/

/* local variables ************************************************************/

static const char SysKonnectFileId[] =
	"@(#)$Id: skgeinit.c,v 1.1.1.4 2003/10/14 08:08:27 sparq Exp $ (C) SK ";

struct s_QOffTab {
	int	RxQOff;		/* Receive Queue Address Offset */
	int	XsQOff;		/* Sync Tx Queue Address Offset */
	int	XaQOff;		/* Async Tx Queue Address Offset */
};
static struct s_QOffTab QOffTab[] = {
	{Q_R1, Q_XS1, Q_XA1}, {Q_R2, Q_XS2, Q_XA2}
};


/******************************************************************************
 *
 *	SkGePollRxD() - Enable/Disable Descriptor Polling of RxD Ring
 *
 * Description:
 *	Enable or disable the descriptor polling the receive descriptor
 *	ring (RxD) of port 'port'.
 *	The new configuration is *not* saved over any SkGeStopPort() and
 *	SkGeInitPort() calls.
 *
 * Returns:
 *	nothing
 */
void SkGePollRxD(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC,		/* IO context */
int		Port,		/* Port Index (MAC_1 + n) */
SK_BOOL PollRxD)	/* SK_TRUE (enable pol.), SK_FALSE (disable pol.) */
{
	SK_GEPORT *pPrt;

	pPrt = &pAC->GIni.GP[Port];

	if (PollRxD) {
		SK_OUT32(IoC, Q_ADDR(pPrt->PRxQOff, Q_CSR), CSR_ENA_POL);
	}
	else {
		SK_OUT32(IoC, Q_ADDR(pPrt->PRxQOff, Q_CSR), CSR_DIS_POL);
	}
}	/* SkGePollRxD */


/******************************************************************************
 *
 *	SkGePollTxD() - Enable/Disable Descriptor Polling of TxD Rings
 *
 * Description:
 *	Enable or disable the descriptor polling the transmit descriptor
 *	ring(s) (RxD) of port 'port'.
 *	The new configuration is *not* saved over any SkGeStopPort() and
 *	SkGeInitPort() calls.
 *
 * Returns:
 *	nothing
 */
void SkGePollTxD(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC,		/* IO context */
int		Port,		/* Port Index (MAC_1 + n) */
SK_BOOL PollTxD)	/* SK_TRUE (enable pol.), SK_FALSE (disable pol.) */
{
	SK_GEPORT *pPrt;
	SK_U32	DWord;

	pPrt = &pAC->GIni.GP[Port];

	if (PollTxD) {
		DWord = CSR_ENA_POL;
	}
	else {
		DWord = CSR_DIS_POL;
	}

	if (pPrt->PXSQSize != 0) {
		SK_OUT32(IoC, Q_ADDR(pPrt->PXsQOff, Q_CSR), DWord);
	}
	if (pPrt->PXAQSize != 0) {
		SK_OUT32(IoC, Q_ADDR(pPrt->PXaQOff, Q_CSR), DWord);
	}
}	/* SkGePollTxD */


/******************************************************************************
 *
 *	SkGeYellowLED() - Switch the yellow LED on or off.
 *
 * Description:
 *	Switch the yellow LED on or off.
 *
 * Note:
 *	This function may be called any time after SkGeInit(Level 1).
 *
 * Returns:
 *	nothing
 */
void	SkGeYellowLED(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC,		/* IO context */
int		State)		/* yellow LED state, 0 = OFF, 0 != ON */
{
	if (State == 0) {
		/* Switch yellow LED OFF */
		SK_OUT8(IoC, B0_LED, LED_STAT_OFF);
	}
	else {
		/* Switch yellow LED ON */
		SK_OUT8(IoC, B0_LED, LED_STAT_ON);
	}
}	/* SkGeYellowLED */


/******************************************************************************
 *
 *	SkGeXmitLED() - Modify the Operational Mode of a transmission LED.
 *
 * Description:
 *	The Rx or Tx LED which is specified by 'Led' will be
 *	enabled, disabled or switched on in test mode.
 *
 * Note:
 *	'Led' must contain the address offset of the LEDs INI register.
 *
 * Usage:
 *	SkGeXmitLED(pAC, IoC, MR_ADDR(Port, TX_LED_INI), SK_LED_ENA);
 *
 * Returns:
 *	nothing
 */
void	SkGeXmitLED(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC,		/* IO context */
int		Led,		/* offset to the LED Init Value register */
int		Mode)		/* Mode may be SK_LED_DIS, SK_LED_ENA, SK_LED_TST */
{
	SK_U32	LedIni;

	switch (Mode) {
	case SK_LED_ENA:
		LedIni = SK_XMIT_DUR * (SK_U32)pAC->GIni.GIHstClkFact / 100;
		SK_OUT32(IoC, Led + XMIT_LED_INI, LedIni);
		SK_OUT8(IoC, Led + XMIT_LED_CTRL, LED_START);
		break;
	case SK_LED_TST:
		SK_OUT8(IoC, Led + XMIT_LED_TST, LED_T_ON);
		SK_OUT32(IoC, Led + XMIT_LED_CNT, 100);
		SK_OUT8(IoC, Led + XMIT_LED_CTRL, LED_START);
		break;
	case SK_LED_DIS:
	default:
		/*
		 * Do NOT stop the LED Timer here. The LED might be
		 * in on state. But it needs to go off.
		 */
		SK_OUT32(IoC, Led + XMIT_LED_CNT, 0);
		SK_OUT8(IoC, Led + XMIT_LED_TST, LED_T_OFF);
		break;
	}
			
}	/* SkGeXmitLED */


/******************************************************************************
 *
 *	DoCalcAddr() - Calculates the start and the end address of a queue.
 *
 * Description:
 *	This function calculates the start- end the end address
 *	of a queue. Afterwards the 'StartVal' is incremented to the
 *	next start position.
 *	If the port is already initialized the calculated values
 *	will be checked against the configured values and an
 *	error will be returned, if they are not equal.
 *	If the port is not initialized the values will be written to
 *	*StartAdr and *EndAddr.
 *
 * Returns:
 *	0:	success
 *	1:	configuration error
 */
static int DoCalcAddr(
SK_AC		*pAC, 			/* adapter context */
SK_GEPORT	*pPrt,			/* port index */
int			QuSize,			/* size of the queue to configure in kB */
SK_U32		*StartVal,		/* start value for address calculation */
SK_U32		*QuStartAddr,	/* start addr to calculate */
SK_U32		*QuEndAddr)		/* end address to calculate */
{
	SK_U32	EndVal;
	SK_U32	NextStart;
	int	Rtv;

	Rtv = 0;
	if (QuSize == 0) {
		EndVal = *StartVal;
		NextStart = EndVal;
	}
	else {
		EndVal = *StartVal + ((SK_U32)QuSize * 1024) - 1;
		NextStart = EndVal + 1;
	}

	if (pPrt->PState >= SK_PRT_INIT) {
		if (*StartVal != *QuStartAddr || EndVal != *QuEndAddr) {
			Rtv = 1;
		}
	}
	else {
		*QuStartAddr = *StartVal;
		*QuEndAddr = EndVal;
	}

	*StartVal = NextStart;
	return (Rtv);
}	/* DoCalcAddr */


/******************************************************************************
 *
 *	SkGeCheckQSize() - Checks the Adapters Queue Size Configuration
 *
 * Description:
 *	This function verifies the Queue Size Configuration specified
 *	in the variabels PRxQSize, PXSQSize, and PXAQSize of all
 *	used ports.
 *	This requirements must be fullfilled to have a valid configuration:
 *		- The size of all queues must not exceed GIRamSize.
 *		- The queue sizes must be specified in units of 8 kB.
 *		- The size of rx queues of available ports must not be
 *		  smaller than 16kB.
 *		- The RAM start and end addresses must not be changed
 *		  for ports which are already initialized.
 *	Furthermore SkGeCheckQSize() defines the Start and End
 *	Addresses of all ports and stores them into the HWAC port
 *	structure.
 *
 * Returns:
 *	0:	Queue Size Configuration valid
 *	1:	Queue Size Configuration invalid
 */
static int SkGeCheckQSize(
SK_AC	 *pAC,		/* adapter context */
int		 Port)		/* port index */
{
	SK_GEPORT *pPrt;
	int	UsedMem;
	int	i;
	int	Rtv;
	int	Rtv2;
	SK_U32	StartAddr;

	UsedMem = 0;
	Rtv = 0;
	for (i = 0; i < pAC->GIni.GIMacsFound; i++) {
		pPrt = &pAC->GIni.GP[i];

		if (( pPrt->PRxQSize & QZ_UNITS) ||
			(pPrt->PXSQSize & QZ_UNITS) ||
			(pPrt->PXAQSize & QZ_UNITS)) {

			SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E012, SKERR_HWI_E012MSG);
			Rtv = 1;
			goto CheckQSizeEnd;
		}

		UsedMem += pPrt->PRxQSize + pPrt->PXSQSize + pPrt->PXAQSize;

		if (i == Port && pPrt->PRxQSize < SK_MIN_RXQ_SIZE) {
			SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E011, SKERR_HWI_E011MSG);
			Rtv = 1;
			goto CheckQSizeEnd;
		}
	}
	if (UsedMem > pAC->GIni.GIRamSize) {
		SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E012, SKERR_HWI_E012MSG);
		Rtv = 1;
		goto CheckQSizeEnd;
	}

	/* Now start address calculation */
	StartAddr = pAC->GIni.GIRamOffs;
	for (i = 0; i < pAC->GIni.GIMacsFound; i++) {
		pPrt = &pAC->GIni.GP[i];

		/* Calculate/Check values for the receive queue */
		Rtv2 = DoCalcAddr(pAC, pPrt, pPrt->PRxQSize, &StartAddr,
			&pPrt->PRxQRamStart, &pPrt->PRxQRamEnd);
		Rtv |= Rtv2;

		/* Calculate/Check values for the synchronous tx queue */
		Rtv2 = DoCalcAddr(pAC, pPrt, pPrt->PXSQSize, &StartAddr,
			&pPrt->PXsQRamStart, &pPrt->PXsQRamEnd);
		Rtv |= Rtv2;

		/* Calculate/Check values for the asynchronous tx queue */
		Rtv2 = DoCalcAddr(pAC, pPrt, pPrt->PXAQSize, &StartAddr,
			&pPrt->PXaQRamStart, &pPrt->PXaQRamEnd);
		Rtv |= Rtv2;

		if (Rtv) {
			SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E013, SKERR_HWI_E013MSG);
			break;
		}
	}

CheckQSizeEnd:
	return (Rtv);
}	/* SkGeCheckQSize */


/******************************************************************************
 *
 *	SkGeInitMacArb() - Initialize the MAC Arbiter
 *
 * Description:
 *	This function initializes the MAC Arbiter.
 *	It must not be called if there is still an
 *	initilaized or active port.
 *
 * Returns:
 *	nothing:
 */
static void SkGeInitMacArb(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC)		/* IO context */
{
	/* release local reset */
	SK_OUT16(IoC, B3_MA_TO_CTRL, MA_RST_CLR);

	/* configure timeout values */
	SK_OUT8(IoC, B3_MA_TOINI_RX1, SK_MAC_TO_53);
	SK_OUT8(IoC, B3_MA_TOINI_RX2, SK_MAC_TO_53);
	SK_OUT8(IoC, B3_MA_TOINI_TX1, SK_MAC_TO_53);
	SK_OUT8(IoC, B3_MA_TOINI_TX2, SK_MAC_TO_53);

	SK_OUT8(IoC, B3_MA_RCINI_RX1, 0);
	SK_OUT8(IoC, B3_MA_RCINI_RX2, 0);
	SK_OUT8(IoC, B3_MA_RCINI_TX1, 0);
	SK_OUT8(IoC, B3_MA_RCINI_TX2, 0);

	/* recovery values are needed for XMAC II Rev. B2 only */
	/* Fast Output Enable Mode was intended to use with Rev. B2, but now? */

	/*
	 * There is not start or enable buttom to push, therefore
	 * the MAC arbiter is configured and enabled now.
	 */
}	/* SkGeInitMacArb */


/******************************************************************************
 *
 *	SkGeInitPktArb() - Initialize the Packet Arbiter
 *
 * Description:
 *	This function initializes the Packet Arbiter.
 *	It must not be called if there is still an
 *	initilaized or active port.
 *
 * Returns:
 *	nothing:
 */
static void SkGeInitPktArb(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC)		/* IO context */
{
	/* release local reset */
	SK_OUT16(IoC, B3_PA_CTRL, PA_RST_CLR);

	/* configure timeout values */
	SK_OUT16(IoC, B3_PA_TOINI_RX1, SK_PKT_TO_MAX);
	SK_OUT16(IoC, B3_PA_TOINI_RX2, SK_PKT_TO_MAX);
	SK_OUT16(IoC, B3_PA_TOINI_TX1, SK_PKT_TO_MAX);
	SK_OUT16(IoC, B3_PA_TOINI_TX2, SK_PKT_TO_MAX);

	if (pAC->GIni.GIPortUsage != SK_JUMBO_LINK) {
		if (pAC->GIni.GIMacsFound == 1) {
			SK_OUT16(IoC, B3_PA_CTRL, PA_ENA_TO_TX1);
		}
		else {
			SK_OUT16(IoC, B3_PA_CTRL,(PA_ENA_TO_TX1 | PA_ENA_TO_TX2));
		}
	}
}	/* SkGeInitPktArb */


/******************************************************************************
 *
 *	SkGeInitMacFifo() - Initialize the MAC FIFOs
 *
 * Description:
 *	Initialize all MAC FIFOs of the specified port
 *
 * Returns:
 *	nothing
 */
static void SkGeInitMacFifo(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC,		/* IO context */
int		Port)		/* Port Index (MAC_1 + n) */
{
	/*
	 * For each FIFO:
	 *	- release local reset
	 *	- use default value for MAC FIFO size
	 *	- setup defaults for the control register
	 *	- enable the FIFO
	 */
	/* Configure RX MAC FIFO */
	SK_OUT8(IoC, MR_ADDR(Port, RX_MFF_CTRL2), MFF_RST_CLR);
	SK_OUT16(IoC, MR_ADDR(Port, RX_MFF_CTRL1), MFF_RX_CTRL_DEF);
	SK_OUT8(IoC, MR_ADDR(Port, RX_MFF_CTRL2), MFF_ENA_OP_MD);

	/* Configure TX MAC FIFO */
	SK_OUT8(IoC, MR_ADDR(Port, TX_MFF_CTRL2), MFF_RST_CLR);
	SK_OUT16(IoC, MR_ADDR(Port, TX_MFF_CTRL1), MFF_TX_CTRL_DEF);
	SK_OUT8(IoC, MR_ADDR(Port, TX_MFF_CTRL2), MFF_ENA_OP_MD);

	/* Enable frame flushing if jumbo frames used */
	if (pAC->GIni.GIPortUsage == SK_JUMBO_LINK) {
		SK_OUT16(IoC, MR_ADDR(Port, RX_MFF_CTRL1), MFF_ENA_FLUSH);
	}
}	/* SkGeInitMacFifo */


/******************************************************************************
 *
 *	SkGeLoadLnkSyncCnt() - Load the Link Sync Counter and starts counting
 *
 * Description:
 *	This function starts the Link Sync Counter of the specified
 *	port and enables the generation of an Link Sync IRQ.
 *	The Link Sync Counter may be used to detect an active link,
 *	if autonegotiation is not used.
 *
 * Note:
 *	o To ensure receiving the Link Sync Event the LinkSyncCounter
 *	  should be initialized BEFORE clearing the XMACs reset!
 *	o Enable IS_LNK_SYNC_M1 and IS_LNK_SYNC_M2 after calling this
 *	  function.
 *
 * Retruns:
 *	nothing
 */
void SkGeLoadLnkSyncCnt(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC,		/* IO context */
int		Port,		/* Port Index (MAC_1 + n) */
SK_U32	CntVal)		/* Counter value */
{
	SK_U32	OrgIMsk;
	SK_U32	NewIMsk;
	SK_U32	ISrc;
	SK_BOOL	IrqPend;

	/* stop counter */
	SK_OUT8(IoC, MR_ADDR(Port, LNK_SYNC_CTRL), LED_STOP);

	IrqPend = SK_FALSE;
	SK_IN32(IoC, B0_ISRC, &ISrc);
	SK_IN32(IoC, B0_IMSK, &OrgIMsk);
	if (Port == MAC_1) {
		NewIMsk = OrgIMsk & ~IS_LNK_SYNC_M1;
		if (ISrc & IS_LNK_SYNC_M1) {
			IrqPend = SK_TRUE;
		}
	}
	else {
		NewIMsk = OrgIMsk & ~IS_LNK_SYNC_M2;
		if (ISrc & IS_LNK_SYNC_M2) {
			IrqPend = SK_TRUE;
		}
	}
	if (!IrqPend) {
		SK_OUT32(IoC, B0_IMSK, NewIMsk);
	}

	/* load counter */
	SK_OUT32(IoC, MR_ADDR(Port, LNK_SYNC_INI), CntVal);

	/* start counter */
	SK_OUT8(IoC, MR_ADDR(Port, LNK_SYNC_CTRL), LED_START);

	if (!IrqPend) {
		/* clear the unexpected IRQ, and restore the interrupt mask */
		SK_OUT8(IoC, MR_ADDR(Port, LNK_SYNC_CTRL), LED_CLR_IRQ);
		SK_OUT32(IoC, B0_IMSK, OrgIMsk);
	}
}	/* SkGeLoadLnkSyncCnt*/


/******************************************************************************
 *
 *	SkGeCfgSync() - Configure synchronous bandwidth for this port.
 *
 * Description:
 *	This function may be used to configure synchronous bandwidth
 *	to the specified port. This may be done any time after
 *	initializing the port. The configuration values are NOT saved
 *	in the HWAC port structure and will be overwritten any
 *	time when stopping and starting the port.
 *	Any values for the synchronous configuration will be ignored
 *	if the size of the synchronous queue is zero!
 *
 *	The default configuration for the synchronous service is
 *	TXA_ENA_FSYNC. This means if the size of
 *	the synchronous queue is unequal zero but no specific
 *	synchronous bandwidth is configured, the synchronous queue
 *	will always have the 'unlimitted' transmit priority!
 *
 *	This mode will be restored if the synchronous bandwidth is
 *	deallocated ('IntTime' = 0 and 'LimCount' = 0).
 *
 * Returns:
 *	0:	success
 *	1:	paramter configuration error
 *	2:	try to configure quality of service although no
 *		synchronous queue is configured
 */
int SkGeCfgSync(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC,		/* IO context */
int		Port,		/* Port Index (MAC_1 + n) */
SK_U32	IntTime,	/* Interval Timer Value in units of 8ns */
SK_U32	LimCount,	/* Number of bytes to transfer during IntTime */
int		SyncMode)	/* Sync Mode: TXA_ENA_ALLOC | TXA_DIS_ALLOC | 0 */
{
	int Rtv;

	Rtv = 0;

	/* check the parameters */
	if (LimCount > IntTime ||
		(LimCount == 0 && IntTime != 0) ||
		(LimCount !=0 && IntTime == 0)) {

		SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E010, SKERR_HWI_E010MSG);
		Rtv = 1;
		goto CfgSyncEnd;
	}
	if (pAC->GIni.GP[Port].PXSQSize != 0) {
		/* calculate register values */
		IntTime = (IntTime / 2) * pAC->GIni.GIHstClkFact / 100;
		LimCount = LimCount / 8;
		if (IntTime > TXA_MAX_VAL || LimCount > TXA_MAX_VAL) {
			SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E010, SKERR_HWI_E010MSG);
			Rtv = 1;
			goto CfgSyncEnd;
		}

		/*
		 * - Enable 'Force Sync' to ensure the synchronous queue
		 *   has the priority while configuring the new values.
		 * - Also 'disable alloc' to ensure the settings complies
		 *   to the SyncMode parameter.
		 * - Disable 'Rate Control' to configure the new values.
		 * - write IntTime and Limcount
		 * - start 'Rate Control' and disable 'Force Sync'
		 *   if Interval Timer or Limit Counter not zero.
		 */
		SK_OUT8(IoC, MR_ADDR(Port, TXA_CTRL),
			TXA_ENA_FSYNC | TXA_DIS_ALLOC | TXA_STOP_RC);
		SK_OUT32(IoC, MR_ADDR(Port, TXA_ITI_INI), IntTime);
		SK_OUT32(IoC, MR_ADDR(Port, TXA_LIM_INI), LimCount);
		SK_OUT8(IoC, MR_ADDR(Port, TXA_CTRL),
			(SyncMode & (TXA_ENA_ALLOC|TXA_DIS_ALLOC)));
		if (IntTime != 0 || LimCount != 0) {
			SK_OUT8(IoC, MR_ADDR(Port, TXA_CTRL),
				TXA_DIS_FSYNC|TXA_START_RC);
		}
	}
	else {
		SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E009, SKERR_HWI_E009MSG);
		Rtv = 2;
	}

CfgSyncEnd:
	return (Rtv);
}	/* SkGeCfgSync */


/******************************************************************************
 *
 *	DoInitRamQueue() - Initilaize the RAM Buffer Address of a single Queue
 *
 * Desccription:
 *	If the queue is used, enable and initilaize it.
 *	Make sure the queue is still reset, if it is not used.
 *
 * Returns:
 *	nothing
 */
static void DoInitRamQueue(
SK_AC	*pAC,			/* adapter context */
SK_IOC	IoC,			/* IO context */
int		QuIoOffs,		/* Queue IO Address Offset */
SK_U32	QuStartAddr,	/* Queue Start Address */
SK_U32	QuEndAddr,		/* Queue End Address */
int		QuType)			/* Queue Type (SK_RX_SRAM_Q|SK_RX_BRAM_Q|SK_TX_RAM_Q) */
{
	SK_U32	RxUpThresVal;
	SK_U32	RxLoThresVal;

	if (QuStartAddr != QuEndAddr) {
		/* calculate thresholds, assume we have a big Rx queue */
		RxUpThresVal = (QuEndAddr + 1 - QuStartAddr - SK_RB_ULPP) / 8;
		RxLoThresVal = (QuEndAddr + 1 - QuStartAddr - SK_RB_LLPP_B)/8;

		/* build HW address format */
		QuStartAddr = QuStartAddr / 8;
		QuEndAddr = QuEndAddr / 8;

		/* release local reset */
		SK_OUT8(IoC, RB_ADDR(QuIoOffs, RB_CTRL), RB_RST_CLR);

		/* configure addresses */
		SK_OUT32(IoC, RB_ADDR(QuIoOffs, RB_START), QuStartAddr);
		SK_OUT32(IoC, RB_ADDR(QuIoOffs, RB_END), QuEndAddr);
		SK_OUT32(IoC, RB_ADDR(QuIoOffs, RB_WP), QuStartAddr);
		SK_OUT32(IoC, RB_ADDR(QuIoOffs, RB_RP), QuStartAddr);

		switch (QuType) {
		case SK_RX_SRAM_Q:
			/* configure threshold for small Rx Queue */
			RxLoThresVal += (SK_RB_LLPP_B - SK_RB_LLPP_S) / 8;

			/* continue with SK_RX_BRAM_Q */
		case SK_RX_BRAM_Q:
			/* write threshold for Rx Queue */

			SK_OUT32(IoC, RB_ADDR(QuIoOffs, RB_RX_UTPP), RxUpThresVal);
			SK_OUT32(IoC, RB_ADDR(QuIoOffs,RB_RX_LTPP), RxLoThresVal);

			/* the high priority threshold not used */
			break;
		case SK_TX_RAM_Q:
			/*
			 * Do NOT use Store and forward under normal
			 * operation due to performance optimization.
			 * But if Jumbo frames are configured we NEED
			 * the store and forward of the RAM buffer.
			 */
			if (pAC->GIni.GIPortUsage == SK_JUMBO_LINK) {
				/*
				 * enable Store & Forward Mode for the
				 * Tx Side
				 */
				SK_OUT8(IoC, RB_ADDR(QuIoOffs, RB_CTRL), RB_ENA_STFWD);
			}
			break;
		}

		/* set queue operational */
		SK_OUT8(IoC, RB_ADDR(QuIoOffs, RB_CTRL), RB_ENA_OP_MD);
	}
	else {
		/* ensure the queue is still disabled */
		SK_OUT8(IoC, RB_ADDR(QuIoOffs, RB_CTRL), RB_RST_SET);
	}
}	/* DoInitRamQueue*/


/******************************************************************************
 *
 *	SkGeInitRamBufs() - Initialize the RAM Buffer Queues
 *
 * Description:
 *	Initialize all RAM Buffer Queues of the specified port
 *
 * Returns:
 *	nothing
 */
static void SkGeInitRamBufs(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC,		/* IO context */
int		Port)		/* Port Index (MAC_1 + n) */
{
	SK_GEPORT *pPrt;
	int RxQType;

	pPrt = &pAC->GIni.GP[Port];

	if (pPrt->PRxQSize == SK_MIN_RXQ_SIZE) {
		RxQType = SK_RX_SRAM_Q; 	/* small Rx Queue */
	}
	else {
		RxQType = SK_RX_BRAM_Q;		/* big Rx Queue */
	}

	DoInitRamQueue(pAC, IoC, pPrt->PRxQOff, pPrt->PRxQRamStart,
		pPrt->PRxQRamEnd, RxQType);
	DoInitRamQueue(pAC, IoC, pPrt->PXsQOff, pPrt->PXsQRamStart,
		pPrt->PXsQRamEnd, SK_TX_RAM_Q);
	DoInitRamQueue(pAC, IoC, pPrt->PXaQOff, pPrt->PXaQRamStart,
		pPrt->PXaQRamEnd, SK_TX_RAM_Q);
}	/* SkGeInitRamBufs */


/******************************************************************************
 *
 *	SkGeInitRamIface() - Initialize the RAM Interface
 *
 * Description:
 *	This function initializes the Adapbers RAM Interface.
 *
 * Note:
 *	This function is used in the diagnostics.
 *
 * Returns:
 *	nothing
 */
void SkGeInitRamIface(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC)		/* IO context */
{
	/* release local reset */
	SK_OUT16(IoC, B3_RI_CTRL, RI_RST_CLR);

	/* configure timeout values */
	SK_OUT8(IoC, B3_RI_WTO_R1, SK_RI_TO_53);
	SK_OUT8(IoC, B3_RI_WTO_XA1, SK_RI_TO_53);
	SK_OUT8(IoC, B3_RI_WTO_XS1, SK_RI_TO_53);
	SK_OUT8(IoC, B3_RI_RTO_R1, SK_RI_TO_53);
	SK_OUT8(IoC, B3_RI_RTO_XA1, SK_RI_TO_53);
	SK_OUT8(IoC, B3_RI_RTO_XS1, SK_RI_TO_53);
	SK_OUT8(IoC, B3_RI_WTO_R2, SK_RI_TO_53);
	SK_OUT8(IoC, B3_RI_WTO_XA2, SK_RI_TO_53);
	SK_OUT8(IoC, B3_RI_WTO_XS2, SK_RI_TO_53);
	SK_OUT8(IoC, B3_RI_RTO_R2, SK_RI_TO_53);
	SK_OUT8(IoC, B3_RI_RTO_XA2, SK_RI_TO_53);
	SK_OUT8(IoC, B3_RI_RTO_XS2, SK_RI_TO_53);
}	/* SkGeInitRamIface */


/******************************************************************************
 *
 *	SkGeInitBmu() - Initialize the BMU state machines
 *
 * Description:
 *	Initialize all BMU state machines of the specified port
 *
 * Returns:
 *	nothing
 */
static void SkGeInitBmu(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC,		/* IO context */
int		Port)		/* Port Index (MAC_1 + n) */
{
	SK_GEPORT *pPrt;

	pPrt = &pAC->GIni.GP[Port];

	/* Rx Queue: Release all local resets and set the watermark */
	SK_OUT32(IoC, Q_ADDR(pPrt->PRxQOff, Q_CSR), CSR_CLR_RESET);
	SK_OUT32(IoC, Q_ADDR(pPrt->PRxQOff, Q_F), SK_BMU_RX_WM);

	/*
	 * Tx Queue: Release all local resets if the queue is used!
	 * 		set watermark
	 */
	if (pPrt->PXSQSize != 0) {
		SK_OUT32(IoC, Q_ADDR(pPrt->PXsQOff, Q_CSR), CSR_CLR_RESET);
		SK_OUT32(IoC, Q_ADDR(pPrt->PXsQOff, Q_F), SK_BMU_TX_WM);
	}
	if (pPrt->PXAQSize != 0) {
		SK_OUT32(IoC, Q_ADDR(pPrt->PXaQOff, Q_CSR), CSR_CLR_RESET);
		SK_OUT32(IoC, Q_ADDR(pPrt->PXaQOff, Q_F), SK_BMU_TX_WM);
	}
	/*
	 * Do NOT enable the descriptor poll timers here, because
	 * the descriptor addresses are not specified yet.
	 */
}	/* SkGeInitBmu */


/******************************************************************************
 *
 *	TestStopBit() -	Test the stop bit of the queue
 *
 * Description:
 *	Stopping a queue is not as simple as it seems to be.
 *	If descriptor polling is enabled, it may happen
 *	that RX/TX stop is done and SV idle is NOT set.
 *	In this case we have to issue another stop command.
 *
 * Retruns:
 *	The queues control status register
 */
static SK_U32 TestStopBit(
SK_AC	*pAC,		/* Adapter Context */
SK_IOC	IoC,		/* IO Context */
int		QuIoOffs)	/* Queue IO Address Offset */
{
	SK_U32	QuCsr;	/* CSR contents */

	SK_IN32(IoC, Q_ADDR(QuIoOffs, Q_CSR), &QuCsr);
	if ((QuCsr & (CSR_STOP|CSR_SV_IDLE)) == 0) {
		SK_OUT32(IoC, Q_ADDR(QuIoOffs, Q_CSR), CSR_STOP);
		SK_IN32(IoC, Q_ADDR(QuIoOffs, Q_CSR), &QuCsr);
	}
	return (QuCsr);
}	/* TestStopBit*/


void	SkGeStopPort(
SK_AC	*pAC,	/* adapter context */
SK_IOC	IoC,	/* I/O context */
int		Port,	/* port to stop (MAC_1 + n) */
int		Dir,	/* Direction to Stop (SK_STOP_RX, SK_STOP_TX, SK_STOP_ALL) */
int		RstMode)/* Reset Mode (SK_SOFT_RST, SK_HARD_RST) */
{
#ifndef	SK_DIAG
	SK_EVPARA Para;
#endif	/* !SK_DIAG */
	SK_GEPORT *pPrt;
	SK_U32	DWord;
	SK_U16	Word;
	SK_U32	XsCsr;
	SK_U32	XaCsr;
	int		i;
	SK_BOOL	AllPortsDis;
	SK_U64	ToutStart;
	int		ToutCnt;

	pPrt = &pAC->GIni.GP[Port];

	if (Dir & SK_STOP_TX) {
		/* disable the XMACs receiver and transmitter */
		XM_IN16(IoC, Port, XM_MMU_CMD, &Word);
		XM_OUT16(IoC, Port, XM_MMU_CMD, Word & ~(XM_MMU_ENA_RX | XM_MMU_ENA_TX));

		/* dummy read to ensure writing */
		XM_IN16(IoC, Port, XM_MMU_CMD, &Word);

		/* stop both transmit queues */
		/*
		 * If the BMU is in the reset state CSR_STOP will terminate
		 * immediately.
		 */
		SK_OUT32(IoC, Q_ADDR(pPrt->PXsQOff, Q_CSR), CSR_STOP);
		SK_OUT32(IoC, Q_ADDR(pPrt->PXaQOff, Q_CSR), CSR_STOP);

		ToutStart = SkOsGetTime(pAC);
		ToutCnt = 0;
		do {
			/*
			 * Clear packet arbiter timeout to make sure
			 * this loop will terminate.
			 */
			if (Port == MAC_1) {
				Word = PA_CLR_TO_TX1;
			}
			else {
				Word = PA_CLR_TO_TX2;
			}
			SK_OUT16(IoC, B3_PA_CTRL, Word);

			/*
			 * If the transfer stucks at the XMAC the STOP command will not
			 * terminate if we don't flush the XMAC's transmit FIFO!
			 */
			XM_IN32(IoC, Port, XM_MODE, &DWord);
			DWord |= XM_MD_FTF;
			XM_OUT32(IoC, Port, XM_MODE, DWord);

			XsCsr = TestStopBit(pAC, IoC, pPrt->PXsQOff);
			XaCsr = TestStopBit(pAC, IoC, pPrt->PXaQOff);

			if (SkOsGetTime(pAC) - ToutStart > (SK_TICKS_PER_SEC / 18)) {
				/*
				 * Timeout of 1/18 second reached.
				 * This needs to be checked at 1/18 sec only.
				 */
				ToutCnt++;
				switch (ToutCnt) {
				case 1:
					ToutStart = SkOsGetTime(pAC);
					if (XsCsr & CSR_STOP) {
						SK_OUT32(IoC, Q_ADDR(pPrt->PXsQOff, Q_CSR), CSR_START);
					}
					if (XaCsr & CSR_STOP) {
						SK_OUT32(IoC, Q_ADDR(pPrt->PXaQOff, Q_CSR), CSR_START);
					}
					break;
				case 2:
				default:

					/* Fatal Error, Loop aborted */
					/* Create an Error Log Entry */
					SK_ERR_LOG(
						pAC,
						SK_ERRCL_HW,
						SKERR_HWI_E018,
						SKERR_HWI_E018MSG);
#ifndef SK_DIAG
					Para.Para64 = Port;
					SkEventQueue(pAC, SKGE_DRV, SK_DRV_PORT_FAIL, Para);
#endif	/* !SK_DIAG */
					return;
				}
			}

			/*
			 * Because of the ASIC problem report entry from 21.08.1998 it is
			 * required to wait until CSR_STOP is reset and CSR_SV_IDLE is set.
			 */
		} while ((XsCsr & (CSR_STOP | CSR_SV_IDLE)) != CSR_SV_IDLE ||
			 (XaCsr & (CSR_STOP | CSR_SV_IDLE)) != CSR_SV_IDLE);

		/* reset the XMAC depending on the RstMode */
		if (RstMode == SK_SOFT_RST) {
			SkXmSoftRst(pAC, IoC, Port);
		}
		else {
			SkXmHardRst(pAC, IoC, Port);
		}

 		/*
		 * Stop Interval Timer and Limit Counter of Tx Arbiter,
 		 * also disable Force Sync bit and Enable Alloc bit.
		 */
		SK_OUT8(IoC, MR_ADDR(Port, TXA_CTRL),
			TXA_DIS_FSYNC | TXA_DIS_ALLOC | TXA_STOP_RC);
		SK_OUT32(IoC, MR_ADDR(Port, TXA_ITI_INI), 0x00000000L);
		SK_OUT32(IoC, MR_ADDR(Port, TXA_LIM_INI), 0x00000000L);

		/*
		 * perform a local reset of the port's tx path
		 *	- reset the PCI FIFO of the async tx queue
		 *	- reset the PCI FIFO of the sync tx queue
		 *	- reset the RAM Buffer async tx queue
		 *	- reset the RAM Butter sync tx queue
		 *	- reset the MAC Tx FIFO
		 */
		SK_OUT32(IoC, Q_ADDR(pPrt->PXaQOff, Q_CSR), CSR_SET_RESET);
		SK_OUT32(IoC, Q_ADDR(pPrt->PXsQOff, Q_CSR), CSR_SET_RESET);
		SK_OUT8(IoC, RB_ADDR(pPrt->PXaQOff, RB_CTRL), RB_RST_SET);
		SK_OUT8(IoC, RB_ADDR(pPrt->PXsQOff, RB_CTRL), RB_RST_SET);
		/* Note: MFF_RST_SET does NOT reset the XMAC! */
		SK_OUT8(IoC, MR_ADDR(Port, TX_MFF_CTRL2), MFF_RST_SET);

		/* switch Link and Tx LED off, stop the LED counters */
		/* Link LED is switched off by the RLMT and the Diag itself */
		SkGeXmitLED(pAC, IoC, MR_ADDR(Port, TX_LED_INI), SK_LED_DIS);
	}

	if (Dir & SK_STOP_RX) {
		/*
		 * The RX Stop Command will not terminate if no buffers
		 * are queued in the RxD ring. But it will always reach
		 * the Idle state. Therefore we can use this feature to
		 * stop the transfer of received packets.
		 */
		/* stop the port's receive queue */
		SK_OUT32(IoC, Q_ADDR(pPrt->PRxQOff, Q_CSR), CSR_STOP);
		i = 100;
		do {
			/*
			 * Clear packet arbiter timeout to make sure
			 * this loop will terminate
			 */
			if (Port == MAC_1) {
				Word = PA_CLR_TO_RX1;
			}
			else {
				Word = PA_CLR_TO_RX2;
			}
			SK_OUT16(IoC, B3_PA_CTRL, Word);

			DWord = TestStopBit(pAC, IoC, pPrt->PRxQOff);
			if (i != 0) {
				i--;
			}

			/* finish if CSR_STOP is done or CSR_SV_IDLE is true and i==0 */
			/*
			 * because of the ASIC problem report entry from 21.08.98
			 * it is required to wait until CSR_STOP is reset and
			 * CSR_SV_IDLE is set.
			 */
		} while ((DWord & (CSR_STOP | CSR_SV_IDLE)) != CSR_SV_IDLE &&
			((DWord & CSR_SV_IDLE) == 0 || i != 0));

		/* The path data transfer activity is fully stopped now. */

		/*
		 * perform a local reset of the port's rx path
		 *	- reset the PCI FIFO of the rx queue
		 *	- reset the RAM Buffer receive queue
		 *	- reset the MAC Rx FIFO
		 */
		SK_OUT32(IoC, Q_ADDR(pPrt->PRxQOff, Q_CSR), CSR_SET_RESET);
		SK_OUT8(IoC, RB_ADDR(pPrt->PRxQOff, RB_CTRL), RB_RST_SET);
		SK_OUT8(IoC, MR_ADDR(Port, RX_MFF_CTRL2), MFF_RST_SET);

		/* switch Rx LED off, stop the LED counter */
		SkGeXmitLED(pAC, IoC, MR_ADDR(Port, RX_LED_INI), SK_LED_DIS);

	}

 	/*
	 * If all ports are stopped reset the RAM Interface.
	 */
	for (i = 0, AllPortsDis = SK_TRUE; i < pAC->GIni.GIMacsFound; i++) {
		if (pAC->GIni.GP[i].PState != SK_PRT_RESET &&
			pAC->GIni.GP[i].PState != SK_PRT_STOP) {

			AllPortsDis = SK_FALSE;
			break;
		}
	}
	if (AllPortsDis) {
		pAC->GIni.GIAnyPortAct = SK_FALSE;
	}
}	/* SkGeStopPort */


/******************************************************************************
 *
 *	SkGeInit0() - Level 0 Initialization
 *
 * Description:
 *	- Initialize the BMU address offsets
 *
 * Returns:
 *	nothing
 */
static void SkGeInit0(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC)		/* IO context */
{
	int i;
	SK_GEPORT *pPrt;

	for (i = 0; i < SK_MAX_MACS; i++) {
		pPrt = &pAC->GIni.GP[i];
		pPrt->PState = SK_PRT_RESET;
		pPrt->PRxQOff = QOffTab[i].RxQOff;
		pPrt->PXsQOff = QOffTab[i].XsQOff;
		pPrt->PXaQOff = QOffTab[i].XaQOff;
		pPrt->PCheckPar = SK_FALSE;
		pPrt->PRxCmd = XM_RX_STRIP_FCS | XM_RX_LENERR_OK;
		pPrt->PIsave = 0;
		pPrt->PPrevShorts = 0;
		pPrt->PLinkResCt = 0;
		pPrt->PAutoNegTOCt = 0;
		pPrt->PPrevRx = 0;
		pPrt->PPrevFcs = 0;
		pPrt->PRxLim = SK_DEF_RX_WA_LIM;
		pPrt->PLinkMode = SK_LMODE_AUTOFULL;
		pPrt->PLinkModeConf = SK_LMODE_AUTOSENSE;
		pPrt->PFlowCtrlMode = SK_FLOW_MODE_SYM_OR_REM;
		pPrt->PLinkBroken = SK_TRUE; /* See WA code */
		pPrt->PLinkCap = (SK_LMODE_CAP_HALF | SK_LMODE_CAP_FULL |
				SK_LMODE_CAP_AUTOHALF | SK_LMODE_CAP_AUTOFULL);
		pPrt->PLinkModeStatus = SK_LMODE_STAT_UNKNOWN;
		pPrt->PFlowCtrlCap = SK_FLOW_MODE_SYM_OR_REM;
		pPrt->PFlowCtrlStatus = SK_FLOW_STAT_NONE;
		pPrt->PMSCap = (SK_MS_CAP_AUTO | SK_MS_CAP_MASTER | 
				SK_MS_CAP_SLAVE);
		pPrt->PMSMode = SK_MS_MODE_AUTO;
		pPrt->PMSStatus = SK_MS_STAT_UNSET;
		pPrt->PAutoNegFail = SK_FALSE;
		pPrt->PLipaAutoNeg = SK_LIPA_UNKNOWN;
		pPrt->PHWLinkUp = SK_FALSE;
	}

	pAC->GIni.GIPortUsage = SK_RED_LINK;
	pAC->GIni.GIAnyPortAct = SK_FALSE;
}	/* SkGeInit0*/

#ifdef SK_PCI_RESET

/******************************************************************************
 *
 *	SkGePciReset() - Reset PCI interface
 *
 * Description:
 *	o Read PCI configuration.
 *	o Change power state to 3.
 *	o Change power state to 0.
 *	o Restore PCI configuration.
 *
 * Returns:
 *	0:	Success.
 *	1:	Power state could not be changed to 3.
 */
static int SkGePciReset(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC)		/* IO context */
{
	int		i;
	SK_U16	PmCtlSts;
	SK_U32	Bp1;
	SK_U32	Bp2;
	SK_U16	PciCmd;
	SK_U8	Cls;
	SK_U8	Lat;
	SK_U8	ConfigSpace[PCI_CFG_SIZE];

	/*
	 * Note: Switching to D3 state is like a software reset.
	 *		 Switching from D3 to D0 is a hardware reset.
	 *		 We have to save and restore the configuration space.
	 */
	for (i = 0; i < PCI_CFG_SIZE; i++) {
		SkPciReadCfgDWord(pAC, i*4, &ConfigSpace[i]);
	}

	/* We know the RAM Interface Arbiter is enabled. */
	SkPciWriteCfgWord(pAC, PCI_PM_CTL_STS, PCI_PM_STATE_D3);
	SkPciReadCfgWord(pAC, PCI_PM_CTL_STS, &PmCtlSts);
	if ((PmCtlSts & PCI_PM_STATE) != PCI_PM_STATE_D3) {
		return (1);
	}

	/*
	 * Return to D0 state.
	 */
	SkPciWriteCfgWord(pAC, PCI_PM_CTL_STS, PCI_PM_STATE_D0);

	/* Check for D0 state. */
	SkPciReadCfgWord(pAC, PCI_PM_CTL_STS, &PmCtlSts);
	if ((PmCtlSts & PCI_PM_STATE) != PCI_PM_STATE_D0) {
		return (1);
	}

	/*
	 * Check PCI Config Registers.
	 */
	SkPciReadCfgWord(pAC, PCI_COMMAND, &PciCmd);
	SkPciReadCfgByte(pAC, PCI_CACHE_LSZ, &Cls);
	SkPciReadCfgDWord(pAC, PCI_BASE_1ST, &Bp1);
	SkPciReadCfgDWord(pAC, PCI_BASE_2ND, &Bp2);
	SkPciReadCfgByte(pAC, PCI_LAT_TIM, &lat);
	if (PciCmd != 0 || Cls != 0 || (Bp1 & 0xfffffff0L) != 0 || Bp2 != 1 ||
		Lat != 0 ) {
		return (0);
	}

	/*
	 * Restore Config Space.
	 */
	for (i = 0; i < PCI_CFG_SIZE; i++) {
		SkPciWriteCfgDWord(pAC, i*4, ConfigSpace[i]);
	}

	return (0);
}	/* SkGePciReset */

#endif	/* SK_PCI_RESET */

/******************************************************************************
 *
 *	SkGeInit1() - Level 1 Initialization
 *
 * Description:
 *	o Do a software reset.
 *	o Clear all reset bits.
 *	o Verify that the detected hardware is present.
 *	  Return an error if not.
 *	o Get the hardware configuration
 *		+ Read the number of MACs/Ports.
 *		+ Read the RAM size.
 *		+ Read the PCI Revision ID.
 *		+ Find out the adapters host clock speed
 *		+ Read and check the PHY type
 *
 * Returns:
 *	0:	success
 *	5:	Unexpected PHY type detected
 */
static int SkGeInit1(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC)		/* IO context */
{
	SK_U8	Byte;
	SK_U16	Word;
	int	RetVal;
	int	i;

	RetVal = 0;

#ifdef SK_PCI_RESET
	(void)SkGePciReset(pAC, IoC);
#endif	/* SK_PCI_RESET */

	/* Do the reset */
	SK_OUT8(IoC, B0_CTST, CS_RST_SET);

	/* Release the reset */
	SK_OUT8(IoC, B0_CTST, CS_RST_CLR);

	/* Reset all error bits in the PCI STATUS register */
	/*
	 * Note: Cfg cycles cannot be used, because they are not
	 *		 available on some platforms after 'boot time'.
	 */
	SK_OUT8(IoC, B2_TST_CTRL1, TST_CFG_WRITE_ON);
	SK_IN16(IoC, PCI_C(PCI_STATUS), &Word);
	SK_OUT16(IoC, PCI_C(PCI_STATUS), Word | PCI_ERRBITS);
	SK_OUT8(IoC, B2_TST_CTRL1, TST_CFG_WRITE_OFF);

	/* Release Master_Reset */
	SK_OUT8(IoC, B0_CTST, CS_MRST_CLR);

	/* Read number of MACs */
	SK_IN8(IoC, B2_MAC_CFG, &Byte);
	if (Byte & CFG_SNG_MAC) {
		pAC->GIni.GIMacsFound = 1;
	}
	else {
		pAC->GIni.GIMacsFound = 2;
	}
	SK_IN8(IoC, PCI_C(PCI_REV_ID), &Byte);
	pAC->GIni.GIPciHwRev = (int) Byte;

	/* Read the adapters RAM size */
	SK_IN8(IoC, B2_E_0, &Byte);
	if (Byte == 3) {
		pAC->GIni.GIRamSize = (int)(Byte-1) * 512;
		pAC->GIni.GIRamOffs = (SK_U32)512 * 1024;
	}
	else {
		pAC->GIni.GIRamSize = (int)Byte * 512;
		pAC->GIni.GIRamOffs = 0;
	}

	/* All known GE Adapters works with 53.125 MHz host clock */
	pAC->GIni.GIHstClkFact = SK_FACT_53;
	pAC->GIni.GIPollTimerVal =
		SK_DPOLL_DEF * (SK_U32)pAC->GIni.GIHstClkFact / 100;
	
	/* Read the PHY type */
	SK_IN8(IoC, B2_E_1, &Byte);
	Byte &= 0x0f;	/* the PHY type is stored in the lower nibble */
	for (i=0; i<pAC->GIni.GIMacsFound; i++) {
		pAC->GIni.GP[i].PhyType = Byte;
		switch (Byte) {
		case SK_PHY_XMAC:
			pAC->GIni.GP[i].PhyAddr = PHY_ADDR_XMAC;
			break;
		case SK_PHY_BCOM:
			pAC->GIni.GP[i].PhyAddr = PHY_ADDR_BCOM;
			break;
		case SK_PHY_LONE:
			pAC->GIni.GP[i].PhyAddr = PHY_ADDR_LONE;
			break;
		case SK_PHY_NAT:
			pAC->GIni.GP[i].PhyAddr = PHY_ADDR_NAT;
			break;
		default:
			/* ERROR: unexpected PHY typ detected */
			RetVal = 5;
			break;
		}
	}
	SK_DBG_MSG(pAC, SK_DBGMOD_HWM, SK_DBGCAT_INIT,
		("PHY type: %d  PHY addr: %x\n", pAC->GIni.GP[i].PhyType,
		pAC->GIni.GP[i].PhyAddr));

	return (RetVal);
}	/* SkGeInit1*/


/******************************************************************************
 *
 *	SkGeInit2() - Level 2 Initialization
 *
 * Description:
 *	- start the Blink Source Counter
 *	- start the Descriptor Poll Timer
 *	- configure the MAC-Arbiter
 *	- configure the Packet-Arbiter
 *	- enable the Tx Arbiters
 *	- enable the RAM Interface Arbiter
 *
 * Returns:
 *	nothing
 */
static void SkGeInit2(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC)		/* IO context */
{
	SK_GEPORT *pPrt;
	SK_U32	DWord;
	int	i;

	/* start the Blink Source Counter */
	DWord = SK_BLK_DUR * (SK_U32)pAC->GIni.GIHstClkFact / 100;
	SK_OUT32(IoC, B2_BSC_INI, DWord);
	SK_OUT8(IoC, B2_BSC_CTRL, BSC_START);

	/* start the Descriptor Poll Timer */
	if (pAC->GIni.GIPollTimerVal != 0) {
		if (pAC->GIni.GIPollTimerVal > SK_DPOLL_MAX) {
			pAC->GIni.GIPollTimerVal = SK_DPOLL_MAX;

			/* Create an Error Log Entry */
			SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E017, SKERR_HWI_E017MSG);
		}
		SK_OUT32(IoC, B28_DPT_INI, pAC->GIni.GIPollTimerVal);
		SK_OUT8(IoC, B28_DPT_CTRL, DPT_START);
	}

	/*
	 * Configure
	 *	- the MAC-Arbiter and
	 *	- the Paket Arbiter
	 *
	 * The MAC and the packet arbiter will be started once
	 * and never be stopped.
	 */
	SkGeInitMacArb(pAC, IoC);
	SkGeInitPktArb(pAC, IoC);

	/* enable the Tx Arbiters */
	SK_OUT8(IoC, MR_ADDR(MAC_1, TXA_CTRL), TXA_ENA_ARB);
	if (pAC->GIni.GIMacsFound > 1) {
		SK_OUT8(IoC, MR_ADDR(MAC_2, TXA_CTRL), TXA_ENA_ARB);
	}

	/* enable the RAM Interface Arbiter */
	SkGeInitRamIface(pAC, IoC);

	for (i = 0; i < SK_MAX_MACS; i++) {
		pPrt = &pAC->GIni.GP[i];
		if (pAC->GIni.GIPortUsage == SK_JUMBO_LINK) {
			pPrt->PRxCmd |= XM_RX_BIG_PK_OK;
		}

		if (pPrt->PLinkModeConf == SK_LMODE_HALF) {
			/*
			 * If in manual half duplex mode
			 * the other side might be in full duplex mode
			 * so ignore if a carrier extension is not seen on
			 * frames received
			 */
			pPrt->PRxCmd |= XM_RX_DIS_CEXT;
		}

	}
}	/* SkGeInit2 */

/******************************************************************************
 *
 *	SkGeInit() - Initialize the GE Adapter with the specified level.
 *
 * Description:
 *	Level	0:	Initialize the Module structures.
 *	Level	1:	Generic Hardware Initialization. The
 *			IOP/MemBase pointer has to be set before
 *			calling this level.
 *
 *			o Do a software reset.
 *			o Clear all reset bits.
 *			o Verify that the detected hardware is present.
 *			  Return an error if not.
 *			o Get the hardware configuration
 *				+ Set GIMacsFound with the number of MACs.
 *				+ Store the RAM size in GIRamSize.
 *				+ Save the PCI Revision ID in GIPciHwRev.
 *			o return an error
 *				if Number of MACs > SK_MAX_MACS
 *
 *			After returning from Level 0 the adapter
 *			may be accessed with IO operations.
 *
 *	Level	2:	start the Blink Source Counter
 *
 * Returns:
 *	0:	success
 *	1:	Number of MACs exceeds SK_MAX_MACS	( after level 1)
 *	2:	Adapter not present or not accessable
 *	3:	Illegal initialization level
 *	4:	Initialization Level 1 Call missing
 *	5:	Unexpected PHY type detected
 */
int	SkGeInit(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC,		/* IO context */
int		Level)		/* initialization level */
{
	int	RetVal;		/* return value */
	SK_U32	DWord;

	RetVal = 0;
	SK_DBG_MSG(pAC, SK_DBGMOD_HWM, SK_DBGCAT_INIT,
		("SkGeInit(Level %d)\n", Level));

	switch (Level) {
	case SK_INIT_DATA:
		/* Initialization Level 0 */
		SkGeInit0(pAC, IoC);
		pAC->GIni.GILevel = SK_INIT_DATA;
		break;
	case SK_INIT_IO:
		/* Initialization Level 1 */
		RetVal = SkGeInit1(pAC, IoC);

		/* Check if the adapter seems to be accessable */
		SK_OUT32(IoC, B2_IRQM_INI, 0x11335577L);
		SK_IN32(IoC, B2_IRQM_INI, &DWord);
		SK_OUT32(IoC, B2_IRQM_INI, 0x00000000L);
		if (DWord != 0x11335577L) {
			RetVal = 2;
			break;
		}

		/* Check if the number of GIMacsFound matches SK_MAX_MACS */
		if (pAC->GIni.GIMacsFound > SK_MAX_MACS) {
			RetVal = 1;
			break;
		}

		/* Level 1 successfully passed */
		pAC->GIni.GILevel = SK_INIT_IO;
		break;
	case SK_INIT_RUN:
		/* Initialization Level 2 */
		if (pAC->GIni.GILevel != SK_INIT_IO) {
#ifndef	SK_DIAG
			SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E002, SKERR_HWI_E002MSG);
#endif
			RetVal = 4;
			break;
		}
		SkGeInit2(pAC, IoC);

		/* Level 2 successfully passed */
		pAC->GIni.GILevel = SK_INIT_RUN;
		break;
	default:
		/* Create an Error Log Entry */
		SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E003, SKERR_HWI_E003MSG);
		RetVal = 3;
		break;
	}

	return (RetVal);
}	/* SkGeInit*/


/******************************************************************************
 *
 *	SkGeDeInit() - Deinitialize the adapter.
 *
 * Description:
 *	All ports of the adapter will be stopped if not already done.
 *	Do a software reset and switch off all LEDs.
 *
 * Returns:
 *	nothing
 */
void	SkGeDeInit(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC)		/* IO context */
{
	int	i;
	SK_U16	Word;

	/* Ensure I2C is ready. */
	SkI2cWaitIrq(pAC, IoC);

	/* Stop all current transfer activity */
	for (i = 0; i < pAC->GIni.GIMacsFound; i++) {
		if (pAC->GIni.GP[i].PState != SK_PRT_STOP &&
			pAC->GIni.GP[i].PState != SK_PRT_RESET) {

			SkGeStopPort(pAC, IoC, i, SK_STOP_ALL, SK_HARD_RST);
		}
	}

	/* Reset all bits in the PCI STATUS register */
	/*
	 * Note: Cfg cycles cannot be used, because they are not
	 *	 available on some platforms after 'boot time'.
	 */
	SK_OUT8(IoC, B2_TST_CTRL1, TST_CFG_WRITE_ON);
	SK_IN16(IoC, PCI_C(PCI_STATUS), &Word);
	SK_OUT16(IoC, PCI_C(PCI_STATUS), Word | PCI_ERRBITS);
	SK_OUT8(IoC, B2_TST_CTRL1, TST_CFG_WRITE_OFF);

	/* Do the reset, all LEDs are switched off now */
	SK_OUT8(IoC, B0_CTST, CS_RST_SET);
}	/* SkGeDeInit*/


/******************************************************************************
 *
 *	SkGeInitPort()	Initialize the specified prot.
 *
 * Description:
 *	PRxQSize, PXSQSize, and PXAQSize has to be
 *	configured for the specified port before calling this
 *	function. The descriptor rings has to be initialized, too.
 *
 *	o (Re)configure queues of the specified port.
 *	o configure the XMAC of the specified port.
 *	o put ASIC and XMAC(s) in operational mode.
 *	o initialize Rx/Tx and Sync LED
 *	o initialize RAM Buffers and MAC FIFOs
 *
 *	The port is ready to connect when returning.
 *
 * Note:
 *	The XMACs Rx and Tx state machine is still disabled when
 *	returning.
 *
 * Returns:
 *	0:	success
 *	1:	Queue size initialization error. The configured values
 *		for PRxQSize, PXSQSize, or PXAQSize are invalid for one
 *		or more queues. The specified port was NOT initialized.
 *		An error log entry was generated.
 *	2:	The port has to be stopped before it can be initialized again.
 */
int SkGeInitPort(
SK_AC	*pAC,		/* adapter context */
SK_IOC	IoC,		/* IO context */
int		Port)		/* Port to configure */
{
	SK_GEPORT *pPrt;

	pPrt = &pAC->GIni.GP[Port];

	if (SkGeCheckQSize(pAC, Port) != 0) {
		SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E004, SKERR_HWI_E004MSG);
		return (1);
	}
	if (pPrt->PState == SK_PRT_INIT || pPrt->PState == SK_PRT_RUN) {
		SK_ERR_LOG(pAC, SK_ERRCL_SW, SKERR_HWI_E005, SKERR_HWI_E005MSG);
		return (2);
	}

	/* Configuration ok, initialize the Port now */

	/* Initialize Rx, Tx and Link LED */
	/*
	 * If 1000BT Phy needs LED initialization than swap
	 * LED and XMAC initialization order
	 */
 	SkGeXmitLED(pAC, IoC, MR_ADDR(Port, TX_LED_INI), SK_LED_ENA);
 	SkGeXmitLED(pAC, IoC, MR_ADDR(Port, RX_LED_INI), SK_LED_ENA);
	/* The Link LED is initialized by RLMT or Diagnostics itself */ 

	/* Do NOT initialize the Link Sync Counter */

	/*
	 * Configure
	 *	- XMAC
	 *	- MAC FIFOs
	 *	- RAM Buffers
	 *	- enable Force Sync bit if synchronous queue available
	 *	- BMUs
	 */
	SkXmInitMac(pAC, IoC, Port);
	SkGeInitMacFifo(pAC, IoC, Port);
	SkGeInitRamBufs(pAC, IoC, Port);
	if (pPrt->PXSQSize != 0) {
		SK_OUT8(IoC, MR_ADDR(Port, TXA_CTRL), TXA_ENA_FSYNC);
	}
	SkGeInitBmu(pAC, IoC, Port);

	/* Mark port as initialized. */
	pPrt->PState = SK_PRT_INIT;
	pAC->GIni.GIAnyPortAct = SK_TRUE;

	return (0);
}	/* SkGeInitPort */
