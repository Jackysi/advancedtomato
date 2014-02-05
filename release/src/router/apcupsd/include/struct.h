/*
 * struct.h
 *
 * Common apcupsd structures.
 */

/*
 * Copyright (C) 2000-2005 Kern Sibbald
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

#ifndef _STRUCT_H
#define _STRUCT_H

typedef enum {
   NO_CABLE = 0,        /* Default Disable            */

   // All simple cable types
   CUSTOM_SIMPLE,       /* SIMPLE cable simple        */
   APC_940_0119A,       /* APC cable number 940-0119A */
   APC_940_0127A,       /* APC cable number 940-0127A */
   APC_940_0128A,       /* APC cable number 940-0128A */
   APC_940_0020B,       /* APC cable number 940-0020B */
   APC_940_0020C,       /* APC cable number 940-0020C identical to 20B */
   APC_940_0023A,       /* APC cable number 940-0023A */
   MAM_CABLE,           /* MAM cable for Alfatronic SPS500X */

   // These can (apparently) act as smart or simple cables; for our
   // purposes we treat them as simple cables.
   APC_940_0095A,       /* APC cable number 940-0095A */
   APC_940_0095B,       /* APC cable number 940-0095B */
   APC_940_0095C,       /* APC cable number 940-0095C */

   // All smart cable types
   CABLE_SMART,         /* SMART cable smart          */

   // Other cables
   APC_NET,             /* Ethernet Link              */
   USB_CABLE            /* USB cable */
} UpsCable;

typedef enum {
   NO_UPS = 0,          /* Default Disable      */
   DUMB_UPS,            /* Dumb UPS driver      */
   APCSMART_UPS,        /* APC Smart UPS (any)  */
   USB_UPS,             /* USB UPS driver       */
   SNMP_UPS,            /* SNMP UPS driver      */
   NETWORK_UPS,         /* NETWORK UPS driver   */
   TEST_UPS,            /* TEST UPS Driver      */
   PCNET_UPS,           /* PCNET UPS Driver     */
   SNMPLITE_UPS,        /* SNMP Lite UPS Driver */
} UpsMode;

typedef enum {
   NO_CLASS = 0,
   STANDALONE,
   SHARESLAVE,
   SHAREMASTER,
} ClassMode;

typedef enum {
   NO_SHARE_NET = 0,
   DISABLE,             /* Disable Share or Net UPS  */
   SHARE,               /* ShareUPS Internal         */
} ShareNetMode;

typedef enum {
   NO_LOGON = 0,
   NEVER,               /* Disable Setting NoLogon               */
   TIMEOUT,             /* Based on TIMEOUT + 10 percent         */
   PERCENT,             /* Based on PERCENT + 10 percent         */
   MINUTES,             /* Based on MINUTES + 10 percent         */
   ALWAYS               /* Stop All New Login Attempts. but ROOT */
} NoLoginMode;

/* List all the internal self tests allowed. */
typedef enum {
   SMART_TEST_LEDS = 0,
   SMART_TEST_SELFTEST,
   SMART_TEST_POWERFAIL,
   SMART_TEST_CALIBRATION,
   SMART_CHANGE_NAME,
   SMART_CHANGE_BATTDATE,
   SMART_CHANGE_EPROM,
   SMART_TEST_MAX
} SelfTests;

typedef enum {
   XFER_NA = 0,          /* Not supported by this UPS */
   XFER_NONE,            /* No xfer since power on */
   XFER_OVERVOLT,        /* Utility voltage too high */
   XFER_UNDERVOLT,       /* Utility voltage too low */
   XFER_NOTCHSPIKE,      /* Line voltage notch or spike */
   XFER_RIPPLE,          /* Excessive utility voltage rate of change */
   XFER_SELFTEST,        /* Auto or manual self test */
   XFER_FORCED,          /* Forced onto battery by sw command */
   XFER_FREQ,            /* Input frequency out of range */
   XFER_UNKNOWN
} LastXferCause;

typedef enum {
   TEST_NA = 0,          /* Not supported by this UPS */
   TEST_NONE,            /* No self test result available */
   TEST_FAILED,          /* Test failed (reason unknown) */
   TEST_WARNING,         /* Test passed with warning */
   TEST_INPROGRESS,      /* Test currently in progress */
   TEST_PASSED,          /* Test passed */
   TEST_FAILCAP,         /* Test failed due to insufficient capacity */
   TEST_FAILLOAD,        /* Test failed due to overload */
   TEST_UNKNOWN
} SelfTestResult;

