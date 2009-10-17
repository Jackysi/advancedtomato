/*
  Mode switching tool for controlling flip flop (multiple device) USB gear
  Version 1.0.5, 2009/08/26

  Copyright (C) 2007, 2008, 2009 Josua Dietze (mail to "usb_admin" at the
  domain from the README; please do not post the complete address to The Net!
  Or write a personal message through the forum to "Josh")

  Command line parsing, decent usage/config output/handling, bugfixes and advanced
  options added by:
    Joakim Wennergren (jokedst) (gmail.com)

  TargetClass parameter implementation to support new Option devices/firmware:
    Paul Hardwick (http://www.pharscape.org)

  Created with initial help from:
    "usbsnoop2libusb.pl" by Timo Lindfors (http://iki.fi/lindi/usb/usbsnoop2libusb.pl)

  Config file parsing stuff borrowed from:
    Guillaume Dargaud (http://www.gdargaud.net/Hack/SourceCode.html)

  Hexstr2bin function borrowed from:
    Jouni Malinen (http://hostap.epitest.fi/wpa_supplicant, from "common.c")

  Code, fixes and ideas from:
    Aki Makkonen
    Denis Sutter
    Lucas Benedicic
    Roman Laube
    Vincent Teoh
    Tommy Cheng
    Daniel Cooper
    Andrew Bird
    Steven Fernandez

  More contributors are listed in the config file.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details:

  http://www.gnu.org/licenses/gpl.txt

*/

/* Recommended tab size: 4 */

char *version="1.0.5";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <ctype.h>
#include <getopt.h>

#ifdef USE_SYSLOG
#include <syslog.h>
#endif

#include <usb.h>
#include "usb_modeswitch.h"

#define LINE_DIM 1024
#define BUF_SIZE 4096
#define DESCR_MAX 129

#define SHOW_PROGRESS if (show_progress) printf

int write_bulk(int endpoint, char *message, int length);
int read_bulk(int endpoint, char *buffer, int length);

int find_first_bulk_output_endpoint(struct usb_device *dev);
int find_first_bulk_input_endpoint(struct usb_device *dev);

char *TempPP=NULL;

struct usb_device *dev;
struct usb_dev_handle *devh;

int DefaultVendor=0, DefaultProduct=0, TargetVendor=0, TargetProduct=0, TargetClass=0;
int MessageEndpoint=0, ResponseEndpoint=0, defaultClass=0;
int targetDeviceCount=0;
int devnum=-1, busnum=-1;
int ret;

char DetachStorageOnly=0, HuaweiMode=0, SierraMode=0, SonyMode=0;
char verbose=0, show_progress=1, ResetUSB=0, CheckSuccess=0, config_read=0;
char NeedResponse=0, InquireDevice=1;

char imanufact[DESCR_MAX], iproduct[DESCR_MAX], iserial[DESCR_MAX];

char MessageContent[LINE_DIM];
char TargetProductList[LINE_DIM];
char ByteString[LINE_DIM/2];
char buffer[BUF_SIZE];

// Settable Interface and Configuration (for debugging mostly) (jmw)
int Interface = 0, Configuration = -1, AltSetting = -1;


static struct option long_options[] = {
	{"help",				no_argument, 0, 'h'},
	{"version",				no_argument, 0, 'e'},
	{"default-vendor",		required_argument, 0, 'v'},
	{"default-product",		required_argument, 0, 'p'},
	{"target-vendor",		required_argument, 0, 'V'},
	{"target-product",		required_argument, 0, 'P'},
	{"target-class",		required_argument, 0, 'C'},
	{"message-endpoint",	required_argument, 0, 'm'},
	{"message-content",		required_argument, 0, 'M'},
	{"response-endpoint",	required_argument, 0, 'r'},
	{"detach-only",			no_argument, 0, 'd'},
	{"huawei-mode",			no_argument, 0, 'H'},
	{"sierra-mode",			no_argument, 0, 'S'},
	{"sony-mode",			no_argument, 0, 'O'},
	{"need-response",		no_argument, 0, 'n'},
	{"reset-usb",			no_argument, 0, 'R'},
	{"config",				required_argument, 0, 'c'},
	{"verbose",				no_argument, 0, 'W'},
	{"quiet",				no_argument, 0, 'Q'},
	{"no-inquire",			no_argument, 0, 'I'},
	{"check-success",		required_argument, 0, 's'},
	{"interface",			required_argument, 0, 'i'},
	{"configuration",		required_argument, 0, 'u'},
	{"altsetting",			required_argument, 0, 'a'},
	{0, 0, 0, 0}
};


void readConfigFile(const char *configFilename)
{
	if (verbose) printf("Reading config file: %s\n", configFilename);
	ParseParamHex(configFilename, TargetVendor);
	ParseParamHex(configFilename, TargetProduct);
	if (!TargetProduct)
		ParseParamString(configFilename, TargetProductList);
	ParseParamHex(configFilename, TargetClass);
	ParseParamHex(configFilename, DefaultVendor);
	ParseParamHex(configFilename, DefaultProduct);
	ParseParamBool(configFilename, DetachStorageOnly);
	ParseParamBool(configFilename, HuaweiMode);
	ParseParamBool(configFilename, SierraMode);
	ParseParamBool(configFilename, SonyMode);
	ParseParamHex(configFilename, MessageEndpoint);
	ParseParamString(configFilename, MessageContent);
	ParseParamHex(configFilename, NeedResponse);
	ParseParamHex(configFilename, ResponseEndpoint);
	ParseParamHex(configFilename, ResetUSB);
	ParseParamHex(configFilename, InquireDevice);
	ParseParamInt(configFilename, CheckSuccess);
	ParseParamHex(configFilename, Interface);
	ParseParamHex(configFilename, Configuration);
	ParseParamHex(configFilename, AltSetting);
	config_read = 1;
}


