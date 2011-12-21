/*
 * traps.h
 *
 * Mappings for APC UPS SNMP trap ids
 */

#ifndef __TRAPS_H_
#define __TRAPS_H_

#define TRAP_COMMUNICATIONLOST                     1
#define TRAP_UPSOVERLOAD                           2
#define TRAP_UPSDIAGNOSTICSFAILED                  3
#define TRAP_UPSDISCHARGED                         4
#define TRAP_UPSONBATTERY                          5
#define TRAP_SMARTBOOSTON                          6
#define TRAP_LOWBATTERY                            7
#define TRAP_COMMUNICATIONESTABLISHED              8
#define TRAP_POWERRESTORED                         9
#define TRAP_UPSDIAGNOSTICSPASSED                  10
#define TRAP_RETURNFROMLOWBATTERY                  11
#define TRAP_UPSTURNEDOFF                          12
#define TRAP_UPSSLEEPING                           13
#define TRAP_UPSWOKEUP                             14
#define TRAP_UPSREBOOTSTARTED                      15
#define TRAP_UPSDIPSWITCHCHANGED                   16
#define TRAP_UPSBATTERYNEEDSREPLACEMENT            17
#define TRAP_CONTACTFAULT                          18
#define TRAP_CONTACTFAULTRESOLVED                  19
#define TRAP_HARDWAREFAILUREBYPASS                 20
#define TRAP_SOFTWAREBYPASS                        21
#define TRAP_SWITCHEDBYPASS                        22
#define TRAP_RETURNFROMBYPASS                      23
#define TRAP_BYPASSPOWERSUPPLYFAILURE              24
#define TRAP_BASEFANFAILURE                        25
#define TRAP_BATTERYPACKCOMMLOST                   26
#define TRAP_BATTERYPACKCOMMESTABLISHED            27
#define TRAP_CALIBRATIONSTART                      28
#define TRAP_RESTARTAGENT                          29
#define TRAP_UPSTURNEDON                           30
#define TRAP_SMARTAVRREDUCING                      31
#define TRAP_CODEAUTHENTICATIONDONE                32
#define TRAP_UPSOVERLOADCLEARED                    33
#define TRAP_SMARTBOOSTOFF                         34
#define TRAP_SMARTAVRREDUCINGOFF                   35
#define TRAP_UPSBATTERYREPLACED                    36
#define TRAP_CALIBRATIONEND                        37
#define TRAP_DISCHARGECLEARED                      38
#define TRAP_GRACEFULLSHUTDOWN                     39
#define TRAP_OUTLETON                              41
#define TRAP_OUTLETOFF                             42
#define TRAP_OUTLETREBOOT                          43
#define TRAP_CONFIGCHANGESNMP                      44
#define TRAP_CONFIGCHANGEOUTLET                    45
#define TRAP_ACCESSVIOLATIONCONSOLE                46
#define TRAP_ACCESSVIOLATIONHTTP                   47
#define TRAP_PASSWORDCHANGE                        48
#define TRAP_BADVOLTAGE                            49
#define TRAP_BADVOLTAGECLEARED                     50
#define TRAP_CHARGERFAILURE                        51
#define TRAP_CHARGERFAILURECLEARED                 52
#define TRAP_BATTERYOVERTEMPERATURE                53
#define TRAP_BATTERYOVERTEMPERATURECLEARED         54
#define TRAP_SMARTRELAYFAULT                       55
#define TRAP_SMARTRELAYFAULTCLEARED                56
#define TRAP_HUMIDITYTHRESHOLDVIOLATION1           57
#define TRAP_HUMIDITYTHRESHOLDVIOLATIONCLEARED1    58
#define TRAP_TEMPERATURETHRESHOLDVIOLATION1        59
#define TRAP_TEMPERATURETHRESHOLDVIOLATIONCLEARED1 60
#define TRAP_HUMIDITYTHRESHOLDVIOLATION2           61
#define TRAP_HUMIDITYTHRESHOLDVIOLATIONCLEARED2    62
#define TRAP_TEMPERATURETHRESHOLDVIOLATION2        63
#define TRAP_TEMPERATURETHRESHOLDVIOLATIONCLEARED2 64
#define TRAP_MUPSCOMMUNICATIONESTABLISHED          65
#define TRAP_MUPSCOMMUNICATIONLOST                 66
#define TRAP_BATTERYINCREASE                       67
#define TRAP_BATTERYDECREASE                       68
#define TRAP_POWERMODULEINCREASE                   69
#define TRAP_POWERMODULEDECREASE                   70
#define TRAP_INTELLIGENCEMODULEINSERTED            71
#define TRAP_INTELLIGENCEMODULEREMOVED             72
#define TRAP_RINTELLIGENCEMODULEINSERTED           73
#define TRAP_RINTELLIGENCEMODULEREMOVED            74
#define TRAP_EXTBATTERYFRAMEINCEASE                75
#define TRAP_EXTBATTERYFRAMEDECREASE               76
#define TRAP_ABNORMALCONDITION                     77
#define TRAP_ABNORMALCONDITIONCLEARED              78
#define TRAP_DEVICESTATUSCHANGE                    79
#define TRAP_NOBATTERIES                           80
#define TRAP_NOBATTERIESCLEARED                    81
#define TRAP_USERADDED                             82
#define TRAP_USERDELETED                           83
#define TRAP_USERMODIFIED                          84

#endif