typedef struct geninfo {
   const char *name;               /* JHNC: name mustn't contain whitespace */
   const char *long_name;
   int type;
} GENINFO;                         /* for static declaration of data */

typedef struct internalgeninfo {
   char name[MAXSTRING];           /* JHNC: name mustn't contain whitespace */
   char long_name[MAXSTRING];
   int type;
} INTERNALGENINFO;                 /* for assigning into upsinfo */


class UPSINFO {
 public:
   /* Methods */
   void clear_battlow() { Status &= ~UPS_battlow; };
   void clear_boost() { Status &= ~UPS_boost; };
   void clear_calibration() { Status &= ~UPS_calibration; };
   void clear_commlost() { Status &= ~UPS_commlost; };
   void clear_dev_setup() { Status &= ~UPS_dev_setup; };
   void clear_fastpoll() { Status &= ~UPS_fastpoll; };
   void clear_onbatt_msg() { Status &= ~UPS_onbatt_msg; };
   void clear_onbatt() { Status &= ~UPS_onbatt; };
   void clear_online() { Status |= UPS_onbatt; Status &= ~UPS_online; };
   void clear_overload() { Status &= ~UPS_overload; };
   void clear_plugged() { Status &= ~UPS_plugged; };
   void clear_replacebatt() { Status &= ~UPS_replacebatt; };
   void clear_shut_btime() { Status &= ~UPS_shut_btime; };
   void clear_shutdown() { Status &= ~UPS_shutdown; };
   void clear_shut_emerg() { Status &= ~UPS_shut_emerg; };
   void clear_shut_load() { Status &= ~UPS_shut_load; };
   void clear_shut_ltime() { Status &= ~UPS_shut_ltime; };
   void clear_shut_remote() { Status &= ~UPS_shut_remote; };
   void clear_slavedown() { Status &= ~UPS_slavedown; };
   void clear_slave() { Status &= ~UPS_slave; };
   void clear_trim() { Status &= ~UPS_trim; };
   void clear_battpresent() {Status &= ~UPS_battpresent; };

   void set_battlow() { Status |= UPS_battlow; };
   void set_battlow(int val) { if (val) set_battlow(); else clear_battlow(); };
   void set_boost() { Status |= UPS_boost; };
   void set_boost(int val) { if (val) set_boost(); else clear_boost(); };
   void set_calibration() { Status |= UPS_calibration; };
   void set_commlost() { Status |= UPS_commlost; };
   void set_dev_setup() { Status |= UPS_dev_setup; };
   void set_fastpoll() { Status |= UPS_fastpoll; };
   void set_onbatt_msg() { Status |= UPS_onbatt_msg; };
   void set_onbatt() { Status |= UPS_onbatt; };
   void set_online() { Status |= UPS_online; Status &= ~UPS_onbatt; };
   void set_online(int val) { if (val) set_online(); else clear_online(); };
   void set_overload() { Status |= UPS_overload; };
   void set_overload(int val) { if (val) set_overload(); else clear_overload(); };
   void set_plugged() { Status |= UPS_plugged; };
   void set_replacebatt() { Status |= UPS_replacebatt; };
   void set_replacebatt(int val) { if (val) set_replacebatt(); else clear_replacebatt(); };
   void set_shut_btime() { Status |= UPS_shut_btime; };
   void set_shut_btime(int val) { if (val) set_shut_btime(); else clear_shut_btime(); };
   void set_shutdown() { Status |= UPS_shutdown; };
   void set_shut_emerg() { Status |= UPS_shut_emerg; };
   void set_shut_emerg(int val) { if (val) set_shut_emerg(); else clear_shut_emerg(); };
   void set_shut_load() { Status |= UPS_shut_load; };
   void set_shut_load(int val) { if (val) set_shut_load(); else clear_shut_load(); };
   void set_shut_ltime() { Status |= UPS_shut_ltime; };
   void set_shut_ltime(int val) { if (val) set_shut_ltime(); else clear_shut_ltime(); };
   void set_shut_remote() { Status |= UPS_shut_remote; };
   void set_slavedown() { Status |= UPS_slavedown; };
   void set_slavedown(int val) { if (val) set_slavedown(); else clear_slavedown(); };
   void set_slave() { Status |= UPS_slave; };
   void set_trim() { Status |= UPS_trim; };
   void set_trim(int val) { if (val) set_trim(); else clear_trim(); };
   void set_battpresent() { Status |= UPS_battpresent; };
   void set_battpresent(int val) { if (val) set_battpresent(); else clear_battpresent(); };

