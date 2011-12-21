/*
 * mge_mib.cpp
 *
 * CI -> OID mapping for SNMP Lite UPS driver
 */

/*
 * Copyright (C) 2009 Adam Kropelin
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
#include "snmplite-common.h"
#include "mibs.h"
#include "mge-oids.h"

using namespace Asn;

static struct CiOidMap MGE_CiOidMap[] =
{
//  CI                  OID                               type         dynamic?
   {CI_UPSMODEL,        upsmgIdentFamilyName,             OCTETSTRING, false},
   {CI_STATUS,          upsmgOutputOnBattery,             INTEGER,     true },
   {CI_WHY_BATT,        upsmgInputLineFailCause,          INTEGER,     true },
   {CI_ST_STAT,         upsmgTestDiagResult,              INTEGER,     true },
   {CI_VLINE,           mginputVoltage,                   SEQUENCE,    true },
   {CI_VMAX,            mginputMaximumVoltage,            SEQUENCE,    true },
   {CI_VMIN,            mginputMinimumVoltage,            SEQUENCE,    true },
   {CI_VOUT,            mgoutputVoltage,                  SEQUENCE,    true },
   {CI_BATTLEV,         upsmgBatteryLevel,                INTEGER,     true },
   {CI_VBATT,           upsmgBatteryVoltage,              INTEGER,     true },
   {CI_LOAD,            mgoutputLoadPerPhase,             SEQUENCE,    true },
   {CI_FREQ,            mginputFrequency,                 SEQUENCE,    true },
   {CI_RUNTIM,          upsmgBatteryRemainingTime,        INTEGER,     true },
   {CI_ITEMP,           upsmgBatteryTemperature,          INTEGER,     true },
   {CI_DWAKE,           mgreceptacleRestartDelay,         INTEGER,     false},
   {CI_DSHUTD,          upsmgConfigSysShutDuration,       INTEGER,     false},
   {CI_LTRANS,          upsmgConfigLowTransfer,           INTEGER,     false},
   {CI_HTRANS,          upsmgConfigHighTransfer,          INTEGER,     false},
   {CI_RETPCT,          upsmgConfigMinRechargeLevel,      INTEGER,     false},
   {CI_AlarmTimer,      upsmgConfigAlarmTimeDelay,        INTEGER,     false}, // before CI_DALARM !
   {CI_DALARM,          upsmgConfigAlarmAudible,          INTEGER,     false},
   {CI_DLBATT,          upsmgConfigLowBatteryTime,        INTEGER,     false},
// {CI_IDEN,            upsmgIdentModelName,              OCTETSTRING, false},
   {CI_STESTI,          upsmgTestBatterySchedule,         INTEGER,     false},
   {CI_SERNO,           upsmgIdentSerialNumber,           OCTETSTRING, false},
   {CI_NOMBATTV,        upsmgConfigNominalBatteryVoltage, INTEGER,     false},
   {CI_HUMID,           upsmgEnvironAmbientHumidity,      INTEGER,     true },
   {CI_REVNO,           upsmgIdentFirmwareVersion,        OCTETSTRING, false}, // Version + SN of net card
   {CI_ATEMP,           upsmgEnvironAmbientTemp,          INTEGER,     true },
   {CI_NOMOUTV,         upsmgConfigOutputNominalVoltage,  INTEGER,     false},
   {CI_Boost,           upsmgOutputOnBoost,               INTEGER,     true },
   {CI_Trim,            upsmgOutputOnBuck,                INTEGER,     true },
   {CI_Overload,        upsmgOutputOverLoad,              INTEGER,     true },
   {CI_NeedReplacement, upsmgBatteryReplacement,          INTEGER,     true },
   {CI_LowBattery,      upsmgBatteryLowBattery,           INTEGER,     true },

   {-1, NULL, false}   /* END OF TABLE */
};


