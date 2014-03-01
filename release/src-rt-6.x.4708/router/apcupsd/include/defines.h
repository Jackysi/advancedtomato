/*
 * defines.h
 *
 * Public definitions used throughout apcupsd
 */

/*
 * Copyright (C) 1999-2005 Kern Sibbald
 * Copyright (C) 1996-1999 Andre M. Hedrick <andre@suse.com>
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

#ifndef _DEFINES_H
#define _DEFINES_H

#ifndef APCCONTROL_FILE
# define APCCONTROL_FILE        "/apccontrol"
#endif

#ifndef APCCONF_FILE
# define APCCONF_FILE           "/apcupsd.conf"
#endif

#ifndef PWRFAIL_FILE
# define PWRFAIL_FILE           "/powerfail"
#endif

#ifndef NOLOGIN_FILE
# define NOLOGIN_FILE            "/nologin"
#endif

#define APCPID                  PIDDIR "/apcupsd.pid"

/*
 * These two are not to be touched: we can not be sure how the user will
 * insert the locks directory path so we have to prepend the '/' just to be
 * sure: is better have /blah//LCK.. than /blahLCK..
 */
#define APC_LOCK_PREFIX         "/LCK.."
#define LOCK_DEFAULT            "/var/lock"

/*
 * This string should be the first line of the configuration file.
 * Then if we change the format later, we can just change this string.
 * Also, we could write code to use/convert out-of-date config files.
 */
#define APC_CONFIG_MAGIC        "## apcupsd.conf v1.1 ##"

#define POWERFAIL               "POWER FAILURE\n"       /* put in nologin file */

#define MAXSTRING               256
#define MESSAGELEN              256
#define MAXTOKENLEN             100
#define UPSNAMELEN              100

#define DEFAULT_SPEED           B2400


/* bit values for APC UPS Status Byte (ups->Status) */
#define UPS_calibration   0x00000001
#define UPS_trim          0x00000002
#define UPS_boost         0x00000004
#define UPS_online        0x00000008
#define UPS_onbatt        0x00000010
#define UPS_overload      0x00000020
#define UPS_battlow       0x00000040
#define UPS_replacebatt   0x00000080

/* Extended bit values added by apcupsd */
#define UPS_commlost      0x00000100    /* Communications with UPS lost */
#define UPS_shutdown      0x00000200    /* Shutdown in progress */
#define UPS_slave         0x00000400    /* Set if this is a slave */
#define UPS_slavedown     0x00000800    /* Slave not responding */
#define UPS_onbatt_msg    0x00020000    /* Set when UPS_ONBATT message is sent */
#define UPS_fastpoll      0x00040000    /* Set on power failure to poll faster */
#define UPS_shut_load     0x00080000    /* Set when BatLoad <= percent */
#define UPS_shut_btime    0x00100000    /* Set when time on batts > maxtime */
#define UPS_shut_ltime    0x00200000    /* Set when TimeLeft <= runtime */
#define UPS_shut_emerg    0x00400000    /* Set when battery power has failed */
#define UPS_shut_remote   0x00800000    /* Set when remote shutdown */
#define UPS_plugged       0x01000000    /* Set if computer is plugged into UPS */
#define UPS_dev_setup     0x02000000    /* Set if UPS's driver did the setup() */
#define UPS_battpresent   0x04000000    /* Indicates if battery is connected */

#define UPS_LOCAL_BITS (UPS_commlost|UPS_shutdown|UPS_slave|UPS_slavedown| \
            UPS_onbatt_msg|UPS_fastpoll|UPS_plugged|UPS_dev_setup| \
            UPS_shut_load|UPS_shut_btime|UPS_shut_ltime|UPS_shut_emerg)

/*
 * CI_ is Capability or command index
 *
 * If the command is valid for this UPS, UPS_Cap[CI_xxx]
 * will be true.
 */