void printConfig()
{
	printf ("DefaultVendor=  0x%04x\n",		DefaultVendor);
	printf ("DefaultProduct= 0x%04x\n",	DefaultProduct);
	if ( TargetVendor )
		printf ("TargetVendor=   0x%04x\n",		TargetVendor);
	else
		printf ("TargetVendor=   not set\n");
	if ( TargetProduct )
		printf ("TargetProduct=  0x%04x\n",		TargetProduct);
	else
		printf ("TargetProduct=  not set\n");
	if ( TargetClass )
		printf ("TargetClass=    0x%02x\n",		TargetClass);
	else
		printf ("TargetClass=    not set\n");
	printf ("\nDetachStorageOnly=%i\n",	(int)DetachStorageOnly);
	printf ("HuaweiMode=%i\n",			(int)HuaweiMode);
	printf ("SierraMode=%i\n",			(int)SierraMode);
	printf ("SonyMode=%i\n",			(int)SonyMode);
	if ( MessageEndpoint )
		printf ("MessageEndpoint=0x%02x\n",	MessageEndpoint);
	else
		printf ("MessageEndpoint= not set\n");
	if ( strlen(MessageContent) )
		printf ("MessageContent=\"%s\"\n",	MessageContent);
	else
		printf ("MessageContent= not set\n");
	printf ("NeedResponse=%i\n",		(int)NeedResponse);
	if ( ResponseEndpoint )
		printf ("ResponseEndpoint=0x%02x\n",	ResponseEndpoint);
	else
		printf ("ResponseEndpoint= not set\n");
	printf ("Interface=0x%02x\n",			Interface);
	if ( Configuration > -1 )
		printf ("Configuration=0x%02x\n",	Configuration);
	if ( AltSetting > -1 )
		printf ("AltSetting=0x%02x\n",	AltSetting);
	if ( InquireDevice )
		printf ("\nInquireDevice enabled (default)\n");
	else
		printf ("\nInquireDevice disabled\n");
	if ( CheckSuccess )
		printf ("Success check enabled, max. wait time %d seconds\n", CheckSuccess);
	else
		printf ("Success check disabled\n");
	printf ("\n");
}


int readArguments(int argc, char **argv)
{
	int c, option_index = 0, count=0;
	if(argc==1) return 0;

	while (1)
	{
		c = getopt_long (argc, argv, "heWQndHSORIv:p:V:P:C:m:M:r:c:i:u:a:s:",
						long_options, &option_index);
	
		/* Detect the end of the options. */
		if (c == -1)
			break;
		count++;
		switch (c)
		{
			case 'R': ResetUSB = 1; break;
			case 'v': DefaultVendor = strtol(optarg, NULL, 16); break;
			case 'p': DefaultProduct = strtol(optarg, NULL, 16); break;
			case 'V': TargetVendor = strtol(optarg, NULL, 16); break;
			case 'P': TargetProduct = strtol(optarg, NULL, 16); break;
			case 'C': TargetClass = strtol(optarg, NULL, 16); break;
			case 'm': MessageEndpoint = strtol(optarg, NULL, 16); break;
			case 'M': strcpy(MessageContent, optarg); break;
			case 'n': NeedResponse = 1; break;
			case 'r': ResponseEndpoint = strtol(optarg, NULL, 16); break;
			case 'd': DetachStorageOnly = 1; break;
			case 'H': HuaweiMode = 1; break;
			case 'S': SierraMode = 1; break;
			case 'O': SonyMode = 1; break;
			case 'c': readConfigFile(optarg); break;
			case 'W': verbose = 1; show_progress = 1; count--; break;
			case 'Q': show_progress = 0; verbose = 0; count--; break;
			case 's': CheckSuccess = strtol(optarg, NULL, 16); count--; break;
			case 'I': InquireDevice = 0; break;

			case 'i': Interface = strtol(optarg, NULL, 16); break;
			case 'u': Configuration = strtol(optarg, NULL, 16); break;
			case 'a': AltSetting = strtol(optarg, NULL, 16); break;
	
			case 'e':
				printVersion();
				exit(0);
				break;
			case 'h':
				printVersion();
				printf ("Usage: usb_modeswitch [-hvpVPmMrdHn] [-c filename]\n\n");
				printf (" -h, --help                    this help\n");
				printf (" -e, --version                 print version number and exit\n");
				printf (" -v, --default-vendor [nr]     vendor ID to look for (mandatory)\n");
				printf (" -p, --default-product [nr]    product ID to look for (mandatory)\n");
				printf (" -V, --target-vendor [nr]      target vendor (optional, for success check)\n");
				printf (" -P, --target-product [nr]     target model (optional, for success check)\n");
				printf (" -C, --target-class [nr]       target device class\n");
				printf (" -m, --message-endpoint [nr]   where to direct the message (optional)\n");
				printf (" -M, --message-content [str]   command to send (hex number as string)\n");
				printf (" -n, --need-response           read a response to the message transfer\n");
				printf (" -r, --response-endpoint [nr]  where from read the response (optional)\n");
				printf (" -d, --detach-only             just detach the storage driver\n");
				printf (" -H, --huawei-mode             apply a special procedure\n");
				printf (" -S, --sierra-mode             apply a special procedure\n");
				printf (" -O, --sony-mode               apply a special procedure\n");
				printf (" -R, --reset-usb               reset the device in the end\n");
				printf (" -c, --config [filename]       load different config file\n");
				printf (" -Q, --quiet                   don't show progress or error messages\n");
				printf (" -W, --verbose                 print all settings before running\n");
				printf (" -s, --success [nr]            check switching result after [nr] secs\n");
				printf (" -I, --no-inquire              do not get device details (default on)\n\n");
				printf (" -i, --interface               select initial USB interface (default 0)\n");
				printf (" -u, --configuration           select USB configuration\n");
				printf (" -a, --altsetting              select alternative USB interface setting\n\n");
				exit(0);
				break;
		
			default: //Unsupported - error message has already been printed
				printf ("\n");
				exit(1);
		}
	}

	return count;
}


