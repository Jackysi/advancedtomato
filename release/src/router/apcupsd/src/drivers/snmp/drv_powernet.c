/*
 * drv_powernet.c
 *
 * PowerNet driver
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
 * Copyright (C) 1999-2002 Riccardo Facchetti <riccardo@apcupsd.org>
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
#include "snmp.h"
#include "snmp_private.h"

static int powernet_check_comm_lost(UPSINFO *ups)
{
   struct timeval now;
   static struct timeval prev;
   struct snmp_ups_internal_data *Sid =
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   struct snmp_session *s = &Sid->session;
   powernet_mib_t *data = (powernet_mib_t *)Sid->MIB;
   int ret = 1;

   /*
    * Check the Ethernet COMMLOST first, then check the
    * Web/SNMP->UPS serial COMMLOST.
    */
   data->upsComm = NULL;
   if (powernet_mib_mgr_get_upsComm(s, &(data->upsComm)) < 0 ||
       (data->upsComm && data->upsComm->__upsCommStatus == 2)) {

      if (!ups->is_commlost()) {
         generate_event(ups, CMDCOMMFAILURE);
         ups->set_commlost();
         gettimeofday(&prev, NULL);
      }

      /* Log an event every 10 minutes */
      gettimeofday(&now, NULL);
      if (TV_DIFF_MS(prev, now) >= 10*60*1000) {
         log_event(ups, event_msg[CMDCOMMFAILURE].level,
            event_msg[CMDCOMMFAILURE].msg);
         prev = now;
      }

      ret = 0;
   }
   else if (ups->is_commlost())
   {
      generate_event(ups, CMDCOMMOK);
      ups->clear_commlost();
   }

   if (data->upsComm)
      free(data->upsComm);

   return ret;
}


int powernet_snmp_kill_ups_power(UPSINFO *ups)
{
   /* Was 1} change submitted by Kastus Shchuka (kastus@lists.sourceforge.net) 10Dec03 */
   oid upsBasicControlConserveBattery[] =
      { 1, 3, 6, 1, 4, 1, 318, 1, 1, 1, 6, 1, 1, 0 };
   struct snmp_ups_internal_data *Sid =
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   struct snmp_session *s = &Sid->session;
   struct snmp_session *peer;
   struct snmp_pdu *request, *response;
   int status;

   /*
    * Set up the SET request.
    */
   request = snmp_pdu_create(SNMP_MSG_SET);

   /*
    * Set upsBasicControlConserveBattery variable (INTEGER) to
    * turnOffUpsToConserveBattery(2) value. Will turn on the UPS only
    * when power returns.
    */
   if (snmp_add_var(request, upsBasicControlConserveBattery,
         sizeof(upsBasicControlConserveBattery) / sizeof(oid), 'i', "2")) {
      return 0;
   }

   peer = snmp_open(s);

   if (!peer) {
      Dmsg0(0, "Can not open the SNMP connection.\n");
      return 0;
   }

   status = snmp_synch_response(peer, request, &response);

   if (status != STAT_SUCCESS) {
      Dmsg0(0, "Unable to communicate with UPS.\n");
      return 0;
   }

   if (response->errstat != SNMP_ERR_NOERROR) {
      Dmsg1(0, "Unable to kill UPS power: can not set SNMP variable (%d).\n", response->errstat);
      return 0;
   }

   if (response)
      snmp_free_pdu(response);

   snmp_close(peer);

   return 1;
}

int powernet_snmp_ups_get_capabilities(UPSINFO *ups)
{
   int i = 0;

   /*
    * Assume that an UPS with Web/SNMP card has all the capabilities,
    * minus a few.
    */
   for (i = 0; i <= CI_MAX_CAPS; i++)
   {
      if (i != CI_NOMBATTV &&
          i != CI_HUMID    &&
          i != CI_ATEMP    &&
          i != CI_VBATT    &&
          i != CI_NOMINV   &&
          i != CI_REG1     &&
          i != CI_REG2     &&
          i != CI_REG3)
         ups->UPS_Cap[i] = TRUE;
   }

   if (powernet_check_comm_lost(ups) == 0)
      return 0;

   return 1;
}


