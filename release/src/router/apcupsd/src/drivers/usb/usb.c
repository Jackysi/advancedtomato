/*
 * usb.c
 *
 * Public driver interface for all platform USB drivers.
 *
 * Based on linux-usb.c by Kern Sibbald
 */

/*
 * Copyright (C) 2001-2004 Kern Sibbald
 * Copyright (C) 2004-2005 Adam Kropelin
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
#include "usb.h"
#include "usb_common.h"
#include <math.h>

/* Implemented in platform-specific code */
int pusb_ups_get_capabilities(UPSINFO *ups, const struct s_known_info *known_info);
int pusb_ups_open(UPSINFO *ups);
int pusb_ups_close(UPSINFO *ups);
int pusb_get_value(UPSINFO *ups, int ci, USB_VALUE *uval);
int pusb_ups_check_state(UPSINFO *ups);
int pusb_ups_setup(UPSINFO *ups);
int pusb_write_int_to_ups(UPSINFO *ups, int ci, int value, const char *name);
int pusb_read_int_from_ups(UPSINFO *ups, int ci, int *value);

/*
 * A certain semi-ancient BackUPS Pro model breaks the USB spec in a
 * particularly creative way: Some reports read back in ASCII instead of
 * binary. And one value that should be in seconds is returned in minutes
 * instead, just for fun. We detect and work around the breakage.
 */
#define QUIRK_OLD_BACKUPS_PRO_MODEL_STRING "BackUPS Pro 500 FW:16.3.D USB FW:4"
static bool quirk_old_backups_pro = false;

/*
 * This table is used when walking through the USB reports to see
 * what information found in the UPS that we want. If the usage_code 
 * and the physical code match, then we make an entry in the command
 * index table containing the usage information provided by the UPS
 * as well as the data type from this table. Entries in the table 
 * with ci == CI_NONE are not used, for the moment, they are
 * retained just so they are not forgotten.
 */