   bool is_battlow() const { return (Status & UPS_battlow) == UPS_battlow; };
   bool is_boost() const { return (Status & UPS_boost) == UPS_boost; };
   bool is_calibration() const { return (Status & UPS_calibration) == UPS_calibration; };
   bool is_commlost() const { return (Status & UPS_commlost) == UPS_commlost; };
   bool is_dev_setup() const { return (Status & UPS_dev_setup) == UPS_dev_setup; };
   bool is_fastpoll() const { return (Status & UPS_fastpoll) == UPS_fastpoll; };
   bool is_onbatt() const { return (Status & UPS_onbatt) == UPS_onbatt; };
   bool is_onbatt_msg() const { return (Status & UPS_onbatt_msg) == UPS_onbatt_msg; };
   bool is_online() const { return (Status & UPS_online) == UPS_online; };
   bool is_overload() const { return (Status & UPS_overload) == UPS_overload; };
   bool is_plugged() const { return (Status & UPS_plugged) == UPS_plugged; };
   bool is_replacebatt() const { return (Status & UPS_replacebatt) == UPS_replacebatt; };
   bool is_shut_btime() const { return (Status & UPS_shut_btime) == UPS_shut_btime; };
   bool is_shutdown() const { return (Status & UPS_shutdown) == UPS_shutdown; };
   bool is_shut_emerg() const { return (Status & UPS_shut_emerg) == UPS_shut_emerg; };
   bool is_shut_load() const { return (Status & UPS_shut_load) == UPS_shut_load; };
   bool is_shut_ltime() const { return (Status & UPS_shut_ltime) == UPS_shut_ltime; };
   bool is_shut_remote() const { return (Status & UPS_shut_remote) == UPS_shut_remote; };
   bool is_slave() const { return (Status & UPS_slave) == UPS_slave; };
   bool is_slavedown() const { return (Status & UPS_slavedown) == UPS_slavedown; };
   bool is_trim() const { return (Status & UPS_trim) == UPS_trim; };
   bool is_battpresent() const { return (Status & UPS_battpresent) == UPS_battpresent; };

   bool chg_battlow() const { return ((Status ^ PrevStatus) & UPS_battlow) == UPS_battlow; };
   bool chg_onbatt() const { return ((Status ^ PrevStatus) & UPS_onbatt) == UPS_onbatt; };
   bool chg_battpresent() const { return ((Status ^ PrevStatus) & UPS_battpresent) == UPS_battpresent; };
   bool chg_shut_remote() const { return ((Status ^ PrevStatus) & UPS_shut_remote) == UPS_shut_remote; };

   /* DATA */
   int fd;                         /* UPS device node file descriptor */

   /* UPS capability array and codes */
   char UPS_Cap[CI_MAXCI + 1];          /* TRUE if UPS has capability */
   unsigned int UPS_Cmd[CI_MAXCI + 1];  /* Command or function code */

   INTERNALGENINFO cable;          /* UPSCABLE directive */
   INTERNALGENINFO nologin;        /* NOLOGON directive */
   INTERNALGENINFO mode;           /* UPSTYPE directive */
   INTERNALGENINFO upsclass;       /* UPSCLASS directive */
   INTERNALGENINFO sharenet;       /* UPSMODE directive */

   int num_execed_children;        /* children created in execute_command() */

   /* Internal state flags set in response to UPS condition */
   time_t ShutDown;                /* set when doing shutdown */
   time_t SelfTest;                /* start time of self test */
   time_t LastSelfTest;            /* time of last self test */
   time_t poll_time;               /* last time UPS polled -- fillUPS() */
   time_t start_time;              /* time apcupsd started */
   time_t last_onbatt_time;        /* last time on batteries */
   time_t last_offbatt_time;       /* last time off batteries */
   time_t last_time_on_line;
   time_t last_time_annoy;
   time_t last_time_nologon;
   time_t last_time_changeme;
   time_t last_master_connect_time;     /* last time master connected */
   time_t start_shut_ltime;
   time_t start_shut_load;
   time_t start_shut_lbatt;
   int num_xfers;                  /* number of times on batteries */
   int cum_time_on_batt;           /* total time on batteries since startup */
   int wait_time;                  /* suggested wait time for drivers in 
                                    * device_check_state() */

