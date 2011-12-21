/*
 * apcaction.c
 *
 * Actions taken when something happens to the UPS.
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
 * Copyright (C) 1996-1999 Andre M. Hedrick <andre@suse.com>
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

extern int kill_on_powerfail;
static void do_shutdown(UPSINFO *ups, int cmdtype);

/*
 * These are the commands understood by the apccontrol shell script.
 * You _must_ keep the the commands[] array in sync with the defines in
 * include/apc_defines.h
 */
UPSCOMMANDS ups_event[] = {
   {"powerout",      0},           /* CMDPOWEROUT */
   {"onbattery",     0},           /* CMDONBATTERY */
   {"failing",       0},           /* CMDFAILING */
   {"timeout",       0},           /* CMDTIMEOUT */
   {"loadlimit",     0},           /* CMDLOADLIMIT */
   {"runlimit",      0},           /* CMDRUNLIMIT */
   {"doshutdown",    0},           /* CMDDOSHUTDOWN */
   {"mainsback",     0},           /* CMDMAINSBACK */
   {"annoyme",       0},           /* CMDANNOYME */
   {"emergency",     0},           /* CMDEMERGENCY */
   {"changeme",      0},           /* CMDCHANGEME */
   {"remotedown",    0},           /* CMDREMOTEDOWN */
   {"commfailure",   0},           /* CMDCOMMFAILURE */
   {"commok",        0},           /* CMDCOMMOK */
   {"startselftest", 0},           /* CMDSTARTSELFTEST */
   {"endselftest",   0},           /* CMDENDSELFTEST */
   {"offbattery",    0},           /* CMDOFFBATTERY */
   {"battdetach",    0},           /* CMDBATTDETACH */
   {"battattach",    0}            /* CMDBATTATTACH */
};

/*
 * These messages must be kept in sync with the above array
 * and the defines in include/apc_defines.h 
 */
UPSCMDMSG event_msg[] = {
   {LOG_CRIT,    "Power failure."},
   {LOG_CRIT,    "Running on UPS batteries."},
   {LOG_ALERT,   "Battery power exhausted."},
   {LOG_ALERT,   "Reached run time limit on batteries."},
   {LOG_ALERT,   "Battery charge below low limit."},
   {LOG_ALERT,   "Reached remaining time percentage limit on batteries."},
   {LOG_ALERT,   "Initiating system shutdown!"},
   {LOG_ALERT,   "Power is back. UPS running on mains."},
   {LOG_ALERT,   "Users requested to logoff."},
   {LOG_ALERT,   "Battery failure. Emergency."},
   {LOG_CRIT,    "UPS battery must be replaced."},
   {LOG_CRIT,    "Remote shutdown requested."},
   {LOG_WARNING, "Communications with UPS lost."},
   {LOG_WARNING, "Communications with UPS restored."},
   {LOG_WARNING, "UPS Self Test switch to battery."},
   {LOG_WARNING, "UPS Self Test completed."},
   {LOG_CRIT,    "Mains returned. No longer on UPS batteries."},
   {LOG_CRIT,    "Battery disconnected."},
   {LOG_CRIT,    "Battery reattached."}
};

void generate_event(UPSINFO *ups, int event)
{
   /* Log message and execute script for this event */
   log_event(ups, event_msg[event].level, event_msg[event].msg);
   Dmsg2(80, "calling execute_ups_event %s event=%d\n", ups_event[event], event);
   execute_command(ups, ups_event[event]);

   /*
    * Additional possible actions. For certain, we now do a
    * shutdown   
    */
   switch (event) {
      /*
       * For the following, in addition to the basic,
       * message logged and executed above, we do a 
       * system shutdown.
       */
   case CMDFAILING:
   case CMDTIMEOUT:
   case CMDRUNLIMIT:
   case CMDLOADLIMIT:
   case CMDEMERGENCY:
   case CMDREMOTEDOWN:
      log_event(ups, event_msg[CMDDOSHUTDOWN].level,
         event_msg[CMDDOSHUTDOWN].msg);
      do_shutdown(ups, CMDDOSHUTDOWN);
      break;

      /* For the following, everything is already done. */
   case CMDSTARTSELFTEST:
   case CMDENDSELFTEST:
   case CMDCOMMFAILURE:
   case CMDCOMMOK:
   case CMDCHANGEME:
   case CMDANNOYME:
   case CMDMAINSBACK:
   case CMDDOSHUTDOWN:            /* Already shutdown, don't recall */
   case CMDPOWEROUT:
   case CMDONBATTERY:
   case CMDOFFBATTERY:
   case CMDBATTDETACH:
   case CMDBATTATTACH:
   default:
      break;

   }
}

