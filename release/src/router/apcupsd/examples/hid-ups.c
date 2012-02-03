/*
 * $Id: hid-ups.c,v 1.13.2.2 2007/07/17 22:54:25 adk0212 Exp $
 *
 *  Copyright (c) 2001 Vojtech Pavlik <vojtech@ucw.cz>
 *  Copyright (c) 2001 Paul Stewart <hiddev@wetlogic.net>
 *
 *    Tweaked by Kern Sibbald <kern@sibbald.com> to learn
 *    about USB UPSes.			      
 *
 *  HID UPS device test program
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1492, Prague 8, 182 00 Czech Republic
 */

#define DEBUG 1 		      /* if set prints full reports */
#define TESTING 1		      /* if set disables actual operation */

#define HID_MAX_USAGES 1024

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <asm/types.h>
#include <linux/hiddev.h>
#include <errno.h>
#include <time.h>

#define PWRSTAT         "/etc/powerstatus"
#define DEBOUNCE_TIMEOUT 15	      /* increase this if you get false alerts */

#define UPS_USAGE		0x840000
#define UPS_SERIAL		0x8400fe
#define BAT_CHEMISTRY		0x850089
#define UPS_CAPACITY_MODE	0x85002c

#define UPS_SHUTDOWN_IMMINENT	0x840069
#define UPS_BATTERY_VOLTAGE	0x840030
#define UPS_BELOW_RCL		0x840042
#define UPS_CHARING		0x840044
#define UPS_DISCHARGING 	0x850045
#define UPS_REMAINING_CAPACITY	0x850066
#define UPS_RUNTIME_TO_EMPTY	0x850068
#define UPS_AC_PRESENT		0x8500d0

#define STATE_NORMAL 0		      /* unit powered */
#define STATE_DEBOUNCE 1	      /* power failure */
#define STATE_BATTERY 2 	      /* power failure confirmed */

/* USB Vendor ID's */
#define Vendor_APC 0x51D
#define Vendor_MGE 0x463


#ifdef DEBUG
/*
 *    The type field indicates the type or units to be
 *    applied to the value.
 */

#define T_NONE	   0		      /* No units */
#define T_INDEX    1		      /* String index */
#define T_CAPACITY 2		      /* Capacity (usually %) */
#define T_BITS	   3		      /* bit field */
#define T_UNITS    4		      /* use units/exponent field */
#define T_DATE	   5		      /* date */
#define T_APCDATE  6		      /* APC date format */

