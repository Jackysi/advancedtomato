/* cntmr.c - GT counters/timers functions */

/* Copyright - Galileo technology. 9/3/2000 */

/*
DESCRIPTION
This file contains function which serves the user with a complete interface
to the GT internal counters and timers, please advise: each counter/timer unit
can function only as a counter or a timer at current time.
Counter/timer 0 is 32 bit wide.
Counters/timers 1-3 are 24 bit wide.
*/

/* includes */

#ifdef __linux__
#include <asm/galileo/evb64120A/cntmr.h>
#include <asm/galileo/evb64120A/core.h>
#else
#include "cntmr.h"
#include "core.h"
#endif

/********************************************************************
* cntTmrStart - Starts a counter/timer with given an initiate value.
*
* INPUTS:  unsigned int countNum - Selects one of the 8 counters/timers.
*          unsigned int countValue - Initial value for count down.
*          CNT_TMR_OP_MODES opMode - Set Mode, Counter or Timer.
*
* RETURNS: false if one of the parameters is erroneous, true otherwise.
*********************************************************************/

bool cntTmrStart(CNTMR_NUM countNum, unsigned int countValue,
		 CNT_TMR_OP_MODES opMode)
{
	unsigned int command = 1;
	unsigned int value;

	if (countNum > LAST_CNTMR)
		return false;
	else {
		GT_REG_READ(TIMER_COUNTER_CONTROL, &value);
		cntTmrDisable(countNum);
		GT_REG_WRITE((TIMER_COUNTER0 + (4 * countNum)),
			     countValue);
		command = command << countNum * 2;
		value = value | command;
		command = command << 1;
		switch (opMode) {
		case TIMER:	/* The Timer/Counter bit set to logic '1' */
			value = value | command;
			break;
		case COUNTER:	/* The Timer/Counter bit set to logic '0' */
			value = value & ~command;
			break;
		default:
			return false;
		}
		GT_REG_WRITE(TIMER_COUNTER_CONTROL, value);
		return true;
	}
}

/********************************************************************
* cntTmrDisable - Disables the timer/counter operation and return its
*                 value.
*
* INPUTS:  unsigned int countNum - Selects one of the 8 counters/timers.
* RETURNS: The counter/timer value (unsigned int), if any of the arguments are
*          erroneous return 0.
*********************************************************************/

unsigned int cntTmrDisable(CNTMR_NUM countNum)
{
	unsigned int command = 1;
	unsigned int regValue;
	unsigned int value;

	GT_REG_READ(TIMER_COUNTER_CONTROL, &value);
	if (countNum > LAST_CNTMR)
		return 0;
	GT_REG_READ(TIMER_COUNTER0 + 4 * countNum, &regValue);
	command = command << countNum * 2;	/* Disable the timer/counter */
	value = value & ~command;
	GT_REG_WRITE(TIMER_COUNTER_CONTROL, value);
	return regValue;
}

/********************************************************************
* cntTmrRead - Reads a timer or a counter value. (This operation can be
*              perform while the counter/timer is active).
*
* RETURNS: The counter/timer value. If wrong input value, return 0.
*********************************************************************/

unsigned int cntTmrRead(CNTMR_NUM countNum)
{
	unsigned int value;
	if (countNum > LAST_CNTMR)
		return 0;
	else
		GT_REG_READ(TIMER_COUNTER0 + countNum * 4, &value);
	return value;
}

/********************************************************************
* cntTmrEnable - Set enable-bit of timer/counter.
*                Be aware: If the counter/timer is active, this function
*                          will terminate with an false.
*
* INPUTS:  unsigned int countNum - Selects one of the 8 counters/timers.
* RETURNS: false if one of the parameters is erroneous, true otherwise.
*********************************************************************/

bool cntTmrEnable(CNTMR_NUM countNum)
{
	unsigned int command = 1;
	unsigned int value;
	GT_REG_READ(TIMER_COUNTER_CONTROL, &value);
	if (countNum > LAST_CNTMR)
		return false;
	else {
		command = command << countNum * 2;
		if ((command & value) != 0)	/* ==> The counter/timer is enabled */
			return false;	/* doesn't make sense to Enable an "enabled" counter */
		value = value | command;
		GT_REG_WRITE(TIMER_COUNTER_CONTROL, value);
		return true;
	}
}

/********************************************************************
* cntTmrLoad - loading value for timer number countNum.
*              Be aware: If this function try to load value to an enabled
*                        counter/timer it terminate with false.
*
* INPUTS:  unsigned int countNum - Selects one of the 8 counters/timers.
*          unsigned int countValue - The value for load the register.
* RETURNS: false if one of the parameters is erroneous, true otherwise.
*********************************************************************/

bool cntTmrLoad(unsigned int countNum, unsigned int countValue)
{
	unsigned int command = 1;
	unsigned int value;
	GT_REG_READ(TIMER_COUNTER_CONTROL, &value);
	if (countNum > LAST_CNTMR)
		return false;
	else {
		command = command << countNum * 2;
		value = value & command;
		if (value != 0) {	/* ==> The counter/timer is enabled */
			return false;	/* can't reload value when counter/timer is enabled */
		} else {
			GT_REG_WRITE((TIMER_COUNTER0 + (4 * countNum)),
				     countValue);
			return true;
		}

	}
}

/********************************************************************
* cntTmrSetMode - Configurate the Mode of the channel to work as a counter
*                 or as a timer. (for more details on the different between
*                 those two modes is written in the Data Sheet).
*                 NOTE: This function only set the counter/timer mode and
*                 don't enable it.
*                 Be aware: If this function try to load value to an enabled
*                           counter/timer it terminate with false.
*
* INPUTS:  unsigned int countNum - Selects one of the 8 counters/timers.
*          CNT_TMR_OP_MODES opMode - TIMER or COUNTER mode.
* RETURNS: false if one of the parameters is erroneous true otherwise .
*********************************************************************/

bool cntTmrSetMode(CNTMR_NUM countNum, CNT_TMR_OP_MODES opMode)
{
	unsigned int command = 1;
	unsigned int value;

	GT_REG_READ(TIMER_COUNTER_CONTROL, &value);
	if (countNum > LAST_CNTMR)
		return false;
	else {
		command = command << countNum * 2;
		value = value & command;
		if (value != 0) {	/* ==> The counter/timer is enabled */
			return false;	/* can't set the Mode when counter/timer is enabled */
		} else {
			command = command << 1;
			switch (opMode) {
			case TIMER:
				value = value | command;	/* The Timer/Counter bit set to logic '1' */
				break;
			case COUNTER:
				value = value & ~command;	/*The Timer/Counter bit set to logic '0' */
				break;
			default:
				return false;
			}
			GT_REG_WRITE(TIMER_COUNTER_CONTROL, value);
			return true;
		}
	}
}
