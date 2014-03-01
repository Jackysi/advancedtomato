/*
 * mib.cpp
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
#include "apc-oids.h"

using namespace Asn;

static struct CiOidMap CiOidMap[] =
{
//  CI                  OID                              type         dynamic?
   {CI_UPSMODEL,        upsBasicIdentModel,              OCTETSTRING, false},
   {CI_SERNO,           upsAdvIdentSerialNumber,         OCTETSTRING, false},
   {CI_IDEN,            upsBasicIdentName,               OCTETSTRING, false},
   {CI_REVNO,           upsAdvIdentFirmwareRevision,     OCTETSTRING, false},
   {CI_MANDAT,          upsAdvIdentDateOfManufacture,    OCTETSTRING, false},
   {CI_BATTDAT,         upsBasicBatteryLastReplaceDate,  OCTETSTRING, false},
   {CI_NOMBATTV,        upsAdvBatteryNominalVoltage,     INTEGER,     false},
   {CI_NOMOUTV,         upsAdvConfigRatedOutputVoltage,  INTEGER,     false},
   {CI_LTRANS,          upsAdvConfigLowTransferVolt,     INTEGER,     false},
   {CI_HTRANS,          upsAdvConfigHighTransferVolt,    INTEGER,     false},
   {CI_DWAKE,           upsAdvConfigReturnDelay,         TIMETICKS,   false},
   {CI_AlarmTimer,      upsAdvConfigAlarmTimer,          TIMETICKS,   false}, // Must be before CI_DALARM
   {CI_DALARM,          upsAdvConfigAlarm,               INTEGER,     false},
   {CI_DLBATT,          upsAdvConfigLowBatteryRunTime,   TIMETICKS,   false},
   {CI_DSHUTD,          upsAdvConfigShutoffDelay,        TIMETICKS,   false},
   {CI_RETPCT,          upsAdvConfigMinReturnCapacity,   INTEGER,     false},
   {CI_SENS,            upsAdvConfigSensitivity,         INTEGER,     false},
   {CI_EXTBATTS,        upsAdvBatteryNumOfBattPacks,     INTEGER,     false},
   {CI_STESTI,          upsAdvTestDiagnosticSchedule,    INTEGER,     false},
   {CI_VLINE,           upsAdvInputLineVoltage,          GAUGE,       true },
   {CI_VOUT,            upsAdvOutputVoltage,             GAUGE,       true },
   {CI_VBATT,           upsAdvBatteryActualVoltage,      INTEGER,     true },
   {CI_FREQ,            upsAdvInputFrequency,            GAUGE,       true },
   {CI_LOAD,            upsAdvOutputLoad,                GAUGE,       true },
   {CI_ITEMP,           upsAdvBatteryTemperature,        GAUGE,       true },
   {CI_ATEMP,           mUpsEnvironAmbientTemperature,   GAUGE,       true },
   {CI_HUMID,           mUpsEnvironRelativeHumidity,     GAUGE,       true },
   {CI_ST_STAT,         upsAdvTestDiagnosticsResults,    INTEGER,     true },
   {CI_BATTLEV,         upsAdvBatteryCapacity,           GAUGE,       true },
   {CI_RUNTIM,          upsAdvBatteryRunTimeRemaining,   TIMETICKS,   true },
   {CI_WHY_BATT,        upsAdvInputLineFailCause,        INTEGER,     true },
   {CI_BADBATTS,        upsAdvBatteryNumOfBadBattPacks,  INTEGER,     true },
   {CI_VMIN,            upsAdvInputMinLineVoltage,       GAUGE,       true },
   {CI_VMAX,            upsAdvInputMaxLineVoltage,       GAUGE,       true },

   // These 5 collectively are used to obtain the data for CI_STATUS.
   // All bits are available in upsBasicStateOutputState at once but 
   // the old AP960x cards do not appear to support that OID, so we use 
   // it only for the overload flag which is not available elsewhere.
   {CI_STATUS,          upsBasicOutputStatus,            INTEGER,     true },
   {CI_NeedReplacement, upsAdvBatteryReplaceIndicator,   INTEGER,     true },
   {CI_LowBattery,      upsBasicBatteryStatus,           INTEGER,     true },
   {CI_Calibration,     upsAdvTestCalibrationResults,    INTEGER,     true },
   {CI_Overload,        upsBasicStateOutputState,        OCTETSTRING, true },

   {-1, NULL, false}   /* END OF TABLE */
};

#define TIMETICKS_TO_SECS 100
#define SECS_TO_MINS      60