const struct s_known_info known_info[] = {
   /*  Page 0x84 is the Power Device Page */
   /* CI                        USAGE       PHYSICAL   LOGICAL  TYPE        VOLATILE? */
   {CI_NONE,                    0x00840001, P_ANY,     P_ANY,   T_INDEX,    false},  /* iName */
   {CI_VLINE,                   0x00840030, P_INPUT,   P_ANY,   T_UNITS,    true },  /* Line Voltage */
   {CI_VOUT,                    0x00840030, P_OUTPUT,  P_ANY,   T_UNITS,    true },  /* Output Voltage */
   {CI_VBATT,                   0x00840030, P_BATTERY, P_ANY,   T_UNITS,    true },  /* Battery Voltage */
   {CI_VBATT,                   0x00840030, P_ANY,     P_PWSUM, T_UNITS,    true },  /* Battery Voltage (alternative) */
   {CI_NONE,                    0x00840031, P_ANY,     P_ANY,   T_UNITS,    false},  /* Current */
   {CI_FREQ,                    0x00840032, P_OUTPUT,  P_ANY,   T_UNITS,    true },  /* Frequency */
   {CI_NONE,                    0x00840033, P_ANY,     P_ANY,   T_UNITS,    false},  /* ApparentPower */
   {CI_NONE,                    0x00840034, P_ANY,     P_ANY,   T_UNITS,    false},  /* ActivePower */
   {CI_LOAD,                    0x00840035, P_ANY,     P_ANY,   T_UNITS,    true },  /* PercentLoad */
   {CI_ITEMP,                   0x00840036, P_BATTERY, P_ANY,   T_UNITS,    true },  /* Internal Temperature */
   {CI_ATEMP,                   0x00840036, P_APC1,    P_ANY,   T_UNITS,    true },  /* Ambient Temperature */
   {CI_HUMID,                   0x00840037, P_ANY,     P_ANY,   T_UNITS,    true },  /* Humidity */
   {CI_NOMBATTV,                0x00840040, P_BATTERY, P_ANY,   T_UNITS,    false},  /* ConfigVoltage (battery) */
   {CI_NOMBATTV,                0x00840040, P_ANY,     P_PWSUM, T_UNITS,    false},  /* ConfigVoltage (battery, alternate) */
   {CI_NOMOUTV,                 0x00840040, P_OUTPUT,  P_ANY,   T_UNITS,    false},  /* ConfigVoltage (output) */
   {CI_NOMINV,                  0x00840040, P_INPUT,   P_ANY,   T_UNITS,    false},  /* ConfigVoltage (input) */
   {CI_NONE,                    0x00840042, P_ANY,     P_ANY,   T_UNITS,    false},  /* ConfigFrequency */
   {CI_NONE,                    0x00840043, P_ANY,     P_ANY,   T_UNITS,    false},  /* ConfigApparentPower */
   {CI_NOMPOWER,                0x00840044, P_ANY,     P_ANY,   T_UNITS,    false},  /* ConfigActivePower */
   {CI_LTRANS,                  0x00840053, P_ANY,     P_ANY,   T_UNITS,    false},  /* LowVoltageTransfer */
   {CI_HTRANS,                  0x00840054, P_ANY,     P_ANY,   T_UNITS,    false},  /* HighVoltageTransfer */
   {CI_DelayBeforeReboot,       0x00840055, P_ANY,     P_ANY,   T_UNITS,    false},  /* DelayBeforeReboot */
   {CI_DWAKE,                   0x00840056, P_ANY,     P_ANY,   T_UNITS,    false},  /* DelayBeforeStartup */
   {CI_DelayBeforeShutdown,     0x00840057, P_ANY,     P_ANY,   T_UNITS,    false},  /* DelayBeforeShutdown */
   {CI_ST_STAT,                 0x00840058, P_ANY,     P_ANY,   T_NONE,     false},  /* Test */
   {CI_DALARM,                  0x0084005a, P_ANY,     P_ANY,   T_NONE,     true },  /* AudibleAlarmControl */
   {CI_NONE,                    0x00840061, P_ANY,     P_ANY,   T_NONE,     false},  /* Good */
   {CI_IFailure,                0x00840062, P_ANY,     P_ANY,   T_NONE,     false},  /* InternalFailure */
   {CI_PWVoltageOOR,            0x00840063, P_ANY,     P_ANY,   T_NONE,     false},  /* Volt out-of-range */
   {CI_PWFrequencyOOR,          0x00840064, P_ANY,     P_ANY,   T_NONE,     false},  /* Freq out-of-range */
   {CI_Overload,                0x00840065, P_ANY,     P_ANY,   T_NONE,     true },  /* Overload */
   {CI_OverCharged,             0x00840066, P_ANY,     P_ANY,   T_NONE,     false},  /* Overcharged */
   {CI_OverTemp,                0x00840067, P_ANY,     P_ANY,   T_NONE,     false},  /* Overtemp */
   {CI_ShutdownRequested,       0x00840068, P_ANY,     P_ANY,   T_NONE,     false},  /* ShutdownRequested */
   {CI_ShutdownImminent,        0x00840069, P_ANY,     P_ANY,   T_NONE,     true },  /* ShutdownImminent */
   {CI_NONE,                    0x0084006b, P_ANY,     P_ANY,   T_NONE,     false},  /* Switch On/Off */
   {CI_NONE,                    0x0084006c, P_ANY,     P_ANY,   T_NONE,     false},  /* Switchable */
   {CI_Boost,                   0x0084006e, P_ANY,     P_ANY,   T_NONE,     true },  /* Boost */
   {CI_Trim,                    0x0084006f, P_ANY,     P_ANY,   T_NONE,     true },  /* Buck */
   {CI_CommunicationLost,       0x00840073, P_ANY,     P_ANY,   T_NONE,     false},  /* CommunicationLost */
   {CI_Manufacturer,            0x008400fd, P_ANY,     P_ANY,   T_INDEX,    false},  /* iManufacturer */
   {CI_UPSMODEL,                0x008400fe, P_ANY,     P_ANY,   T_INDEX,    false},  /* iProduct */
   {CI_SERNO,                   0x008400ff, P_ANY,     P_ANY,   T_INDEX,    false},  /* iSerialNumber */
   {CI_MANDAT,                  0x00850085, P_ANY,     P_PWSUM, T_DATE,     false},  /* ManufactureDate */

   /*  Page 0x85 is the Battery System Page */
   /* CI                        USAGE       PHYSICAL   LOGICAL  TYPE        VOLATILE? */
   {CI_RemCapLimit,             0x00850029, P_ANY,     P_ANY,   T_CAPACITY, false},  /* RemCapLimit */
   {CI_RemTimeLimit,            0x0085002a, P_ANY,     P_ANY,   T_UNITS,    false},  /* RemTimeLimit */
   {CI_NONE,                    0x0085002c, P_ANY,     P_ANY,   T_CAPACITY, false},  /* CapacityMode */
   {CI_BelowRemCapLimit,        0x00850042, P_ANY,     P_ANY,   T_NONE,     true },  /* BelowRemCapLimit */
   {CI_RemTimeLimitExpired,     0x00850043, P_ANY,     P_ANY,   T_NONE,     true },  /* RemTimeLimitExpired */
   {CI_Charging,                0x00850044, P_ANY,     P_ANY,   T_NONE,     false},  /* Charging */
   {CI_Discharging,             0x00850045, P_ANY,     P_ANY,   T_NONE ,    true },  /* Discharging */
   {CI_NeedReplacement,         0x0085004b, P_ANY,     P_ANY,   T_NONE ,    true },  /* NeedReplacement */
   {CI_BATTLEV,                 0x00850066, P_ANY,     P_ANY,   T_CAPACITY, true },  /* RemainingCapacity */
   {CI_NONE,                    0x00850067, P_ANY,     P_ANY,   T_CAPACITY, false},  /* FullChargeCapacity */
   {CI_RUNTIM,                  0x00850068, P_ANY,     P_ANY,   T_UNITS,    true },  /* RunTimeToEmpty */
   {CI_CycleCount,              0x0085006b, P_ANY,     P_ANY,   T_NONE,     false},
   {CI_BattPackLevel,           0x00850080, P_ANY,     P_ANY,   T_NONE,     false},  /* BattPackLevel */
   {CI_NONE,                    0x00850083, P_ANY,     P_ANY,   T_CAPACITY, false},  /* DesignCapacity */
   {CI_BATTDAT,                 0x00850085, P_BATTERY, P_ANY,   T_DATE,     false},  /* ManufactureDate */
   {CI_IDEN,                    0x00850088, P_ANY,     P_ANY,   T_INDEX,    false},  /* iDeviceName */
   {CI_NONE,                    0x00850089, P_ANY,     P_ANY,   T_INDEX,    false},  /* iDeviceChemistry */
   {CI_NONE,                    0x0085008b, P_ANY,     P_ANY,   T_NONE,     false},  /* Rechargeable */
   {CI_WarningCapacityLimit,    0x0085008c, P_ANY,     P_ANY,   T_CAPACITY, false},  /* WarningCapacityLimit */
   {CI_NONE,                    0x0085008d, P_ANY,     P_ANY,   T_CAPACITY, false},  /* CapacityGranularity1 */
   {CI_NONE,                    0x0085008e, P_ANY,     P_ANY,   T_CAPACITY, false},  /* CapacityGranularity2 */
   {CI_NONE,                    0x0085008f, P_ANY,     P_ANY,   T_INDEX,    false},  /* iOEMInformation */
   {CI_ACPresent,               0x008500d0, P_ANY,     P_ANY,   T_NONE,     true },  /* ACPresent */
   {CI_BatteryPresent,          0x008500d1, P_ANY,     P_ANY,   T_NONE,     true },  /* BatteryPresent */
   {CI_ChargerVoltageOOR,       0x008500d8, P_ANY,     P_ANY,   T_NONE,     false},  /* Volt out-of-range */
   {CI_ChargerCurrentOOR,       0x008500d9, P_ANY,     P_ANY,   T_NONE,     false},  /* Current out-of-range */
   {CI_CurrentNotRegulated,     0x008500da, P_ANY,     P_ANY,   T_NONE,     false},  /* Current not regulated */
   {CI_VoltageNotRegulated,     0x008500db, P_ANY,     P_ANY,   T_NONE,     false},  /* VoltageNotRegulated */

   /*  Pages 0xFF00 to 0xFFFF are vendor specific */
   /* CI                        USAGE       PHYSICAL   LOGICAL  TYPE        VOLATILE? */
   {CI_STESTI,                  0xFF86001a, P_ANY,     P_ANY,   T_NONE,     false},  /* APCSelfTestInterval */
   {CI_STATUS,                  0xFF860060, P_ANY,     P_ANY,   T_BITS,     true },  /* APCStatusFlag */
   {CI_DSHUTD,                  0xFF860076, P_ANY,     P_ANY,   T_UNITS,    false},  /* APCShutdownAfterDelay */
   {CI_NONE,                    0xFF860005, P_ANY,     P_ANY,   T_NONE,     false},  /* APCGeneralCollection */
   {CI_APCForceShutdown,        0xFF86007C, P_ANY,     P_ANY,   T_NONE,     false},  /* APCForceShutdown */
   {CI_TESTALARM,               0xFF860072, P_ANY,	    P_ANY,   T_NONE,     false},  /* APCTestAlarm */
// Removed the below due to all recent UPSes having the same garbage in this field
// {CI_BattReplaceDate,         0xFF860016, P_ANY,     P_ANY,   T_APCDATE,  false},  /* APCBattReplaceDate */
   {CI_NONE,                    0xFF860042, P_ANY,     P_ANY,   T_NONE,     false},  /* APC_UPS_FirmwareRevision */
   {CI_NONE,                    0xFF860079, P_ANY,     P_ANY,   T_NONE,     false},  /* APC_USB_FirmwareRevision */
   {CI_RETPCT,                  0xFF860019, P_ANY,     P_ANY,   T_CAPACITY, false},  /* APCBattCapBeforeStartup */
   {CI_APCDelayBeforeStartup,   0xFF86007E, P_ANY,     P_ANY,   T_UNITS,    false},  /* APCDelayBeforeStartup */
   {CI_APCDelayBeforeShutdown,  0xFF86007D, P_ANY,     P_ANY,   T_UNITS,    false},  /* APCDelayBeforeShutdown */
   {CI_APCLineFailCause,        0xFF860052, P_ANY,     P_ANY,   T_NONE,     true},   /* APCLineFailCause */
   {CI_SENS,                    0xFF860061, P_ANY,     P_ANY,   T_NONE,     false},  /* APCSensitivity */
   {CI_BUPBattCapBeforeStartup, 0x00860012, P_ANY,     P_ANY,   T_NONE,     false},  /* BUPBattCapBeforeStartup */
   {CI_BUPDelayBeforeStartup,   0x00860076, P_ANY,     P_ANY,   T_NONE,     false},  /* BUPDelayBeforeStartup */
   {CI_BUPSelfTest,             0x00860010, P_ANY,     P_ANY,   T_NONE,     false},  /* BUPSelfTest */
   {CI_BUPHibernate,            0x00850058, P_ANY,     P_ANY,   T_NONE,     false},  /* BUPHibernate */
   
   /* END OF TABLE */
   {CI_NONE,                    0x00000000, P_ANY,     P_ANY,   T_NONE,     false}   /* END OF TABLE */
};