enum { 
   CI_UPSMODEL = 0,                /* Model number */
   CI_STATUS,                      /* status function */
   CI_LQUAL,                       /* line quality status */
   CI_WHY_BATT,                    /* why transferred to battery */
   CI_ST_STAT,                     /* self test stat */
   CI_VLINE,                       /* line voltage */
   CI_VMAX,                        /* max voltage */
   CI_VMIN,                        /* min line voltage */
   CI_VOUT,                        /* Output voltage */
   CI_BATTLEV,                     /* Battery level percentage */
   CI_VBATT,                       /* Battery voltage */
   CI_LOAD,                        /* UPS Load */
   CI_FREQ,                        /* Line Frequency */
   CI_RUNTIM,                      /* Est. Runtime left */
   CI_ITEMP,                       /* Internal UPS temperature */
   CI_DIPSW,                       /* Dip switch settings */
   CI_SENS,                        /* Sensitivity */
   CI_DWAKE,                       /* Wakeup delay */
   CI_DSHUTD,                      /* Shutdown delay */
   CI_LTRANS,                      /* Low transfer voltage */
   CI_HTRANS,                      /* High transfer voltage */
   CI_RETPCT,                      /* Return percent threshhold */
   CI_DALARM,                      /* Alarm delay */
   CI_DLBATT,                      /* low battery warning, mins */
   CI_IDEN,                        /* UPS Identification (name) */
   CI_STESTI,                      /* Self test interval */
   CI_MANDAT,                      /* Manufacture date */
   CI_SERNO,                       /* serial number */
   CI_BATTDAT,                     /* Last battery change */
   CI_NOMBATTV,                    /* Nominal battery voltage */
   CI_HUMID,                       /* UPS Humidity percentage */
   CI_REVNO,                       /* Firmware revision */
   CI_REG1,                        /* Register 1 */
   CI_REG2,                        /* Register 2 */
   CI_REG3,                        /* Register 3 */
   CI_EXTBATTS,                    /* Number of external batteries */
   CI_ATEMP,                       /* Ambient temp */
   CI_NOMOUTV,                     /* Nominal output voltage */
   CI_BADBATTS,                    /* Number of bad battery packs */
   CI_EPROM,                       /* Valid eprom values */
   CI_ST_TIME,                     /* hours since last self test */
   CI_TESTALARM,                   /* Test alarm */
   CI_Manufacturer,             
   CI_ShutdownRequested,        
   CI_ShutdownImminent,         
   CI_DelayBeforeReboot,        
   CI_BelowRemCapLimit,         
   CI_RemTimeLimitExpired,      
   CI_Charging,                 
   CI_Discharging,              
   CI_RemCapLimit,              
   CI_RemTimeLimit,             
   CI_WarningCapacityLimit,     
   CI_CapacityMode,             
   CI_BattPackLevel,            
   CI_CycleCount,               
   CI_ACPresent,                
   CI_Boost,                    
   CI_Trim,                     
   CI_Overload,                 
   CI_NeedReplacement,          
   CI_BattReplaceDate,          
   CI_APCForceShutdown,         
   CI_DelayBeforeShutdown,      
   CI_APCDelayBeforeStartup,    
   CI_APCDelayBeforeShutdown,   
   CI_APCLineFailCause,         
   CI_NOMINV,                   
   CI_NOMPOWER,
   CI_LowBattery,
   CI_Calibration,
   CI_AlarmTimer,

   /* Only seen on the BackUPS Pro USB (so far) */
   CI_BUPBattCapBeforeStartup,  
   CI_BUPDelayBeforeStartup,    
   CI_BUPSelfTest,              
   CI_BUPHibernate,             