static void apc_update_ci(UPSINFO *ups, int ci, Snmp::Variable &data)
{
   static unsigned int alarmtimer = 0;

   switch (ci)
   {
   case CI_VLINE:
      Dmsg1(80, "Got CI_VLINE: %d\n", data.u32);
      ups->LineVoltage = data.u32;
      break;

   case CI_VOUT:
      Dmsg1(80, "Got CI_VOUT: %d\n", data.u32);
      ups->OutputVoltage = data.u32;
      break;

   case CI_VBATT:
      Dmsg1(80, "Got CI_VBATT: %d\n", data.u32);
      ups->BattVoltage = data.u32;
      break;

   case CI_FREQ:
      Dmsg1(80, "Got CI_FREQ: %d\n", data.u32);
      ups->LineFreq = data.u32;
      break;

   case CI_LOAD:
      Dmsg1(80, "Got CI_LOAD: %d\n", data.u32);
      ups->UPSLoad = data.u32;
      break;

   case CI_ITEMP:
      Dmsg1(80, "Got CI_ITEMP: %d\n", data.u32);
      ups->UPSTemp = data.u32;
      break;

   case CI_ATEMP:
      Dmsg1(80, "Got CI_ATEMP: %d\n", data.u32);
      ups->ambtemp = data.u32;
      break;

   case CI_HUMID:
      Dmsg1(80, "Got CI_HUMID: %d\n", data.u32);
      ups->humidity = data.u32;
      break;

   case CI_NOMBATTV:
      Dmsg1(80, "Got CI_NOMBATTV: %d\n", data.u32);
      ups->nombattv = data.u32;
      break;

   case CI_NOMOUTV:
      Dmsg1(80, "Got CI_NOMOUTV: %d\n", data.u32);
      ups->NomOutputVoltage = data.u32;
      break;

   case CI_NOMINV:
      Dmsg1(80, "Got CI_NOMINV: %d\n", data.u32);
      ups->NomInputVoltage = data.u32;
      break;

   case CI_NOMPOWER:
      Dmsg1(80, "Got CI_NOMPOWER: %d\n", data.u32);
      ups->NomPower = data.u32;
      break;

   case CI_LTRANS:
      Dmsg1(80, "Got CI_LTRANS: %d\n", data.u32);
      ups->lotrans = data.u32;
      break;

   case CI_HTRANS:
      Dmsg1(80, "Got CI_HTRANS: %d\n", data.u32);
      ups->hitrans = data.u32;
      break;

   case CI_DWAKE:
      Dmsg1(80, "Got CI_DWAKE: %d\n", data.u32);
      ups->dwake = data.u32;
      break;

   case CI_ST_STAT:
      Dmsg1(80, "Got CI_ST_STAT: %d\n", data.u32);
      switch (data.u32)
      {
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
      break;

   case CI_AlarmTimer:
      Dmsg1(80, "Got CI_AlarmTimer: %d\n", data.u32);
      // Remember alarm timer setting; we will use it for CI_DALARM
      alarmtimer = data.u32;
      break;

   case CI_DALARM:
      Dmsg1(80, "Got CI_DALARM: %d\n", data.u32);
      switch (data.u32)
      {
      case 1: // Timed (uses CI_AlarmTimer)
         if (ups->UPS_Cap[CI_AlarmTimer] && alarmtimer < 30)
            astrncpy(ups->beepstate, "0", sizeof(ups->beepstate)); // 5 secs
         else
            astrncpy(ups->beepstate, "T", sizeof(ups->beepstate)); // 30 secs
         break;
      case 2: // LowBatt
         astrncpy(ups->beepstate, "L", sizeof(ups->beepstate));
         break;
      case 3: // None
         astrncpy(ups->beepstate, "N", sizeof(ups->beepstate));
         break;
      default:
         astrncpy(ups->beepstate, "T", sizeof(ups->beepstate));
         break;
      }
      break;

   case CI_UPSMODEL:
      Dmsg1(80, "Got CI_UPSMODEL: %s\n", data.str.str());
      astrncpy(ups->upsmodel, data.str, sizeof(ups->upsmodel));
      break;

   case CI_SERNO:
      Dmsg1(80, "Got CI_SERNO: %s\n", data.str.str());
      astrncpy(ups->serial, data.str, sizeof(ups->serial));
      break;

   case CI_MANDAT:
      Dmsg1(80, "Got CI_MANDAT: %s\n", data.str.str());
      astrncpy(ups->birth, data.str, sizeof(ups->birth));
      break;

   case CI_BATTLEV:
      Dmsg1(80, "Got CI_BATTLEV: %d\n", data.u32);
      ups->BattChg = data.u32;
      break;

   case CI_RUNTIM:
      Dmsg1(80, "Got CI_RUNTIM: %d\n", data.u32);
      ups->TimeLeft = data.u32 / TIMETICKS_TO_SECS / SECS_TO_MINS;
      break;

   case CI_BATTDAT:
      Dmsg1(80, "Got CI_BATTDAT: %s\n", data.str.str());
      astrncpy(ups->battdat, data.str, sizeof(ups->battdat));
      break;

   case CI_IDEN:
      Dmsg1(80, "Got CI_IDEN: %s\n", data.str.str());
      astrncpy(ups->upsname, data.str, sizeof(ups->upsname));
      break;

   case CI_STATUS:
      Dmsg1(80, "Got CI_STATUS: %d\n", data.u32);
      /* Clear the following flags: only one status will be TRUE */
      ups->clear_online();
      ups->clear_onbatt();
      ups->clear_boost();
      ups->clear_trim();
      switch (data.u32) {
      case 2:
         ups->set_online();
         break;
      case 3:
         ups->set_onbatt();
         break;
      case 4:
         ups->set_online();
         ups->set_boost();
         break;
      case 12:
         ups->set_online();
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
      break;

   case CI_NeedReplacement:
      Dmsg1(80, "Got CI_NeedReplacement: %d\n", data.u32);
      if (data.u32 == 2)
         ups->set_replacebatt();
      else
         ups->clear_replacebatt();
      break;

   case CI_LowBattery:
      Dmsg1(80, "Got CI_LowBattery: %d\n", data.u32);
      if (data.u32 == 3)
         ups->set_battlow();
      else
         ups->clear_battlow();
      break;

   case CI_Calibration:
      Dmsg1(80, "Got CI_Calibration: %d\n", data.u32);
      if (data.u32 == 3)
         ups->set_calibration();
      else
         ups->clear_calibration();
      break;

   case CI_Overload:
      Dmsg1(80, "Got CI_Overload: %c\n", data.str[8]);
      if (data.str[8] == '1')
         ups->set_overload();
      else
         ups->clear_overload();
      break;

   case CI_DSHUTD:
      Dmsg1(80, "Got CI_DSHUTD: %d\n", data.u32);
      ups->dshutd = data.u32 / TIMETICKS_TO_SECS;
      break;

   case CI_RETPCT:
      Dmsg1(80, "Got CI_RETPCT: %d\n", data.u32);
      ups->rtnpct = data.u32;
      break;

   case CI_WHY_BATT:
      switch (data.u32)
      {
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
      break;

   case CI_SENS:
      Dmsg1(80, "Got CI_SENS: %d\n", data.u32);
      switch (data.u32)
      {
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
      break;

   case CI_REVNO:
      Dmsg1(80, "Got CI_REVNO: %s\n", data.str.str());
      astrncpy(ups->firmrev, data.str, sizeof(ups->firmrev));
      break;

   case CI_EXTBATTS:
      Dmsg1(80, "Got CI_EXTBATTS: %d\n", data.u32);
      ups->extbatts = data.u32;
      break;
   
   case CI_BADBATTS:
      Dmsg1(80, "Got CI_BADBATTS: %d\n", data.u32);
      ups->badbatts = data.u32;
      break;

   case CI_DLBATT:
      Dmsg1(80, "Got CI_DLBATT: %d\n", data.u32);
      ups->dlowbatt = data.u32 / TIMETICKS_TO_SECS / SECS_TO_MINS;
      break;

   case CI_STESTI:
      Dmsg1(80, "Got CI_STESTI: %d\n", data.u32);
      switch (data.u32) {
      case 2:
         astrncpy(ups->selftest, "336", sizeof(ups->selftest));
         break;
      case 3:
         astrncpy(ups->selftest, "168", sizeof(ups->selftest));
         break;
      case 4:
         astrncpy(ups->selftest, "ON", sizeof(ups->selftest));
         break;
      case 1:
      case 5:
      default:
         astrncpy(ups->selftest, "OFF", sizeof(ups->selftest));
         break;
      }
      break;

   case CI_VMIN:
      Dmsg1(80, "Got CI_VMIN: %d\n", data.u32);
      ups->LineMin = data.u32;
      break;

   case CI_VMAX:
      Dmsg1(80, "Got CI_VMAX: %d\n", data.u32);
      ups->LineMax = data.u32;
      break;
   }
}

static int apc_killpower(UPSINFO *ups)
{
   struct snmplite_ups_internal_data *sid =
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   Snmp::Variable var = { Asn::INTEGER, 2 };
   return sid->snmp->Set(upsBasicControlConserveBattery, &var);
}

static int apc_shutdown(UPSINFO *ups)
{
   struct snmplite_ups_internal_data *sid =
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   Snmp::Variable var = { Asn::INTEGER, 2 };
   return sid->snmp->Set(upsAdvControlUpsOff, &var);
}

// Export strategy to snmplite.cpp
struct MibStrategy ApcMibStrategy =
{
   "APC",
   CiOidMap,
   apc_update_ci,
   apc_killpower,
   apc_shutdown,
};