/*
 * USB USAGE NOTES
 *
 * From the NUT project   
 *    
 *  0x860060 == "441HMLL" - looks like a 'capability' string     
 *           == locale 4, 4 choices, 1 byte each                 
 *           == line sensitivity (high, medium, low, low)        
 *  NOTE! the above does not seem to correspond to my info 
 *
 *  0x860013 == 44200155090 - capability again                   
 *           == locale 4, 4 choices, 2 bytes, 00, 15, 50, 90     
 *           == minimum charge to return online                  
 *
 *  0x860062 == D43133136127130                                  
 *           == locale D, 4 choices, 3 bytes, 133, 136, 127, 130 
 *           == high transfer voltage                            
 *
 *  0x860064 == D43103100097106                                  
 *           == locale D, 4 choices, 3 bytes, 103, 100, 097, 106 
 *           == low transfer voltage                             
 *
 *  0x860066 == 441HMLL (see 860060)                                   
 *
 *  0x860074 == 4410TLN                                          
 *           == locale 4, 4 choices, 1 byte, 0, T, L, N          
 *           == alarm setting (5s, 30s, low battery, none)       
 *
 *  0x860077 == 443060180300600                                  
 *           == locale 4, 4 choices, 3 bytes, 060,180,300,600    
 *           == wake-up delay (after power returns)              
 *
 *
 * From MGE -- MGE specific items
 *
 * TestPeriod                      0xffff0045
 * RemainingCapacityLimitSetting   0xffff004d
 * LowVoltageBoostTransfer         0xffff0050
 * HighVoltageBoostTransfer        0xffff0051
 * LowVoltageBuckTransfer          0xffff0052
 * HighVoltageBuckTransfer         0xffff0053
 * iModel                          0xffff00f0
 */


/*
 * Operations that must be handled by platform-specific code
 */

int usb_ups_check_state(UPSINFO *ups)
{
   return pusb_ups_check_state(ups);
}

int usb_ups_open(UPSINFO *ups)
{
   return pusb_ups_open(ups);
}

int usb_ups_close(UPSINFO *ups)
{
   return pusb_ups_close(ups);
}

int usb_ups_setup(UPSINFO *ups)
{
   return pusb_ups_setup(ups);
}

int usb_write_int_to_ups(UPSINFO *ups, int ci, int value, const char *name)
{
   return pusb_write_int_to_ups(ups, ci, value, name);
}

int usb_read_int_from_ups(UPSINFO *ups, int ci, int *value)
{
   return pusb_read_int_from_ups(ups, ci, value);
}

/* Fetch the given CI from the UPS */
#define URB_DELAY_MS 20
static bool usb_get_value(UPSINFO *ups, int ci, USB_VALUE *uval)
{
   static struct timeval prev = {0};
   struct timeval now;
   struct timespec delay;
   int diff;

   /*
    * Some UPSes (650 CS and 800 RS, possibly others) lock up if
    * control transfers are issued too quickly, so we throttle a
    * bit here.
    */
   if (prev.tv_sec) {
      gettimeofday(&now, NULL);
      diff = TV_DIFF_MS(prev, now);
      if (diff >= 0 && diff < URB_DELAY_MS) {
         delay.tv_sec = 0;
         delay.tv_nsec = (URB_DELAY_MS-diff)*1000000;
         nanosleep(&delay, NULL);
      }
   }
   gettimeofday(&prev, NULL);

   return pusb_get_value(ups, ci, uval);
}

