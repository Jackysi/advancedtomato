/*
 * mib.cpp
 *
 * CI -> OID mapping for SNMP Lite UPS driver
 */

/*
 * Copyright (C) 2010 Adam Kropelin
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
#include "rfc1628-oids.h"

using namespace Asn;

static struct CiOidMap Rfc1628CiOidMap[] =
{
//  CI                  OID                                type         dynamic?
   {CI_UPSMODEL,        upsIdentModel,                     OCTETSTRING, false},
   {CI_IDEN,            upsIdentName,                      OCTETSTRING, false},
   {CI_REVNO,           upsIdentUPSSoftwareVersion,        OCTETSTRING, false},
// {CI_MANDAT,          upsAdvIdentDateOfManufacture,      OCTETSTRING, false},
// {CI_BATTDAT,         upsBasicBatteryLastReplaceDate,    OCTETSTRING, false},
// {CI_NOMBATTV,        upsAdvBatteryNominalVoltage,       INTEGER,     false},
   {CI_NOMOUTV,         upsConfigOutputVoltage,            INTEGER,     false},
   {CI_NOMINV,          upsConfigInputVoltage,             INTEGER,     false},
   {CI_NOMPOWER,        upsConfigOutputPower,              INTEGER,     false},
   {CI_LTRANS,          upsConfigLowVoltageTransferPoint,  INTEGER,     false},
   {CI_HTRANS,          upsConfigHighVoltageTransferPoint, INTEGER,     false},
// {CI_DWAKE,           upsAdvConfigReturnDelay,           TIMETICKS,   false},
   {CI_DALARM,          upsConfigAudibleStatus,            INTEGER,     false},
   {CI_DLBATT,          upsConfigLowBattTime,              INTEGER,     false},
// {CI_DSHUTD,          upsAdvConfigShutoffDelay,          TIMETICKS,   false},
// {CI_RETPCT,          upsAdvConfigMinReturnCapacity,     INTEGER,     false},
// {CI_SENS,            upsAdvConfigSensitivity,           INTEGER,     false},
// {CI_EXTBATTS,        upsAdvBatteryNumOfBattPacks,       INTEGER,     false},
// {CI_STESTI,          upsAdvTestDiagnosticSchedule,      INTEGER,     false},
   {CI_VLINE,           upsInputTableInputVoltage,         SEQUENCE,    true },
   {CI_VOUT,            upsOutputTableOutputVoltage,       SEQUENCE,    true },
   {CI_VBATT,           upsBatteryVoltage,                 INTEGER,     true },
   {CI_FREQ,            upsInputTableInputFrequency,       SEQUENCE,    true },
   {CI_LOAD,            upsOutputTableOutputPercentLoad,   SEQUENCE,    true },
   {CI_ITEMP,           upsBatteryTemperature,             INTEGER,     true },
// {CI_ATEMP,           mUpsEnvironAmbientTemperature,     GAUGE,       true },
// {CI_HUMID,           mUpsEnvironRelativeHumidity,       GAUGE,       true },
   {CI_ST_STAT,         upsTestResultsSummary,             INTEGER,     true },
   {CI_BATTLEV,         upsEstimatedChargeRemaining,       INTEGER,     true },
   {CI_RUNTIM,          upsEstimatedMinutesRemaining,      INTEGER,     true },
// {CI_WHY_BATT,        upsAdvInputLineFailCause,          INTEGER,     true },
// {CI_BADBATTS,        upsAdvBatteryNumOfBadBattPacks,    INTEGER,     true },
// {CI_VMIN,            upsAdvInputMinLineVoltage,         GAUGE,       true },
// {CI_VMAX,            upsAdvInputMaxLineVoltage,         GAUGE,       true },
   {CI_STATUS,          upsOutputSource,                   INTEGER,     true },
   {CI_LowBattery,      upsBatteryStatus,                  INTEGER,     true },

   {-1, NULL, false}   /* END OF TABLE */
};

