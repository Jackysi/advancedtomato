#ifndef __RFC1628_OIDS_H_
#define __RFC1628_OIDS_H_

/*
 * rfc1628-oids.h
 *
 * UPS-related OIDs from RFC1628 MIB
 *
 * Generated from rfc1628.mib
 *
 */

#ifdef __GNUC__
#define __UNUSED__ __attribute__((unused))
#else
#define __UNUSED__
#endif

//
// Note that because these are all declared static, the compiler will eliminate
// those that we do not reference. It will warn about it, however, so need to 
// suppress that where possible.
//

__UNUSED__ static int upsIdentManufacturer[] = {1, 3, 6, 1, 2, 1, 33, 1, 1, 1, -1};
__UNUSED__ static int upsIdentModel[] = {1, 3, 6, 1, 2, 1, 33, 1, 1, 2, -1};
__UNUSED__ static int upsIdentUPSSoftwareVersion[] = {1, 3, 6, 1, 2, 1, 33, 1, 1, 3, -1};
__UNUSED__ static int upsIdentAgentSoftwareVersion[] = {1, 3, 6, 1, 2, 1, 33, 1, 1, 4, -1};
__UNUSED__ static int upsIdentName[] = {1, 3, 6, 1, 2, 1, 33, 1, 1, 5, -1};
__UNUSED__ static int upsIdentAttachedDevices[] = {1, 3, 6, 1, 2, 1, 33, 1, 1, 6, -1};
__UNUSED__ static int upsBatteryStatus[] = {1, 3, 6, 1, 2, 1, 33, 1, 2, 1, -1};
__UNUSED__ static int upsSecondsOnBattery[] = {1, 3, 6, 1, 2, 1, 33, 1, 2, 2, -1};
__UNUSED__ static int upsEstimatedMinutesRemaining[] = {1, 3, 6, 1, 2, 1, 33, 1, 2, 3, -1};
__UNUSED__ static int upsEstimatedChargeRemaining[] = {1, 3, 6, 1, 2, 1, 33, 1, 2, 4, -1};
__UNUSED__ static int upsBatteryVoltage[] = {1, 3, 6, 1, 2, 1, 33, 1, 2, 5, -1};
__UNUSED__ static int upsBatteryCurrent[] = {1, 3, 6, 1, 2, 1, 33, 1, 2, 6, -1};
__UNUSED__ static int upsBatteryTemperature[] = {1, 3, 6, 1, 2, 1, 33, 1, 2, 7, -1};
__UNUSED__ static int upsInputLineBads[] = {1, 3, 6, 1, 2, 1, 33, 1, 3, 1, -1};
__UNUSED__ static int upsInputNumLines[] = {1, 3, 6, 1, 2, 1, 33, 1, 3, 2, -1};
__UNUSED__ static int upsInputTableInputLineIndex[] = {1, 3, 6, 1, 2, 1, 33, 1, 3, 3, 1, 1, -1};
__UNUSED__ static int upsInputTableInputFrequency[] = {1, 3, 6, 1, 2, 1, 33, 1, 3, 3, 1, 2, -1};
__UNUSED__ static int upsInputTableInputVoltage[] = {1, 3, 6, 1, 2, 1, 33, 1, 3, 3, 1, 3, -1};
__UNUSED__ static int upsInputTableInputCurrent[] = {1, 3, 6, 1, 2, 1, 33, 1, 3, 3, 1, 4, -1};
__UNUSED__ static int upsInputTableInputTruePower[] = {1, 3, 6, 1, 2, 1, 33, 1, 3, 3, 1, 5, -1};
__UNUSED__ static int upsOutputSource[] = {1, 3, 6, 1, 2, 1, 33, 1, 4, 1, -1};
__UNUSED__ static int upsOutputFrequency[] = {1, 3, 6, 1, 2, 1, 33, 1, 4, 2, -1};
__UNUSED__ static int upsOutputNumLines[] = {1, 3, 6, 1, 2, 1, 33, 1, 4, 3, -1};
__UNUSED__ static int upsOutputTableOutputLineIndex[] = {1, 3, 6, 1, 2, 1, 33, 1, 4, 4, 1, 1, -1};
__UNUSED__ static int upsOutputTableOutputVoltage[] = {1, 3, 6, 1, 2, 1, 33, 1, 4, 4, 1, 2, -1};
__UNUSED__ static int upsOutputTableOutputCurrent[] = {1, 3, 6, 1, 2, 1, 33, 1, 4, 4, 1, 3, -1};
__UNUSED__ static int upsOutputTableOutputPower[] = {1, 3, 6, 1, 2, 1, 33, 1, 4, 4, 1, 4, -1};
__UNUSED__ static int upsOutputTableOutputPercentLoad[] = {1, 3, 6, 1, 2, 1, 33, 1, 4, 4, 1, 5, -1};
__UNUSED__ static int upsBypassFrequency[] = {1, 3, 6, 1, 2, 1, 33, 1, 5, 1, -1};
__UNUSED__ static int upsBypassNumLines[] = {1, 3, 6, 1, 2, 1, 33, 1, 5, 2, -1};
__UNUSED__ static int upsBypassLineIndex[] = {1, 3, 6, 1, 2, 1, 33, 1, 5, 3, 1, 1, -1};
__UNUSED__ static int upsBypassVoltage[] = {1, 3, 6, 1, 2, 1, 33, 1, 5, 3, 1, 2, -1};
__UNUSED__ static int upsBypassCurrent[] = {1, 3, 6, 1, 2, 1, 33, 1, 5, 3, 1, 3, -1};
__UNUSED__ static int upsBypassPower[] = {1, 3, 6, 1, 2, 1, 33, 1, 5, 3, 1, 4, -1};
__UNUSED__ static int upsAlarmsPresent[] = {1, 3, 6, 1, 2, 1, 33, 1, 6, 1, -1};
__UNUSED__ static int upsAlarmId[] = {1, 3, 6, 1, 2, 1, 33, 1, 6, 2, 1, 1, -1};
__UNUSED__ static int upsAlarmDescr[] = {1, 3, 6, 1, 2, 1, 33, 1, 6, 2, 1, 2, -1};
__UNUSED__ static int upsAlarmTime[] = {1, 3, 6, 1, 2, 1, 33, 1, 6, 2, 1, 3, -1};
__UNUSED__ static int upsTestId[] = {1, 3, 6, 1, 2, 1, 33, 1, 7, 1, -1};
__UNUSED__ static int upsTestSpinLock[] = {1, 3, 6, 1, 2, 1, 33, 1, 7, 2, -1};
__UNUSED__ static int upsTestResultsSummary[] = {1, 3, 6, 1, 2, 1, 33, 1, 7, 3, -1};
__UNUSED__ static int upsTestResultsDetail[] = {1, 3, 6, 1, 2, 1, 33, 1, 7, 4, -1};
__UNUSED__ static int upsTestStartTime[] = {1, 3, 6, 1, 2, 1, 33, 1, 7, 5, -1};
__UNUSED__ static int upsTestElapsedTime[] = {1, 3, 6, 1, 2, 1, 33, 1, 7, 6, -1};
__UNUSED__ static int upsShutdownType[] = {1, 3, 6, 1, 2, 1, 33, 1, 8, 1, -1};
__UNUSED__ static int upsShutdownAfterDelay[] = {1, 3, 6, 1, 2, 1, 33, 1, 8, 2, -1};
__UNUSED__ static int upsStartupAfterDelay[] = {1, 3, 6, 1, 2, 1, 33, 1, 8, 3, -1};
__UNUSED__ static int upsRebootWithDuration[] = {1, 3, 6, 1, 2, 1, 33, 1, 8, 4, -1};
__UNUSED__ static int upsAutoRestart[] = {1, 3, 6, 1, 2, 1, 33, 1, 8, 5, -1};
__UNUSED__ static int upsConfigInputVoltage[] = {1, 3, 6, 1, 2, 1, 33, 1, 9, 1, -1};
__UNUSED__ static int upsConfigInputFreq[] = {1, 3, 6, 1, 2, 1, 33, 1, 9, 2, -1};
__UNUSED__ static int upsConfigOutputVoltage[] = {1, 3, 6, 1, 2, 1, 33, 1, 9, 3, -1};
__UNUSED__ static int upsConfigOutputFreq[] = {1, 3, 6, 1, 2, 1, 33, 1, 9, 4, -1};
__UNUSED__ static int upsConfigOutputVA[] = {1, 3, 6, 1, 2, 1, 33, 1, 9, 5, -1};
__UNUSED__ static int upsConfigOutputPower[] = {1, 3, 6, 1, 2, 1, 33, 1, 9, 6, -1};
__UNUSED__ static int upsConfigLowBattTime[] = {1, 3, 6, 1, 2, 1, 33, 1, 9, 7, -1};
__UNUSED__ static int upsConfigAudibleStatus[] = {1, 3, 6, 1, 2, 1, 33, 1, 9, 8, -1};
__UNUSED__ static int upsConfigLowVoltageTransferPoint[] = {1, 3, 6, 1, 2, 1, 33, 1, 9, 9, -1};
__UNUSED__ static int upsConfigHighVoltageTransferPoint[] = {1, 3, 6, 1, 2, 1, 33, 1, 9, 10, -1};

#endif
