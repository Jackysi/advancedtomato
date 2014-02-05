/* Network Authentication Service deamon (vxWorks)
 *
 * Copyright (C) 2010, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: nas_vx.c 241388 2011-02-18 03:33:22Z stakita $
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <assert.h>

#include <vxWorks.h>
#include <ioLib.h>
#include <ifLib.h>
#include <muxLib.h>
#include <muxTkLib.h>
#include <tickLib.h>
#include <taskLib.h>
#include <taskVarLib.h>
#include <errnoLib.h>

extern int gettimeofday(struct timeval *tv, struct timezone *tz);
extern int sysClkRateGet(void);
extern void sys_reboot();

#include <typedefs.h>
#include <bcmutils.h>
#include <bcmnvram.h>
#include <proto/ethernet.h>
#include <proto/eapol.h>
#include <bcmcrypto/md5.h>
#include <wlutils.h>

#include <nas_wksp.h>
#include <eapd.h>

static nas_wksp_t * nas_nwksp = NULL;

void
nas_sleep_ms(uint ms)
{
	taskDelay(ms*sysClkRateGet()/1000);
}

void
nas_rand128(uint8 *rand128)
{
	struct timeval tv;
	struct timezone tz;
	MD5_CTX md5;

	gettimeofday(&tv, &tz);
	tv.tv_sec ^= rand();
	MD5Init(&md5);
	MD5Update(&md5, (unsigned char *) &tv, sizeof(tv));
	MD5Update(&md5, (unsigned char *) &tz, sizeof(tz));
	MD5Final(rand128, &md5);
}

static void
hup_hdlr(int sig)
{
	if (nas_nwksp)
		nas_nwksp->flags = NAS_WKSP_FLAG_SHUTDOWN;

	return;
}

/*
 * Configuration APIs
 */
int
nas_safe_get_conf(char *outval, int outval_size, char *name)
{
	char *val;

	if (name == NULL || outval == NULL) {
		if (outval)
			memset(outval, 0, outval_size);
		return -1;
	}

	val = nvram_safe_get(name);
	if (!strcmp(val, ""))
		memset(outval, 0, outval_size);
	else
		snprintf(outval, outval_size, "%s", val);
	return 0;
}

/* nas task entry */
int
nas_main()
{
#ifdef BCMDBG
	char debug[8];
#endif

	/* clear rootnwksp */
	nas_nwksp = NULL;

	/* alloc nas/wpa work space */
	if (!(nas_nwksp = nas_wksp_alloc_workspace())) {
		NASMSG("Unable to allocate work space memory. Quitting...\n");
		return 0;
	}

#ifdef BCMDBG
	/* verbose - 0:no | others:yes */
	/* for workspace */
	if (nas_safe_get_conf(debug, sizeof(debug), "nas_dbg") == 0)
		debug_nwksp = (int)atoi(debug);
#endif
	/* establish a handler to handle SIGTERM. */
	signal(SIGTERM, hup_hdlr);

	/* run main loop to dispatch message */
	nas_wksp_main_loop(nas_nwksp);

	return 0;
}

void
nas_reset_board()
{
	sys_reboot();
	return;
}

void
nasStart(void)
{
	int tid = taskNameToId("NAS");
	ULONG ticks;

	if (tid == ERROR) {
		/* clear nas wksp initialization flag */
		nas_wksp_clear_inited();

		taskSpawn("NAS",
			 60, /* priority of new task */
			 0, /* task option word */
			 30000,  /* size (bytes) of stack needed plus name */
			 (FUNCPTR)nas_main,   /* entry point of new task */
			 0,
			 0,
			 0, 0, 0, 0, 0, 0, 0, 0);
		printf("NAS task started.\n");

		/* wait until nas initialization finished */
		ticks = tickGet();
		do {
			if (tickGet() - ticks < 3 * sysClkRateGet())
				taskDelay(sysClkRateGet());
			else {
				printf("Unable to wait NAS initialization finished!.\n");
				return;
			}
		} while (taskNameToId("NAS") != ERROR && !nas_wksp_is_inited());
	}
	else
		printf("NAS task is already running.\n");
}

void
nasStop(void)
{
	int tid = taskNameToId("NAS");

	if (tid != ERROR) {
		ULONG ticks;

		kill(tid, SIGTERM);

		/* wait till the task is dead */
		ticks = tickGet();
		do {
			if (tickGet() - ticks < 3 * sysClkRateGet())
				taskDelay(sysClkRateGet());
			else {
				printf("Unable to kill NAS task!.\n");
				return;
			}
		}
		while (taskNameToId("NAS") != ERROR);
		printf("NAS task killed.\n");
	}
	else
		printf("NAS task is not running.\n");
}