int usb_ups_get_capabilities(UPSINFO *ups)
{
   int rc;

   /* Run platform-specific capabilities code */   
   rc = pusb_ups_get_capabilities(ups, known_info);
   if (!rc)
      return 0;

   /*
    * If the hardware supports CI_Discharging, ignore CI_ACPresent.
    * Some hardware (RS 1500, possibly others) reports confusing
    * values for these during self test. (Discharging=1 && ACPresent=1)
    */
   if (ups->UPS_Cap[CI_Discharging])
      ups->UPS_Cap[CI_ACPresent] = false;

   /*
    * Disable CI_NOMPOWER if UPS does not report it accurately.
    * Several models appear to always return 0 for this value.
    */
   USB_VALUE uval;
   if (ups->UPS_Cap[CI_NOMPOWER] &&
       (!usb_get_value(ups, CI_NOMPOWER, &uval) ||
        ((int)uval.dValue == 0))) {
      Dmsg0(100, "NOMPOWER disabled due to invalid reading from UPS\n");
      ups->UPS_Cap[CI_NOMPOWER] = false;
   }

   /* Detect broken BackUPS Pro model */
   quirk_old_backups_pro = false;
   if (ups->UPS_Cap[CI_UPSMODEL] && usb_get_value(ups, CI_UPSMODEL, &uval)) {
      Dmsg1(250, "Checking for BackUPS Pro quirk \"%s\"\n", uval.sValue);
      if (!strcmp(uval.sValue, QUIRK_OLD_BACKUPS_PRO_MODEL_STRING)) {
         quirk_old_backups_pro = true;
         Dmsg0(100, "BackUPS Pro quirk enabled\n");
      }
   }

   return 1;
}


/*
 * Operations that are not supported
 */

int usb_ups_program_eeprom(UPSINFO *ups, int command, const char *data)
{
   /* We don't support this for USB */
   return 0;
}


/*
 * Operations which are platform agnostic and therefore can be
 * implemented here
 */

/*
 * Given a CI and a raw uval, update the UPSINFO structure with the
 * new value. Special handling for certain BackUPS Pro reports.
 *
 * Thanks to David Fries <David@Fries.net> for this code.
 */
static bool usb_process_value_bup(UPSINFO* ups, int ci, USB_VALUE* uval)
{
   int val = (int)uval->dValue;
   char digits[] = { (val>>16) & 0xff, (val>>8) & 0xff, val & 0xff, 0 };

   /* UPS_RUNTIME_LEFT */
   if(ci == CI_RUNTIM)
   {
      ups->TimeLeft = uval->dValue; /* already minutes */
      Dmsg1(200, "TimeLeft = %d\n", (int)ups->TimeLeft);
      return true;
   }

   /* Bail if this value doesn't look to be ASCII encoded */
   if(!isdigit(digits[0]) || !isdigit(digits[1]) || !isdigit(digits[2]))
      return false;

   switch(ci)
   {
   /* UPS_LOAD */
   case CI_LOAD:
      ups->UPSLoad = atoi(digits);
      Dmsg1(200, "UPSLoad = %d\n", (int)ups->UPSLoad);
      return true;

   /* LOW_TRANSFER_LEVEL */
   case CI_LTRANS:
      ups->lotrans = atoi(digits);
      return true;

   /* HIGH_TRANSFER_LEVEL */
   case CI_HTRANS:
      ups->hitrans = atoi(digits);
      return true;

   /* LINE_FREQ */
   case CI_FREQ:
      ups->LineFreq = atoi(digits);
      return true;

   default:
      return false;
   }
}

/*
 * Given a CI and a raw uval, update the UPSINFO structure with the
 * new value.
 */
