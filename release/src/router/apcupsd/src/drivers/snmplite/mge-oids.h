#ifndef __MGE_OIDS_H_
#define __MGE_OIDS_H_

/*
 * oids.h
 *
 * UPS-related OIDs from MGE's UPS MIB
 *
 * Generated from mgeups_mib17_ad.mib (rev 1.7ad)
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

__UNUSED__ static int upsmgIdentFamilyName[] = {1, 3, 6, 1, 4, 1, 705, 1, 1, 1, -1};
__UNUSED__ static int upsmgIdentModelName[] = {1, 3, 6, 1, 4, 1, 705, 1, 1, 2, -1};
__UNUSED__ static int upsmgIdentRevisionLevel[] = {1, 3, 6, 1, 4, 1, 705, 1, 1, 3, -1};
__UNUSED__ static int upsmgIdentFirmwareVersion[] = {1, 3, 6, 1, 4, 1, 705, 1, 1, 4, -1};
__UNUSED__ static int upsmgIdentUserID[] = {1, 3, 6, 1, 4, 1, 705, 1, 1, 5, -1};
__UNUSED__ static int upsmgIdentInstallationDate[] = {1, 3, 6, 1, 4, 1, 705, 1, 1, 6, -1};
__UNUSED__ static int upsmgIdentSerialNumber[] = {1, 3, 6, 1, 4, 1, 705, 1, 1, 7, -1};
__UNUSED__ static int upsmgManagersNum[] = {1, 3, 6, 1, 4, 1, 705, 1, 2, 1, -1};
__UNUSED__ static int mgmanagerIndex[] = {1, 3, 6, 1, 4, 1, 705, 1, 2, 2, 1, 1, -1};
__UNUSED__ static int mgmanagerDeviceNumber[] = {1, 3, 6, 1, 4, 1, 705, 1, 2, 2, 1, 2, -1};
__UNUSED__ static int mgmanagerNMSType[] = {1, 3, 6, 1, 4, 1, 705, 1, 2, 2, 1, 3, -1};
__UNUSED__ static int mgmanagerCommType[] = {1, 3, 6, 1, 4, 1, 705, 1, 2, 2, 1, 4, -1};
__UNUSED__ static int mgmanagerDescr[] = {1, 3, 6, 1, 4, 1, 705, 1, 2, 2, 1, 5, -1};
__UNUSED__ static int mgmanagerAddress[] = {1, 3, 6, 1, 4, 1, 705, 1, 2, 2, 1, 6, -1};
__UNUSED__ static int mgmanagerCommunity[] = {1, 3, 6, 1, 4, 1, 705, 1, 2, 2, 1, 7, -1};
__UNUSED__ static int mgmanagerSeverityLevel[] = {1, 3, 6, 1, 4, 1, 705, 1, 2, 2, 1, 8, -1};
__UNUSED__ static int mgmanagerTrapAck[] = {1, 3, 6, 1, 4, 1, 705, 1, 2, 2, 1, 9, -1};
__UNUSED__ static int upsmgReceptaclesNum[] = {1, 3, 6, 1, 4, 1, 705, 1, 3, 1, -1};
__UNUSED__ static int mgreceptacleIndex[] = {1, 3, 6, 1, 4, 1, 705, 1, 3, 2, 1, 1, -1};
__UNUSED__ static int mgreceptacleLevel[] = {1, 3, 6, 1, 4, 1, 705, 1, 3, 2, 1, 2, -1};
__UNUSED__ static int mgreceptacleType[] = {1, 3, 6, 1, 4, 1, 705, 1, 3, 2, 1, 3, -1};
__UNUSED__ static int mgreceptacleIdent[] = {1, 3, 6, 1, 4, 1, 705, 1, 3, 2, 1, 4, -1};
__UNUSED__ static int mgreceptacleState[] = {1, 3, 6, 1, 4, 1, 705, 1, 3, 2, 1, 5, -1};
__UNUSED__ static int mgreceptacleReceptacle[] = {1, 3, 6, 1, 4, 1, 705, 1, 3, 2, 1, 6, -1};
__UNUSED__ static int mgreceptaclePowerCons[] = {1, 3, 6, 1, 4, 1, 705, 1, 3, 2, 1, 7, -1};
__UNUSED__ static int mgreceptacleOverload[] = {1, 3, 6, 1, 4, 1, 705, 1, 3, 2, 1, 8, -1};
__UNUSED__ static int mgreceptacleAutonomy[] = {1, 3, 6, 1, 4, 1, 705, 1, 3, 2, 1, 9, -1};
__UNUSED__ static int upsmgConfigBatteryInstalled[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 1, -1};
__UNUSED__ static int upsmgConfigNominalBatteryVoltage[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 2, -1};
__UNUSED__ static int upsmgConfigNominalBatteryTime[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 3, -1};
__UNUSED__ static int upsmgConfigNominalRechargeTime[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 4, -1};
__UNUSED__ static int upsmgConfigMinRechargeLevel[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 5, -1};
__UNUSED__ static int upsmgConfigMaxRechargeTime[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 6, -1};
__UNUSED__ static int upsmgConfigLowBatteryTime[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 7, -1};
__UNUSED__ static int upsmgConfigLowBatteryLevel[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 8, -1};
__UNUSED__ static int upsmgConfigAutoRestart[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 9, -1};
__UNUSED__ static int upsmgConfigShutdownTimer[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 10, -1};
__UNUSED__ static int upsmgConfigSysShutDuration[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 11, -1};
__UNUSED__ static int upsmgConfigVARating[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 12, -1};
__UNUSED__ static int upsmgConfigLowTransfer[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 13, -1};
__UNUSED__ static int upsmgConfigHighTransfer[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 14, -1};
__UNUSED__ static int upsmgConfigOutputNominalVoltage[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 15, -1};
__UNUSED__ static int upsmgConfigOutputNominalCurrent[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 16, -1};
__UNUSED__ static int upsmgConfigOutputNomFrequency[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 17, -1};
__UNUSED__ static int upsmgConfigByPassType[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 18, -1};
__UNUSED__ static int upsmgConfigAlarmAudible[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 19, -1};
__UNUSED__ static int upsmgConfigAlarmTimeDelay[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 20, -1};
__UNUSED__ static int upsmgConfigDevicesNum[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 21, -1};
__UNUSED__ static int mgdeviceIndex[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 22, 1, 1, -1};
__UNUSED__ static int mgdeviceReceptacleNum[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 22, 1, 2, -1};
__UNUSED__ static int mgdeviceIdent[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 22, 1, 3, -1};
__UNUSED__ static int mgdeviceVaRating[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 22, 1, 4, -1};
__UNUSED__ static int mgdeviceSequenceOff[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 22, 1, 5, -1};
__UNUSED__ static int mgdeviceSequenceOn[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 22, 1, 6, -1};
__UNUSED__ static int mgdeviceShutdownDuration[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 22, 1, 7, -1};
__UNUSED__ static int mgdeviceBootUpDuration[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 22, 1, 8, -1};
__UNUSED__ static int mgreceptacleIndexb[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 23, 1, 1, -1};
__UNUSED__ static int mgreceptacleStateTurnOn[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 23, 1, 2, -1};
__UNUSED__ static int mgreceptacleStateMainReturn[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 23, 1, 3, -1};
__UNUSED__ static int mgreceptacleStateDischarge[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 23, 1, 4, -1};
__UNUSED__ static int mgreceptacleShutoffLevel[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 23, 1, 5, -1};
__UNUSED__ static int mgreceptacleShutoffTimer[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 23, 1, 6, -1};
__UNUSED__ static int mgreceptacleRestartLevel[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 23, 1, 7, -1};
__UNUSED__ static int mgreceptacleRestartDelay[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 23, 1, 8, -1};
__UNUSED__ static int mgreceptacleShutdownDuration[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 23, 1, 9, -1};
__UNUSED__ static int mgreceptacleBootUpDuration[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 23, 1, 10, -1};
__UNUSED__ static int upsmgConfigExtAlarmNum[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 24, -1};
__UNUSED__ static int mgextAlarmIndex[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 25, 1, 1, -1};
__UNUSED__ static int mgextAlarmUID[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 25, 1, 2, -1};
__UNUSED__ static int upsmgConfigEmergencyTestFail[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 26, -1};
__UNUSED__ static int upsmgConfigEmergencyOnByPass[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 27, -1};
__UNUSED__ static int upsmgConfigEmergencyOverload[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 28, -1};
__UNUSED__ static int mgcontrolDayIndex[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 29, 1, 1, -1};
__UNUSED__ static int mgcontrolDayOn[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 29, 1, 2, -1};
__UNUSED__ static int mgcontrolDayOff[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 29, 1, 3, -1};
__UNUSED__ static int upsmgConfigLowBooster[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 30, -1};
__UNUSED__ static int upsmgConfigHighBooster[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 31, -1};
__UNUSED__ static int upsmgConfigLowFader[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 32, -1};
__UNUSED__ static int upsmgConfigHighFader[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 33, -1};
__UNUSED__ static int upsmgConfigSensorIndex[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 1, -1};
__UNUSED__ static int upsmgConfigSensorName[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 2, -1};
__UNUSED__ static int upsmgConfigTemperatureLow[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 3, -1};
__UNUSED__ static int upsmgConfigTemperatureHigh[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 4, -1};
__UNUSED__ static int upsmgConfigTemperatureHysteresis[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 5, -1};
__UNUSED__ static int upsmgConfigHumidityLow[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 6, -1};
__UNUSED__ static int upsmgConfigHumidityHigh[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 7, -1};
__UNUSED__ static int upsmgConfigHumidityHysteresis[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 8, -1};
__UNUSED__ static int upsmgConfigInput1Name[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 9, -1};
__UNUSED__ static int upsmgConfigInput1ClosedLabel[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 10, -1};
__UNUSED__ static int upsmgConfigInput1OpenLabel[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 11, -1};
__UNUSED__ static int upsmgConfigInput2Name[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 12, -1};
__UNUSED__ static int upsmgConfigInput2ClosedLabel[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 13, -1};
__UNUSED__ static int upsmgConfigInput2OpenLabel[] = {1, 3, 6, 1, 4, 1, 705, 1, 4, 34, 1, 14, -1};
__UNUSED__ static int upsmgBatteryRemainingTime[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 1, -1};
__UNUSED__ static int upsmgBatteryLevel[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 2, -1};
__UNUSED__ static int upsmgBatteryRechargeTime[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 3, -1};
__UNUSED__ static int upsmgBatteryRechargeLevel[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 4, -1};
__UNUSED__ static int upsmgBatteryVoltage[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 5, -1};
__UNUSED__ static int upsmgBatteryCurrent[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 6, -1};
__UNUSED__ static int upsmgBatteryTemperature[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 7, -1};
__UNUSED__ static int upsmgBatteryFullRechargeTime[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 8, -1};
__UNUSED__ static int upsmgBatteryFaultBattery[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 9, -1};
__UNUSED__ static int upsmgBatteryNoBattery[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 10, -1};
__UNUSED__ static int upsmgBatteryReplacement[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 11, -1};
__UNUSED__ static int upsmgBatteryUnavailableBattery[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 12, -1};
__UNUSED__ static int upsmgBatteryNotHighCharge[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 13, -1};
__UNUSED__ static int upsmgBatteryLowBattery[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 14, -1};
__UNUSED__ static int upsmgBatteryChargerFault[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 15, -1};
__UNUSED__ static int upsmgBatteryLowCondition[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 16, -1};
__UNUSED__ static int upsmgBatteryLowRecharge[] = {1, 3, 6, 1, 4, 1, 705, 1, 5, 17, -1};
__UNUSED__ static int upsmgInputPhaseNum[] = {1, 3, 6, 1, 4, 1, 705, 1, 6, 1, -1};
__UNUSED__ static int mginputIndex[] = {1, 3, 6, 1, 4, 1, 705, 1, 6, 2, 1, 1, -1};
__UNUSED__ static int mginputVoltage[] = {1, 3, 6, 1, 4, 1, 705, 1, 6, 2, 1, 2, -1};
__UNUSED__ static int mginputFrequency[] = {1, 3, 6, 1, 4, 1, 705, 1, 6, 2, 1, 3, -1};
__UNUSED__ static int mginputMinimumVoltage[] = {1, 3, 6, 1, 4, 1, 705, 1, 6, 2, 1, 4, -1};
__UNUSED__ static int mginputMaximumVoltage[] = {1, 3, 6, 1, 4, 1, 705, 1, 6, 2, 1, 5, -1};
__UNUSED__ static int mginputCurrent[] = {1, 3, 6, 1, 4, 1, 705, 1, 6, 2, 1, 6, -1};
__UNUSED__ static int upsmgInputBadStatus[] = {1, 3, 6, 1, 4, 1, 705, 1, 6, 3, -1};
__UNUSED__ static int upsmgInputLineFailCause[] = {1, 3, 6, 1, 4, 1, 705, 1, 6, 4, -1};
__UNUSED__ static int upsmgOutputPhaseNum[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 1, -1};
__UNUSED__ static int mgoutputPhaseIndex[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 2, 1, 1, -1};
__UNUSED__ static int mgoutputVoltage[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 2, 1, 2, -1};
__UNUSED__ static int mgoutputFrequency[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 2, 1, 3, -1};
__UNUSED__ static int mgoutputLoadPerPhase[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 2, 1, 4, -1};
__UNUSED__ static int mgoutputCurrent[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 2, 1, 5, -1};
__UNUSED__ static int upsmgOutputOnBattery[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 3, -1};
__UNUSED__ static int upsmgOutputOnByPass[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 4, -1};
__UNUSED__ static int upsmgOutputUnavailableByPass[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 5, -1};
__UNUSED__ static int upsmgOutputNoByPass[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 6, -1};
__UNUSED__ static int upsmgOutputUtilityOff[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 7, -1};
__UNUSED__ static int upsmgOutputOnBoost[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 8, -1};
__UNUSED__ static int upsmgOutputInverterOff[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 9, -1};
__UNUSED__ static int upsmgOutputOverLoad[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 10, -1};
__UNUSED__ static int upsmgOutputOverTemp[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 11, -1};
__UNUSED__ static int upsmgOutputOnBuck[] = {1, 3, 6, 1, 4, 1, 705, 1, 7, 12, -1};
__UNUSED__ static int upsmgEnvironAmbientTemp[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 1, -1};
__UNUSED__ static int upsmgEnvironAmbientHumidity[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 2, -1};
__UNUSED__ static int mgalarmNum[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 3, 1, 1, -1};
__UNUSED__ static int mgalarmState[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 3, 1, 2, -1};
__UNUSED__ static int upsmgEnvironSensorNum[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 4, -1};
__UNUSED__ static int mgsensorNum[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 5, 1, 1, -1};
__UNUSED__ static int mgsensorTemp[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 5, 1, 2, -1};
__UNUSED__ static int mgsensorHumidity[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 5, 1, 3, -1};
__UNUSED__ static int upsmgEnvironmentNum[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 6, -1};
__UNUSED__ static int upsmgEnvironmentIndex[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 7, 1, 1, -1};
__UNUSED__ static int upsmgEnvironmentComFailure[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 7, 1, 2, -1};
__UNUSED__ static int upsmgEnvironmentTemperature[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 7, 1, 3, -1};
__UNUSED__ static int upsmgEnvironmentTemperatureLow[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 7, 1, 4, -1};
__UNUSED__ static int upsmgEnvironmentTemperatureHigh[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 7, 1, 5, -1};
__UNUSED__ static int upsmgEnvironmentHumidity[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 7, 1, 6, -1};
__UNUSED__ static int upsmgEnvironmentHumidityLow[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 7, 1, 7, -1};
__UNUSED__ static int upsmgEnvironmentHumidityHigh[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 7, 1, 8, -1};
__UNUSED__ static int upsmgEnvironmentInput1State[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 7, 1, 9, -1};
__UNUSED__ static int upsmgEnvironmentInput2State[] = {1, 3, 6, 1, 4, 1, 705, 1, 8, 7, 1, 10, -1};
__UNUSED__ static int mgreceptacleIndexc[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 1, 1, 1, -1};
__UNUSED__ static int mgreceptacleOnDelay[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 1, 1, 2, -1};
__UNUSED__ static int mgreceptacleOnCtrl[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 1, 1, 3, -1};
__UNUSED__ static int mgreceptacleOnStatus[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 1, 1, 4, -1};
__UNUSED__ static int mgreceptacleOffDelay[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 1, 1, 5, -1};
__UNUSED__ static int mgreceptacleOffCtrl[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 1, 1, 6, -1};
__UNUSED__ static int mgreceptacleOffStatus[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 1, 1, 7, -1};
__UNUSED__ static int mgreceptacleToggleDelay[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 1, 1, 8, -1};
__UNUSED__ static int mgreceptacleToggleCtrl[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 1, 1, 9, -1};
__UNUSED__ static int mgreceptacleToggleStatus[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 1, 1, 10, -1};
__UNUSED__ static int mgreceptacleToggleDuration[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 1, 1, 11, -1};
__UNUSED__ static int upsmgControlDayOff[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 2, -1};
__UNUSED__ static int upsmgControlDayOn[] = {1, 3, 6, 1, 4, 1, 705, 1, 9, 3, -1};
__UNUSED__ static int upsmgTestBatterySchedule[] = {1, 3, 6, 1, 4, 1, 705, 1, 10, 1, -1};
__UNUSED__ static int upsmgTestDiagnostics[] = {1, 3, 6, 1, 4, 1, 705, 1, 10, 2, -1};
__UNUSED__ static int upsmgTestDiagResult[] = {1, 3, 6, 1, 4, 1, 705, 1, 10, 3, -1};
__UNUSED__ static int upsmgTestBatteryCalibration[] = {1, 3, 6, 1, 4, 1, 705, 1, 10, 4, -1};
__UNUSED__ static int upsmgTestLastCalibration[] = {1, 3, 6, 1, 4, 1, 705, 1, 10, 5, -1};
__UNUSED__ static int upsmgTestIndicators[] = {1, 3, 6, 1, 4, 1, 705, 1, 10, 6, -1};
__UNUSED__ static int upsmgTestCommandLine[] = {1, 3, 6, 1, 4, 1, 705, 1, 10, 7, -1};
__UNUSED__ static int upsmgTestCommandReady[] = {1, 3, 6, 1, 4, 1, 705, 1, 10, 8, -1};
__UNUSED__ static int upsmgTestResponseLine[] = {1, 3, 6, 1, 4, 1, 705, 1, 10, 9, -1};
__UNUSED__ static int upsmgTestResponseReady[] = {1, 3, 6, 1, 4, 1, 705, 1, 10, 10, -1};
__UNUSED__ static int upsmgTestBatteryResult[] = {1, 3, 6, 1, 4, 1, 705, 1, 10, 11, -1};
__UNUSED__ static int upsmgAgentIpaddress[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 1, -1};
__UNUSED__ static int upsmgAgentSubnetMask[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 2, -1};
__UNUSED__ static int upsmgAgentDefGateway[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 3, -1};
__UNUSED__ static int upsmgAgentBaudRate[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 4, -1};
__UNUSED__ static int upsmgAgentPollRate[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 5, -1};
__UNUSED__ static int upsmgAgentType[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 6, -1};
__UNUSED__ static int upsmgAgentTrapAlarmDelay[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 7, -1};
__UNUSED__ static int upsmgAgentTrapAlarmRetry[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 8, -1};
__UNUSED__ static int upsmgAgentReset[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 9, -1};
__UNUSED__ static int upsmgAgentFactReset[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 10, -1};
__UNUSED__ static int upsmgAgentMibVersion[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 11, -1};
__UNUSED__ static int upsmgAgentFirmwareVersion[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 12, -1};
__UNUSED__ static int upsmgAgentCommUPS[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 13, -1};
__UNUSED__ static int upsmgAgentTrapAck[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 14, -1};
__UNUSED__ static int upsmgAgentAutoLearning[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 15, -1};
__UNUSED__ static int upsmgAgentBootP[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 16, -1};
__UNUSED__ static int upsmgAgentTFTP[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 17, -1};
__UNUSED__ static int upsmgAgentTrapSignature[] = {1, 3, 6, 1, 4, 1, 705, 1, 12, 18, -1};
__UNUSED__ static int upsmgRemoteOnBattery[] = {1, 3, 6, 1, 4, 1, 705, 1, 13, 1, -1};
__UNUSED__ static int upsmgRemoteIpAddress[] = {1, 3, 6, 1, 4, 1, 705, 1, 13, 2, -1};

#endif
