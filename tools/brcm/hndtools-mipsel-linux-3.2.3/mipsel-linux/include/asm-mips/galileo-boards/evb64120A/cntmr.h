/* cntmr.h - Timer/Counter interface header file */

/* Copyright - Galileo technology */

#ifndef __INCtimerCounterDrvh
#define __INCtimerCounterDrvh

/* includes */

#include "core.h"

/* defines */

#define FIRST_CNTMR   0
#define LAST_CNTMR    3

#define CNTMR0_READ(pData)\
        GT_REG_READ(CNTMR0, pData)

#define CNTMR1_READ(pData)\
        GT_REG_READ(CNTMR1, pData)

#define CNTMR2_READ(pData)\
        GT_REG_READ(CNTMR2, pData)

#define CNTMR3_READ(pData)\
        GT_REG_READ(CNTMR3, pData)

/* typedefs */

typedef enum counterTimer{CNTMR_0,CNTMR_1,CNTMR_2,CNTMR_3} CNTMR_NUM;
typedef enum cntTmrOpModes{COUNTER, TIMER} CNT_TMR_OP_MODES;

bool    cntTmrLoad(unsigned int countNum, unsigned int value);
bool    cntTmrSetMode(CNTMR_NUM countNum, CNT_TMR_OP_MODES opMode);
bool    cntTmrEnable(CNTMR_NUM countNum);
bool    cntTmrStart (CNTMR_NUM countNum,unsigned int countValue,
                   CNT_TMR_OP_MODES opMode);
unsigned int    cntTmrDisable(CNTMR_NUM countNum);
unsigned int    cntTmrRead(CNTMR_NUM countNum);

#endif /* __INCtimerCounterDrvh */