int powernet_snmp_ups_read_static_data(UPSINFO *ups)
{
   struct snmp_ups_internal_data *Sid = 
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   struct snmp_session *s = &Sid->session;
   powernet_mib_t *data = (powernet_mib_t *)Sid->MIB;

   if (powernet_check_comm_lost(ups) == 0)
      return 0;

   data->upsBasicIdent = NULL;
   powernet_mib_mgr_get_upsBasicIdent(s, &(data->upsBasicIdent));
   if (data->upsBasicIdent) {
      SNMP_STRING(upsBasicIdent, Model, upsmodel);
      SNMP_STRING(upsBasicIdent, Name, upsname);
      free(data->upsBasicIdent);
   }

   data->upsAdvIdent = NULL;
   powernet_mib_mgr_get_upsAdvIdent(s, &(data->upsAdvIdent));
   if (data->upsAdvIdent) {
      SNMP_STRING(upsAdvIdent, FirmwareRevision, firmrev);
      SNMP_STRING(upsAdvIdent, DateOfManufacture, birth);
      SNMP_STRING(upsAdvIdent, SerialNumber, serial);
      free(data->upsAdvIdent);
   }

   data->upsBasicBattery = NULL;
   powernet_mib_mgr_get_upsBasicBattery(s, &(data->upsBasicBattery));
   if (data->upsBasicBattery) {
      SNMP_STRING(upsBasicBattery, LastReplaceDate, battdat);
      free(data->upsBasicBattery);
   }

   data->upsAdvBattery = NULL;
   powernet_mib_mgr_get_upsAdvBattery(s, &(data->upsAdvBattery));
   if (data->upsAdvBattery) {
      ups->extbatts = data->upsAdvBattery->__upsAdvBatteryNumOfBattPacks;
      ups->badbatts = data->upsAdvBattery->__upsAdvBatteryNumOfBadBattPacks;
      free(data->upsAdvBattery);
   }

   data->upsAdvConfig = NULL;
   powernet_mib_mgr_get_upsAdvConfig(s, &(data->upsAdvConfig));
   if (data->upsAdvConfig) {
      ups->NomOutputVoltage = data->upsAdvConfig->__upsAdvConfigRatedOutputVoltage;
      ups->hitrans = data->upsAdvConfig->__upsAdvConfigHighTransferVolt;
      ups->lotrans = data->upsAdvConfig->__upsAdvConfigLowTransferVolt;
      switch (data->upsAdvConfig->__upsAdvConfigAlarm) {
      case 1:
         if (data->upsAdvConfig->__upsAdvConfigAlarmTimer / 100 < 30)
            astrncpy(ups->beepstate, "0 Seconds", sizeof(ups->beepstate));
         else
            astrncpy(ups->beepstate, "Timed", sizeof(ups->beepstate));
         break;
      case 2:
         astrncpy(ups->beepstate, "LowBatt", sizeof(ups->beepstate));
         break;
      case 3:
         astrncpy(ups->beepstate, "NoAlarm", sizeof(ups->beepstate));
         break;
      default:
         astrncpy(ups->beepstate, "Timed", sizeof(ups->beepstate));
         break;
      }

      ups->rtnpct = data->upsAdvConfig->__upsAdvConfigMinReturnCapacity;

      switch (data->upsAdvConfig->__upsAdvConfigSensitivity) {
      case 1:
         astrncpy(ups->sensitivity, "Auto", sizeof(ups->sensitivity));
         break;
      case 2:
         astrncpy(ups->sensitivity, "Low", sizeof(ups->sensitivity));
         break;
      case 3:
         astrncpy(ups->sensitivity, "Medium", sizeof(ups->sensitivity));
         break;
      case 4:
         astrncpy(ups->sensitivity, "High", sizeof(ups->sensitivity));
         break;
      default:
         astrncpy(ups->sensitivity, "Unknown", sizeof(ups->sensitivity));
         break;
      }

      /* Data in Timeticks (1/100th sec). */
      ups->dlowbatt = data->upsAdvConfig->__upsAdvConfigLowBatteryRunTime / 6000;
      ups->dwake = data->upsAdvConfig->__upsAdvConfigReturnDelay / 100;
      ups->dshutd = data->upsAdvConfig->__upsAdvConfigShutoffDelay / 100;
      free(data->upsAdvConfig);
   }

   data->upsAdvTest = NULL;
   powernet_mib_mgr_get_upsAdvTest(s, &(data->upsAdvTest));
   if (data->upsAdvTest) {
      switch (data->upsAdvTest->__upsAdvTestDiagnosticSchedule) {
      case 1:
         astrncpy(ups->selftest, "unknown", sizeof(ups->selftest));
         break;
      case 2:
         astrncpy(ups->selftest, "biweekly", sizeof(ups->selftest));
         break;
      case 3:
         astrncpy(ups->selftest, "weekly", sizeof(ups->selftest));
         break;
      case 4:
         astrncpy(ups->selftest, "atTurnOn", sizeof(ups->selftest));
         break;
      case 5:
         astrncpy(ups->selftest, "never", sizeof(ups->selftest));
         break;
      default:
         astrncpy(ups->selftest, "unknown", sizeof(ups->selftest));
         break;
      }

      switch (data->upsAdvTest->__upsAdvTestDiagnosticsResults) {
      case 1:  /* Passed */
         ups->testresult = TEST_PASSED;
         break;
      case 2:  /* Failed */
      case 3:  /* Invalid test */
         ups->testresult = TEST_FAILED;
         break;
      case 4:  /* Test in progress */
         ups->testresult = TEST_INPROGRESS;
         break;
      default:
         ups->testresult = TEST_UNKNOWN;
         break;
      }

      free(data->upsAdvTest);
   }

   return 1;
}

