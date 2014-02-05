/*
 * apcsignal.c
 *
 * signal() managing functions
 */

/*
 * Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "apc.h"

#ifndef HAVE_MINGW
static void *terminate(void *arg)
{
   // Create signal set containing SIGHUP, SIGINT, and SIGTERM
   sigset_t sigset;
   sigemptyset(&sigset);
   sigaddset(&sigset, SIGHUP);
   sigaddset(&sigset, SIGINT);
   sigaddset(&sigset, SIGTERM);

   // Wait for signal delivery
   int signum, err;
   do
   {
      err = sigwait(&sigset, &signum);
   }
   while(err == EINTR);

   // Caught a signal; invoke handler
   void (*handler) (int) = (void (*) (int))arg;
   handler(signum);
   return NULL;
}
#endif

void init_signals(void (*handler) (int))
{
#ifndef HAVE_MINGW
   // Block SIGPIPE and termination signals
   sigset_t sigset;
   sigemptyset(&sigset);
   sigaddset(&sigset, SIGPIPE); // Don't care
   sigaddset(&sigset, SIGHUP);  // Will be handled by terminate thread
   sigaddset(&sigset, SIGINT);  // Will be handled by terminate thread
   sigaddset(&sigset, SIGTERM); // Will be handled by terminate thread
   pthread_sigmask(SIG_BLOCK, &sigset, NULL);

   // Launch thread to synchronously wait for termination signals
   pthread_t tid;
   pthread_create(&tid, NULL, terminate, (void*)handler);
#endif
}