/*
 * Closes procfile and logfile to preserve information.
 *
 * ok = 1  => power is back
 * ok = 2  => power failure
 * ok = 3  => remote shutdown
 */
static void powerfail(int ok)
{
   /*
    * If apcupsd terminates here, it will never get a chance to
    * report the event of returning mains-power. I think apcupsd
    * has no need to force terminate() by itself. It will receive
    * a SIGTERM from init, when system goes down. This signal is
    * trapped and will trigger apcupsd's terminate() function.
    */

   if (ok == 2) {
      clear_files();
      if (terminate_on_powerfail) {
         /*
          * This sends a SIGTERM signal to itself.
          * The SIGTERM is bound to apcupsd_ or apctest_terminate(),
          * depending on which program is running this code, so it will
          * do in anyway the right thing.
          */
         sendsig_terminate();
      }
   }

   /*
    * For network slaves, apcupsd needs to terminate here for now.
    * This is sloppy, but it works. If you are networked, then the
    * master must fall also. This is required so that the UPS
    * can reboot the slaves.
    */
   if (ok == 3)
      sendsig_terminate();
}

/*
 * If called with zero, prevent users from logging in. 
 * If called with one, allow users to login.
 */
static void logonfail(UPSINFO *ups, int ok)
{
   int lgnfd;

   unlink(ups->nologinpath);

   if (ok == 0 &&
       ((lgnfd = open(ups->nologinpath, O_CREAT | O_WRONLY, 0644)) >= 0)) {
      write(lgnfd, POWERFAIL, strlen(POWERFAIL));
      close(lgnfd);
   }
}

static void prohibit_logins(UPSINFO *ups)
{
   if (ups->nologin_file)
      return;                      /* already done */

   logonfail(ups, 0);
   ups->nologin_file = true;

   log_event(ups, LOG_ALERT, "User logins prohibited");
}

static void do_shutdown(UPSINFO *ups, int cmdtype)
{
   if (ups->is_shutdown())
      return;                      /* already done */

   ups->ShutDown = time(NULL);
   ups->set_shutdown();
   delete_lockfile(ups);
   ups->set_fastpoll();
   make_file(ups, ups->pwrfailpath);
   prohibit_logins(ups);

   if (!ups->is_slave()) {
      /*
       * Note, try avoid using this option if at all possible
       * as it will shutoff the UPS power, and you cannot
       * be guaranteed that the shutdown command will have
       * succeeded. This PROBABLY should be executed AFTER
       * the shutdown command is given (the execute_command below).
       */
      if (kill_on_powerfail)
         initiate_hibernate(ups);
   }

   /* Now execute the shutdown command */
   execute_command(ups, ups_event[cmdtype]);

   /*
    * On some systems we may stop on the previous
    * line if a SIGTERM signal is sent to us.        
    */

   if (cmdtype == CMDREMOTEDOWN)
      powerfail(3);
   else
      powerfail(2);
}

/* These are the different "states" that the UPS can be in. */
enum a_state {
   st_PowerFailure,
   st_SelfTest,
   st_OnBattery,
   st_MainsBack,
   st_OnMains
};

/*
 * Figure out what "state" the UPS is in and
 * return it for use in do_action()
 */
static enum a_state get_state(UPSINFO *ups, time_t now)
{
   enum a_state state;

   /* If we're on battery for calibration, treat as not on battery */
   if (ups->is_onbatt() && !ups->is_calibration()) {
      if (ups->chg_onbatt()) {
         state = st_PowerFailure;  /* Power failure just detected */
      } else {
         if (ups->SelfTest)        /* see if UPS is doing self test */
            state = st_SelfTest;   /*   yes */
         else
            state = st_OnBattery;  /* No, this must be real power failure */
      }
   } else {
      if (ups->chg_onbatt())       /* if we were on batteries */
         state = st_MainsBack;     /* then we just got power back */
      else
         state = st_OnMains;       /* Solid on mains, normal condition */
   }
   return state;
}

static const char *testresult_to_string(SelfTestResult res)
{
   switch (res) {
   case TEST_NA:
      return "Not supported";
   case TEST_NONE:
      return "No test results available";
   case TEST_FAILED:
      return "Test failed";
   case TEST_WARNING:
      return "Warning";
   case TEST_INPROGRESS:
      return "In progress";
   case TEST_PASSED:
      return "Battery OK";
   case TEST_FAILCAP:
      return "Test failed -- insufficient battery capacity";
   case TEST_FAILLOAD:
      return "Test failed -- battery overloaded";
   case TEST_UNKNOWN:
   default:
      return "Unknown";
   }
}