static void rfc1628_update_ci(UPSINFO *ups, int ci, Snmp::Variable &data)
{
   switch (ci)
   {
   case CI_VLINE:
      // We just take the voltage from the first input line and ignore the rest
      Dmsg1(80, "Got CI_VLINE: %d\n", data.seq.begin()->u32);
      ups->LineVoltage = data.seq.begin()->u32;
      break;

   case CI_VOUT:
      // We just take the voltage from the first input line and ignore the rest
      Dmsg1(80, "Got CI_VOUT: %d\n", data.seq.begin()->u32);
      ups->OutputVoltage = data.seq.begin()->u32;
      break;

   case CI_VBATT:
      Dmsg1(80, "Got CI_VBATT: %d\n", data.u32);
      ups->BattVoltage = ((double)data.u32) / 10;
      break;

   case CI_FREQ:
      // We just take the freq from the first input line and ignore the rest
      Dmsg1(80, "Got CI_FREQ: %d\n", data.seq.begin()->u32);
      ups->LineFreq = ((double)data.seq.begin()->u32) / 10;
      break;

   case CI_LOAD:
      // MIB defines this as "The percentage of the UPS power capacity 
      // presently being used on this output line" so we should be able to
      // add all these up to get a total load for the UPS as a whole.
      // HOWEVER, manufacturers seem to actually be returning either the same
      // value in each slot (APC implementation of RFC1628) or percent of
      // power the line is capable of (Generex CS121 SNMP/WEB Adapter on a 
      // Newave Conceptpower DPA UPS). So we will average the values and hope
      // for the best.
      ups->UPSLoad = 0;
      for (alist<Snmp::Variable>::iterator iter = data.seq.begin();
           iter != data.seq.end();
           ++iter)
      {
         Dmsg1(80, "Got CI_LOAD: %d\n", iter->u32);
         ups->UPSLoad += iter->u32;
      }
      ups->UPSLoad /= data.seq.size();
      break;

   case CI_ITEMP:
      Dmsg1(80, "Got CI_ITEMP: %d\n", data.u32);
      ups->UPSTemp = data.u32;
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

   case CI_ST_STAT:
      Dmsg1(80, "Got CI_ST_STAT: %d\n", data.u32);
      switch (data.u32)
      {
      case 1:  /* Passed */
         ups->testresult = TEST_PASSED;
         break;
      case 2:  /* Warning */
         ups->testresult = TEST_WARNING;
         break;
      case 3:  /* Error */
         ups->testresult = TEST_FAILED;
         break;
      case 5:  /* Test in progress */
         ups->testresult = TEST_INPROGRESS;
         break;
      case 4:  /* Aborted */
      case 6:  /* No test initiated */
         ups->testresult = TEST_NONE;
         break;
      default:
         ups->testresult = TEST_UNKNOWN;
         break;
      }
      break;

   case CI_DALARM:
      Dmsg1(80, "Got CI_DALARM: %d\n", data.u32);
      switch (data.u32)
      {
      case 1: // Disabled ("None")
         astrncpy(ups->beepstate, "N", sizeof(ups->beepstate));
         break;
      case 2: // Enabled (T = 30 seconds...just a guess)
      case 3: // Muted (but enabled)
      default:
         astrncpy(ups->beepstate, "T", sizeof(ups->beepstate));
         break;
      }
      break;

   case CI_UPSMODEL:
      Dmsg1(80, "Got CI_UPSMODEL: %s\n", data.str.str());
      astrncpy(ups->upsmodel, data.str, sizeof(ups->upsmodel));
      break;

   case CI_BATTLEV:
      Dmsg1(80, "Got CI_BATTLEV: %d\n", data.u32);
      ups->BattChg = data.u32;
      break;

   case CI_RUNTIM:
      Dmsg1(80, "Got CI_RUNTIM: %d\n", data.u32);
      ups->TimeLeft = data.u32;
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
      case 3:
         ups->set_online();
         break;
      case 5:
         ups->set_onbatt();
         break;
      case 6:
         ups->set_online();
         ups->set_boost();
         break;
      case 7:
         ups->set_online();
         ups->set_trim();
         break;
      case 1:                     /* other */
      case 2:                     /* output turned off */
      case 4:                     /* bypass */
      default:                    /* unknown */
         break;
      }
      break;

   case CI_LowBattery:
      Dmsg1(80, "Got CI_LowBattery: %d\n", data.u32);
      switch (data.u32)
      {
      default:
      case 1:  // Unknown
      case 2:  // Normal
         ups->clear_battlow();
         break;
      case 3:  // Low
      case 4:  // Depleted
         ups->set_battlow();
         break;
      }
      break;

   case CI_REVNO:
      Dmsg1(80, "Got CI_REVNO: %s\n", data.str.str());
      astrncpy(ups->firmrev, data.str, sizeof(ups->firmrev));
      break;

   case CI_DLBATT:
      Dmsg1(80, "Got CI_DLBATT: %d\n", data.u32);
      ups->dlowbatt = data.u32;
      break;
   }
}

static int rfc1628_killpower(UPSINFO *ups)
{
   struct snmplite_ups_internal_data *sid =
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   // Configure UPS to turn off output only (not entire UPS)
   Snmp::Variable shutdownType = { Asn::INTEGER, 1 };
   sid->snmp->Set(upsShutdownType, &shutdownType);

   // Configure UPS to automatically restart when power is restored
   Snmp::Variable autoRestart = { Asn::INTEGER, 1 };
   sid->snmp->Set(upsAutoRestart, &autoRestart);

   // Instruct UPS to turn off after 60 secs
   Snmp::Variable shutdownDelay = { Asn::INTEGER, 60 };
   sid->snmp->Set(upsShutdownAfterDelay, &shutdownDelay);

   return 0;
}

static int rfc1628_shutdown(UPSINFO *ups)
{
   struct snmplite_ups_internal_data *sid =
      (struct snmplite_ups_internal_data *)ups->driver_internal_data;

   // Configure UPS to turn off entire system
   Snmp::Variable shutdownType = { Asn::INTEGER, 2 };
   sid->snmp->Set(upsShutdownType, &shutdownType);

   // Configure UPS to NOT automatically restart when power is restored
   Snmp::Variable autoRestart = { Asn::INTEGER, 2 };
   sid->snmp->Set(upsAutoRestart, &autoRestart);

   // Instruct UPS to turn off after 60 secs
   Snmp::Variable shutdownDelay = { Asn::INTEGER, 60 };
   sid->snmp->Set(upsShutdownAfterDelay, &shutdownDelay);

   return 0;
}

// Export strategy to snmplite.cpp
struct MibStrategy Rfc1628MibStrategy =
{
   "RFC",
   Rfc1628CiOidMap,
   rfc1628_update_ci,
   rfc1628_killpower,
   rfc1628_shutdown,
};
