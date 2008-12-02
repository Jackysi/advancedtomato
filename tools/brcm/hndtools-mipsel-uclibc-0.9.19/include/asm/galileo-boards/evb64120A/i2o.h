/* i2o.h - Header file for the I2O`s interface */

/* Copyright - Galileo technology. */

#ifndef __INCi2oh
#define __INCi2oh

/* includes */

#include "core.h"

/* typedefs */

typedef enum _i2oMessageReg{MESSAGE_REG_0,MESSAGE_REG_1} I2O_MESSAGE_REG;
typedef enum _cirQueSize{I20_16K = 0x1,I20_32K = 0x2,I20_64K = 0x4,\
                  I20_128K = 0x8,I20_256K = 0xc} CIRCULAR_QUEUE_SIZE;

/* Message handle Functions */
unsigned int    getInBoundMassege(I2O_MESSAGE_REG messageRegNum);
bool  sendOutBoundMassege(I2O_MESSAGE_REG messageRegNum,unsigned int message);
bool  checkInBoundIntAndClear(I2O_MESSAGE_REG messageRegNum);
bool  outBoundMessageAcknowledge(I2O_MESSAGE_REG messageRegNum);
bool  maskInBoundMessageInterrupt(I2O_MESSAGE_REG messageRegNum);
bool  enableInBoundMessageInterrupt(I2O_MESSAGE_REG messageRegNum);
bool  maskOutBoundMessageInterrupt(I2O_MESSAGE_REG messageRegNum);
bool  enableOutBoundMessageInterrupt(I2O_MESSAGE_REG messageRegNum);

/* Doorbell handle Functions */
unsigned int    readInBoundDoorBellInt(void);
bool  initiateOutBoundDoorBellInt(unsigned int interruptBits);
bool  clearInBoundDoorBellInt(unsigned int interruptBits);
bool  isInBoundDoorBellInterruptSet(void);
bool  isOutBoundDoorBellInterruptSet(void); /* For acknowledge */
bool  maskInBoundDoorBellInterrupt(void);
bool  enableInBoundDoorBellInterrupt(void);
bool  maskOutBoundDoorBellInterrupt(void);
bool  enableOutBoundDoorBellInterrupt(void);

/* I2O - Circular Queues handle Functions */

/* initialization */
bool  circularQueueEnable(CIRCULAR_QUEUE_SIZE cirQueSize,
                          unsigned int queueBaseAddr);

/* Inbound Post Queue */
unsigned int    inBoundPostQueuePop(void);
bool    isInBoundPostQueueInterruptSet(void);
bool  clearInBoundPostQueueInterrupt(void);
void    maskInBoundPostQueueInterrupt(void);
void    enableInBoundPostQueueInterrupt(void);
/* Outbound Post Queue */
bool  outBoundPostQueuePush(unsigned int data);
bool    isOutBoundPostQueueEmpty(void);
/* Inbound Free Queue */
bool  inBoundFreeQueuePush(unsigned int data);
bool    isInBoundFreeQueueEmpty(void);
/* Outbound Free Queue */
unsigned int    outBoundFreeQueuePop(void);

#endif  /* __INCi2oh */