/*
 * Carl Lindberg <lindberg@clindberg.org> patch applied 24Dec04
 *
 * The APC network management cards have options to shut down, reboot, or 
 * "sleep" (really just a delayed reboot) the UPS.  For all of these, it 
 * has a "graceful" option, meaning it gives the PowerChute software a 
 * chance to cleanly shutdown the machine before the UPS is shut down.  To 
 * do this, the card sets the ONBATT and LOWBATT statuses at the same 
 * time, waits several minutes, then cuts power.  PowerChute (presumably) 
 * notices this and shuts the machine down, but unfortunately apcupsd did 
 * not.
 *
 * The problem happens because in this situation, apcupsd sets the 
 * UPS_prev_battlow status before testing for it.       In the do_action() 
 * function, apcupsd notices the ONBATT status, and uses the 
 * "st_PowerFailure" state to send off an initial power failure event.   
 * After a short delay, do_action() is invoked again. If ONBATT is 
 * still set, the "st_OnBattery" state is used, and the onbattery event 
 * (among other things) is sent.
 *
 * The test for LOWBATT to see if shutdown is needed is only done in the 
 * st_OnBattery state, and it's done if LOWBATT is set but 
 * UPS_prev_battlow is not set yet.  In normal operation, LOWBATT will 
 * only come on after a period of ONBATT, and this situation works fine.  
 * However, since ONBATT and LOWBATT were set simultaneously, the 
 * UPS_prev_battlow was set the first time through, when the 
 * st_PowerFailure was used, meaning the test for LOWBATT was not 
 * performed.  The second time through in st_OnBattery, UPS_prev_battlow 
 * is already set, meaning apcupsd is assuming that the needed shutdown 
 * has already been invoked.
 *
 * The code fix just moves setting of the UPS_prev_battlow status to 
 * inside the block that tests for it, ensuring that LOWBATT will never be 
 * ignored.  Clearing the UPS_prev_battlow status remains where it is in 
 * the code, and it will always be turned off if LOWBATT is no longer set.
 *
 * After the fix, UPS_prev_battlow is not prematurely set, and apcupsd 
 * catches the signal from the management card to shut down.  I've had the 
 * code in for over a month, and it's worked fine, both from using the 
 * management card and regular pull-the-plug tests as well.   This was 
 * only tested with a serial UPS, but I assume it would be a problem with 
 * USB and SNMP connections as well.
 */