int main(int argc, char **argv)
{
	int numDefaults = 0, specialMode = 0, sonySuccess = 0;

	/*
	 * Parameter parsing, USB preparation/diagnosis, plausibility checks
	 */

	// Check command arguments, use params instead of config file when given
	switch (readArguments(argc, argv)) {
		case 0:						// no argument or -W, -q or -s
			readConfigFile("/etc/usb_modeswitch.conf");
			break;
		default:					// one or more arguments except -W, -q or -s 
			if (!config_read)		// if arguments contain -c, the config file was already processed
				if (verbose) printf("Taking all parameters from the command line\n\n");
	}

	if (verbose)
		printVersion();

	if (verbose)
		printConfig();

	// libusb initialization
	usb_init();

	if (verbose)
		usb_set_debug(15);

	usb_find_busses();
	usb_find_devices();

	// Plausibility checks. The default IDs are mandatory
	if (!(DefaultVendor && DefaultProduct)) {
		SHOW_PROGRESS("No default vendor/product ID given. Aborting.\n\n");
		exit(1);
	}
	if (strlen(MessageContent)) {
		if (strlen(MessageContent) % 2 != 0) {
			fprintf(stderr, "Error: MessageContent hex string has uneven length. Aborting.\n\n");
			exit(1);
		}
		if ( hexstr2bin(MessageContent, ByteString, strlen(MessageContent)/2) == -1) {
			fprintf(stderr, "Error: MessageContent %s\n is not a hex string. Aborting.\n\n", MessageContent);
			exit(1);
		}
	}
	SHOW_PROGRESS("\n");
	if (show_progress)
		if (CheckSuccess && !(TargetVendor || TargetProduct) && !TargetClass)
			printf("Note: target parameter missing; success check limited\n");

	// Count existing target devices (remember for success check)
	if (TargetVendor || TargetClass) {
		SHOW_PROGRESS("Looking for target devices ...\n");
		search_devices(&targetDeviceCount, TargetVendor, TargetProduct, TargetProductList, TargetClass);
		if (targetDeviceCount) {
			SHOW_PROGRESS(" Found devices in target mode or class (%d)\n", targetDeviceCount);
		} else
			SHOW_PROGRESS(" No devices in target mode or class found\n");
	}

	// Count default devices, return the last one found
	SHOW_PROGRESS("Looking for default devices ...\n");
	dev = search_devices(&numDefaults, DefaultVendor, DefaultProduct, NULL, TargetClass);
	if (numDefaults) {
		SHOW_PROGRESS(" Found default devices (%d)\n", numDefaults);
		if (TargetClass && !(TargetVendor || TargetProduct)) {
			if ( dev != NULL ) {
				SHOW_PROGRESS(" Found a default device NOT in target class mode\n");
			} else {
				SHOW_PROGRESS(" All devices in target class mode. Nothing to do. Bye.\n\n");
				exit(0);
			}
		}
	}
	if (dev != NULL) {
		devnum = dev->devnum;
		busnum = (int)strtol(dev->bus->dirname,NULL,10);
		SHOW_PROGRESS("Accessing device %03d on bus %03d ...\n", devnum, busnum);
		devh = usb_open(dev);
	} else {
		SHOW_PROGRESS(" No default device found. Is it connected? Bye.\n\n");
		exit(0);
	}

	// Get class of default device
	defaultClass = dev->descriptor.bDeviceClass;
	if (defaultClass == 0)
		defaultClass = dev->config[0].interface[0].altsetting[0].bInterfaceClass;

	// Check or get endpoints if needed
	if (!MessageEndpoint && (strlen(MessageContent) || InquireDevice) ) {
//		SHOW_PROGRESS(" Finding endpoints ...\n");
		MessageEndpoint = find_first_bulk_output_endpoint(dev);
		if (!MessageEndpoint && strlen(MessageContent)) {
			fprintf(stderr,"Error: message endpoint not given or found. Aborting.\n\n");
			exit(1);
		}
	}
	if (!ResponseEndpoint && (NeedResponse || InquireDevice) ) {
		ResponseEndpoint = find_first_bulk_input_endpoint(dev);
		if (!ResponseEndpoint && NeedResponse) {
			fprintf(stderr,"Error: response endpoint not given or found. Aborting.\n\n");
			exit(1);
		}
	}
	if (MessageEndpoint && ResponseEndpoint) {
		SHOW_PROGRESS("Using endpoints 0x%02x (out) and 0x%02x (in)\n", MessageEndpoint, ResponseEndpoint);
	} else
		if (InquireDevice && defaultClass == 0x08) {
			SHOW_PROGRESS("Endpoints not found, skipping SCSI inquiry\n");
			InquireDevice = 0;
		}
		
	if (InquireDevice && show_progress) {
		if (defaultClass == 0x08) {
			SHOW_PROGRESS("Inquiring device details; driver will be detached ...\n");
			detachDriver();
			if (deviceInquire() >= 0)
				InquireDevice = 2;
		} else
			SHOW_PROGRESS("Not a storage device, skipping SCSI inquiry\n");
	}

	deviceDescription();
	if (show_progress) {
		printf("\nDevice description data (identification)\n");
		printf("-------------------------\n");
		printf("Manufacturer: %s\n", imanufact);
		printf("     Product: %s\n", iproduct);
		printf("  Serial No.: %s\n", iserial);
		printf("-------------------------\n");
	}

	// Some scenarios are exclusive, so check for unwanted combinations
	specialMode = DetachStorageOnly + HuaweiMode + SierraMode + SonyMode;
	if ( specialMode > 1 ) {
		SHOW_PROGRESS("Invalid mode combination. Check your configuration. Aborting.\n\n");
		exit(1);
	}

	if ( !specialMode && !strlen(MessageContent) )
		SHOW_PROGRESS("Warning: no switching method given.\n");

	/*
	 * The switching actions
	 */

#ifdef USE_SYSLOG
	openlog("usb_modeswitch", 0, LOG_SYSLOG);
//	syslog(LOG_NOTICE, "v1.0.3 (C) 2009 Josua Dietze");
	syslog(LOG_NOTICE, "switching %04x:%04x (%s: %s)", DefaultVendor, DefaultProduct, imanufact, iproduct);
#endif

	if (DetachStorageOnly) {
		SHOW_PROGRESS("Only detaching storage driver for switching ...\n");
		if (InquireDevice == 2) {
			SHOW_PROGRESS(" Any driver was already detached for inquiry\n");
		} else {
			ret = detachDriver();
			if (ret == 2)
				SHOW_PROGRESS(" You may want to remove the storage driver manually\n");
		}
	}

	if (HuaweiMode) {
		switchHuaweiMode();
	}
	if (SierraMode) {
		switchSierraMode();
	}
	if (SonyMode) {
		sonySuccess = switchSonyMode();
	}

	if (strlen(MessageContent) && MessageEndpoint) {
		if (specialMode == 0) {
			if (InquireDevice != 2)
				detachDriver();
			switchSendMessage();
		} else
			SHOW_PROGRESS("Note: ignoring MessageContent. Can't combine with special mode\n");
	}

	if (Configuration != -1) {
		switchConfiguration ();
	}

	if (AltSetting != -1) {
		switchAltSetting();
	}

	if (ResetUSB) {
		resetUSB();
	}

	if (devh)
		usb_close(devh);

	if (CheckSuccess) {
		signal(SIGTERM, release_usb_device);
		if (checkSuccess()) {
			if (!show_progress)
				printf("ok:%04x:%04x\n", TargetVendor, TargetProduct);
			else {
#ifdef USE_SYSLOG
				printf("ok:%04x:%04x\n", TargetVendor, TargetProduct);
#endif
			}
			exit(0);
		} else{
			if (!show_progress)
				printf("fail:\n");
			else {
#ifdef USE_SYSLOG
				printf("fail:\n");
#endif
			}
			exit(1);
		}
	} else {
		if (SonyMode)
			if (sonySuccess) {
#ifdef USE_SYSLOG
				syslog(LOG_NOTICE, "switched S.E. MD400 to modem mode");
#endif
				if (!show_progress)
					printf("ok:\n"); // ACM device, no driver action
				else {
					printf("-> device should be stable now. Bye.\n\n");
#ifdef USE_SYSLOG
					printf("ok:\n");
#endif
				}
			} else {
				if (!show_progress)
					printf("fail:\n");
				else {
					printf("-> switching was not completed. Bye.\n\n");
#ifdef USE_SYSLOG
					printf("ok:\n");
#endif
				}
			}
		else
			SHOW_PROGRESS("-> Run lsusb to note any changes. Bye.\n\n");
#ifdef USE_SYSLOG
		closelog();
#endif
		exit(0);
	}
}