struct s_ups_info {
    unsigned usage;
    int type;
    char *label;
} ups_info[] = {

    /* MGE & APC */
    { 0x000000, T_NONE,   "---" },
    /* Page 0x84 is the Power Device Page */
    { 0x840000, T_NONE,    "UPS-Power" },
    { 0x840001, T_INDEX,   "iName" },
    { 0x840004, T_NONE,    "UPS" },
    { 0x840012, T_NONE,    "Battery" },
    { 0x840016, T_NONE,    "PowerConverter" },
    { 0x840018, T_NONE,    "OutletSystem" },
    { 0x840017, T_NONE,    "PowerConverterID" },
    { 0x840019, T_NONE,    "OutletSystemID" },
    { 0x84001a, T_NONE,    "Input" },
    { 0x84001c, T_NONE,    "Output" },
    { 0x84001e, T_NONE,    "Flow" }, 
    { 0x84001d, T_NONE,    "OutputID" },
    { 0x84001f, T_NONE,    "FlowID" },
    { 0x840020, T_NONE,    "Outlet" },
    { 0x840021, T_NONE,    "OutletID" },
    { 0x840024, T_NONE,    "PowerSummary" },
    { 0x840025, T_NONE,    "PowerSummaryID" },
    { 0x840030, T_UNITS,   "Voltage" },
    { 0x840031, T_UNITS,   "Current" },
    { 0x840032, T_UNITS,   "Frequency" },
    { 0x840033, T_UNITS,   "ApparentPower" },
    { 0x840034, T_UNITS,   "ActivePower" },
    { 0x840035, T_UNITS,   "PercentLoad" },
    { 0x840036, T_UNITS,   "Temperature" },
    { 0x840037, T_UNITS,   "Humidity" },
    { 0x840040, T_UNITS,   "ConfigVoltage" },
    { 0x840042, T_UNITS,   "ConfigFrequency" },
    { 0x840043, T_UNITS,   "ConfigApparentPower" },
    { 0x840044, T_UNITS,   "ConfigActivePower" },
    { 0x840053, T_UNITS,   "LowVoltageTransfer" },
    { 0x840054, T_UNITS,   "HighVoltageTransfer" },
    { 0x840055, T_UNITS,   "DelayBeforeReboot" },
    { 0x840056, T_UNITS,   "DelayBeforeStartup" },
    { 0x840057, T_NONE,    "DelayBeforeShutdown" },
    { 0x840058, T_NONE,    "Test" },
    { 0x84005a, T_NONE,    "AudibleAlarmControl" },
    { 0x840061, T_NONE,    "Good" },
    { 0x840062, T_NONE,    "InternalFailure" },
    { 0x840065, T_NONE,    "Overload" },
    { 0x840068, T_NONE,    "ShutdownRequested" },
    { 0x840069, T_NONE,    "ShutdownImminent" },
    { 0x84006b, T_NONE,    "Switch On/Off" },
    { 0x84006c, T_NONE,    "Switchable" },
    { 0x84006e, T_NONE,    "Boost" },
    { 0x84006f, T_NONE,    "Trim" },
    { 0x840073, T_NONE,    "CommunicationLost" },
    { 0x8400fd, T_INDEX,   "iManufacturer" },
    { 0x8400fe, T_INDEX,   "iProduct" },
    { 0x8400ff, T_INDEX,   "iSerialNumber" },
    /* Page 0x85 is the Battery System Page */
    { 0x850000, T_NONE,     "UPS-Batt" },
    { 0x850029, T_CAPACITY, "RemainingCapacityLimit" },
    { 0x85002a, T_UNITS,    "RemainingTimeLimit" },
    { 0x85002c, T_CAPACITY, "CapacityMode" },
    { 0x850042, T_NONE,     "BelowRemainingCapacityLimit" },
    { 0x850043, T_NONE,     "RemainingTimeLimitExpired" },
    { 0x850044, T_NONE,     "Charging" },
    { 0x850045, T_NONE,     "Discharging" },
    { 0x85004b, T_NONE,     "NeedReplacement" },
    { 0x850058, T_NONE,     "BUPHibernate" },           /* APC proprietary */
    { 0x850066, T_CAPACITY, "RemainingCapacity" },
    { 0x850067, T_CAPACITY, "FullChargeCapacity" },
    { 0x850068, T_UNITS,    "RunTimeToEmpty" },
    { 0x85006b, T_NONE,     "CycleCount" },
    { 0x850080, T_NONE,     "BattPackLevel" },
    { 0x850083, T_CAPACITY, "DesignCapacity" },
    { 0x850085, T_DATE,     "ManufactureDate" },
    { 0x850088, T_INDEX,    "iDeviceName" },
    { 0x850089, T_INDEX,    "iDeviceChemistry" },
    { 0x85008b, T_NONE,     "Rechargeable" },
    { 0x85008c, T_CAPACITY, "WarningCapacityLimit" },
    { 0x85008d, T_CAPACITY, "CapacityGranularity1" },
    { 0x85008e, T_CAPACITY, "CapacityGranularity2" },
    { 0x85008f, T_INDEX,    "iOEMInformation" },
    { 0x8500d0, T_NONE,     "ACPresent" },
    { 0x8500d1, T_NONE,     "BatteryPresent" },
    { 0x8500db, T_NONE,     "VoltageNotRegulated" },
    /*
     * Page 0x86 is reserved for Power Devices, but not defined in the HID
     * standard. APC has defined a few usages on this page for themselves.
     */
    { 0x860010, T_NONE,     "BUPSelfTest" },             /* APC proprietary */
    { 0x860012, T_NONE,     "BUPBattCapBeforeStartup" }, /* APC proprietary */
    { 0x860076, T_NONE,     "BUPDelayBeforeStartup" },   /* APC proprietary */
    /* Pages 0xFF00 to 0xFFFF are vendor specific */
    { 0xFF860005, T_NONE,   "APCGeneralCollection" },
    { 0xFF860013, T_NONE,   "APC860013_SetMinReturn?" },
    { 0xFF860016, T_APCDATE,"APCBattReplacementDate" },
    { 0xFF860019, T_UNITS,  "APCBattCapBeforeStartup" },
    { 0xFF860023, T_NONE,   "APC860023_??????" },
    { 0xFF860024, T_NONE,   "APC860024_??????" },
    { 0xFF860025, T_NONE,   "APC860025_??????" },
    { 0xFF860026, T_NONE,   "APC860026_??????" },
    { 0xFF860029, T_NONE,   "APC860029_??????" },
    { 0xFF86002A, T_NONE,   "APC86002A_??????" },
    { 0xFF860042, T_NONE,   "APC_UPS_FirmwareRevision" },
    { 0xFF860052, T_NONE,   "APCLineFailCause" },
    { 0xFF860060, T_BITS,   "APCStatusFlag" },
    { 0xFF860061, T_NONE,   "APCSensitivity" },
    { 0xFF860062, T_NONE,   "APC860062_SetHiTransV?" }, 
    { 0xFF860064, T_NONE,   "APC860064_SetLoTransV?" },
    { 0xFF860072, T_NONE,   "APCPanelTest" },
    { 0xFF860074, T_NONE,   "APC860074_SetSens?" },
    { 0xFF860076, T_UNITS,  "APCShutdownAfterDelay" },
    { 0xFF860077, T_NONE,   "APC860077_SetWakeUpDelay?" },
    { 0xFF860079, T_NONE,   "APC_USB_FirmwareRevision" },
    { 0xFF86007C, T_NONE,   "APCForceShutdown" },
    { 0xFF86007D, T_UNITS,  "APCDelayBeforeShutdown" },
    { 0xFF86007E, T_UNITS,  "APCDelayBeforeStartup" },

};
#define UPS_INFO_SZ (sizeof(ups_info)/sizeof(ups_info[0]))