static void mge_update_ci(UPSINFO *ups, int ci, Snmp::Variable &data)
{
   static int alarmtimer = 0;

   switch (ci)
   {
   case CI_UPSMODEL:
      Dmsg1(80, "Got CI_UPSMODEL: %s\n", data.str.str());
      astrncpy(ups->upsmodel, data.str, sizeof(ups->upsmodel));
      break;

   case CI_STATUS:
      Dmsg1(80, "Got CI_STATUS: %d\n", data.u32);
      if (data.u32 == 1)
      {
         ups->set_onbatt();
         ups->clear_online();
      } else {
         ups->set_online();
         ups->clear_onbatt();
      }
      break;

   case CI_WHY_BATT:
      switch (data.u32)
      {
      case 1:
         ups->lastxfer = XFER_NONE;
         break;
      case 2:  /* voltage out of tolerance */
         ups->lastxfer = XFER_NOTCHSPIKE;
         break;
      case 3:  /* freq out of tolerance */
         ups->lastxfer = XFER_FREQ;
         break;
      case 4:  /* No power on line */
      default:
         ups->lastxfer = XFER_UNKNOWN;
         break;
      }
      break;

   case CI_ST_STAT:
      Dmsg1(80, "Got CI_ST_STAT: %d\n", data.u32);
      switch (data.u32)
      {
      case 1:  /* Passed */
         ups->testresult = TEST_PASSED;
         break;
      case 2:  /* Failed */
         ups->testresult = TEST_FAILED;
         break;
      default:
         ups->testresult = TEST_UNKNOWN;
         break;
      }
      break;

   case CI_VLINE: // take only first value of sequence
      if (data.seq.begin() != data.seq.end()) {
         ups->LineVoltage = ((double)data.seq.begin()->u32) / 10;
         for (alist<Snmp::Variable>::iterator iter = data.seq.begin();
              iter != data.seq.end();
              ++iter)
            Dmsg1(80, "Got CI_VLINE: %d\n", iter->u32);
      } else {
         Dmsg0(80, "CI_VLINE: empty reply received");
      }
      break;

   case CI_VMAX: // take maximum of sequence values
      if (data.seq.begin() != data.seq.end()) {
         ups->LineMax = 0;
         for (alist<Snmp::Variable>::iterator iter = data.seq.begin();
              iter != data.seq.end();
              ++iter) {
            Dmsg1(80, "Got CI_VMAX: %d\n", iter->u32);
            if (ups->LineMax < iter->u32)
               ups->LineMax = iter->u32;
         }
         ups->LineMax /= 10;
         Dmsg1(80, "CI_VMAX= %g\n", ups->LineMax);
      } else {
         Dmsg0(80, "CI_VMAX: empty reply received");
      }
      break;

   case CI_VMIN: // take minimum of sequence values
      if (data.seq.begin() != data.seq.end()) {
         ups->LineMin = data.seq.begin()->u32;
         for (alist<Snmp::Variable>::iterator iter = data.seq.begin();
              iter != data.seq.end();
              ++iter) {
            Dmsg1(80, "Got CI_VMIN: %d\n", iter->u32);
            if (ups->LineMin > iter->u32)
               ups->LineMin = iter->u32;
         }
         ups->LineMin /= 10;
         Dmsg1(80, "CI_VMIN= %g\n", ups->LineMin);
      } else {
         Dmsg0(80, "CI_VMIN: empty reply received");
      }
      break;

   case CI_VOUT: // take only first value of sequence
      if (data.seq.begin() != data.seq.end()) {
         ups->OutputVoltage = ((double)data.seq.begin()->u32) / 10;
         for (alist<Snmp::Variable>::iterator iter = data.seq.begin();
              iter != data.seq.end();
              ++iter)
            Dmsg1(80, "Got CI_VOUT: %d\n", iter->u32);
      } else {
         Dmsg0(80, "CI_VMIN: empty reply received");
      }
      break;

   case CI_BATTLEV:
      Dmsg1(80, "Got CI_BATTLEV: %d\n", data.u32);
      ups->BattChg = data.u32;
      break;

   case CI_VBATT:
      Dmsg1(80, "Got CI_VBATT: %d\n", data.u32);
      ups->BattVoltage = ((double)data.u32) / 10;
      break;

   case CI_LOAD:
      // caclulate the arithmetic average of all sequence values
      if (data.seq.begin() != data.seq.end()) {
         ups->UPSLoad = 0;
         for (alist<Snmp::Variable>::iterator iter = data.seq.begin();
              iter != data.seq.end();
              ++iter)
         {
            Dmsg1(80, "Got CI_LOAD: %d\n", iter->u32);
            ups->UPSLoad += iter->u32;
         }
         ups->UPSLoad /= data.seq.size();
         Dmsg1(80, "CI_LOAD= %g\n", ups->UPSLoad);
      } else {
         Dmsg0(80, "CI_LOAD: empty reply received");
      }
      break;

   case CI_FREQ: // take only first value of sequence
      if (data.seq.begin() != data.seq.end()) {
         ups->LineFreq = ((double)data.seq.begin()->u32) / 10;
         for (alist<Snmp::Variable>::iterator iter = data.seq.begin();
              iter != data.seq.end();
              ++iter)
            Dmsg1(80, "Got CI_FREQ: %d\n", iter->u32);
      } else {
         Dmsg0(80, "CI_FREQ: empty reply received");
      }
      break;

   case CI_RUNTIM:
      Dmsg1(80, "Got CI_RUNTIM: %d\n", data.u32);
      ups->TimeLeft = ((double)data.u32) / 60;
      break;

   case CI_ITEMP:
      Dmsg1(80, "Got CI_ITEMP: %d\n", data.u32);
      ups->UPSTemp = data.u32;
      break;

   case CI_DWAKE:
      Dmsg1(80, "Got CI_DWAKE: %d\n", data.u32);
      ups->dwake = data.u32;
      break;

   case CI_DSHUTD:
      Dmsg1(80, "Got CI_DSHUTD: %d\n", data.u32);
      ups->dshutd = data.u32;
      break;

   case CI_LTRANS:
      Dmsg1(80, "Got CI_LTRANS: %d\n", data.u32);
      ups->lotrans = data.u32;
      break;

   case CI_HTRANS:
      Dmsg1(80, "Got CI_HTRANS: %d\n", data.u32);
      ups->hitrans = data.u32;
      break;

   case CI_RETPCT:
      Dmsg1(80, "Got CI_RETPCT: %d\n", data.u32);
      ups->rtnpct = data.u32;
      break;

   case CI_DALARM:
      Dmsg1(80, "Got CI_DALARM: %d\n", data.u32);
      if (data.u32 == 1)
         if (alarmtimer)
            astrncpy(ups->beepstate, "Timed", sizeof(ups->beepstate));
         else
            astrncpy(ups->beepstate, "NoDelay", sizeof(ups->beepstate));
      else
         astrncpy(ups->beepstate, "NoAlarm", sizeof(ups->beepstate));
      break;

   case CI_DLBATT:
      Dmsg1(80, "Got CI_DLBATT: %d\n", data.u32);
      ups->dlowbatt = data.u32;
      break;

   case CI_IDEN:
      Dmsg1(80, "Got CI_IDEN: %s\n", data.str.str());
      astrncpy(ups->upsname, data.str, sizeof(ups->upsname));
      break;

   case CI_STESTI:
      Dmsg1(80, "Got CI_STESTI: %d\n", data.u32);
      switch (data.u32) {
      case 2:
         astrncpy(ups->selftest, "weekly", sizeof(ups->selftest));
         break;
      case 3:
         astrncpy(ups->selftest, "monthly", sizeof(ups->selftest));
         break;
      case 4:
         astrncpy(ups->selftest, "atTurnOn", sizeof(ups->selftest));
         break;
      case 6:
         astrncpy(ups->selftest, "daily", sizeof(ups->selftest));
         break;
      case 1: // unknown
      case 5: // none
      default:
         astrncpy(ups->selftest, "OFF", sizeof(ups->selftest));
         break;
      }
      break;

   case CI_SERNO:
      Dmsg1(80, "Got CI_SERNO: %s\n", data.str.str());
      astrncpy(ups->serial, data.str, sizeof(ups->serial));
      break;

   case CI_NOMBATTV:
      Dmsg1(80, "Got CI_NOMBATTV: %d\n", data.u32);
      ups->nombattv = ((double)data.u32) / 10;
      break;

   case CI_HUMID:
      Dmsg1(80, "Got CI_HUMID: %d\n", data.u32);
      ups->humidity = data.u32;
      break;

   case CI_REVNO:
      Dmsg1(80, "Got CI_REVNO: %s\n", data.str.str());
      astrncpy(ups->firmrev, data.str, sizeof(ups->firmrev));
      break;

   case CI_ATEMP:
      Dmsg1(80, "Got CI_ATEMP: %d\n", data.u32);
      ups->ambtemp = data.u32;
      break;

   case CI_NOMOUTV:
      Dmsg1(80, "Got CI_NOMOUTV: %d\n", data.u32);
      ups->NomOutputVoltage = data.u32 / 10;
      break;

   case CI_Boost:
      Dmsg1(200, "Got CI_Boost: %d\n", data.u32);
      if (data.u32 == 1)
         ups->set_boost();
      else
         ups->clear_boost();
      break;

   case CI_Trim:
      Dmsg1(200, "Got CI_Trim(Buck): %d\n", data.u32);
      if (data.u32 == 1)
         ups->set_trim();
      else
         ups->clear_trim();
      break;

   case CI_Overload:
      Dmsg1(80, "Got CI_Overload: %d\n", data.u32);
      if (data.u32 == 1)
         ups->set_overload();
      else
         ups->clear_overload();
      break;

   case CI_NeedReplacement:
      Dmsg1(80, "Got CI_NeedReplacement: %d\n", data.u32);
      if (data.u32 == 1)
         ups->set_replacebatt();
      else
         ups->clear_replacebatt();
      break;

   case CI_LowBattery:
      Dmsg1(80, "Got CI_LowBattery: %d\n", data.u32);
      if (data.u32 == 1)
         ups->set_battlow();
      else
         ups->clear_battlow();
      break;

   case CI_AlarmTimer:
      Dmsg1(80, "Got CI_AlarmTimer: %d\n", data.u32);
      // Remember alarm timer setting; we will use it for CI_DALARM
      alarmtimer = data.u32;
      break;
   }
}

// Export strategy to snmplite.cpp
struct MibStrategy MGEMibStrategy =
{
   "MGE",
   MGE_CiOidMap,
   mge_update_ci,
   NULL,
   NULL,
};