static void usb_process_value(UPSINFO* ups, int ci, USB_VALUE* uval)
{
   int v, yy, mm, dd;
   char *p;
   static int bpcnt = 0;

   /*
    * If BackUPS Pro quirk is enabled, try special decoding. If special decode
    * fails, we continue with the normal protocol.
    */
   if (quirk_old_backups_pro && usb_process_value_bup(ups, ci, uval))
      return;

   /*
    * ADK FIXME: This switch statement is really excessive. Consider
    * breaking it into volatile vs. non-volatile or perhaps an array
    * of function pointers with handler functions for each CI.
    */

   switch(ci)
   {
   /* UPS_STATUS -- this is the most important status for apcupsd */
   case CI_STATUS:
      ups->Status &= ~0xff;
      ups->Status |= uval->iValue & 0xff;
      Dmsg1(200, "Status=0x%08x\n", ups->Status);
      break;

   case CI_ACPresent:
      if (uval->iValue)
         ups->set_online();
      Dmsg1(200, "ACPresent=%d\n", uval->iValue);
      break;

   case CI_Discharging:
      ups->set_online(!uval->iValue);
      Dmsg1(200, "Discharging=%d\n", uval->iValue);
      break;

   case CI_BelowRemCapLimit:
      if (uval->iValue)
         ups->set_battlow();
      Dmsg1(200, "BelowRemCapLimit=%d\n", uval->iValue);
      break;

   case CI_RemTimeLimitExpired:
      if (uval->iValue)
         ups->set_battlow();
      Dmsg1(200, "RemTimeLimitExpired=%d\n", uval->iValue);
      break;

   case CI_ShutdownImminent:
      if (uval->iValue)
         ups->set_battlow();
      Dmsg1(200, "ShutdownImminent=%d\n", uval->iValue);
      break;

   case CI_Boost:
      if (uval->iValue)
         ups->set_boost();
      Dmsg1(200, "Boost=%d\n", uval->iValue);
      break;

   case CI_Trim:
      if (uval->iValue)
         ups->set_trim();
      Dmsg1(200, "Trim=%d\n", uval->iValue);
      break;

   case CI_Overload:
      if (uval->iValue)
         ups->set_overload();
      Dmsg1(200, "Overload=%d\n", uval->iValue);
      break;

   case CI_NeedReplacement:
      if (uval->iValue)
         ups->set_replacebatt(uval->iValue);
      Dmsg1(200, "ReplaceBatt=%d\n", uval->iValue);
      break;

   /* LINE_VOLTAGE */
   case CI_VLINE:
      ups->LineVoltage = uval->dValue;
      Dmsg1(200, "LineVoltage = %d\n", (int)ups->LineVoltage);
      break;

   /* OUTPUT_VOLTAGE */
   case CI_VOUT:
      ups->OutputVoltage = uval->dValue;
      Dmsg1(200, "OutputVoltage = %d\n", (int)ups->OutputVoltage);
      break;

   /* BATT_FULL Battery level percentage */
   case CI_BATTLEV:
      ups->BattChg = uval->dValue;
      Dmsg1(200, "BattCharge = %d\n", (int)ups->BattChg);
      break;

   /* BATT_VOLTAGE */
   case CI_VBATT:
      ups->BattVoltage = uval->dValue;
      Dmsg1(200, "BattVoltage = %d\n", (int)ups->BattVoltage);
      break;

   /* UPS_LOAD */
   case CI_LOAD:
      ups->UPSLoad = uval->dValue;
      Dmsg1(200, "UPSLoad = %d\n", (int)ups->UPSLoad);
      break;

   /* LINE_FREQ */
   case CI_FREQ:
      ups->LineFreq = uval->dValue;
      Dmsg1(200, "LineFreq = %d\n", (int)ups->LineFreq);
      break;

   /* UPS_RUNTIME_LEFT */
   case CI_RUNTIM:
      ups->TimeLeft = uval->dValue / 60; /* convert to minutes */
      Dmsg1(200, "TimeLeft = %d\n", (int)ups->TimeLeft);
      break;

   /* UPS_TEMP */
   case CI_ITEMP:
      ups->UPSTemp = uval->dValue - 273.15;      /* convert to deg C. */
      Dmsg1(200, "ITemp = %d\n", (int)ups->UPSTemp);
      break;

   /*  Humidity percentage */ 
   case CI_HUMID:
      ups->humidity = uval->dValue;
      Dmsg1(200, "Humidity = %d\n", (int)ups->humidity);
      break;

   /*  Ambient temperature */ 
   case CI_ATEMP:
      ups->ambtemp = uval->dValue - 273.15;      /* convert to deg C. */;
      Dmsg1(200, "ATemp = %d\n", (int)ups->ambtemp);
      break;

   /* Self test results */
   case CI_ST_STAT:
      switch (uval->iValue) {
      case 1:  /* Passed */
         ups->testresult = TEST_PASSED;
         break;
      case 2:  /* Warning */
         ups->testresult = TEST_WARNING;
         break;
      case 3:  /* Error */
      case 4:  /* Aborted */
         ups->testresult = TEST_FAILED;
         break;
      case 5:  /* In progress */
         ups->testresult = TEST_INPROGRESS;
         break;
      case 6:  /* None */
         ups->testresult = TEST_NONE;
         break;
      default:
         ups->testresult = TEST_UNKNOWN;
         break;
      }
      break;

   /* Self test interval */
   case CI_STESTI:
      switch (uval->iValue) {
      case 0:
         astrncpy(ups->selftest, "None", sizeof(ups->selftest));
         break;
      case 1:
         astrncpy(ups->selftest, "Power On", sizeof(ups->selftest));
         break;
      case 2:
         astrncpy(ups->selftest, "7 days", sizeof(ups->selftest));
         break;
      default:
         astrncpy(ups->selftest, "14 days", sizeof(ups->selftest));
         break;
      }
      break;

   /* Reason for last xfer to battery */
   case CI_APCLineFailCause:
      Dmsg1(100, "CI_APCLineFailCause=%d\n", uval->iValue);
      switch (uval->iValue) {
      case 0:  /* No transfers have ocurred */
         ups->lastxfer = XFER_NONE;
         break;
      case 2:  /* High line voltage */
         ups->lastxfer = XFER_OVERVOLT;
         break;
      case 3:  /* Ripple */
         ups->lastxfer = XFER_RIPPLE;
         break;
      case 1:  /* Low line voltage */
      case 4:  /* notch, spike, or blackout */
      case 8:  /* Notch or blackout */
      case 9:  /* Spike or blackout */
         ups->lastxfer = XFER_UNDERVOLT;
         break;
      case 6:  /* DelayBeforeShutdown or APCDelayBeforeShutdown */
      case 10: /* Graceful shutdown by accessories */
         ups->lastxfer = XFER_FORCED;
         break;
      case 7: /* Input frequency out of range */
         ups->lastxfer = XFER_FREQ;
         break;
      case 5:  /* Self Test or Discharge Calibration commanded thru */
               /* Test usage, front button, or 2 week self test */
      case 11: /* Test usage invoked */
      case 12: /* Front button initiated self test */
      case 13: /* 2 week self test */
         ups->lastxfer = XFER_SELFTEST;
         break;
      default:
         ups->lastxfer = XFER_UNKNOWN;
         break;
      }
      break;

   /* Battery connected/disconnected */
   case CI_BatteryPresent:
      /*
       * Work around a firmware bug in some models (RS 1500,
       * possibly others) where BatteryPresent=1 is sporadically
       * reported while the battery is disconnected. The work-
       * around is to ignore BatteryPresent=1 until we see it
       * at least twice in a row. The down side of this approach
       * is that legitimate BATTATTCH events are unnecessarily
       * delayed. C'est la vie.
       *
       * ADK FIXME: 'bpcnt' should be kept in the UPS structure
       * in order to allow multiple UPSes to be managed by this
       * driver. To avoid bloating UPSINFO with USB-specific
       * junk we really need a USB private structure akin to
       * USB_INFO in the platform specific drivers. The URB 
       * delay timer in usb_get_value() could also live in such
       * a structure.
       */
      if (uval->iValue) {
         if (bpcnt++)
            ups->set_battpresent();
      } else {
         bpcnt = 0;
         ups->clear_battpresent();
      }
      Dmsg1(200, "BatteryPresent=%d\n", uval->iValue);
      break;

   /* UPS_NAME */
   case CI_IDEN:
      if (ups->upsname[0] == 0 && uval->sValue[0] != 0)
         astrncpy(ups->upsname, uval->sValue, sizeof(ups->upsname));
      break;

   /* model, firmware */
   case CI_UPSMODEL:
      /* Truncate Firmware info on APC Product string */
      if ((p = strstr(uval->sValue, "FW:"))) {
         *p = '\0';           // Terminate model name
         p += 3;              // Skip "FW:"
         while (isspace(*p))  // Skip whitespace after "FW:"
            p++;
         astrncpy(ups->firmrev, p, sizeof(ups->firmrev));
         ups->UPS_Cap[CI_REVNO] = true;
      }

      /* Kill leading whitespace on model name */
      p = uval->sValue;
      while (isspace(*p))
         p++;

      astrncpy(ups->upsmodel, p, sizeof(ups->upsmodel));
      break;

   /* WAKEUP_DELAY */
   case CI_DWAKE:
      ups->dwake = (int)uval->dValue;
      break;

   /* SLEEP_DELAY */
   case CI_DSHUTD:
      ups->dshutd = (int)uval->dValue;
      break;

   /* LOW_TRANSFER_LEVEL */
   case CI_LTRANS:
      ups->lotrans = (int)uval->dValue;
      break;

   /* HIGH_TRANSFER_LEVEL */
   case CI_HTRANS:
      ups->hitrans = (int)uval->dValue;
      break;

   /* UPS_BATT_CAP_RETURN */
   case CI_RETPCT:
      ups->rtnpct = (int)uval->dValue;
      break;

   /* LOWBATT_SHUTDOWN_LEVEL */
   case CI_DLBATT:
      ups->dlowbatt = (int)uval->dValue;
      break;

   /* UPS_MANUFACTURE_DATE */
   case CI_MANDAT:
      asnprintf(ups->birth, sizeof(ups->birth), "%4d-%02d-%02d",
         (uval->iValue >> 9) + 1980, (uval->iValue >> 5) & 0xF,
         uval->iValue & 0x1F);
      break;

   /* Last UPS_BATTERY_REPLACE */
   case CI_BATTDAT:
      asnprintf(ups->battdat, sizeof(ups->battdat), "%4d-%02d-%02d",
         (uval->iValue >> 9) + 1980, (uval->iValue >> 5) & 0xF,
         uval->iValue & 0x1F);
      break;

   /* APC_BATTERY_DATE */
   case CI_BattReplaceDate:
      v = uval->iValue;
      yy = ((v >> 4) & 0xF) * 10 + (v & 0xF) + 2000;
      v >>= 8;
      dd = ((v >> 4) & 0xF) * 10 + (v & 0xF);
      v >>= 8;
      mm = ((v >> 4) & 0xF) * 10 + (v & 0xF);
      asnprintf(ups->battdat, sizeof(ups->battdat), "%4d-%02d-%02d", yy, mm, dd);
      break;

   /* UPS_SERIAL_NUMBER */
   case CI_SERNO:
      astrncpy(ups->serial, uval->sValue, sizeof(ups->serial));

      /*
       * If serial number has garbage, trash it.
       */
      for (p = ups->serial; *p; p++) {
         if (*p < ' ' || *p > 'z') {
            *ups->serial = 0;
            ups->UPS_Cap[CI_SERNO] = false;
         }
      }
      break;

   /* Nominal output voltage when on batteries */
   case CI_NOMOUTV:
      ups->NomOutputVoltage = (int)uval->dValue;
      while (ups->NomOutputVoltage > 1000)
         ups->NomOutputVoltage /= 10; // Some UPSes get the units wrong
      break;

   /* Nominal input voltage */
   case CI_NOMINV:
      ups->NomInputVoltage = (int)uval->dValue;
      while (ups->NomInputVoltage > 1000)
         ups->NomInputVoltage /= 10; // Some UPSes get the units wrong
      break;

   /* Nominal battery voltage */
   case CI_NOMBATTV:
      ups->nombattv = uval->dValue;
      break;

   /* Nominal power */
   case CI_NOMPOWER:
      ups->NomPower = (int)uval->dValue;
      break;

   /* Sensitivity */
   case CI_SENS:
      switch (uval->iValue) {
      case 0:
         astrncpy(ups->sensitivity, "Low", sizeof(ups->sensitivity));
         break;
      case 1:
         astrncpy(ups->sensitivity, "Medium", sizeof(ups->sensitivity));
         break;
      case 2:
         astrncpy(ups->sensitivity, "High", sizeof(ups->sensitivity));
         break;
      default:
         astrncpy(ups->sensitivity, "Unknown", sizeof(ups->sensitivity));
         break;
      }
      break;

   case CI_DALARM:
      switch (uval->iValue) {
      case 1: // Never
         astrncpy(ups->beepstate, "N", sizeof(ups->beepstate));
         break;
      case 2: // 30 seconds
      default:
         astrncpy(ups->beepstate, "T", sizeof(ups->beepstate));
         break;
      }
      break;

   default:
      break;
   }
}

