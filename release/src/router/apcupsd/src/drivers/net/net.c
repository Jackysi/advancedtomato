/*
 * net.c
 *
 * Network client driver.
 */

/*
 * Copyright (C) 2001-2006 Kern Sibbald
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
#include "net.h"

/*
 * List of variables that can be read by getupsvar().
 * First field is that name given to getupsvar(),
 * Second field is our internal name as produced by the STATUS 
 * output from apcupsd.
 * Third field, if 0 returns everything to the end of the
 * line, and if 1 returns only to first space (e.g. integers,
 * and floating point values.
 */
static const struct {
   const char *request;
   const char *upskeyword;
   int nfields;
} cmdtrans[] = {
   {"battcap",    "BCHARGE",  1},
   {"battdate",   "BATTDATE", 1},
   {"battpct",    "BCHARGE",  1},
   {"battvolt",   "BATTV",    1},
   {"cable",      "CABLE",    0},
   {"date",       "DATE",     0},
   {"firmware",   "FIRMWARE", 0},
   {"highxfer",   "HITRANS",  1},
   {"hostname",   "HOSTNAME", 1},
   {"laststest",  "LASTSTEST",0},
   {"lastxfer",   "LASTXFER", 0},      /* reason for last xfer to batteries */
   {"linemax",    "MAXLINEV", 1},
   {"linemin",    "MINLINEV", 1},
   {"loadpct",    "LOADPCT",  1},
   {"lowbatt",    "DLOWBATT", 1},      /* low battery power off delay */
   {"lowxfer",    "LOTRANS",  1},
   {"maxtime",    "MAXTIME",  1},
   {"mbattchg",   "MBATTCHG", 1},
   {"mintimel",   "MINTIMEL", 1},
   {"model",      "MODEL",    0},
   {"nombattv",   "NOMBATTV", 1},
   {"nominv",     "NOMINV",   1},
   {"nomoutv",    "NOMOUTV",  1},
   {"nompower",   "NOMPOWER", 1},
   {"outputfreq", "LINEFREQ", 1},
   {"outputv",    "OUTPUTV",  1},
   {"version",    "VERSION",  1},
   {"retpct",     "RETPCT",   1},      /* min batt to turn on UPS */
   {"runtime",    "TIMELEFT", 1},
   {"selftest",   "SELFTEST", 1},      /* results of last self test */
   {"sense",      "SENSE",    1},
   {"serialno",   "SERIALNO", 1},
   {"status",     "STATFLAG", 1},
   {"transhi",    "HITRANS",  1},
   {"translo",    "LOTRANS",  1},
   {"upsload",    "LOADPCT",  1},
   {"upsmode",    "UPSMODE",  0},
   {"upsname",    "UPSNAME",  1},
   {"upstemp",    "ITEMP",    1},
   {"utility",    "LINEV",    1},
   {NULL, NULL}
};

/* Convert UPS response to enum */
static SelfTestResult decode_testresult(char* str)
{
   if (!strncmp(str, "OK", 2))
      return TEST_PASSED;
   else if (!strncmp(str, "NO", 2))
      return TEST_NONE;
   else if (!strncmp(str, "BT", 2))
      return TEST_FAILCAP;
   else if (!strncmp(str, "NG", 2))
      return TEST_FAILED;
   else if (!strncmp(str, "WN", 2))
      return TEST_WARNING;
   else if (!strncmp(str, "IP", 2))
      return TEST_INPROGRESS;
   else
      return TEST_UNKNOWN;
}