int powernet_snmp_ups_read_volatile_data(UPSINFO *ups)
{
   struct snmp_ups_internal_data *Sid =
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   struct snmp_session *s = &Sid->session;
   powernet_mib_t *data = (powernet_mib_t *)Sid->MIB;

   if (powernet_check_comm_lost(ups) == 0)
      return 0;

   data->upsBasicBattery = NULL;
   powernet_mib_mgr_get_upsBasicBattery(s, &(data->upsBasicBattery));
   if (data->upsBasicBattery) {
      switch (data->upsBasicBattery->__upsBasicBatteryStatus) {
      case 2:
         ups->clear_battlow();
         break;
      case 3:
         ups->set_battlow();
         break;
      default:                    /* Unknown, assume battery is ok */
         ups->clear_battlow();
         break;
      }
      free(data->upsBasicBattery);
   }

   data->upsAdvBattery = NULL;
   powernet_mib_mgr_get_upsAdvBattery(s, &(data->upsAdvBattery));
   if (data->upsAdvBattery) {
      ups->BattChg = data->upsAdvBattery->__upsAdvBatteryCapacity;
      ups->UPSTemp = data->upsAdvBattery->__upsAdvBatteryTemperature;
      ups->TimeLeft = data->upsAdvBattery->__upsAdvBatteryRunTimeRemaining / 6000;

      if (data->upsAdvBattery->__upsAdvBatteryReplaceIndicator == 2)
         ups->set_replacebatt();
      else
         ups->clear_replacebatt();

      free(data->upsAdvBattery);
   }

   data->upsBasicInput = NULL;
   powernet_mib_mgr_get_upsBasicInput(s, &(data->upsBasicInput));
   if (data->upsBasicInput) {
      ups->InputPhase = data->upsBasicInput->__upsBasicInputPhase;
      free(data->upsBasicInput);
   }

   data->upsAdvInput = NULL;
   powernet_mib_mgr_get_upsAdvInput(s, &(data->upsAdvInput));
   if (data->upsAdvInput) {
      ups->LineVoltage = data->upsAdvInput->__upsAdvInputLineVoltage;
      ups->LineMax = data->upsAdvInput->__upsAdvInputMaxLineVoltage;
      ups->LineMin = data->upsAdvInput->__upsAdvInputMinLineVoltage;
      ups->LineFreq = data->upsAdvInput->__upsAdvInputFrequency;
      switch (data->upsAdvInput->__upsAdvInputLineFailCause) {
      case 1:
         ups->lastxfer = XFER_NONE;
         break;
      case 2:  /* High line voltage */
         ups->lastxfer = XFER_OVERVOLT;
         break;
      case 3:  /* Brownout */
      case 4:  /* Blackout */
         ups->lastxfer = XFER_UNDERVOLT;
         break;
      case 5:  /* Small sag */
      case 6:  /* Deep sag */
      case 7:  /* Small spike */
      case 8:  /* Deep spike */
         ups->lastxfer = XFER_NOTCHSPIKE;
         break;
      case 9:
         ups->lastxfer = XFER_SELFTEST;
         break;
      case 10:
         ups->lastxfer = XFER_RIPPLE;
         break;
      default:
         ups->lastxfer = XFER_UNKNOWN;
         break;
      }
      free(data->upsAdvInput);
   }

   data->upsBasicOutput = NULL;
   powernet_mib_mgr_get_upsBasicOutput(s, &(data->upsBasicOutput));
   if (data->upsBasicOutput) {
      /* Clear the following flags: only one status will be TRUE */
      Dmsg1(99, "Status before clearing: 0x%08x\n", ups->Status);
      ups->clear_online();
      ups->clear_onbatt();
      ups->clear_boost();
      ups->clear_trim();
      Dmsg1(99, "Status after clearing: 0x%08x\n", ups->Status);

      switch (data->upsBasicOutput->__upsBasicOutputStatus) {
      case 2:
         ups->set_online();
         break;
      case 3:
         ups->set_onbatt();
         break;
      case 4:
         ups->set_boost();
         break;
      case 12:
         ups->set_trim();
         break;
      case 1:                     /* unknown */
      case 5:                     /* timed sleeping */
      case 6:                     /* software bypass */
      case 7:                     /* UPS off */
      case 8:                     /* UPS rebooting */
      case 9:                     /* switched bypass */
      case 10:                    /* hardware failure bypass */
      case 11:                    /* sleeping until power returns */
      default:                    /* unknown */
         break;
      }
      ups->OutputPhase = data->upsBasicOutput->__upsBasicOutputPhase;
      free(data->upsBasicOutput);
   }

   data->upsAdvOutput = NULL;
   powernet_mib_mgr_get_upsAdvOutput(s, &(data->upsAdvOutput));
   if (data->upsAdvOutput) {
      ups->OutputVoltage = data->upsAdvOutput->__upsAdvOutputVoltage;
      ups->OutputFreq = data->upsAdvOutput->__upsAdvOutputFrequency;
      ups->UPSLoad = data->upsAdvOutput->__upsAdvOutputLoad;
      ups->OutputCurrent = data->upsAdvOutput->__upsAdvOutputCurrent;
      free(data->upsAdvOutput);
   }

   data->upsAdvTest = NULL;
   powernet_mib_mgr_get_upsAdvTest(s, &(data->upsAdvTest));
   if (data->upsAdvTest) {
      switch (data->upsAdvTest->__upsAdvTestDiagnosticsResults) {
      case 1:  /* Passed */
         ups->testresult = TEST_PASSED;
         break;
      case 2:  /* Failed */
      case 3:  /* Invalid test */
         ups->testresult = TEST_FAILED;
         break;
      case 4:  /* Test in progress */
         ups->testresult = TEST_INPROGRESS;
         break;
      default:
         ups->testresult = TEST_UNKNOWN;
         break;
      }

      /* Not implemented. Needs transform date(mm/dd/yy)->hours. */
      // ups->LastSTTime = data->upsAdvTest->upsAdvTestLastDiagnosticsDate;

      if (data->upsAdvTest->__upsAdvTestCalibrationResults == 3)
         ups->set_calibration();
      else
         ups->clear_calibration();

      free(data->upsAdvTest);
   }

   return 1;
}