/*********************************************************************/
void do_action(UPSINFO *ups)
{
   time_t now;
   static int requested_logoff = 0;     /* asked user to logoff */
   static int first = 1;
   enum a_state state;

   write_lock(ups);

   time(&now);                     /* get current time */
   if (first) {
      first = 0;
      ups->last_time_nologon = ups->last_time_annoy = now;
      ups->last_time_on_line = now;
      
      /*
       * This is cheating slightly. We want to initialize the previous
       * status to zero so all set bits in current status will appear
       * as changes, thus allowing us to handle starting up when power
       * has already failed, for instance. However, we don't want to
       * get a BATTATTACHED event every time the daemon starts, so we
       * set the UPS_battpresent bit in the previous status.
       */
      ups->PrevStatus = UPS_battpresent;
   }

   if (ups->is_replacebatt()) {   /* Replace battery */
      /*
       * Complain every 9 hours, this causes the complaint to
       * cycle around the clock and hopefully be more noticable
       * without being too annoying. Also, ignore all change battery
       * indications for the first 10 minutes of running time to
       * prevent false alerts. Finally, issue the event 5 times, then
       * clear the flag to silence false alarms. If the battery is
       * really dead, the flag will be reset in apcsmart.c
       *
       * UPS_replacebatt is a flag. To count use a static local counter.
       * The counter is initialized only one time at startup.
       */
      if (now - ups->start_time < 60 * 10 || ups->ChangeBattCounter > 5) {
         ups->clear_replacebatt();
         ups->ChangeBattCounter = 0;
      } else if (now - ups->last_time_changeme > 60 * 60 * 9) {
         generate_event(ups, CMDCHANGEME);
         ups->last_time_changeme = now;
         ups->ChangeBattCounter++;
      }
   }

   /* Remote is shutting down, so must we. */
   if (ups->is_shut_remote()) {
      if (ups->chg_shut_remote()) {
         generate_event(ups, CMDREMOTEDOWN);
      }
      ups->PrevStatus = ups->Status;
      write_unlock(ups);
      return;
   }

   /* Generate event if battery is disconnected or reattached */
   if (ups->chg_battpresent()) {
      if (ups->is_battpresent())
         generate_event(ups, CMDBATTATTACH);
      else
         generate_event(ups, CMDBATTDETACH);
   }

   /*
    * Did BattLow bit go high? If so, start the battlow shutdown
    * timer. We will only act on this timer if we switch to battery
    * (or are already on battery). It is possible that this event occurs
    * at the same time as or even slightly before we switch to battery.
    * Therefore we must check it every time we get new status.
    */
   if (ups->chg_battlow()) {
      if (ups->is_battlow()) {
         Dmsg0(100, "BATTLOW asserted\n");
         ups->start_shut_lbatt = now;
      } else {
         Dmsg0(100, "BATTLOW glitch\n");
      }
   }

   state = get_state(ups, now);
   switch (state) {
   case st_OnMains:
      /* If power is good, update the timers. */
      ups->last_time_nologon = ups->last_time_annoy = now;
      ups->last_time_on_line = now;
      ups->clear_fastpoll();
      break;

   case st_PowerFailure:
      /* This is our first indication of a power problem */
      ups->set_fastpoll();       /* speed up polling */

      /* Check if selftest */
      Dmsg1(80, "Power failure detected. 0x%x\n", ups->Status);
      device_entry_point(ups, DEVICE_CMD_CHECK_SELFTEST, NULL);

      if (ups->SelfTest)
         generate_event(ups, CMDSTARTSELFTEST);
      else
         generate_event(ups, CMDPOWEROUT);

      ups->last_time_nologon = ups->last_time_annoy = now;
      ups->last_time_on_line = now;
      ups->last_onbatt_time = now;
      ups->num_xfers++;

      /* Enable DTR on dumb UPSes with CUSTOM_SIMPLE cable. */
      device_entry_point(ups, DEVICE_CMD_DTR_ENABLE, NULL);
      break;

   case st_SelfTest:
      /* allow 40 seconds max for selftest */
      if (now - ups->SelfTest < 40 && !ups->is_battlow())
         break;

      /* Cancel self test, announce power failure */
      ups->SelfTest = 0;
      Dmsg1(80, "UPS Self Test cancelled, fall-thru to On Battery. 0x%x\n",
         ups->Status);

      /* ...FALL-THRU to st_OnBattery... */

   case st_OnBattery:
      /* Did the second test verify the power is failing? */
      if (!ups->is_onbatt_msg() &&
         time(NULL) - ups->last_time_on_line >= ups->onbattdelay) {
            ups->set_onbatt_msg();  /* it is confirmed, we are on batteries */
            generate_event(ups, CMDONBATTERY);
            ups->last_time_nologon = ups->last_time_annoy = now;
            ups->last_time_on_line = now;
            break;
      }

      /* shutdown requested but still running */
      if (ups->is_shutdown()) {
         if (ups->killdelay && now - ups->ShutDown >= ups->killdelay) {
            if (!ups->is_slave())
               initiate_hibernate(ups);
            ups->ShutDown = now;   /* wait a bit before doing again */
            ups->set_shutdown();
         }
      } else {                     /* not shutdown yet */
         /*
          * Did MaxTimeOnBattery Expire?  (TIMEOUT in apcupsd.conf)
          * Normal Power down during Power Failure: Shutdown immediately.
          */
         if ((ups->maxtime > 0) && ((now - ups->last_time_on_line) > ups->maxtime)) {
            ups->set_shut_btime();
            generate_event(ups, CMDTIMEOUT);
            break;
         }

         /*
          * Did Battery Charge or Runtime go below percent cutoff?
          * Normal Power down during Power Failure: Start shutdown timer.
          */
         if (ups->UPS_Cap[CI_BATTLEV] && ups->BattChg <= ups->percent) {
            if (!ups->is_shut_load()) {
               Dmsg0(100, "CI_BATTLEV shutdown\n");
               ups->set_shut_load();
               ups->start_shut_load = now;
            }
         } else {
            if (ups->UPS_Cap[CI_BATTLEV] && ups->is_shut_load())
               Dmsg0(100, "CI_BATTLEV glitch\n");
            ups->clear_shut_load();
         }

         if (ups->UPS_Cap[CI_RUNTIM] && ups->TimeLeft <= ups->runtime) {
            if (!ups->is_shut_ltime()) {
               Dmsg0(100, "CI_RUNTIM shutdown\n");
               ups->set_shut_ltime();
               ups->start_shut_ltime = now;
            }
         } else {
            if (ups->UPS_Cap[CI_RUNTIM] && ups->is_shut_ltime())
               Dmsg0(100, "CI_RUNTIM glitch\n");
            ups->clear_shut_ltime();
         }

         /*
          * Check for expired shutdown timers and act on them.
          */
         if (ups->is_battlow() && ((now - ups->start_shut_lbatt) >= 5)) {
            generate_event(ups, CMDFAILING);
            break;
         }
         if (ups->is_shut_load() && ((now - ups->start_shut_load) >= 5)) {
            generate_event(ups, CMDLOADLIMIT);
            break;
         }
         if (ups->is_shut_ltime() && ((now - ups->start_shut_ltime) >= 5)) {
            generate_event(ups, CMDRUNLIMIT);
            break;
         }

         /*
          * We are on batteries, the battery is low, and the power is not
          * down ==> the battery is dead.  KES Sept 2000
          *
          * Then the battery has failed!!!
          * Must do Emergency Shutdown NOW
          */
         if (ups->is_battlow() && ups->is_online()) {
            ups->set_shut_emerg();
            generate_event(ups, CMDEMERGENCY);
         }

         /* Announce to LogOff, with initial delay. */
         if (((now - ups->last_time_on_line) > ups->annoydelay) &&
             ((now - ups->last_time_annoy) > ups->annoy) && ups->nologin_file) {
            if (!requested_logoff) {
               /* generate log message once */
               generate_event(ups, CMDANNOYME);
            } else {
               /* but execute script every time */
               execute_command(ups, ups_event[CMDANNOYME]);
            }

            time(&ups->last_time_annoy);
            requested_logoff = true;
         }

         /* Delay NoLogons. */
         if (!ups->nologin_file) {
            switch (ups->nologin.type) {
            case NEVER:
               break;
            case TIMEOUT:
               if ((now - ups->last_time_nologon) > ups->nologin_time)
                  prohibit_logins(ups);
               break;
            case PERCENT:
               if (ups->UPS_Cap[CI_BATTLEV] && ups->nologin_time >= ups->BattChg)
                  prohibit_logins(ups);
               break;
            case MINUTES:
               if (ups->UPS_Cap[CI_RUNTIM] && ups->nologin_time >= ups->TimeLeft)
                  prohibit_logins(ups);
               break;
            case ALWAYS:
            default:
               prohibit_logins(ups);
               break;
            }
         }
      }
      break;

   case st_MainsBack:
      /* The power is back after a power failure or a self test */
      if (ups->is_onbatt_msg()) {
         ups->clear_onbatt_msg();
         generate_event(ups, CMDOFFBATTERY);
      }

      if (ups->SelfTest) {
         ups->LastSelfTest = ups->SelfTest;
         ups->SelfTest = 0;

         /* Get last selftest results, only for smart UPSes. */
         device_entry_point(ups, DEVICE_CMD_GET_SELFTEST_MSG, NULL);
         log_event(ups, LOG_ALERT, "UPS Self Test completed: %s",
            testresult_to_string(ups->testresult));
         execute_command(ups, ups_event[CMDENDSELFTEST]);
      } else {
         generate_event(ups, CMDMAINSBACK);
      }

      if (ups->nologin_file)
         log_event(ups, LOG_ALERT, "Allowing logins");

      logonfail(ups, 1);
      ups->nologin_file = false;
      requested_logoff = false;
      device_entry_point(ups, DEVICE_CMD_DTR_ST_DISABLE, NULL);
      ups->last_offbatt_time = now;

      /*
       * Sanity check. Sometimes only first power problem trips    
       * thus last_onbatt_time is not set when we get here.
       */
      if (ups->last_onbatt_time <= 0)
         ups->last_onbatt_time = ups->last_offbatt_time;

      ups->cum_time_on_batt += (ups->last_offbatt_time - ups->last_onbatt_time);
      break;

   default:
      break;
   }

   /* Do a non-blocking wait on any exec()ed children */
   if (ups->num_execed_children > 0) {
      while (waitpid(-1, NULL, WNOHANG) > 0)
         ups->num_execed_children--;
   }

   /* Remember status */
   ups->PrevStatus = ups->Status;

   write_unlock(ups);
}