/* Convert UPS response to enum */
static LastXferCause decode_lastxfer(char *str)
{
   Dmsg1(80, "Transfer reason: %s\n", str);

   if (!strcmp(str, "No transfers since turnon"))
      return XFER_NONE;
   else if (!strcmp(str, "Automatic or explicit self test"))
      return XFER_SELFTEST;
   else if (!strcmp(str, "Forced by software"))
      return XFER_FORCED;
   else if (!strcmp(str, "Low line voltage"))
      return XFER_UNDERVOLT;
   else if (!strcmp(str, "High line voltage"))
      return XFER_OVERVOLT;
   else if (!strcmp(str, "Line voltage notch or spike"))
      return XFER_NOTCHSPIKE;
   else if (!strcmp(str, "Unacceptable line voltage changes"))
      return XFER_RIPPLE;
   else if (!strcmp(str, "Input frequency out of range"))
      return XFER_FREQ;
   else
      return XFER_UNKNOWN;
}

/*
 * The remote server DEVICE entry in apcupsd.conf is
 * in the form:
 *
 * DEVICE hostname[:port]
 *
 */
static int initialize_device_data(UPSINFO *ups)
{
   struct driver_data *nid = (struct driver_data *)ups->driver_internal_data;
   char *cp;

   astrncpy(nid->device, ups->device, sizeof(nid->device));
   astrncpy(ups->master_name, ups->device, sizeof(ups->master_name));
   astrncpy(ups->upsclass.long_name, "Net Slave", sizeof(ups->upsclass.long_name));

   /* Now split the device. */
   nid->hostname = nid->device;

   cp = strchr(nid->device, ':');
   if (cp) {
      *cp = '\0';
      cp++;
      nid->port = atoi(cp);
   } else {
      /* use NIS port as default */
      nid->port = ups->statusport;
   }

   nid->statbuf[0] = 0;
   nid->statlen = 0;

   Dmsg0(90, "Exit initialize_device_data\n");
   return SUCCESS;
}

/*
 * Returns 1 if var found
 *   answer has var
 * Returns 0 if variable name not found
 *   answer has "Not found" is variable name not found
 *   answer may have "N/A" if the UPS does not support this
 *           feature
 * Returns -1 if network problem
 *   answer has "N/A" if host is not available or network error
 */
static int getupsvar(UPSINFO *ups, const char *request, char *answer, int anslen)
{
   struct driver_data *nid = (struct driver_data *)ups->driver_internal_data;
   int i;
   const char *stat_match = NULL;
   char *find;
   int nfields = 0;
   char format[21];

   for (i = 0; cmdtrans[i].request; i++) {
      if (!(strcmp(cmdtrans[i].request, request))) {
         stat_match = cmdtrans[i].upskeyword;
         nfields = cmdtrans[i].nfields;
      }
   }

   if (stat_match) {
      if ((find = strstr(nid->statbuf, stat_match)) != NULL) {
         if (nfields == 1) {       /* get one field */
            asnprintf(format, sizeof(format), "%%*s %%*s %%%ds", anslen);
            sscanf(find, format, answer);
         } else {                  /* get everything to eol */
            i = 0;
            find += 11;            /* skip label */

            while (*find != '\n' && i < anslen - 1)
               answer[i++] = *find++;

            answer[i] = 0;
         }
         if (strcmp(answer, "N/A") == 0) {
            return 0;
         }
         Dmsg2(100, "Return 1 for getupsvar %s %s\n", request, answer);
         return 1;
      }
   } else {
      Dmsg1(100, "Hey!!! No match in getupsvar for %s!\n", request);
   }

   astrncpy(answer, "Not found", anslen);
   return 0;
}