char *reports[] = { "Unknown", "Input", "Output", "Feature" };

static int CapacityMode = 2;	      /* default = % */

char unknown[24];

#define MADDR "stewart@wetlogic.net"

void log_status(char *msg) {
#ifndef TESTING
    char buf[256];
    printf("[Log message \"%s\"]\n", msg);
    sprintf(buf, "/bin/echo %s | /bin/mail -s \"UPS System\" %s", msg, MADDR);
    system(buf);
#else
    printf("[Log message \"%s\"]\n", msg);
#endif
}

static inline char* info(unsigned int detail) {
    int i;
    
    for (i = 0; i < UPS_INFO_SZ; i++) {
	if (ups_info[i].usage == detail) {
	    return ups_info[i].label;
	}
    }

    sprintf(unknown, "[%06x]", detail);

    return unknown;
}

static struct s_ups_info *info_entry(unsigned int detail) {
    int i;
    static struct s_ups_info info = {0, T_NONE, unknown};
    
    for (i = 0; i < UPS_INFO_SZ; i++) {
	if (ups_info[i].usage == detail) {
	    return &ups_info[i];
	}
    }
    sprintf(unknown, "[%06x]", detail);
    return &info;    
}

#else /* DEBUG */
#define log_status(s)
#endif /* DEBUG */

/* Tell init the power has either gone or is back. */
void powerfail(int state) {
#ifndef TESTING
    int fd;
    
    /* Create an info file needed by init to shutdown/cancel shutdown */
    unlink(PWRSTAT);
    if ((fd = open(PWRSTAT, O_CREAT|O_WRONLY, 0644)) >= 0) {
	if (state > 0)
            write(fd, "FAIL\n", 5);
	else if (state < 0)
            write(fd, "LOW\n", 4);
	else
            write(fd, "OK\n", 3);
	close(fd);
    }
    kill(1, SIGPWR);
#else
    printf("We are in powerfail() with state=%d ", state);
    if (state > 0)
        printf("POWER FAILURE\n");
    else if (state < 0)
        printf("BATTERY LOW\n");
    else
        printf("OK\n");
#endif
    
}

static inline int find_application(int fd, unsigned usage) {
	int i = 0, ret;
	while ((ret = ioctl(fd, HIDIOCAPPLICATION, i)) > 0 &&
	       (ret & 0xffff0000) != (usage & 0xffff0000)) i++;
	return ((ret & 0xffff000) == (usage & 0xffff0000));
}

