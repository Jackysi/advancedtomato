/*
 *  Copyright (c) 2002 Paul Stewart
 *
 *  UPS (HID) set values in UPS
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
 * e-mail - mail your message to <hiddev@wetlogic.net>.
 */


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

#define UPS_USAGE		0x840004

static inline int find_application(int fd, unsigned usage) {
	int i = 0;
	unsigned ret;

	while ((ret = ioctl(fd, HIDIOCAPPLICATION, i)) != -1 &&
               ret != usage) { printf("App: %08x\n", ret); i++; }
	if (ret < 0) {
                fprintf(stderr, "GUSAGE returns %x", ret);
                perror("");
	}
	return (ret == usage);
}

int main (int argc, char **argv) {
    int fd = -1, i, is_ups = 0;
    struct hiddev_report_info rinfo;
    struct hiddev_usage_ref uref;

    char evdev[20];
    for (i = 0; i < 4; i++) {
        sprintf(evdev, "/dev/usb/hid/hiddev%d", i);
	if ((fd = open(evdev, O_RDONLY)) >= 0) {
	    if (find_application(fd, UPS_USAGE)) {
                printf("It's a UPS!\n");
		is_ups = 1;
	    }
	    break;
	}
    }
    if (i >= 4) {
        fprintf(stderr, "Couldn't find hiddev device.\n");
	exit(1);
    }

   /* 
   Usage: 00840057 val -1 (Feature/53/0/0) [DelayBeforeShutdown]
   Usage: 00840056 val -1 (Feature/54/0/0) [DelayBeforeStartup]
    */

    memset(&uref, 0, sizeof(uref));
    uref.report_type = HID_REPORT_TYPE_FEATURE;
    if (argc > 1 && strcmp(argv[1], "off")) {
	uref.report_id = 54;
    } else {
	uref.report_id = 53;
    }

    uref.field_index = 0;
    uref.usage_index = 0;
    uref.value = 5;
    if (ioctl(fd, HIDIOCSUSAGE, &uref) != 0) perror("SUSAGE/S");
    rinfo.report_type = uref.report_type;
    rinfo.report_id = uref.report_id;
    if (ioctl(fd, HIDIOCSREPORT, &rinfo) != 0) perror("SREPORT/S");

    printf("Set 5 in report id %d\n", uref.report_id);

    exit(0);
}