   /* Items reported by smart UPS */
   /* Volatile items -- i.e. they change with the state of the UPS */
   char linequal[8];               /* Line quality */
   unsigned int reg1;              /* register 1 */
   unsigned int reg2;              /* register 2 */
   unsigned int reg3;              /* register 3 */
   unsigned int dipsw;             /* dip switch info */
   unsigned int InputPhase;        /* The current AC input phase. */
   unsigned int OutputPhase;       /* The current AC output phase. */
   LastXferCause lastxfer;         /* Reason for last xfer to battery */
   SelfTestResult testresult;      /* results of last seft test */
   double BattChg;                 /* remaining UPS charge % */
   double LineMin;                 /* min line voltage seen */
   double LineMax;                 /* max line voltage seen */
   double UPSLoad;                 /* battery load percentage */
   double LineFreq;                /* line freq. */
   double LineVoltage;             /* Line Voltage */
   double OutputVoltage;           /* Output Voltage */
   double OutputFreq;              /* Output Frequency */
   double OutputCurrent;           /* Output Current */
   double UPSTemp;                 /* UPS internal temperature */
   double BattVoltage;             /* Actual Battery voltage -- about 24V */
   double LastSTTime;              /* hours since last self test -- not yet implemented */
   int32_t Status;                 /* UPS status (Bitmapped) */
   int32_t PrevStatus;             /* Previous UPS status */
   double TimeLeft;                /* Est. time UPS can run on batt. */
   double humidity;                /* Humidity */
   double ambtemp;                 /* Ambient temperature */
   char eprom[500];                /* Eprom values */

   /* Items reported by smart UPS */
   /* Static items that normally do not change during UPS operation */
   int NomOutputVoltage;           /* Nominal voltage when on batteries */
   int NomInputVoltage;            /* Nominal input voltage */
   int NomPower;                   /* Nominal power (watts) */
   double nombattv;                /* Nominal batt. voltage -- not actual */
   int extbatts;                   /* number of external batteries attached */
   int badbatts;                   /* number of bad batteries */
   int lotrans;                    /* min line voltage before using batt. */
   int hitrans;                    /* max line voltage before using batt. */
   int rtnpct;                     /* % batt charge necessary for return */
   int dlowbatt;                   /* low batt warning in mins. */
   int dwake;                      /* wakeup delay seconds */
   int dshutd;                     /* shutdown delay seconds */
   char birth[20];                 /* manufacture date */
   char serial[32];                /* serial number */
   char battdat[20];               /* battery installation date */
   char selftest[9];               /* selftest interval as ASCII */
   char firmrev[20];               /* firmware revision */
   char upsname[UPSNAMELEN];       /* UPS internal name */
   char upsmodel[MAXSTRING];       /* ups model number */
   char sensitivity[8];            /* sensitivity to line fluxuations */
   char beepstate[8];              /* when to beep on power failure. */

   /* Items specified from config file */
   int annoy;
   int maxtime;
   int annoydelay;                 /* delay before annoying users with logoff request */
   int onbattdelay;                /* delay before reacting to a power failure */
   int killdelay;                  /* delay after pwrfail before issuing UPS shutdown */
   int nologin_time;
   int nologin_file;
   int stattime;
   int datatime;
   int sysfac;
   int polltime;                   /* Time interval to poll the UPS */
   int percent;                    /* shutdown when batt % less than this */
   int runtime;                    /* shutdown when runtime less than this */
   char nisip[64];                 /* IP for NIS */
   int statusport;                 /* NIS port */
   int netstats;                   /* turn on/off network status */
   int logstats;                   /* turn on/off logging of status info */
   char device[MAXSTRING];         /* device name in use */
   char configfile[APC_FILENAME_MAX];   /* config filename */
   char statfile[APC_FILENAME_MAX];     /* status filename */
   char eventfile[APC_FILENAME_MAX];    /* temp events file */
   int eventfilemax;               /* max size of eventfile in kilobytes */
   int event_fd;                   /* fd for eventfile */

   char master_name[APC_FILENAME_MAX];
   char lockpath[APC_FILENAME_MAX];
   int lockfile;

   char scriptdir[APC_FILENAME_MAX];    /* Path to apccontrol dir */
   char pwrfailpath[APC_FILENAME_MAX];  /* Path to powerfail flag file dir */
   char nologinpath[APC_FILENAME_MAX];  /* Path to nologin dir */

   int ChangeBattCounter;          /* For UPS_REPLACEBATT, see apcaction.c */

   pthread_mutex_t mutex;
   int refcnt;                     /* thread attach count */

   const struct upsdriver *driver; /* UPS driver for this UPSINFO */
   void *driver_internal_data;     /* Driver private data */
};


/*These are needed for commands executed in action.c */
typedef struct {
   const char *command;
   int pid;
} UPSCOMMANDS;

typedef struct s_cmd_msg {
   int level;
   const char *msg;
} UPSCMDMSG;

#endif   /* _STRUCT_H */