/*
 * Get a string from the device given the string's index
 */
static char *get_string(int fd, int sindex) {
    static struct hiddev_string_descriptor sdesc;
    static char buf[200];

    if (sindex == 0) {
       return "";
    }
    sdesc.index = sindex;
    if (ioctl(fd, HIDIOCGSTRING, &sdesc) < 0) {
        sprintf(buf, "String index %d returned ERR=%s\n", sindex,
	    strerror(errno));
	return buf;
    }
    return sdesc.value;
}

/*
 *   Give units code and exponent, returns string 
 *    describing the units used.  (doesn't work for percentages).
 */
static char *get_units(unsigned unit, unsigned exponent) {
    static char buf[200];

    if (exponent > 7) {
       exponent = exponent - 16;
    }
    switch (unit) {
    case 1:			      /* special kludge for CapacityMode */
	switch (exponent) {
	case 0:
           return "maH";
	case 1:
           return "mwH";
	case 2:
           return "percent";
	case 3:
           return "boolean";
	default:
           return "";
	}
    case 0x00F0D121:
	if (exponent == 7) {
           return "Volts";
	} else if (exponent == 5) {
           return "CentiVolts";
	} else if (exponent == 6) {
           return "DeciVolts";
	} else {
           sprintf(buf, "Volts with %d exponent", exponent);
	   return buf;
	}
    case 0x00100001:
	if (exponent == -2) {
           return "CentiAmps";
	} else if (exponent == 0) {
           return "Amps";
	} else {
           sprintf(buf, "Amps with %d exponent", exponent);
	   return buf;
	}
    case 0xF001:
	if (exponent == 0) {
           return "Hertz";
	} else if (exponent == -2) {
           return "CentiHertz";
	} else {
           sprintf(buf, "Hertz with %d exponent", exponent);
	   return buf;
	}
    case 0x1001:
	if (exponent == 0) {
           return "Seconds";
	} else {
           sprintf(buf, "Seconds with %d exponent", exponent);
	   return buf;
	}
    case 0xD121:
        return "Watts";
    case 0x010001:
	if (exponent == 0) {
           return "Degrees K";
	} else {
           sprintf(buf, "Degrees K with %d exponent", exponent);
	   return buf;
	}
    case 0x0101001:
        return "AmpSecs";
    case 0:
        return "";
    default:
        sprintf(buf, "0x%x", unit);
	return buf;
    }
}

       
static char evdev[50];

static int vendor = 0;