   /*
    * We don't actually handle these, but use them as a signal
    * to re-examine the other UPS data items. (USB only)
    */
   CI_IFailure,                    /* Internal failure */
   CI_PWVoltageOOR,                /* Power sys voltage out of range */
   CI_PWFrequencyOOR,              /* Power sys frequency out of range */
   CI_OverCharged,                 /* Battery overcharged */
   CI_OverTemp,                    /* Over temperature */
   CI_CommunicationLost,           /* USB comms with subsystem lost */
   CI_ChargerVoltageOOR,           /* Charger voltage our of range */
   CI_ChargerCurrentOOR,           /* Charger current our of range */
   CI_CurrentNotRegulated,         /* Charger current not regulated */
   CI_VoltageNotRegulated,         /* Charger voltage not regulated */
   CI_BatteryPresent,              /* Battery is present */
   CI_LAST_PROBE,                  /* MUST BE LAST IN SECTION */

   /* Items below this line are not "probed" for */
   CI_CYCLE_EPROM,                 /* Cycle programmable EPROM values */
   CI_UPS_CAPS,                    /* Get UPS capabilities (command) string */
   CI_LAST                         /* MUST BE LAST */
};

#define CI_MAXCI         (CI_LAST-1)    /* maximum UPS commands we handle */
#define CI_MAX_CAPS      (CI_LAST_PROBE-1)

#define CI_RemainingCapacity    CI_BATTLEV
#define CI_RunTimeToEmpty       CI_RUNTIM

/*
 * APC_CMD_ is the command code sent to UPS for APC Smart UPSes
 *
 * NOTE: the APC_CMD_s are never used in the actual code,
 * except to initialize the UPS_Cmd[] structure. This way,
 * we will be able to support other UPSes later. The actual
 * command is obtained by reference to UPS_Cmd[CI_xxx]    
 */
#define    APC_CMD_UPSMODEL       0x1
#define    APC_CMD_OLDFWREV       'V'
#define    APC_CMD_STATUS         'Q'
#define    APC_CMD_LQUAL          '9'
#define    APC_CMD_WHY_BATT       'G'
#define    APC_CMD_ST_STAT        'X'
#define    APC_CMD_VLINE          'L'
#define    APC_CMD_VMAX           'M'
#define    APC_CMD_VMIN           'N'
#define    APC_CMD_VOUT           'O'
#define    APC_CMD_BATTLEV        'f'
#define    APC_CMD_VBATT          'B'
#define    APC_CMD_LOAD           'P'
#define    APC_CMD_FREQ           'F'
#define    APC_CMD_RUNTIM         'j'
#define    APC_CMD_ITEMP          'C'
#define    APC_CMD_DIPSW          '7'
#define    APC_CMD_SENS           's'
#define    APC_CMD_DWAKE          'r'
#define    APC_CMD_DSHUTD         'p'
#define    APC_CMD_LTRANS         'l'
#define    APC_CMD_HTRANS         'u'
#define    APC_CMD_RETPCT         'e'
#define    APC_CMD_DALARM         'k'
#define    APC_CMD_DLBATT         'q'
#define    APC_CMD_IDEN           'c'
#define    APC_CMD_STESTI         'E'
#define    APC_CMD_MANDAT         'm'
#define    APC_CMD_SERNO          'n'
#define    APC_CMD_BATTDAT        'x'
#define    APC_CMD_NOMBATTV       'g'
#define    APC_CMD_HUMID          'h'
#define    APC_CMD_REVNO          'b'
#define    APC_CMD_REG1           '~'
#define    APC_CMD_REG2           '\''
#define    APC_CMD_REG3           '8'
#define    APC_CMD_EXTBATTS       '>'
#define    APC_CMD_ATEMP          't'
#define    APC_CMD_NOMOUTV        'o'
#define    APC_CMD_BADBATTS       '<'
#define    APC_CMD_EPROM          0x1a
#define    APC_CMD_ST_TIME        'd'
#define    APC_CMD_CYCLE_EPROM    '-'
#define    APC_CMD_UPS_CAPS       'a'
#define    GO_ON_BATT             'W'
#define    GO_ON_LINE             'X'
#define    LIGHTS_TEST            'A'
#define    FAILURE_TEST           'U'

/*
 * Future additions for contolled discharing of batteries
 * extend lifetimes.
 */