/* Callback invoked by SNMP library when an async event arrives */
static int powernet_snmp_callback(
   int operation, snmp_session *session, 
   int reqid, snmp_pdu *pdu, void *magic)
{
   struct snmp_ups_internal_data *my_data =
      (struct snmp_ups_internal_data *)magic;

   Dmsg1(100, "powernet_snmp_callback: %d\n", reqid);

   if (reqid == 0)
      my_data->trap_received = true;

   return 1;
}

int powernet_snmp_ups_check_state(UPSINFO *ups)
{
   struct snmp_ups_internal_data *Sid =
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   fd_set fds;
   int numfds, rc, block;
   struct timeval tmo, exit, now;
   int sleep_time;

   /* Check for commlost under lock since UPS status might be changed */
   write_lock(ups);
   rc = powernet_check_comm_lost(ups);
   write_unlock(ups);
   if (rc == 0)
      return 0;

   sleep_time = ups->wait_time;

   /* If we're not doing SNMP traps, just sleep and exit */
   if (!Sid->trap_session) {
      sleep(sleep_time);
      return 1;
   }

   /* Figure out when we need to exit by */
   gettimeofday(&exit, NULL);
   exit.tv_sec += sleep_time;

   while(1)
   {
      /* Figure out how long until we have to exit */
      gettimeofday(&now, NULL);

      if (now.tv_sec > exit.tv_sec ||
         (now.tv_sec == exit.tv_sec &&
            now.tv_usec >= exit.tv_usec)) {
         /* Done already? How time flies... */
         return 0;
      }

      tmo.tv_sec = exit.tv_sec - now.tv_sec;
      tmo.tv_usec = exit.tv_usec - now.tv_usec;
      if (tmo.tv_usec < 0) {
         tmo.tv_sec--;              /* Normalize */
         tmo.tv_usec += 1000000;
      }

      /* Get select parameters from SNMP library */
      FD_ZERO(&fds);
      block = 0;
      numfds = 0;
      snmp_select_info(&numfds, &fds, &tmo, &block);

      /* Wait for something to happen */
      rc = select(numfds, &fds, NULL, NULL, &tmo);
      switch (rc) {
      case 0:  /* Timeout */
         /* Tell SNMP library about the timeout */
         snmp_timeout();
         break;

      case -1: /* Error */
         if (errno == EINTR || errno == EAGAIN)
            continue;            /* assume SIGCHLD */

         Dmsg1(200, "select error: ERR=%s\n", strerror(errno));
         return 0;

      default: /* Data available */
         /* Reset trap flag and run callback processing */
         Sid->trap_received = false;
         snmp_read(&fds);

         /* If callback processing set the flag, we got a trap */
         if (Sid->trap_received)
            return 1;

         break;
      }
   }
}

int powernet_snmp_ups_open(UPSINFO *ups)
{
   struct snmp_ups_internal_data *my_data =
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   struct snmp_session *s = &my_data->session;
   struct snmp_session tmp;

   /*
    * If we're configured to not use traps, simply rename
    * DeviceVendor to 'APC' and exit.
    */
   if (!strcmp(my_data->DeviceVendor, "APC_NOTRAP")) {
      my_data->DeviceVendor[3] = '\0';
      Dmsg0(100, "User requested no traps\n");
      return 1;
   }

   /* Trap session is a copy of client session with some tweaks */
   tmp = *s;
   tmp.peername = (char*)"0.0.0.0:162";  /* Listen to snmptrap port on all interfaces */
   tmp.local_port = 1;                   /* We're a server, not a client */
   tmp.callback = powernet_snmp_callback;
   tmp.callback_magic = my_data;

   /*
    * Open the trap session and store it in my_data.
    * It's ok if this fails; the code will fall back to polling.
    */
   my_data->trap_session = snmp_open(&tmp);
   if (!my_data->trap_session) {
      Dmsg0(100, "Trap session failed to open\n");
      return 1;
   }

   return 1;
}