/* Fetch the given CI from the UPS and update the UPSINFO structure */
static bool usb_update_value(UPSINFO* ups, int ci)
{
   USB_VALUE uval;

   if (!usb_get_value(ups, ci, &uval))
      return false;
      
   usb_process_value(ups, ci, &uval);
   return true;
}

/* Process commands from the main loop */
int usb_ups_entry_point(UPSINFO *ups, int command, void *data)
{
   struct timespec delay = {0, 40000000};

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
      nanosleep(&delay, NULL); /* Give UPS a chance to update the value */
      if (usb_update_value(ups, CI_WHY_BATT) ||
          usb_update_value(ups, CI_APCLineFailCause))
      {
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
      nanosleep(&delay, NULL); /* Give UPS a chance to update the value */
      return usb_update_value(ups, CI_ST_STAT);

   default:
      return FAILURE;
   }

   return SUCCESS;
}

/*
 * Read UPS info that changes -- e.g. voltage, temperature, etc.
 *
 * This routine is called once every N seconds to get a current
 * idea of what the UPS is doing.
 */
int usb_ups_read_volatile_data(UPSINFO *ups)
{
   time_t last_poll = ups->poll_time;
   time_t now = time(NULL);

   Dmsg0(200, "Enter usb_ups_read_volatile_data\n");

   /* 
    * If we are not on batteries, update this maximum once every
    * MAX_VOLATILE_POLL_RATE seconds. This prevents flailing around
    * too much if the UPS state is rapidly changing while on mains.
    */
   if (ups->is_onbatt() && last_poll &&
       (now - last_poll < MAX_VOLATILE_POLL_RATE)) {
      return 1;
   }

   write_lock(ups);
   ups->poll_time = now;           /* save time stamp */

   /* Clear APC status bits; let the various CIs set them again */
   ups->Status &= ~0xFF;

   /* Loop through all known data, polling those marked volatile */
   for (int i=0; known_info[i].usage_code; i++) {
      if (known_info[i].isvolatile && known_info[i].ci != CI_NONE)
         usb_update_value(ups, known_info[i].ci);
   }

   write_unlock(ups);
   return 1;
}

/*
 * Read UPS info that remains unchanged -- e.g. transfer voltages, 
 * shutdown delay, etc.
 *
 * This routine is called once when apcupsd is starting.
 */
int usb_ups_read_static_data(UPSINFO *ups)
{
   write_lock(ups);

   /* Loop through all known data, polling those marked non-volatile */
   for (int i=0; known_info[i].usage_code; i++) {
      if (!known_info[i].isvolatile && known_info[i].ci != CI_NONE)
         usb_update_value(ups, known_info[i].ci);
   }

   write_unlock(ups);
   return 1;
}

/*
 * How long to wait before killing output power.
 * This value is NOT used on BackUPS Pro models.
 */