#define DISCHARGE               'D'
#define CHARGE_LIM              25

#define UPS_ENABLED             '?'
#define UPS_ON_BATT             '!'
#define UPS_ON_LINE             '$'
#define UPS_REPLACE_BATTERY     '#'
#define BATT_LOW                '%'
#define BATT_OK                 '+'
#define UPS_EPROM_CHANGE        '|'
#define UPS_TRAILOR             ':'
#define UPS_LF                  '\n'
#define UPS_CR                  '\r'

/* For apclock.c functions */
#define LCKSUCCESS              0  /* lock file does not exist so go */
#define LCKERROR                1  /* lock file not our own and error encountered */
#define LCKEXIST                2  /* lock file is our own lock file */
#define LCKNOLOCK               3  /* lock file not needed: APC_NET */

/* Generic defines for boolean return values. */
#define SUCCESS                 0  /* Function successfull */
#define FAILURE                 1  /* Function failure */

/* These seem unavoidable :-( */
#ifndef TRUE
# define TRUE                   1
#endif
#ifndef FALSE
# define FALSE                  0
#endif


/*
 * We have a timer for the read() for Win32.
 * We have a timer for the select when nothing is expected,
 *   i.e. we prefer waiting for an state change.
 * We have a fast timer, when we are on batteries or when
 *   we expect a response (i.e. we sent a character).
 * And we have a timer for dumb UPSes for doing the sleep().
 */
#define TIMER_READ              10 /* read() timeout, max 25 sec */
#define TIMER_SELECT            60 /* Select when not expecting anything */
#define TIMER_FAST              1  /* Value for fast poll */
#define TIMER_DUMB              5  /* for Dumb (ioctl) UPSes -- keep short */

/* Make the size of these strings the next multiple of 4 */
#define APC_MAGIC               "apcupsd-linux-6.0"
#define APC_MAGIC_SIZE          4 * ((sizeof(APC_MAGIC) + 3) / 4)

#define ACCESS_MAGIC            "apcaccess-linux-4.0"
#define ACCESS_MAGIC_SIZE       4 * ((sizeof(APC_MAGIC) + 3) / 4)


#define MAX_THREADS             7

/* Find members position in the UPSINFO and GLOBALCFG structures. */
#define WHERE(MEMBER) ((size_t) &((UPSINFO *)0)->MEMBER)
#define AT(UPS,OFFSET) ((size_t)UPS + OFFSET)
#define SIZE(MEMBER) ((GENINFO *)sizeof(((UPSINFO *)0)->MEMBER))


/*
 * These are the commands understood by the apccontrol shell script.
 * You _must_ keep the #defines in sync with the commands[] array in
 * action.c
 */
enum {
   CMDPOWEROUT = 0,
   CMDONBATTERY,
   CMDFAILING,
   CMDTIMEOUT,
   CMDLOADLIMIT,
   CMDRUNLIMIT,
   CMDDOSHUTDOWN,
   CMDMAINSBACK,
   CMDANNOYME,
   CMDEMERGENCY,
   CMDCHANGEME,
   CMDREMOTEDOWN,
   CMDCOMMFAILURE,
   CMDCOMMOK,
   CMDSTARTSELFTEST,
   CMDENDSELFTEST,
   CMDOFFBATTERY,        /* off battery power */
   CMDBATTDETACH,        /* Battery disconnected */
   CMDBATTATTACH         /* Battery reconnected */
};

/*
 * Simple way of handling varargs for those compilers that
 * don't support varargs in #defines.
 */