static int poll_ups(UPSINFO *ups)
{
   struct driver_data *nid = (struct driver_data *)ups->driver_internal_data;
   int n, stat = 1;
   char buf[1000];

   nid->statbuf[0] = 0;
   nid->statlen = 0;

   Dmsg2(20, "Opening connection to %s:%d\n", nid->hostname, nid->port);
   if ((nid->sockfd = net_open(nid->hostname, NULL, nid->port)) < 0) {
      Dmsg0(90, "Exit poll_ups 0 comm lost\n");
      if (!ups->is_commlost()) {
         ups->set_commlost();
      }
      return 0;
   }

   if (net_send(nid->sockfd, "status", 6) != 6) {
      net_close(nid->sockfd);
      Dmsg0(90, "Exit poll_ups 0 no status flag\n");
      ups->set_commlost();
      return 0;
   }

   Dmsg0(99, "===============\n");
   while ((n = net_recv(nid->sockfd, buf, sizeof(buf) - 1)) > 0) {
      buf[n] = 0;
      astrncat(nid->statbuf, buf, sizeof(nid->statbuf));
      Dmsg3(99, "Partial buf (%d, %d):\n%s", n, strlen(nid->statbuf), buf);
   }
   Dmsg0(99, "===============\n");

   if (n < 0) {
      stat = 0;
      Dmsg0(90, "Exit poll_ups 0 bad stat net_recv\n");
      ups->set_commlost();
   } else {
      ups->clear_commlost();
   }
   net_close(nid->sockfd);

   Dmsg1(99, "Buffer:\n%s\n", nid->statbuf);
   nid->statlen = strlen(nid->statbuf);
   Dmsg1(90, "Exit poll_ups, stat=%d\n", stat);
   return stat;
}

/*
 * Fill buffer with data from UPS network daemon   
 * Returns false on error
 * Returns true if OK
 */
#define SLEEP_TIME 2
static bool fill_status_buffer(UPSINFO *ups)
{
   struct driver_data *nid = (struct driver_data *)ups->driver_internal_data;
   time_t now;
   static time_t tlog = 0;
   static bool comm_err = false;

   /* Poll or fill the buffer maximum one time per second */
   now = time(NULL);
   if ((now - nid->last_fill_time) < 2) {
      Dmsg0(90, "Exit fill_status_buffer OK less than 2 sec\n");
      return true;
   }

   if (!poll_ups(ups)) {
      /* generate event once */
      if (!comm_err) {
         execute_command(ups, ups_event[CMDCOMMFAILURE]);
         comm_err = true;
      }

      /* log every 10 minutes */
      if (now - tlog >= 10 * 60) {
         tlog = now;
         log_event(ups, event_msg[CMDCOMMFAILURE].level,
            event_msg[CMDCOMMFAILURE].msg);
      }
   } else {
      if (comm_err) {
         generate_event(ups, CMDCOMMOK);
         tlog = 0;
         comm_err = false;
      }

      nid->last_fill_time = now;

      if (!nid->got_caps)
         net_ups_get_capabilities(ups);

      if (nid->got_caps && !nid->got_static_data)
         net_ups_read_static_data(ups);
   }

   return !comm_err;
}