/* Get descriptor strings if available (identification details) */
deviceDescription ()
{
	int i, ret;
	char* c;
	memset (imanufact, ' ', DESCR_MAX);
	memset (iproduct, ' ', DESCR_MAX);
	memset (iserial, ' ', DESCR_MAX);

	if (dev->descriptor.iManufacturer) {
		ret = usb_get_string_simple(devh, dev->descriptor.iManufacturer, imanufact, DESCR_MAX);
		if (ret < 0)
			fprintf(stderr, "Error: could not get description string \"manufacturer\"\n");
	} else
		strcpy(imanufact, "not provided");
	c = strstr(imanufact, "    ");
	if (c)
		memset((void*)c, '\0', 1);

	if (dev->descriptor.iProduct) {
		ret = usb_get_string_simple(devh, dev->descriptor.iProduct, iproduct, DESCR_MAX);
		if (ret < 0)
			fprintf(stderr, "Error: could not get description string \"product\"\n");
	} else
		strcpy(iproduct, "not provided");
	c = strstr(iproduct, "    ");
	if (c)
		memset((void*)c, '\0', 1);

	if (dev->descriptor.iSerialNumber) {
		ret = usb_get_string_simple(devh, dev->descriptor.iSerialNumber, iserial, DESCR_MAX);
		if (ret < 0)
			fprintf(stderr, "Error: could not get description string \"serial number\"\n");
	} else
		strcpy(iserial, "not provided");
	c = strstr(iserial, "    ");
	if (c)
		memset((void*)c, '\0', 1);

}