#define SHUTDOWN_DELAY  60

/*
 * How many seconds of good utility power before turning output back on.
 * This value is NOT used on BackUPS Pro models.
 */
#define STARTUP_DELAY   10

/*
 * What percentage battery charge before turning output back on.
 * On at least some models this must be a multiple of 15%.
 * This value is NOT used on BackUPS Pro models.
 */
#define STARTUP_PERCENT 0

int usb_ups_kill_power(UPSINFO *ups)
{
   const char *func;
   int hibernate = 0;
   int val;

   Dmsg0(200, "Enter usb_ups_kill_power\n");

   /*
    * We try various different ways to put the UPS into hibernation
    * mode (i.e. killpower). Some of these commands are not supported
    * on all UPSes, but that should cause no harm.
    */

   /*
    * First, set required battery capacity before startup to 0 so UPS
    * will not wait for the battery to charge before turning back on.
    * Not all UPSes have this capability, so this setting is allowed
    * to fail. The value we program here should be made configurable
    * some day.
    */
   if (UPS_HAS_CAP(CI_RETPCT)) {
      func = "CI_RETPCT";
      if (!usb_write_int_to_ups(ups, CI_RETPCT, STARTUP_PERCENT, func))
         Dmsg1(100, "Unable to set %s (not an error)\n", func);
   }

   /*
    * BackUPS Pro uses an enumerated setting (reads percent in 
    * ASCII). The value advances to the next higher setting by
    * writing a '1' and to the next lower setting when writing a 
    * '2'. The value wraps around when advanced past the max or min
    * setting.
    *
    * We walk the setting down to the minimum of 0.
    *
    * Credit goes to John Zielinski <grim@undead.cc> for figuring
    * this out.
    */
   if (UPS_HAS_CAP(CI_BUPBattCapBeforeStartup)) {
      if (pusb_read_int_from_ups(ups, CI_BUPBattCapBeforeStartup, &val)) {
         func = "CI_BUPBattCapBeforeStartup";
         switch (val) {
         case 0x3930:             /* 90% */
            pusb_write_int_to_ups(ups, CI_BUPBattCapBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x3630:             /* 60% */
            pusb_write_int_to_ups(ups, CI_BUPBattCapBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x3135:             /* 15% */
            pusb_write_int_to_ups(ups, CI_BUPBattCapBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x3030:             /* 00% */
            break;

         default:
            Dmsg1(100, "Unknown BUPBattCapBeforeStartup value (%04x)\n", val);
            break;
         }
      }
   }

   /*
    * Second, set the length of time to wait after power returns
    * before starting up. We set it to something pretty low, but it
    * seems the UPS rounds this value up to the nearest multiple of
    * 60 seconds. Not all UPSes have this capability, so this setting
    * is allowed to fail.  The value we program here should be made
    * configurable some day.
    */
   if (UPS_HAS_CAP(CI_APCDelayBeforeStartup)) {
      func = "CI_APCDelayBeforeStartup";
      if (!usb_write_int_to_ups(ups, CI_APCDelayBeforeStartup, STARTUP_DELAY, func)) {
         Dmsg1(100, "Unable to set %s (not an error)\n", func);
      }
   }

   /*
    * BackUPS Pro uses an enumerated setting (reads seconds in ASCII).
    * The value advances to the next higher setting by writing a '1' 
    * and to the next lower setting when writing a '2'. The value 
    * wraps around when advanced past the max or min setting.
    *
    * We walk the setting down to the minimum of 60.
    *
    * Credit goes to John Zielinski <grim@undead.cc> for figuring
    * this out.
    */
   if (UPS_HAS_CAP(CI_BUPDelayBeforeStartup)) {
      if (pusb_read_int_from_ups(ups, CI_BUPDelayBeforeStartup, &val)) {
         func = "CI_BUPDelayBeforeStartup";
         switch (val) {
         case 0x363030:           /* 600 sec */
            pusb_write_int_to_ups(ups, CI_BUPDelayBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x333030:           /* 300 sec */
            pusb_write_int_to_ups(ups, CI_BUPDelayBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x313830:           /* 180 sec */
            pusb_write_int_to_ups(ups, CI_BUPDelayBeforeStartup, 2, func);
            /* Falls thru... */
         case 0x3630:             /* 60 sec */
            break;

         default:
            Dmsg1(100, "Unknown CI_BUPDelayBeforeStartup value (%04x)\n", val);
            break;
         }
      }
   }

   /*
    * BackUPS hibernate
    *
    * Alternately, if APCDelayBeforeShutdown is available, setting 
    * it will start a countdown after which the UPS will hibernate.
    */
   if (!hibernate && UPS_HAS_CAP(CI_APCDelayBeforeShutdown)) {
      Dmsg0(000, "UPS appears to support BackUPS style hibernate.\n");
      func = "CI_APCDelayBeforeShutdown";
      if (!usb_write_int_to_ups(ups, CI_APCDelayBeforeShutdown,
            SHUTDOWN_DELAY, func)) {
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      } else {
         hibernate = 1;
      }
   }

   /*
    * SmartUPS hibernate
    *
    * If both DWAKE and DelayBeforeShutdown are available, trigger
    * a hibernate by writing DWAKE a few seconds longer than 
    * DelayBeforeShutdown. ORDER IS IMPORTANT. The write to 
    * DelayBeforeShutdown starts both timers ticking down and the
    * UPS will hibernate when DelayBeforeShutdown hits zero.
    */
   if (!hibernate && UPS_HAS_CAP(CI_DWAKE) && UPS_HAS_CAP(CI_DelayBeforeShutdown)) {
      Dmsg0(000, "UPS appears to support SmartUPS style hibernate.\n");
      func = "CI_DWAKE";
      if (!usb_write_int_to_ups(ups, CI_DWAKE, SHUTDOWN_DELAY + 4, func)) {
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      } else {
         func = "CI_DelayBeforeShutdown";
         if (!usb_write_int_to_ups(ups, CI_DelayBeforeShutdown,
               SHUTDOWN_DELAY, func)) {
            Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
            /* reset prev timer */
            usb_write_int_to_ups(ups, CI_DWAKE, -1, "CI_DWAKE");  
         } else {
            hibernate = 1;
         }
      }
   }

   /*
    * BackUPS Pro shutdown
    *
    * Here we see the BackUPS Pro further distinguish itself as 
    * having the most broken firmware of any APC product yet. We have
    * to trigger two magic boolean flags using APC custom usages.
    * First we hit BUPHibernate and follow that with a write to 
    * BUPSelfTest (!).
    *
    * Credit goes to John Zielinski <grim@undead.cc> for figuring 
    * this out.
    */
   if (!hibernate && UPS_HAS_CAP(CI_BUPHibernate) && UPS_HAS_CAP(CI_BUPSelfTest)) {
      Dmsg0(000, "UPS appears to support BackUPS Pro style hibernate.\n");
      func = "CI_BUPHibernate";
      if (!pusb_write_int_to_ups(ups, CI_BUPHibernate, 1, func)) {
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      } else {
         func = "CI_BUPSelfTest";
         if (!pusb_write_int_to_ups(ups, CI_BUPSelfTest, 1, func)) {
            Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
            pusb_write_int_to_ups(ups, CI_BUPHibernate, 0, "CI_BUPHibernate");
         } else {
            hibernate = 1;
         }
      }
   }

   /*
    * All UPSes tested so far are covered by one of the above cases. 
    * However, there are a some other ways to hibernate.
    */

   /*
    * Misc method A
    *
    * Writing CI_DelayBeforeReboot starts a countdown timer, after 
    * which the UPS will hibernate. If utility power is out, the UPS
    * will stay hibernating until power is restored. SmartUPSes seem
    * to support this method, but PowerChute uses the dual countdown
    * method above, so we prefer that one. UPSes seem to round the
    * value up to 90 seconds if it is any lower. Note that the
    * behavior described here DOES NOT comply with the standard set
    * out in the HID Usage Tables for Power Devices spec. 
    */
   if (!hibernate && UPS_HAS_CAP(CI_DelayBeforeReboot)) {
      Dmsg0(000, "UPS appears to support DelayBeforeReboot style hibernate.\n");

      func = "CI_DelayBeforeReboot";
      if (!usb_write_int_to_ups(ups, CI_DelayBeforeReboot, SHUTDOWN_DELAY, func))
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      else
         hibernate = 1;
   }

   /*
    * Misc method B
    *
    * We can set CI_APCForceShutdown to true (it's a boolean flag).
    * We have no control over how long the UPS waits before turning
    * off. Experimentally it seems to be about 60 seconds. Some 
    * BackUPS models support this in addition to the preferred 
    * BackUPS method above. It's included here "just in case".
    */
   if (!hibernate && UPS_HAS_CAP(CI_APCForceShutdown)) {
      Dmsg0(000, "UPS appears to support ForceShutdown style hibernate.\n");

      func = "CI_APCForceShutdown";
      if (!usb_write_int_to_ups(ups, CI_APCForceShutdown, 1, func))
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      else
         hibernate = 1;
   }

   if (!hibernate) {
      Dmsg0(000, "Couldn't put UPS into hibernation mode. Attempting shutdown.\n");
      hibernate = usb_ups_shutdown(ups);
   }

   Dmsg0(200, "Leave usb_ups_kill_power\n");
   return hibernate;
}

int usb_ups_shutdown(UPSINFO *ups)
{
   const char *func;
   int shutdown = 0;

   Dmsg0(200, "Enter usb_ups_shutdown\n");

   /*
    * Complete shutdown
    *
    * This method turns off the UPS off completely after a given delay. 
    * The only way to power the UPS back on is to manually hit the
    * power button.
    */
   if (!shutdown && UPS_HAS_CAP(CI_DelayBeforeShutdown)) {
      Dmsg0(000, "UPS appears to support DelayBeforeShutdown style shutdown.\n");

      func = "CI_DelayBeforeShutdown";
      if (!usb_write_int_to_ups(ups, CI_DelayBeforeShutdown, SHUTDOWN_DELAY, func))
         Dmsg1(000, "Kill power function \"%s\" failed.\n", func);
      else
         shutdown = 1;
   }

   /*
    * I give up.
    */
   if (!shutdown) {
      Dmsg0(000, "I don't know how to turn off this UPS...sorry.\n"
         "Please report this, along with the output from\n"
         "running examples/hid-ups, to the apcupsd-users\n"
         "mailing list (apcupsd-users@lists.sourceforge.net).\n");
   }

   /*
    * Note that there are a couple other CIs that look interesting
    * for shutdown, but they're not what we want.
    *
    * APCShutdownAfterDelay: Tells the UPS how many seconds to wait
    *     after power goes out before asserting ShutdownRequested
    *    (see next item).
    *
    * CI_ShutdownRequested: This is an indicator from the UPS to the
    *     server that it would like the server to begin shutting
    *     down. In conjunction with APCShutdownAfterDelay this can be
    *     used to offload the decision of when to shut down the
    *     server to the UPS.
    */

   Dmsg0(200, "Leave usb_ups_shutdown\n");
   return shutdown;
}

/*
 * Helper functions for use by platform specific code
 */

double pow_ten(int exponent)
{
   int i;
   double val = 1;

   if (exponent < 0) {
      exponent = -exponent;
      for (i = 0; i < exponent; i++)
         val = val / 10;
      return val;
   } else {
      for (i = 0; i < exponent; i++)
         val = val * 10;
   }

   return val;
}

/* Called by platform-specific code to report an interrupt event */
int usb_report_event(UPSINFO *ups, int ci, USB_VALUE *uval)
{
   Dmsg2(200, "USB driver reported event ci=%d, val=%f\n",
      ci, uval->dValue);

   /* Got an event: go process it */
   usb_process_value(ups, ci, uval);

   switch (ci) {
   /*
    * Some important usages cause us to abort interrupt waiting
    * so immediate action can be taken.
    */
   case CI_Discharging:
   case CI_ACPresent:
   case CI_BelowRemCapLimit:
   case CI_BATTLEV:
   case CI_RUNTIM:
   case CI_NeedReplacement:
   case CI_ShutdownImminent:
   case CI_BatteryPresent:
      return true;

   /*
    * We don't handle these directly, but rather use them as a
    * signal to go poll the full set of volatile data.
    */
   case CI_IFailure:
   case CI_Overload:
   case CI_PWVoltageOOR:
   case CI_PWFrequencyOOR:
   case CI_OverCharged:
   case CI_OverTemp:
   case CI_CommunicationLost:
   case CI_ChargerVoltageOOR:
   case CI_ChargerCurrentOOR:
   case CI_CurrentNotRegulated:
   case CI_VoltageNotRegulated:
      return true;

   /*
    * Anything else is relatively unimportant, so we can
    * keep gathering data until the timeout.
    */
   default:
      return false;
   }
}