static int get_ups_status_flag(UPSINFO *ups, int fill)
{
   char answer[200];
   int stat = 1;
   int32_t newStatus;              /* this really should be uint32_t! */
   int32_t masterStatus;           /* status from master */
   static bool comm_loss = false;
   struct driver_data *nid = (struct driver_data *)ups->driver_internal_data;

   if (!nid->got_caps) {
      net_ups_get_capabilities(ups);
      if (!nid->got_caps)
         return 0;
   }

   if (!nid->got_static_data) {
      net_ups_read_static_data(ups);
      if (!nid->got_static_data)
         return 0;
   }

   if (fill) {
      /* 
       * Not locked because no one else should be writing in the 
       * status buffer, and we don't want to lock across I/O
       * operations. 
       */
      stat = fill_status_buffer(ups);
   }

   write_lock(ups);
   answer[0] = 0;
   if (!getupsvar(ups, "status", answer, sizeof(answer))) {
      Dmsg0(100, "HEY!!! Couldn't get status flag.\n");
      stat = 0;
      masterStatus = 0;
   } else {
      /*
       * Make sure we don't override local bits, and that
       * all non-local bits are set/cleared correctly.
       *
       * local bits = UPS_commlost|UPS_shutdown|UPS_slave|UPS_slavedown|
       *              UPS_prev_onbatt|UPS_prev_battlow|UPS_onbatt_msg|
       *              UPS_fastpoll|UPS_plugged|UPS_dev_setup
       */

      /* First transfer set or not set all non-local bits */
      masterStatus = strtol(answer, NULL, 0);
      newStatus = masterStatus & ~UPS_LOCAL_BITS;  /* clear local bits */
      ups->Status &= UPS_LOCAL_BITS;               /* clear non-local bits */
      ups->Status |= newStatus;                    /* set new non-local bits */

      /*
       * Now set any special bits, note this is set only, we do
       * not clear these bits, but let our own core code clear them
       */
      newStatus = masterStatus & (UPS_commlost | UPS_fastpoll);
      ups->Status |= newStatus;
   }

   Dmsg2(100, "Got Status = %s 0x%x\n", answer, ups->Status);

   if (masterStatus & UPS_shutdown && !ups->is_shut_remote()) {
      ups->set_shut_remote();    /* if master is shutting down so do we */
      log_event(ups, LOG_ERR, "Shutdown because NIS master is shutting down.");
      Dmsg0(100, "Set SHUT_REMOTE because of master status.\n");
   }

   /*
    * If we lost connection with master and we
    * are running on batteries, shutdown on the fourth
    * consequtive pass here. While on batteries, this code
    * is called once per second.
    */
   if (stat == 0 && ups->is_onbatt()) {
      if (comm_loss++ == 4 && !ups->is_shut_remote()) {
         ups->set_shut_remote();
         log_event(ups, LOG_ERR,
            "Shutdown because loss of comm with NIS master while on batteries.");
         Dmsg0(100, "Set SHUT_REMOTE because of loss of comm on batteries.\n");
      }
   } else {
      comm_loss = 0;
   }

   write_unlock(ups);
   return stat;
}


int net_ups_open(UPSINFO *ups)
{
   struct driver_data *nid;

   nid = (struct driver_data *)malloc(sizeof(struct driver_data));

   if (nid == NULL) {
      log_event(ups, LOG_ERR, "Out of memory.");
      exit(1);
   }

   memset(nid, 0, sizeof(struct driver_data));
   ups->driver_internal_data = nid;

   initialize_device_data(ups);

   /* Fake core code. Will go away when ups->fd is cleaned up. */
   ups->fd = 1;

   return 1;
}

int net_ups_close(UPSINFO *ups)
{
   if (ups->driver_internal_data == NULL)
      return 1;

   free(ups->driver_internal_data);
   ups->driver_internal_data = NULL;

   /* Fake core code. Will go away when ups->fd will be cleaned up. */
   ups->fd = -1;

   return 1;
}

int net_ups_setup(UPSINFO *ups)
{
   /* Nothing to setup. */
   return 1;
}