int main (int argc, char **argv) {
    time_t start_seconds;
    int fd = -1, rd, i, j, RemainingCapacity;
    struct hiddev_event ev[64];
    struct hiddev_devinfo dinfo;
    char name[256] = "Unknown";
    int state = 0;
    fd_set fdset;
    struct timeval timev, *tv = NULL;
    struct hiddev_usage_ref uref;

    if (argc < 2) {
	struct hiddev_usage_ref uref;
         /* deal with either standard location or Red Hat's */
         const char *hid_dirs[] = {"/dev/usb/hid", "/dev/usb","/dev"};
	 for (i = 0; i < sizeof(hid_dirs)/sizeof(hid_dirs[0]); i++) {
	     for (j = 0; j < 4; j++) {
                 sprintf(evdev, "%s/hiddev%d", hid_dirs[i], j);
		 if ((fd = open(evdev, O_RDONLY)) < 0) {
		     if (errno == EACCES) {
                         fprintf(stderr, "No permission, try this as root.\n");
			 exit(1);
		     }
		 } else {
		     if (find_application(fd, UPS_USAGE)) goto foundit;
		     close(fd);
		 }
	      }
	  }
          fprintf(stderr, "Couldn't find USB UPS device, check your /dev.\n");
	  exit(1);
foundit:
          printf("Found UPS at %s\n", evdev);
      } else {
	   strncpy(evdev, argv[argc -1], sizeof(evdev)-1);
           printf("Found UPS at %s\n", evdev);
	if ((fd = open(evdev, O_RDONLY)) < 0) {
            perror("hiddev open");
	    exit(1);
	}
	if (!find_application(fd, UPS_USAGE)) {
            fprintf(stderr, "%s is not a UPS\n", argv[argc - 1]);
	    exit(1);
	}
    }

#ifdef DEBUG
    {
	unsigned version;

	ioctl(fd, HIDIOCGVERSION, &version);
        printf("hiddev driver version is %d.%d.%d\n",
	       version >> 16, (version >> 8) & 0xff, version & 0xff);
	
	ioctl(fd, HIDIOCGDEVINFO, &dinfo);
        printf("HID: vendor 0x%x product 0x%x version 0x%x ",
	       dinfo.vendor, dinfo.product & 0xffff, dinfo.version);
        printf("app %s", info(ioctl(fd, HIDIOCAPPLICATION, 0)));
	for (i = 1; i < dinfo.num_applications; i++)
            printf(", %s", info(ioctl(fd, HIDIOCAPPLICATION, i)));
        printf("\n");
        printf("HID: bus: %d devnum: %d ifnum: %d\n",
	       dinfo.busnum, dinfo.devnum, dinfo.ifnum);
	vendor = dinfo.vendor;
		   
    }
#endif

    ioctl(fd, HIDIOCINITREPORT, 0);
    ioctl(fd, HIDIOCGNAME(sizeof(name)), name);
    printf("UPS HID device name: \"%s\"\n", name);


    memset(&uref, 0, sizeof(uref));
    uref.report_type = HID_REPORT_TYPE_FEATURE;
    uref.report_id = HID_REPORT_ID_UNKNOWN;
    uref.usage_code = BAT_CHEMISTRY;
    if (ioctl(fd, HIDIOCGUSAGE, &uref) == 0) {
        printf("Battery Chemistry: \"%s\" (%d)\n", get_string(fd, uref.value), 
	    uref.value);
    }

    memset(&uref, 0, sizeof(uref));
    uref.report_type = HID_REPORT_TYPE_FEATURE;
    uref.report_id = HID_REPORT_ID_UNKNOWN;
    uref.usage_code = UPS_CAPACITY_MODE;
    if (ioctl(fd, HIDIOCGUSAGE, &uref) == 0) {
	CapacityMode = uref.value;
    }

#ifdef DEBUG
    /* To traverse the report descriptor info
     */

    {
	struct hiddev_report_info rinfo;
	struct hiddev_field_info finfo;
	struct hiddev_usage_ref uref;
	int rtype, i, j;

	for (rtype = HID_REPORT_TYPE_MIN; rtype <= HID_REPORT_TYPE_MAX;
	     rtype++) {
	    rinfo.report_type = rtype;
	    rinfo.report_id = HID_REPORT_ID_FIRST;
	    while (ioctl(fd, HIDIOCGREPORTINFO, &rinfo) >= 0) {
                printf("\n%sReport %d\n",
		       reports[rinfo.report_type], rinfo.report_id);

		for (i = 0; i < rinfo.num_fields; i++) { 
		    struct s_ups_info *p;

		    memset(&finfo, 0, sizeof(finfo));
		    finfo.report_type = rinfo.report_type;
		    finfo.report_id = rinfo.report_id;
		    finfo.field_index = i;
		    ioctl(fd, HIDIOCGFIELDINFO, &finfo);
                    printf("  Field %d, app %s, phys %s\n", 
			    i, 
			    info(finfo.application), info(finfo.physical));

		    memset(&uref, 0, sizeof(uref));
		    for (j = 0; j < finfo.maxusage; j++) {
			unsigned unit, exponent;
			int v, yy, mm, dd;
			uref.report_type = finfo.report_type;
			uref.report_id = finfo.report_id;
			uref.field_index = i;
			uref.usage_index = j;
			ioctl(fd, HIDIOCGUCODE, &uref);
			ioctl(fd, HIDIOCGUSAGE, &uref);
			p = info_entry(uref.usage_code);
			switch (p->type) {
			case T_CAPACITY:
			   unit = 1;
                           printf("Exponent %d lost.\n", exponent);
			   exponent = CapacityMode;
			   break;
			case T_UNITS:
			   unit = finfo.unit;
			   exponent = finfo.unit_exponent;
			   break;
			default:
			   unit = 0;
			   exponent = 0;
			   break;
			}

                        printf("    Usage %d, %s = %d %s", j, p->label, uref.value, 
			    get_units(unit, exponent));

			switch (p->type) {
			case T_INDEX:
                            printf(" %s\n", get_string(fd, uref.value));
			    break;
			case T_BITS:  /* binary bits */
                            printf(" 0x%x\n", uref.value);
			    break;
			case T_DATE:  /* packed integer date */
                            printf(" %4d-%02d-%02d\n", (uref.value >> 9) + 1980,
				(uref.value >> 5) & 0xF, uref.value & 0x1F);
			    break;
			case T_APCDATE: /* APC date */
			    v = uref.value;
			    yy = ((v>>4) & 0xF)*10 + (v&0xF) + 2000;
			    v >>= 8;
			    dd = ((v>>4) & 0xF)*10 + (v&0xF);
			    v >>= 8;
			    mm = ((v>>4) & 0xF)*10 + (v&0xF);	    
                            printf(" %4d-%02d-%02d\n", yy, mm, dd);
			default:
                            printf("\n");
			    break;
			}
		    }
		}
		rinfo.report_id |= HID_REPORT_ID_NEXT;
	    }
	}
    }

    printf("\nWaiting for events ... (interrupt to exit)\n");
    fflush(stdout);

#endif

    start_seconds = time(NULL);
    FD_ZERO(&fdset);
    while (1) {
	if (fd < 0) {
	    sleep(5);
	    fd = open(evdev, O_RDONLY);
	    if (fd < 0) {
                perror("\nOpen error");
		continue;
	    }
	    if (!find_application(fd, UPS_USAGE)) {
               fprintf(stderr, "\nCould not find_application.\n");
	       close(fd);
	       fd = -1;
	    }
	    continue;
	}
	switch (state) {
	    case STATE_NORMAL:
		tv = NULL;
		break;
	    case STATE_BATTERY:
	    case STATE_DEBOUNCE:
		timev.tv_sec = DEBOUNCE_TIMEOUT;
		timev.tv_usec = 0;
		tv = &timev;
		break;
	}

	FD_SET(fd, &fdset);
	rd = select(fd+1, &fdset, NULL, NULL, tv);

	if (rd > 0) {
	    rd = read(fd, ev, sizeof(ev));
	    if (rd < (int) sizeof(ev[0])) {
		if (rd < 0)
                    perror("\nevtest: error reading");
		else
                    fprintf(stderr, "\nevtest: got short read from device!\n");
//		exit (1);
		close(fd);
		fd = -1;
		continue;
	    } else {
	    	printf("time %lu\n", time(NULL) - start_seconds);
	    }

	    for (i = 0; i < rd / sizeof(ev[0]); i++) {
#ifdef DEBUG
                    printf("Event: %s = %d\n",
			   info(ev[i].hid), ev[i].value);
#endif /* DEBUG */

		if (ev[i].hid == UPS_SHUTDOWN_IMMINENT && ev[i].value == 1) {
                    log_status("UPS shutdown imminent!");
		    powerfail(-1);
		    state = STATE_BATTERY;
		    RemainingCapacity = -1;
		}
		switch (state) {
		case STATE_BATTERY:
		    if (ev[i].hid == UPS_DISCHARGING && ev[i].value == 0) {
                        log_status("System back on AC power");
			powerfail(0);
			state = STATE_NORMAL;
			RemainingCapacity = -1;
		    }
		    break;
		case STATE_DEBOUNCE:
		    if (ev[i].hid == UPS_DISCHARGING && ev[i].value == 0) {
			state = STATE_NORMAL;
			RemainingCapacity = -1;
		    }
		    break;
		case STATE_NORMAL:
		    if (ev[i].hid == UPS_DISCHARGING && ev[i].value == 1) {
			state = STATE_DEBOUNCE;
			RemainingCapacity = -1;
		    }
		    break;
		}
	    }
	} else {
	    /* Our timer has expired */
	    switch (state) {
	    case STATE_DEBOUNCE:
                log_status("System switched to battery power");
		state = STATE_BATTERY;
		powerfail(1);
		break;
	    default:  
		break;
	    }
	}
	fflush(stdout);
    }
}