#define Error_abort0(fmd) error_out(__FILE__, __LINE__, fmd)
#define Error_abort1(fmd, arg1) error_out(__FILE__, __LINE__, fmd, arg1)
#define Error_abort2(fmd, arg1,arg2) error_out(__FILE__, __LINE__, fmd, arg1,arg2)
#define Error_abort3(fmd, arg1,arg2,arg3) error_out(__FILE__, __LINE__, fmd, arg1,arg2,arg3)
#define Error_abort4(fmd, arg1,arg2,arg3,arg4) error_out(__FILE__, __LINE__, fmd, arg1,arg2,arg3,arg4)
#define Error_abort5(fmd, arg1,arg2,arg3,arg4,arg5) error_out(__FILE__, __LINE__, fmd, arg1,arg2,arg3,arg4,arg5)
#define Error_abort6(fmd, arg1,arg2,arg3,arg4,arg5,arg6) error_out(__FILE__, __LINE__, fmd, arg1,arg2,arg3,arg4,arg5,arg5)


/*
 * The digit following Dmsg and Emsg indicates the number of substitutions in
 * the message string. We need to do this kludge because non-GNU compilers
 * do not handle varargs #defines.
 */

/* Debug Messages that are printed */
#ifdef DEBUG

#define Dmsg0(lvl, msg)             d_msg(__FILE__, __LINE__, lvl, msg)
#define Dmsg1(lvl, msg, a1)         d_msg(__FILE__, __LINE__, lvl, msg, a1)
#define Dmsg2(lvl, msg, a1, a2)     d_msg(__FILE__, __LINE__, lvl, msg, a1, a2)
#define Dmsg3(lvl, msg, a1, a2, a3) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3)
#define Dmsg4(lvl, msg, arg1, arg2, arg3, arg4) d_msg(__FILE__, __LINE__, lvl, msg, arg1, arg2, arg3, arg4)
#define Dmsg5(lvl, msg, a1, a2, a3, a4, a5) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5)
#define Dmsg6(lvl, msg, a1, a2, a3, a4, a5, a6) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6)
#define Dmsg7(lvl, msg, a1, a2, a3, a4, a5, a6, a7) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6, a7)
#define Dmsg8(lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8) d_msg(__FILE__, __LINE__, lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8)
#define Dmsg11(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) d_msg(__FILE__,__LINE__,lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)
void d_msg(const char *file, int line, int level, const char *fmt, ...);

#else

#define Dmsg0(lvl, msg)
#define Dmsg1(lvl, msg, a1)
#define Dmsg2(lvl, msg, a1, a2)
#define Dmsg3(lvl, msg, a1, a2, a3)
#define Dmsg4(lvl, msg, arg1, arg2, arg3, arg4)
#define Dmsg5(lvl, msg, a1, a2, a3, a4, a5)
#define Dmsg6(lvl, msg, a1, a2, a3, a4, a5, a6)
#define Dmsg7(lvl, msg, a1, a2, a3, a4, a5, a6, a7)
#define Dmsg8(lvl, msg, a1, a2, a3, a4, a5, a6, a7, a8)
#define Dmsg11(lvl,msg,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)

#endif


/* These probably should be subroutines */
#define P(x) \
   do { \
      int errstat; \
      if ((errstat=pthread_mutex_lock(&(x)))) \
         error_out(__FILE__, __LINE__, "Mutex lock failure. ERR=%s\n", strerror(errstat)); \
   } while(0)

#define V(x) \
   do { \
      int errstat; \
      if ((errstat=pthread_mutex_unlock(&(x)))) \
         error_out(__FILE__, __LINE__, "Mutex unlock failure. ERR=%s\n", strerror(errstat)); \
   } while(0)


/* Send terminate signal to itself. */
#define sendsig_terminate() \
   { \
      kill(getpid(), SIGTERM); \
      exit(0); \
   }

/* Determine the difference, in milliseconds, between two struct timevals. */
#define TV_DIFF_MS(a, b) \
    (((b).tv_sec - (a).tv_sec) * 1000 + ((b).tv_usec - (a).tv_usec) / 1000)

/*
 * Some platforms, like Solaris, hide MIN/MAX in an obscure header.
 * It's easiest just to define them ourselves instead of trying to
 * find the right thing to include.
 */
#ifndef MIN
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#endif
#ifndef MAX
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#endif

#endif   /* _DEFINES_H */