int net_ups_get_capabilities(UPSINFO *ups)
{
   struct driver_data *nid = (struct driver_data *)ups->driver_internal_data;
   char answer[200];

   write_lock(ups);

   if (poll_ups(ups)) {
      ups->UPS_Cap[CI_VLINE] = getupsvar(ups, "utility", answer, sizeof(answer));
      ups->UPS_Cap[CI_LOAD] = getupsvar(ups, "loadpct", answer, sizeof(answer));
      ups->UPS_Cap[CI_BATTLEV] = getupsvar(ups, "battcap", answer, sizeof(answer));
      ups->UPS_Cap[CI_RUNTIM] = getupsvar(ups, "runtime", answer, sizeof(answer));
      ups->UPS_Cap[CI_VMAX] = getupsvar(ups, "linemax", answer, sizeof(answer));
      ups->UPS_Cap[CI_VMIN] = getupsvar(ups, "linemin", answer, sizeof(answer));
      ups->UPS_Cap[CI_VOUT] = getupsvar(ups, "outputv", answer, sizeof(answer));
      ups->UPS_Cap[CI_SENS] = getupsvar(ups, "sense", answer, sizeof(answer));
      ups->UPS_Cap[CI_DLBATT] = getupsvar(ups, "lowbatt", answer, sizeof(answer));
      ups->UPS_Cap[CI_LTRANS] = getupsvar(ups, "lowxfer", answer, sizeof(answer));
      ups->UPS_Cap[CI_HTRANS] = getupsvar(ups, "highxfer", answer, sizeof(answer));
      ups->UPS_Cap[CI_RETPCT] = getupsvar(ups, "retpct", answer, sizeof(answer));
      ups->UPS_Cap[CI_ITEMP] = getupsvar(ups, "upstemp", answer, sizeof(answer));
      ups->UPS_Cap[CI_VBATT] = getupsvar(ups, "battvolt", answer, sizeof(answer));
      ups->UPS_Cap[CI_FREQ] = getupsvar(ups, "outputfreq", answer, sizeof(answer));
      ups->UPS_Cap[CI_WHY_BATT] = getupsvar(ups, "lastxfer", answer, sizeof(answer));
      ups->UPS_Cap[CI_ST_STAT] = getupsvar(ups, "selftest", answer, sizeof(answer));
      ups->UPS_Cap[CI_SERNO] = getupsvar(ups, "serialno", answer, sizeof(answer));
      ups->UPS_Cap[CI_BATTDAT] = getupsvar(ups, "battdate", answer, sizeof(answer));
      ups->UPS_Cap[CI_NOMBATTV] = getupsvar(ups, "nombattv", answer, sizeof(answer));
      ups->UPS_Cap[CI_NOMINV] = getupsvar(ups, "nominv", answer, sizeof(answer));
      ups->UPS_Cap[CI_NOMOUTV] = getupsvar(ups, "nomoutv", answer, sizeof(answer));
      ups->UPS_Cap[CI_NOMPOWER] = getupsvar(ups, "nompower", answer, sizeof(answer));
      ups->UPS_Cap[CI_REVNO] = getupsvar(ups, "firmware", answer, sizeof(answer));
      nid->got_caps = true;
   } else {
      nid->got_caps = false;
   }

   write_unlock(ups);
   return 1;
}

int net_ups_program_eeprom(UPSINFO *ups, int command, const char *data)
{
   return 0;
}

int net_ups_kill_power(UPSINFO *ups)
{
   /* Not possible */
   return 0;
}

int net_ups_check_state(UPSINFO *ups)
{
   int sleep_time;

   sleep_time = ups->wait_time;

   Dmsg1(100, "Sleep %d secs.\n", sleep_time);
   sleep(sleep_time);
   get_ups_status_flag(ups, 1);

   return 1;
}

#define GETVAR(ci,str) \
   (ups->UPS_Cap[ci] && getupsvar(ups, str, answer, sizeof(answer)))

int net_ups_read_volatile_data(UPSINFO *ups)
{
   char answer[200];

   if (!fill_status_buffer(ups))
      return 0;

   write_lock(ups);
   ups->set_slave();

   /* ***FIXME**** poll time needs to be scanned */
   ups->poll_time = time(NULL);
   ups->last_master_connect_time = ups->poll_time;

   if (GETVAR(CI_VLINE, "utility"))
      ups->LineVoltage = atof(answer);

   if (GETVAR(CI_LOAD, "loadpct"))
      ups->UPSLoad = atof(answer);

   if (GETVAR(CI_BATTLEV, "battcap"))
      ups->BattChg = atof(answer);

   if (GETVAR(CI_RUNTIM, "runtime"))
      ups->TimeLeft = atof(answer);

   if (GETVAR(CI_VMAX, "linemax"))
      ups->LineMax = atof(answer);

   if (GETVAR(CI_VMIN, "linemin"))
      ups->LineMin = atof(answer);

   if (GETVAR(CI_VOUT, "outputv"))
      ups->OutputVoltage = atof(answer);

   if (GETVAR(CI_SENS, "sense"))
      ups->sensitivity[0] = answer[0];

   if (GETVAR(CI_DLBATT, "lowbatt"))
      ups->dlowbatt = (int)atof(answer);

   if (GETVAR(CI_LTRANS, "lowxfer"))
      ups->lotrans = (int)atof(answer);

   if (GETVAR(CI_HTRANS, "highxfer"))
      ups->hitrans = (int)atof(answer);

   if (GETVAR(CI_RETPCT, "retpct"))
      ups->rtnpct = (int)atof(answer);

   if (GETVAR(CI_ITEMP, "upstemp"))
      ups->UPSTemp = atof(answer);

   if (GETVAR(CI_VBATT, "battvolt"))
      ups->BattVoltage = atof(answer);

   if (GETVAR(CI_FREQ, "outputfreq"))
      ups->LineFreq = atof(answer);

   if (GETVAR(CI_WHY_BATT, "lastxfer"))
      ups->lastxfer = decode_lastxfer(answer);

   if (GETVAR(CI_ST_STAT, "selftest"))
      ups->testresult = decode_testresult(answer);

   write_unlock(ups);

   get_ups_status_flag(ups, 0);

   return 1;
}

