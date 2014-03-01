/*
 * apcexec.c
 *
 * Fork/exec functions.
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 * Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
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

static pthread_t thread_id[MAX_THREADS];
static const char *thread_name[MAX_THREADS];
static int num_threads = 0;

/* Start a "real" POSIX thread */
int start_thread(UPSINFO *ups, void (*action) (UPSINFO * ups),
   const char *proctitle, char *argv0)
{
   pthread_t tid;
   int status, t_index;

   set_thread_concurrency();
   status = pthread_create(&tid, NULL, (void *(*)(void *))action, (void *)ups);
   if (status != 0) {
      log_event(ups, LOG_WARNING, "Unable to start thread: ERR=%s\n",
         strerror(status));
      return 0;
   }

   t_index = num_threads;
   if (num_threads < MAX_THREADS) {
      thread_id[num_threads] = tid;
      thread_name[num_threads] = proctitle;
      num_threads++;
   } else {
      log_event(ups, LOG_ERR,
         "Something is wrong, we have %d threads, max is %d\n",
         num_threads + 1, MAX_THREADS);
   }

   return t_index;
}

/* Cancel all running threads except ourselves */
void clean_threads(void)
{
   int i;
   pthread_t my_tid;

   my_tid = pthread_self();
   for (i = 0; i < num_threads; i++) {
      if (!pthread_equal(my_tid, thread_id[i])) {
         pthread_cancel(thread_id[i]);
         pthread_detach(thread_id[i]);
      }
   }
}

#ifdef HAVE_MINGW

char sbindir[MAXSTRING];

int execute_command(UPSINFO *ups, UPSCOMMANDS cmd)
{
   char cmdline[MAXSTRING];
   char *comspec;
   PROCESS_INFORMATION procinfo;
   STARTUPINFOA startinfo;
   BOOL rc;

   if (cmd.pid && (kill(cmd.pid, 0) == 0)) {
      /*
       * Command is already running. No point in running it two
       * times.
       */
      return SUCCESS;
   }

   /* Find command interpreter */
   comspec = getenv("COMSPEC");
   if (comspec == NULL)
      return FAILURE;

   /* Build the command line */
   if (g_os_version_info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
      /* Win95/98/ME need environment size parameter and no extra quotes */
      asnprintf(cmdline, sizeof(cmdline), 
         "\"%s\" /E:4096 /c \"%s%s\" %s %s %d %d \"%s\"",
         comspec, ups->scriptdir, APCCONTROL_FILE, cmd.command,
         ups->upsname, !ups->is_slave(), ups->is_plugged(), sbindir);
   } else {
      /* WinNT/2K/Vista need quotes around the entire sub-command */
      asnprintf(cmdline, sizeof(cmdline), 
         "\"%s\" /c \"\"%s%s\" %s %s %d %d \"%s\"\"",
         comspec, ups->scriptdir, APCCONTROL_FILE, cmd.command,
         ups->upsname, !ups->is_slave(), ups->is_plugged(), sbindir);
   }

   /* Initialize the STARTUPINFOA struct to hide the console window */
   memset(&startinfo, 0, sizeof(startinfo));
   startinfo.cb = sizeof(startinfo);
   startinfo.dwFlags = STARTF_USESHOWWINDOW;
   startinfo.wShowWindow = SW_HIDE;

   Dmsg1(200, "execute_command: CreateProcessA(NULL, %s, ...)\n", cmdline);

   /* Execute the process */
   rc = CreateProcessA(NULL,
                       cmdline,        // command line
                       NULL,           // process security attributes
                       NULL,           // primary thread security attributes
                       TRUE,           // handles are inherited
                       0,              // creation flags
                       NULL,           // use parent's environment
                       ups->scriptdir, // working directory
                       &startinfo,     // STARTUPINFO pointer
                       &procinfo);     // receives PROCESS_INFORMATION
   if (!rc) {
      log_event(ups, LOG_WARNING, "execute failed: CreateProcessA(NULL, %s, ...)=%d\n",
         cmdline, GetLastError());
      return FAILURE;
   }

   /* Extract pid */
   cmd.pid = procinfo.dwProcessId;

   /* Don't need handles */
   CloseHandle(procinfo.hProcess);
   CloseHandle(procinfo.hThread);

   return SUCCESS;
}
#else
int execute_command(UPSINFO *ups, UPSCOMMANDS cmd)
{
   const char *argv[6];
   char connected[20], powered[20];
   char apccontrol[MAXSTRING];

   if (cmd.pid && (kill(cmd.pid, 0) == 0)) {
      /*
       * Command is already running. No point in running it two
       * times.
       */
      return SUCCESS;
   }

   asnprintf(connected, sizeof(connected), "%d", !ups->is_slave());
   asnprintf(powered, sizeof(powered), "%d", (int)ups->is_plugged());
   asnprintf(apccontrol, sizeof(apccontrol), "%s%s", ups->scriptdir, APCCONTROL_FILE);

#ifdef HAVE_QNX_OS
   /* fork() is supported only in single-threaded applications */
   argv[0] = apccontrol;        /* Shell script to execute. */
   argv[1] = cmd.command;       /* Parameter to script. */
   argv[2] = ups->upsname;      /* UPS name */
   argv[3] = connected;
   argv[4] = powered;
   argv[5] = NULL;
   if (spawnv(P_NOWAIT, apccontrol, (char * const *)argv) == -1) {
      log_event(ups, LOG_WARNING, "execute: cannot spawn(). ERR=%s",
         strerror(errno));
      return FAILURE;
   }
#else
   /* fork() and exec() */
   switch (cmd.pid = fork()) {
   case -1:     /* error */
      log_event(ups, LOG_WARNING, "execute: cannot fork(). ERR=%s",
         strerror(errno));
      return FAILURE;

   case 0:      /* child */
      /* Don't leak unnecessary fds to child */
      for (int i=0; i<sysconf(_SC_OPEN_MAX); i++) {
         if (i != STDIN_FILENO && i != STDOUT_FILENO && i != STDERR_FILENO) {
            if (close(i) == 0)
               Dmsg1(200, "exec closed fd %d\n", i);
         }
      }

      argv[0] = apccontrol;        /* Shell script to execute. */
      argv[1] = cmd.command;       /* Parameter to script. */
      argv[2] = ups->upsname;      /* UPS name */
      argv[3] = connected;
      argv[4] = powered;
      argv[5] = NULL;
      execv(apccontrol, (char **)argv);

      /* NOT REACHED */
      log_event(ups, LOG_WARNING, "Cannot exec %s %s: %s",
         apccontrol, cmd.command, strerror(errno));

      /* Child must exit if fails exec'ing. */
      exit(-1);
      break;

   default:      /* parent */
      /*
       * NOTE, we do a nonblocking waitpid) here to
       * pick up any previous children. We also]
       * increment a counter, and then in do_action()
       * in apcaction.c, we wait again each pass until
       * all the children are reaped.  This is for
       * BSD systems where SIG_IGN does not prevent
       * zombies.
       */
      if (ups->num_execed_children < 0)
         ups->num_execed_children = 1;
      else
         ups->num_execed_children++;

      while (ups->num_execed_children > 0 && waitpid(-1, NULL, WNOHANG) > 0)
         ups->num_execed_children--;

      break;
   }
#endif /* HAVE_QNX_OS */

   return SUCCESS;
}
#endif /* HAVE_MINGW */