/* Print result of SCSI command INQUIRY (identification details) */
int deviceInquire ()
{
	const unsigned char inquire_msg[] = {
	  0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
	  0x24, 0x00, 0x00, 0x00, 0x80, 0x00, 0x06, 0x12,
	  0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	char *command;
	char data[36];
	int i, ret;

	command = malloc(31);
	if (command == NULL) {
		ret = 1;
		goto out;
	}

	memcpy(command, inquire_msg, sizeof (inquire_msg));

	ret = usb_claim_interface(devh, Interface);
	if (ret != 0) {
		SHOW_PROGRESS(" Could not claim interface (error %d). Skipping device inquiry\n", ret);
		goto out;
	}
	usb_clear_halt(devh, MessageEndpoint);

	ret = usb_bulk_write(devh, MessageEndpoint, (char *)command, 31, 0);
	if (ret < 0) {
		SHOW_PROGRESS(" Could not send INQUIRY message (error %d)\n", ret);
		goto out;
	}

	ret = usb_bulk_read(devh, ResponseEndpoint, data, 36, 0);
	if (ret < 0) {
		SHOW_PROGRESS(" Could not get INQUIRY response (error %d)\n", ret);
		goto out;
	}

	i = usb_bulk_read(devh, ResponseEndpoint, command, 13, 0);

	printf("\nReceived inquiry data (detailed identification)\n");
	printf("-------------------------\n");

	printf("  Vendor String: ");
	for (i = 8; i < 16; i++) printf("%c",data[i]);
	printf("\n");

	printf("   Model String: ");
	for (i = 16; i < 32; i++) printf("%c",data[i]);
	printf("\n");

	printf("Revision String: ");
	for (i = 32; i < 36; i++) printf("%c",data[i]);

	printf("\n-------------------------\n");

out:
	if (strlen(MessageContent) == 0)
		usb_clear_halt(devh, MessageEndpoint);
		usb_release_interface(devh, Interface);
	free(command);
	return ret;
}


int resetUSB ()
{
	int success;
	int bpoint = 0;

	if (show_progress) {
		printf("Resetting usb device ");
		fflush(stdout);
	}

	sleep( 1 );
	do {
		success = usb_reset(devh);
		if ( ((bpoint % 10) == 0) && show_progress ) {
			printf(".");
			fflush(stdout);
		}
		bpoint++;
		if (bpoint > 100)
			success = 1;
	} while (success < 0);

	if ( success ) {
		SHOW_PROGRESS("\n Reset failed. Can be ignored if device switched OK.\n");
	} else
		SHOW_PROGRESS("\n OK, device was reset\n");
}


int switchSendMessage ()
{
	int message_length, ret;

	SHOW_PROGRESS("Setting up communication with interface %d ...\n", Interface);
	if (InquireDevice != 2) {
		ret = usb_claim_interface(devh, Interface);
		if (ret != 0) {
			SHOW_PROGRESS(" Could not claim interface (error %d). Skipping message sending\n", ret);
			return 0;
		}
	}
	usb_clear_halt(devh, MessageEndpoint);
	SHOW_PROGRESS("Trying to send the message to endpoint 0x%02x ...\n", MessageEndpoint);
	fflush(stdout);

	message_length = strlen(MessageContent) / 2;
	ret = write_bulk(MessageEndpoint, ByteString, message_length);
	if (ret == -19)
		goto skip;

	if (NeedResponse) {
		SHOW_PROGRESS("Reading the response to the message ...\n");
		ret = read_bulk(ResponseEndpoint, ByteString, LINE_DIM/2);
		if (ret == -19)
			goto skip;
	}

	ret = usb_clear_halt(devh, MessageEndpoint);
	if (ret)
		goto skip;
	ret = usb_release_interface(devh, Interface);
	if (ret)
		goto skip;
	return 1;

skip:
	SHOW_PROGRESS(" Device is gone, skipping any further commands\n");
	usb_close(devh);
	devh = 0;
	return 2;
}


int switchConfiguration ()
{
	int ret;

	SHOW_PROGRESS("Changing configuration to %i ...\n", Configuration);
	ret = usb_set_configuration(devh, Configuration);
	if (ret == 0 ) {
		SHOW_PROGRESS(" OK, configuration set\n");
		return 1;
	} else {
		SHOW_PROGRESS(" Setting the configuration returned error %d. Trying to continue\n", ret);
		return 0;
	}
}


int switchAltSetting ()
{
	int ret;

	SHOW_PROGRESS("Changing to alt setting %i ...\n", AltSetting);
	ret = usb_claim_interface(devh, Interface);
	ret = usb_set_altinterface(devh, AltSetting);
	usb_release_interface(devh, Interface);
	if (ret != 0) {
		SHOW_PROGRESS(" Changing to alt setting returned error %d. Trying to continue\n", ret);
		return 0;
	} else {
		SHOW_PROGRESS(" OK, changed to alt setting\n");
		return 1;
	}
}


int switchHuaweiMode ()
{
	int ret;

	SHOW_PROGRESS("Sending Huawei control message ...\n");
	ret = usb_control_msg(devh, USB_TYPE_STANDARD + USB_RECIP_DEVICE, USB_REQ_SET_FEATURE, 00000001, 0, buffer, 0, 1000);
	if (ret != 0) {
		fprintf(stderr, "Error: sending Huawei control message failed (error %d). Aborting.\n\n", ret);
		exit(1);
	} else
		SHOW_PROGRESS(" OK, Huawei control message sent\n");
}


int switchSierraMode ()
{
	int ret;

	SHOW_PROGRESS("Trying to send Sierra control message\n");
	ret = usb_control_msg(devh, 0x40, 0x0b, 00000001, 0, buffer, 0, 1000);
	if (ret != 0) {
		fprintf(stderr, "Error: sending Sierra control message failed (error %d). Aborting.\n\n", ret);
	    exit(1);
	} else
		SHOW_PROGRESS(" OK, Sierra control message sent\n");
}


int switchSonyMode ()
{
	int i, found, ret;
	detachDriver();

	if (CheckSuccess) {
		printf("Note: CheckSuccess pointless with Sony mode, disabling\n");
		CheckSuccess = 0;
	}

	SHOW_PROGRESS("Trying to send Sony control message\n");
	ret = usb_control_msg(devh, 0xc0, 0x11, 2, 0, buffer, 3, 100);
	if (ret < 0) {
		fprintf(stderr, "Error: sending Sony control message failed (error %d). Aborting.\n\n", ret);
		exit(1);
	} else
		SHOW_PROGRESS(" OK, control message sent, waiting for device to return ...\n");

	usb_close(devh);

	i=0;
	dev = NULL;
	while ( dev == NULL && i < 30 ) {
		if ( i > 5 ) {
			usb_find_busses();
			usb_find_devices();
			dev = search_devices(&found, DefaultVendor, DefaultProduct, NULL, TargetClass);
		}
		if ( dev != NULL )
			break;
		sleep(1);
		if (show_progress) {
			printf("#");
			fflush(stdout);
		}
		i++;
	}
	SHOW_PROGRESS("\n After %d seconds:",i);
	if ( dev != NULL ) {
		SHOW_PROGRESS(" device came back, proceeding\n");
		devh = NULL;
		devh = usb_open( dev );
		if (devh == NULL) {
			fprintf(stderr, "Error: could not get handle on device\n");
			return 0;
		}
	} else {
		SHOW_PROGRESS(" device still gone, cancelling\n");
		return 0;
	}

	sleep(1);

//	switchAltSetting();
//	sleep(1);

	SHOW_PROGRESS("Sending Sony control message again ...\n");
	ret = usb_control_msg(devh, 0xc0, 0x11, 2, 0, buffer, 3, 100);
	if (ret < 0) {
		fprintf(stderr, "Error: sending Sony control message (2) failed (error %d)\n", ret);
		return 0;
	} else {
		SHOW_PROGRESS(" OK, control message sent\n");
		return 1;
	}

	Interface=8;
	AltSetting=2;
}


// Detach driver either as the main action or as preparation for other modes
int detachDriver()
{
	int ret;

#ifndef LIBUSB_HAS_GET_DRIVER_NP
	printf(" Cant't do driver detection and detaching on this platform.\n");
	return 2;
#endif

	SHOW_PROGRESS("Looking for active driver ...\n");
	ret = usb_get_driver_np(devh, Interface, buffer, BUF_SIZE);
	if (ret != 0) {
		SHOW_PROGRESS(" No driver found. Either detached before or never attached\n");
		return 1;
	}
	SHOW_PROGRESS(" OK, driver found (\"%s\")\n", buffer);
	if (DetachStorageOnly && strcmp(buffer,"usb-storage")) {
		SHOW_PROGRESS(" Driver is not usb-storage, leaving it alone\n");
		return 1;
	}

#ifndef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
	SHOW_PROGRESS(" Can't do driver detaching on this platform\n");
	return 2;
#endif


	ret = usb_detach_kernel_driver_np(devh, Interface);
	if (ret == 0) {
		SHOW_PROGRESS(" OK, driver \"%s\" detached\n", buffer);
//		usb_clear_halt(devh, MessageEndpoint);
//		usb_clear_halt(devh, ResponseEndpoint);
	} else
		SHOW_PROGRESS(" Driver \"%s\" detach failed with error %d. Trying to continue\n", buffer, ret);
	return 1;
}


int checkSuccess()
{
	int i=0, ret, present=1;
	int newTargetCount, success=0;
	
	SHOW_PROGRESS("\nChecking for mode switch (max. %d times, once per second) ...\n", CheckSuccess);
	sleep(1);

	if (devh) // devh is 0 if device vanished during command transmission
		for (i; i < CheckSuccess; i++) {

			// Test if default device still can be accessed; positive result does
			// not necessarily mean failure
			SHOW_PROGRESS(" Waiting for original device to vanish ...\n");
			if (present) {
				devh = usb_open(dev);
				ret = usb_claim_interface(devh, Interface);
				if (ret < 0) {
					SHOW_PROGRESS(" Original device can't be accessed anymore. Good.\n");
					if (i == CheckSuccess-1)
						SHOW_PROGRESS(" If you want target checking, increase 'CheckSuccess' value.\n");
					present = 0;
					break;
				} else
					usb_release_interface(devh, Interface);
				usb_close(devh);
			}
			if (i == CheckSuccess-1) {
				SHOW_PROGRESS(" Original device still present after the timeout\n\nMode switch most likely failed. Bye.\n\n");
			} else
				sleep(1);
		}
	else {
		SHOW_PROGRESS(" Original device is gone already, not checking\n");
		present = 0;
	}

	if ( (TargetVendor && (TargetProduct || TargetProductList)) || TargetClass )

		// Recount target devices (compare with previous count) if target data is given.
		// Target device on the same bus with higher device number is returned,
		// description is read for syslog message
		for (i; i < CheckSuccess; i++) {
			SHOW_PROGRESS(" Searching for target devices ...\n");
			usb_find_devices();
			dev = search_devices(&newTargetCount, TargetVendor, TargetProduct, TargetProductList, TargetClass);
			if (dev && (newTargetCount > targetDeviceCount)) {
				devh = usb_open(dev);
				deviceDescription();
				usb_close(devh);
				if (verbose) {
					printf("\nFound target device %03d on bus %03d\n", \
					dev->devnum, (int)strtol(dev->bus->dirname,NULL,10));
					printf("\nTarget device description data\n");
					printf("-------------------------\n");
					printf("Manufacturer: %s\n", imanufact);
					printf("     Product: %s\n", iproduct);
					printf("  Serial No.: %s\n", iserial);
					printf("-------------------------\n");
				}
				SHOW_PROGRESS(" Found correct target device\n\nMode switch succeeded. Bye.\n\n");
				success = 2;
				break;
			}
			if (i == CheckSuccess-1) {
				SHOW_PROGRESS(" No new devices in target mode or class found\n\nMode switch has failed. Bye.\n\n");
			} else
				sleep(1);
		}
	else
		// No target data given, rely on the vanished device
		if (!present) {
			SHOW_PROGRESS(" (For a better success check provide target IDs or class)\n");
			SHOW_PROGRESS(" Original device vanished after switching\n\nMode switch most likely succeeded. Bye.\n\n");
			success = 1;
		}

	switch (success) {
		case 2: 
#ifdef USE_SYSLOG
			syslog(LOG_NOTICE, "switched to %04x:%04x (%s: %s)", TargetVendor, TargetProduct, imanufact, iproduct);
#endif
			success = 1;
			break;
		case 1:
#ifdef USE_SYSLOG
			syslog(LOG_NOTICE, "device seems to have switched");
#endif
		default:
			;
	}

#ifdef USE_SYSLOG
	closelog();
#endif
	return success;

}


int write_bulk(int endpoint, char *message, int length)
{
	int ret;
	ret = usb_bulk_write(devh, endpoint, message, length, 100);
	if (ret >= 0 ) {
		SHOW_PROGRESS(" OK, message successfully sent\n");
	} else
		if (ret == -19) {
			SHOW_PROGRESS(" Device seems to have vanished right after sending. Good.\n");
		} else
			SHOW_PROGRESS(" Sending the message returned error %d. Trying to continue\n", ret);
	return ret;
}

int read_bulk(int endpoint, char *buffer, int length)
{
	int ret;
	ret = usb_bulk_read(devh, endpoint, buffer, length, 100);
	usb_bulk_read(devh, endpoint, buffer, 13, 100);
	if (ret >= 0 ) {
		SHOW_PROGRESS(" OK, response successfully read (%d bytes).\n", ret);
	} else
		if (ret == -19) {
			SHOW_PROGRESS(" Device seems to have vanished after reading. Good.\n");
		} else
			SHOW_PROGRESS(" Response reading got error %d, can probably be ignored\n", ret);
	return ret;
}

void release_usb_device(int dummy) {
	int ret;
	SHOW_PROGRESS("Program cancelled by system. Bye.\n\n");
	usb_release_interface(devh, Interface);
	usb_close(devh);
#ifdef USE_SYSLOG
	closelog();
#endif
	exit(0);
}


// iterates over busses and devices, counts the ones found and returns the last one of them

struct usb_device* search_devices( int *numFound, int vendor, int product, char* productList, int targetClass)
{
	struct usb_bus *bus;
	char *listcopy, *token, buffer[2];
	int devClass, product_match = 0;
	struct usb_device* right_dev = NULL;
	
	if ( targetClass && !(vendor || product) ) {
		vendor = DefaultVendor;
		product = DefaultProduct;
	}
	*numFound = 0;
	if (!vendor || (!product && productList==NULL))
		return NULL;
	if (productList != NULL)
		listcopy = malloc(strlen(productList)+1);

	for (bus = usb_get_busses(); bus; bus = bus->next) {
		struct usb_device *dev;
		for (dev = bus->devices; dev; dev = dev->next) {
			// product list given
			if (productList != NULL) {
				strcpy(listcopy, productList);
				token = strtok(listcopy, ",");
				while (token != NULL) {
					if (strlen(token) != 4) {
						SHOW_PROGRESS("Error: entry in product ID list has wrong length: %s. Ignoring\n", token);
						goto NextToken;
					}
					if ( hexstr2bin(token, buffer, strlen(token)/2) == -1) {
						SHOW_PROGRESS("Error: entry in product ID list is not a hex string: %s. Ignoring\n", token);
						goto NextToken;
					}
					product = 0;
					product |= (char)buffer[0];
					product <<= 8;
					product |= (char)buffer[1];
					if (dev->descriptor.idVendor == vendor && product == dev->descriptor.idProduct) {
						(*numFound)++;
						if (busnum == -1)
							right_dev = dev;
						else
							if (dev->devnum >= devnum && (int)strtol(dev->bus->dirname,NULL,10) == busnum) {
								right_dev = dev;
								TargetProduct = dev->descriptor.idProduct;
								break;
							}
					}

					NextToken:
					token = strtok(NULL, ",");
				}
			// product is given
			} else
				if (dev->descriptor.idVendor == vendor && product == dev->descriptor.idProduct) {
					(*numFound)++;
					devClass = dev->descriptor.bDeviceClass;
					if (devClass == 0)
						devClass = dev->config[0].interface[0].altsetting[0].bInterfaceClass;
					if (busnum == -1) {
						if (devClass != targetClass || targetClass == 0)
							right_dev = dev;
					} else
						if (devClass == targetClass)
							if (dev->devnum >= devnum && (int)strtol(dev->bus->dirname,NULL,10) == busnum)
								right_dev = dev;
				}
		}
	}
	if (productList != NULL)
		free(listcopy);
	return right_dev;
}


#define USB_DIR_OUT 0x00
#define USB_DIR_IN  0x80

// Autodetect bulk endpoints (ab)

int find_first_bulk_output_endpoint(struct usb_device *dev)
{
	int i;
	struct usb_interface_descriptor *alt = &(dev->config[0].interface[0].altsetting[0]);
	struct usb_endpoint_descriptor *ep;

	for(i=0;i < alt->bNumEndpoints;i++) {
		ep=&(alt->endpoint[i]);
		if( ( (ep->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) &&
		    ( (ep->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT ) ) {
			return ep->bEndpointAddress;
		}
	}

	return 0;
}


int find_first_bulk_input_endpoint(struct usb_device *dev)
{
	int i;
	struct usb_interface_descriptor *alt = &(dev->config[0].interface[0].altsetting[0]);
	struct usb_endpoint_descriptor *ep;

	for(i=0;i < alt->bNumEndpoints;i++) {
		ep=&(alt->endpoint[i]);
		if( ( (ep->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) &&
		    ( (ep->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN ) ) {
			return ep->bEndpointAddress;
		}
	}

	return 0;
}



// the parameter parsing stuff

char* ReadParseParam(const char* FileName, char *VariableName)
{
	static char Str[LINE_DIM];
	char *VarName, *Comment=NULL, *Equal=NULL;
	char *FirstQuote, *LastQuote, *P1, *P2;
	int Line=0, Len=0, Pos=0;
	FILE *file=fopen(FileName, "r");
	
	if (file==NULL) {
		fprintf(stderr, "Error: Could not find file %s\n\n", FileName);
		exit(1);
	}

	while (fgets(Str, LINE_DIM-1, file) != NULL) {
		Line++;
		Len=strlen(Str);
		if (Len==0) goto Next;
		if (Str[Len-1]=='\n' or Str[Len-1]=='\r') Str[--Len]='\0';
		Equal = strchr (Str, '=');			// search for equal sign
		Pos = strcspn (Str, ";#!");			// search for comment
		Comment = (Pos==Len) ? NULL : Str+Pos;
		if (Equal==NULL or ( Comment!=NULL and Comment<=Equal)) goto Next;	// Only comment
		*Equal++ = '\0';
		if (Comment!=NULL) *Comment='\0';

		// String
		FirstQuote=strchr (Equal, '"');		// search for double quote char
		LastQuote=strrchr (Equal, '"');
		if (FirstQuote!=NULL) {
			if (LastQuote==NULL) {
				fprintf(stderr, "Error reading parameter file %s line %d - Missing end quote.\n", FileName, Line);
				goto Next;
			}
			*FirstQuote=*LastQuote='\0';
			Equal=FirstQuote+1;
		}
		
		// removes leading/trailing spaces
		Pos=strspn (Str, " \t");
		if (Pos==strlen(Str)) {
			fprintf(stderr, "Error reading parameter file %s line %d - Missing variable name.\n", FileName, Line);
			goto Next;		// No function name
		}
		while ((P1=strrchr(Str, ' '))!=NULL or (P2=strrchr(Str, '\t'))!=NULL)
			if (P1!=NULL) *P1='\0';
			else if (P2!=NULL) *P2='\0';
		VarName=Str+Pos;
		//while (strspn(VarName, " \t")==strlen(VarName)) VarName++;

		Pos=strspn (Equal, " \t");
		if (Pos==strlen(Equal)) {
			fprintf(stderr, "Error reading parameter file %s line %d - Missing value.\n", FileName, Line);
			goto Next;		// No function name
		}
		Equal+=Pos;

		if (strcmp(VarName, VariableName)==0) {		// Found it
			fclose(file);
			return Equal;
		}
		Next:;
	}
	
	// not found
//	fprintf(stderr, "Error reading parameter file %s - Variable %s not found.", 
//				FileName, VariableName);
	fclose(file);
	return NULL;
}


int hex2num(char c)
{
	if (c >= '0' && c <= '9')
	return c - '0';
	if (c >= 'a' && c <= 'f')
	return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
	return c - 'A' + 10;
	return -1;
}


int hex2byte(const char *hex)
{
	int a, b;
	a = hex2num(*hex++);
	if (a < 0)
		return -1;
	b = hex2num(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}

int hexstr2bin(const char *hex, char *buffer, int len)
{
	int i;
	int a;
	const char *ipos = hex;
	char *opos = buffer;
//    printf("Debug: hexstr2bin bytestring is ");

	for (i = 0; i < len; i++) {
	a = hex2byte(ipos);
//        printf("%02X", a);
	if (a < 0)
		return -1;
	*opos++ = a;
	ipos += 2;
	}
//    printf(" \n");
	return 0;
}

printVersion()
{
	printf("\n * usb_modeswitch: handle USB devices with multiple modes\n");
	printf(" * Version %s (C) Josua Dietze 2009\n", version);
	printf(" * Based on libusb 0.1.12\n\n");
}