int net_ups_read_static_data(UPSINFO *ups)
{
   struct driver_data *nid = (struct driver_data *)ups->driver_internal_data;
   char answer[200];

   write_lock(ups);

   if (poll_ups(ups)) {
      if (!getupsvar(
            ups, "upsname", ups->upsname,
            sizeof(ups->upsname))) {
         log_event(ups, LOG_ERR, "getupsvar: failed for \"upsname\".");
      }
      if (!getupsvar(
            ups, "model", ups->mode.long_name, 
            sizeof(ups->mode.long_name))) {
         log_event(ups, LOG_ERR, "getupsvar: failed for \"model\".");
      }
      if (!getupsvar(
            ups, "upsmode", ups->upsclass.long_name,
            sizeof(ups->upsclass.long_name))) {
         log_event(ups, LOG_ERR, "getupsvar: failed for \"upsmode\".");
      }

      if (GETVAR(CI_SERNO, "serialno"))
         astrncpy(ups->serial, answer, sizeof(ups->serial));

      if (GETVAR(CI_BATTDAT, "battdate"))
         astrncpy(ups->battdat, answer, sizeof(ups->battdat));

      if (GETVAR(CI_NOMBATTV, "nombattv"))
         ups->nombattv = atof(answer);

      if (GETVAR(CI_NOMINV, "nominv"))
         ups->NomInputVoltage = (int)atof(answer);

      if (GETVAR(CI_NOMOUTV, "nomoutv"))
         ups->NomOutputVoltage = (int)atof(answer);

      if (GETVAR(CI_NOMPOWER, "nompower"))
         ups->NomPower = (int)atof(answer);

      if (GETVAR(CI_REVNO, "firmware"))
         astrncpy(ups->firmrev, answer, sizeof(ups->firmrev));

      nid->got_static_data = true;
   } else {
      nid->got_static_data = false;
   }

   write_unlock(ups);
   return 1;
}

int net_ups_entry_point(UPSINFO *ups, int command, void *data)
{
   char answer[200];

   switch (command) {
   case DEVICE_CMD_CHECK_SELFTEST:
      Dmsg0(80, "Checking self test.\n");
      /*
       * XXX FIXME
       *
       * One day we will do this test inside the driver and not as an
       * entry point.
       */
      /* Reason for last transfer to batteries */
      if (GETVAR(CI_WHY_BATT, "lastxfer")) {
         ups->lastxfer = decode_lastxfer(answer);
         Dmsg1(80, "Transfer reason: %d\n", ups->lastxfer);

         /* See if this is a self test rather than power failure */
         if (ups->lastxfer == XFER_SELFTEST) {
            /*
             * set Self Test start time
             */
            ups->SelfTest = time(NULL);
            Dmsg1(80, "Self Test time: %s", ctime(&ups->SelfTest));
         }
      }
      break;

   case DEVICE_CMD_GET_SELFTEST_MSG:
      if (!GETVAR(CI_ST_STAT, "selftest"))
         return FAILURE;

      ups->testresult = decode_testresult(answer);
      break;

   default:
      return FAILURE;
   }

   return SUCCESS;
}
