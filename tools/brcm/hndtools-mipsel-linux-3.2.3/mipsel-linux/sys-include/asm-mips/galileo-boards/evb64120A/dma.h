/* DMA.h - DMA functions and definitions*/

/* Copyright Galileo Technology. */

#ifndef __INCdmah
#define __INCdmah

/* includes */

#include "core.h"

/* defines */

#define FIRST_DMA_ENGINE   0
#define LAST_DMA_ENGINE    3

#define FLY_BY						BIT0
#define RD_WR_FLY					BIT1
#define DECREMENT_SOURCE_ADDRESS	BIT2
#define HOLD_SOURCE_ADDRESS			BIT3
#define DECREMENT_DEST_ADDRESS		BIT4
#define HOLD_DEST_ADDRESS			BIT5
#define DTL_1BYTE					BIT6 | BIT8
#define DTL_2BYTES					BIT7 | BIT8
#define DTL_4BYTES					BIT7
#define DTL_8BYTES					NO_BIT
#define DTL_16BYTES					BIT6
#define DTL_32BYTES					BIT6 | BIT7
#define DTL_64BYTES					BIT6 | BIT7 | BIT8
#define NON_CHAIN_MOD				BIT9
#define INT_EVERY_NULL_POINTER		BIT10
#define BLOCK_TRANSFER_MODE			BIT11
#define CHANNEL_ENABLE				BIT12
#define FETCH_NEXT_RECORED			BIT13
#define DMA_ACTIVITY_STATUS         BIT14
#define ALIGN_TOWARD_DEST			BIT15
#define MASK_DMA_REQ				BIT16
#define ENABLE_DESCRIPTOR			BIT17
#define ENABLE_EOT					BIT18
#define ENABLE_EOT_INTERRUPT		BIT19
#define ABORT_DMA					BIT20
#define SOURCE_ADDR_IN_PCI0			BIT21
#define SOURCE_ADDR_IN_PCI1			BIT22
#define DEST_ADDR_IN_PCI0			BIT23
#define DEST_ADDR_IN_PCI1			BIT24
#define REC_ADDR_IN_PCI0			BIT25
#define REC_ADDR_IN_PCI1			BIT26
#define REQ_FROM_TIMER_COUNTER		BIT28

/* typedefs */

typedef enum dmaEngine{DMA_ENG_0,DMA_ENG_1,DMA_ENG_2,DMA_ENG_3} DMA_ENGINE;

/* priority definitions */
typedef enum prioChan01{ROUND_ROBIN01,CH_1,CH_0} PRIO_CHAN_0_1;
typedef enum prioChan23{ROUND_ROBIN23,CH_3,CH_2} PRIO_CHAN_2_3;
typedef enum prioGroup{ROUND_ROBIN,CH_2_3,CH_0_1} PRIO_GROUP;
typedef enum prioOpt{RETURN_BUS,KEEP_BUS} PRIO_OPT;

typedef struct dmaRecored
{
    unsigned int    ByteCnt;
    unsigned int    SrcAdd;
    unsigned int    DestAdd;
    unsigned int    NextRecPtr;
} DMA_RECORED;

typedef enum __dma_status{CHANNEL_BUSY,NO_SUCH_CHANNEL,DMA_OK,
                            GENERAL_ERROR} DMA_STATUS;

DMA_STATUS dmaTransfer (DMA_ENGINE engine,unsigned int sourceAddr,
                        unsigned int destAddr,unsigned int numOfBytes,
                        unsigned int command,DMA_RECORED * nextRecoredPointer);
bool	dmaCommand (DMA_ENGINE channel,unsigned int command);
bool    isDmaChannelActive (DMA_ENGINE channel);

bool    changeDmaPriority(PRIO_CHAN_0_1 prio_01, PRIO_CHAN_2_3 prio_23,
                          PRIO_GROUP prioGrp, PRIO_OPT prioOpt);

#endif /* __INCdmah */
