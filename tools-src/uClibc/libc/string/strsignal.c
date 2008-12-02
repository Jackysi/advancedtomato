/* vi: set sw=4 ts=4: */
/* strsignal for uClibc
 *
 * Copyright (C) 2000,2001 by Erik Andersen <andersen@uclibc.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Library General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 * Manuel Novoa III       Dec 2000
 *
 * Converted to use my new (un)signed long (long) to string routines, which
 * are smaller than the previous functions and don't require static buffers.
 * Removed dependence on strcat in the process.
 * 
 * Also fixed a bug in the signal name lookup code.  While the table is
 * declared with dimension > 60, there are currently on 32 signals listed.
 *
 * Also appended a test routine ( -DCHECK_BUF ) to allow a quick check
 * on the buffer length and the number of known signals when the sys_errorlist
 * is modified.
 *
 * Added the option WANT_SIGLIST for low-memory applications to omit the
 * signal message strings and only output the signal number.
 *
 * Manuel Novoa III       Feb 2002
 *
 * Change to use _int10tostr and fix a bug in end-of-buf arg.
 */

#define WANT_SIGLIST       1

#define _STDIO_UTILITY			/* For _int10tostr. */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <string.h>

/********************** Function strsignal ************************************/
#ifdef L_strsignal

#if WANT_SIGLIST

const char *const sys_siglist[] = {
	"Unknown signal",
	"Hangup",
	"Interrupt",
	"Quit",
	"Illegal instruction",
	"Trace/breakpoint trap",
	"IOT trap/Abort",
	"Bus error",
	"Floating point exception",
	"Killed",
	"User defined signal 1",
	"Segmentation fault",
	"User defined signal 2",
	"Broken pipe",
	"Alarm clock",
	"Terminated",
	"Stack fault",
	"Child exited",
	"Continued",
	"Stopped (signal)",
	"Stopped",
	"Stopped (tty input)",
	"Stopped (tty output)",
	"Urgent condition",
	"CPU time limit exceeded",
	"File size limit exceeded",
	"Virtual time alarm",
	"Profile signal",
	"Window size changed",
	"Possible I/O",
	"Power failure",
	"Unused signal",
	NULL
};

#endif

#define NUM_KNOWN_SIGNALS    32

#if __BUFLEN_INT10TOSTR > 12
#error currently set up for 32 bit ints max!
#endif

static char retbuf[28];			/* 28 is sufficient for 32 bit ints */
static const char unknown_signal[] = "Unknown Signal:";

char *strsignal(int sig)
{
	char *pos;

#ifdef WANT_SIGLIST
	/* if ((sig >= 0) && (sig < _NSIG)) { */
	/* WARNING!!! NOT ALL _NSIG DEFINED!!! */
	if ((sig >= 0) && (sig < NUM_KNOWN_SIGNALS)) {
		strcpy(retbuf, sys_siglist[sig]);
		return retbuf;
	}
#endif

	pos = _int10tostr(retbuf+sizeof(retbuf)-1, sig) - sizeof(unknown_signal);
	strcpy(pos, unknown_signal);
	*(pos + sizeof(unknown_signal) - 1) = ' ';
	return pos;
}

#endif
/********************** Function psignal ************************************/
#ifdef L_psignal

void psignal(int sig, const char *s)
{
	fprintf(stderr, "%s: %s\n", s, strsignal(sig));
}

#endif
/********************** THE END ********************************************/

#ifdef CHECK_BUF
/* quick way to check for sufficient buffer length */
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
	int max = 0;
	int j, retcode;

	const char *p;
#if WANT_SIGLIST
	int i;
#endif
	retcode = EXIT_SUCCESS;

#if WANT_SIGLIST
	/*printf("_NSIG = %d  from headers\n", _NSIG);*/
	for ( i=0 ; sys_siglist[i] ; i++ ) {
		j = strlen(sys_siglist[i])+1;
		if (j > max) max = j;
	}
	if (i != NUM_KNOWN_SIGNALS) {
		printf("Error: strsignal.c - NUM_KNOWN_SIGNALS should be %d\n", i);
		retcode = EXIT_FAILURE;
	}
#endif

	p = strsignal(INT_MIN);
	j = retbuf+sizeof(retbuf) - p;
	if (j > max) max = j;
	/*printf("strsignal.c - Test of INT_MIN: <%s>  %d\n", p, j);*/

	if (sizeof(retbuf) != max) {
		printf("Error: strsignal.c - dimension of retbuf should be = %d\n", max);
		retcode = EXIT_FAILURE;
	}
	/*printf("strsignal.c - dimension of retbuf correct at %d\n", max);*/
	printf("Passed.\n");

	return retcode;
}
#endif
