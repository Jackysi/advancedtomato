/*
 * mdadm - manage Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2001-2006 Neil Brown <neilb@suse.de>
 *
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    Author: Neil Brown
 *    Email: <neilb@cse.unsw.edu.au>
 *    Paper: Neil Brown
 *           School of Computer Science and Engineering
 *           The University of New South Wales
 *           Sydney, 2052
 *           Australia
 */

#include "mdadm.h"

char Version[] = Name " - v2.6 - 21 December 2006\n";

/*
 * File: ReadMe.c
 *
 * This file contains general comments about the implementation
 * and the various usage messages that can be displayed by mdadm
 *
 * mdadm is a single program that can be used to control Linux md devices.
 * It is intended to provide all the functionality of the mdtools and
 * raidtools but with a very different interface.
 * mdadm can perform all functions without a configuration file.
 * There is the option of using a configuration file, but not in the same
 * way that raidtools uses one
 * raidtools uses a configuration file to describe how to create a RAID
 * array, and also uses this file partially to start a previously
 * created RAID array.  Further, raidtools requires the configuration
 * file for such things as stopping a raid array which needs to know
 * nothing about the array.
 *
 * The configuration file that can be used by mdadm lists two
 * different things:
 * 1/ a mapping from uuid to md device to identify which arrays are
 *    expect and what names (numbers) they should be given
 * 2/ a list of devices that should be scanned for md sub-devices
 *
 *
 */

/*
 * mdadm has 7 major modes of operation:
 * 1/ Create
 *     This mode is used to create a new array with a superblock
 *     It can progress in several step create-add-add-run
 *     or it can all happen with one command
 * 2/ Assemble
 *     This mode is used to assemble the parts of a previously created
 *     array into an active array.  Components can be explicitly given
 *     or can be searched for.  mdadm (optionally) check that the components
 *     do form a bona-fide array, and can, on request, fiddle superblock
 *     version numbers so as to assemble a faulty array.
 * 3/ Build
 *     This is for building legacy arrays without superblocks
 * 4/ Manage
 *     This is for doing something to one or more devices
 *     in an array, such as add,remove,fail.
 *     run/stop/readonly/readwrite are also available
 * 5/ Misc
 *     This is for doing things to individual devices.
 *     They might be parts of an array so
 *       zero-superblock, examine  might be appropriate
 *     They might be md arrays so
 *       run,stop,rw,ro,detail  might be appropriate
 *     Also query will treat it as either
 * 6/ Monitor
 *     This mode never exits but just monitors arrays and reports changes.
 * 7/ Grow
 *     This mode allows for changing of key attributes of a raid array, such
 *     as size, number of devices, and possibly even layout.
 *     At the time if writing, there is only minimal support.
 */

char short_options[]="-ABCDEFGIQhVXWvqbc:i:l:p:m:n:x:u:c:d:z:U:sarfRSow1tye:";
char short_bitmap_auto_options[]=
                   "-ABCDEFGIQhVXWvqb:c:i:l:p:m:n:x:u:c:d:z:U:sa:rfRSow1tye:";

struct option long_options[] = {
    {"manage",    0, 0, '@'},
    {"misc",      0, 0, '#'},
    {"assemble",  0, 0, 'A'},
    {"build",     0, 0, 'B'},
    {"create",    0, 0, 'C'},
    {"detail",    0, 0, 'D'},
    {"examine",   0, 0, 'E'},
    {"follow",    0, 0, 'F'},
    {"grow",      0, 0, 'G'},
    {"incremental",0,0, 'I'},
    {"zero-superblock", 0, 0, 'K'}, /* deliberately no a short_option */
    {"query",	  0, 0, 'Q'},
    {"examine-bitmap", 0, 0, 'X'},

    /* synonyms */
    {"monitor",   0, 0, 'F'},
	    
    /* after those will normally come the name of the md device */
    {"help",      0, 0, 'h'},
    {"help-options",0,0,'h'},
    {"version",	  0, 0, 'V'},
    {"verbose",   0, 0, 'v'},
    {"quiet",	  0, 0, 'q'},

    /* For create or build: */
    {"chunk",	  1, 0, 'c'},
    {"rounding",  1, 0, 'c'}, /* for linear, chunk is really a rounding number */
    {"level",     1, 0, 'l'}, /* 0,1,4,5,6,linear */
    {"parity",    1, 0, 'p'}, /* {left,right}-{a,}symmetric */
    {"layout",    1, 0, 'p'},
    {"raid-disks",1, 0, 'n'},
    {"raid-devices",1, 0, 'n'},
    {"spare-disks",1,0, 'x'},
    {"spare-devices",1,0, 'x'},
    {"size",	  1, 0, 'z'},
    {"auto",	  1, 0, 'a'}, /* also for --assemble */
    {"assume-clean",0,0, AssumeClean },
    {"metadata",  1, 0, 'e'}, /* superblock format */
    {"bitmap",	  1, 0, 'b'},
    {"bitmap-chunk", 1, 0, BitmapChunk},
    {"write-behind", 2, 0, WriteBehind},
    {"write-mostly",0, 0, 'W'},
    {"re-add",    0, 0,  ReAdd},
    {"homehost",  1, 0,  HomeHost},
    {"auto-update-homehost", 0, 0, AutoHomeHost},
    {"symlinks",  1, 0,  Symlinks},

    /* For assemble */
    {"uuid",      1, 0, 'u'},
    {"super-minor",1,0, 'm'},
    {"name",	  1, 0, 'N'},
    {"config",    1, 0, 'c'},
    {"scan",      0, 0, 's'},
    {"force",	  0, 0, 'f'},
    {"update",	  1, 0, 'U'},

    /* Management */
    {"add",       0, 0, 'a'},
    {"remove",    0, 0, 'r'},
    {"fail",      0, 0, 'f'},
    {"set-faulty",0, 0, 'f'},
    {"run",       0, 0, 'R'},
    {"stop",      0, 0, 'S'},
    {"readonly",  0, 0, 'o'},
    {"readwrite", 0, 0, 'w'},
    {"no-degraded",0,0,  NoDegraded },
    {"wait",	  0, 0, 'W'},

    /* For Detail/Examine */
    {"brief",	  0, 0, 'b'},
    {"sparc2.2",  0, 0, Sparc22},
    {"test",      0, 0, 't'},

    /* For Follow/monitor */
    {"mail",      1, 0, 'm'},
    {"program",   1, 0, 'p'},
    {"alert",     1, 0, 'p'},
    {"delay",     1, 0, 'd'},
    {"daemonise", 0, 0, 'f'},
    {"daemonize", 0, 0, 'f'},
    {"oneshot",   0, 0, '1'},
    {"pid-file",  1, 0, 'i'},
    {"syslog",    0, 0, 'y'},
    /* For Grow */
    {"backup-file", 1,0, BackupFile},

    /* For Incremental */
    {"rebuild-map", 0, 0, 'r'},
    {0, 0, 0, 0}
};

char Usage[] =
"Usage: mdadm --help\n"
"  for help\n"
;

char Help[] =
"mdadm is used for building, managing, and monitoring\n"
"Linux md devices (aka RAID arrays)\n"
"Usage: mdadm --create device options...\n"
"            Create a new array from unused devices.\n"
"       mdadm --assemble device options...\n"
"            Assemble a previously created array.\n"
"       mdadm --build device options...\n"
"            Create or assemble an array without metadata.\n"
"       mdadm --manage device options...\n"
"            make changes to an existing array.\n"
"       mdadm --misc options... devices\n"
"            report on or modify various md related devices.\n"
"       mdadm --grow options device\n"
"            resize/reshape an active array\n"
"       mdadm --incremental device\n"
"            add a device to an array as appropriate\n"
"       mdadm --monitor options...\n"
"            Monitor one or more array for significant changes.\n"
"       mdadm device options...\n"
"            Shorthand for --manage.\n"
"Any parameter that does not start with '-' is treated as a device name\n"
"or, for --examine-bitmap, a file name.\n"
"The first such name is often the name of an md device.  Subsequent\n"
"names are often names of component devices.\n"
"\n"
" For detailed help on the above major modes use --help after the mode\n"
" e.g.\n"
"         mdadm --assemble --help\n"
" For general help on options use\n"
"         mdadm --help-options\n"
;

char OptionHelp[] =
"Any parameter that does not start with '-' is treated as a device name\n"
"or, for --examine-bitmap, a file name.\n"
"The first such name is often the name of an md device.  Subsequent\n"
"names are often names of component devices.\n"
"\n"
"Some common options are:\n"
"  --help        -h   : General help message or, after above option,\n"
"                       mode specific help message\n"
"  --help-options     : This help message\n"
"  --version     -V   : Print version information for mdadm\n"
"  --verbose     -v   : Be more verbose about what is happening\n"
"  --quiet       -q   : Don't print un-necessary messages\n"
"  --brief       -b   : Be less verbose, more brief\n"
"  --force       -f   : Override normal checks and be more forceful\n"
"\n"
"  --assemble    -A   : Assemble an array\n"
"  --build       -B   : Build an array without metadata\n"
"  --create      -C   : Create a new array\n"
"  --detail      -D   : Display details of an array\n"
"  --examine     -E   : Examine superblock on an array component\n"
"  --examine-bitmap -X: Display the detail of a bitmap file\n"
"  --monitor     -F   : monitor (follow) some arrays\n"
"  --grow        -G   : resize/ reshape and array\n"
"  --incremental -I   : add a single device to an array as appropriate\n"
"  --query       -Q   : Display general information about how a\n"
"                       device relates to the md driver\n"
;
/*
"\n"
" For create or build:\n"
"  --bitmap=     -b   : File to store bitmap in - may pre-exist for --build\n"
"  --chunk=      -c   : chunk size of kibibytes\n"
"  --rounding=        : rounding factor for linear array (==chunk size)\n"
"  --level=      -l   : raid level: 0,1,4,5,6,linear,mp.  0 or linear for build\n"
"  --parity=     -p   : raid5/6 parity algorithm: {left,right}-{,a}symmetric\n"
"  --layout=          : same as --parity\n"
"  --raid-devices= -n : number of active devices in array\n"
"  --spare-devices= -x: number of spares (eXtras) devices in initial array\n"
"  --size=       -z   : Size (in K) of each drive in RAID1/4/5/6/10 - optional\n"
"  --force       -f   : Honour devices as listed on command line.  Don't\n"
"                     : insert a missing drive for RAID5.\n"
"  --auto(=p)    -a   : Automatically allocate new (partitioned) md array if needed.\n"
"  --assume-clean     : Assume the array is already in-sync. This is dangerous.\n"
"  --bitmap-chunk=    : chunksize of bitmap in bitmap file (Kilobytes)\n"
"  --delay=      -d   : seconds between bitmap updates\n"
"  --write-behind=    : number of simultaneous write-behind requests to allow (requires bitmap)\n"
"  --name=       -N   : Textual name for array - max 32 characters\n"
"\n"
" For assemble:\n"
"  --bitmap=     -b   : File to find bitmap information in\n"
"  --uuid=       -u   : uuid of array to assemble. Devices which don't\n"
"                       have this uuid are excluded\n"
"  --super-minor= -m  : minor number to look for in super-block when\n"
"                       choosing devices to use.\n"
"  --name=       -N   : Array name to look for in super-block.\n"
"  --config=     -c   : config file\n"
"  --scan        -s   : scan config file for missing information\n"
"  --force       -f   : Assemble the array even if some superblocks appear out-of-date\n"
"  --update=     -U   : Update superblock: try '-A --update=?' for list of options.\n"
"  --auto(=p)    -a   : Automatically allocate new (partitioned) md array if needed.\n"
"  --no-degraded      : Do not start any degraded arrays - default unless --scan.\n"
"\n"
" For detail or examine:\n"
"  --brief       -b   : Just print device name and UUID\n"
"\n"
" For follow/monitor:\n"
"  --mail=       -m   : Address to mail alerts of failure to\n"
"  --program=    -p   : Program to run when an event is detected\n"
"  --alert=           : same as --program\n"
"  --delay=      -d   : seconds of delay between polling state. default=60\n"
"\n"
" General management:\n"
"  --add         -a   : add, or hotadd subsequent devices\n"
"  --remove      -r   : remove subsequent devices\n"
"  --fail        -f   : mark subsequent devices a faulty\n"
"  --set-faulty       : same as --fail\n"
"  --run         -R   : start a partially built array\n"
"  --stop        -S   : deactivate array, releasing all resources\n"
"  --readonly    -o   : mark array as readonly\n"
"  --readwrite   -w   : mark array as readwrite\n"
"  --zero-superblock  : erase the MD superblock from a device.\n"
"  --wait        -W   : wait for recovery/resync/reshape to finish.\n"
;
*/

char Help_create[] =
"Usage:  mdadm --create device -chunk=X --level=Y --raid-devices=Z devices\n"
"\n"
" This usage will initialise a new md array, associate some\n"
" devices with it, and activate the array.   In order to create an\n"
" array with some devices missing, use the special word 'missing' in\n"
" place of the relevant device name.\n"
"\n"
" Before devices are added, they are checked to see if they already contain\n"
" raid superblocks or filesystems.  They are also checked to see if\n"
" the variance in device size exceeds 1%.\n"
" If any discrepancy is found, the user will be prompted for confirmation\n"
" before the array is created.  The presence of a '--run' can override this\n"
" caution.\n"
"\n"
" If the --size option is given then only that many kilobytes of each\n"
" device is used, no matter how big each device is.\n"
" If no --size is given, the apparent size of the smallest drive given\n"
" is used for raid level 1 and greater, and the full device is used for\n"
" other levels.\n"
"\n"
" Options that are valid with --create (-C) are:\n"
"  --bitmap=          : Create a bitmap for the array with the given filename\n"
"  --chunk=      -c   : chunk size of kibibytes\n"
"  --rounding=        : rounding factor for linear array (==chunk size)\n"
"  --level=      -l   : raid level: 0,1,4,5,6,linear,multipath and synonyms\n"
"  --parity=     -p   : raid5/6 parity algorithm: {left,right}-{,a}symmetric\n"
"  --layout=          : same as --parity\n"
"  --raid-devices= -n : number of active devices in array\n"
"  --spare-devices= -x: number of spares (eXtras) devices in initial array\n"
"  --size=       -z   : Size (in K) of each drive in RAID1/4/5/6/10 - optional\n"
"  --force       -f   : Honour devices as listed on command line.  Don't\n"
"                     : insert a missing drive for RAID5.\n"
"  --run         -R   : insist of running the array even if not all\n"
"                     : devices are present or some look odd.\n"
"  --readonly    -o   : start the array readonly - not supported yet.\n"
"  --name=       -N   : Textual name for array - max 32 characters\n"
"  --bitmap-chunk=    : bitmap chunksize in Kilobytes.\n"
"  --delay=      -d   : bitmap update delay in seconds.\n"
"\n"
;

char Help_build[] =
"Usage:  mdadm --build device -chunk=X --level=Y --raid-devices=Z devices\n"
"\n"
" This usage is similar to --create.  The difference is that it creates\n"
" a legacy array without a superblock.  With these arrays there is no\n"
" different between initially creating the array and subsequently\n"
" assembling the array, except that hopefully there is useful data\n"
" there in the second case.\n"
"\n"
" The level may only be 0, raid0, or linear.\n"
" All devices must be listed and the array will be started once complete.\n"
" Options that are valid with --build (-B) are:\n"
"  --bitmap=          : file to store/find bitmap information in.\n"
"  --chunk=      -c   : chunk size of kibibytes\n"
"  --rounding=        : rounding factor for linear array (==chunk size)\n"
"  --level=      -l   : 0, raid0, or linear\n"
"  --raid-devices= -n : number of active devices in array\n"
"  --bitmap-chunk=    : bitmap chunksize in Kilobytes.\n"
"  --delay=      -d   : bitmap update delay in seconds.\n"
;

char Help_assemble[] =
"Usage: mdadm --assemble device options...\n"
"       mdadm --assemble --scan options...\n"
"\n"
"This usage assembles one or more raid arrays from pre-existing\n"
"components.\n"
"For each array, mdadm needs to know the md device, the identity of\n"
"the array, and a number of sub devices. These can be found in a number\n"
"of ways.\n"
"\n"
"The md device is either given on the command line or is found listed\n"
"in the config file.  The array identity is determined either from the\n"
"--uuid or --super-minor commandline arguments, from the config file,\n"
"or from the first component device on the command line.\n"
"\n"
"The different combinations of these are as follows:\n"
" If the --scan option is not given, then only devices and identities\n"
" listed on the command line are considered.\n"
" The first device will be the array device, and the remainder will be\n"
" examined when looking for components.\n"
" If an explicit identity is given with --uuid or --super-minor, then\n"
" only devices with a superblock which matches that identity is considered,\n"
" otherwise every device listed is considered.\n"
"\n"
" If the --scan option is given, and no devices are listed, then\n"
" every array listed in the config file is considered for assembly.\n"
" The identity of candidate devices are determined from the config file.\n"
"\n"
" If the --scan option is given as well as one or more devices, then\n"
" Those devices are md devices that are to be assembled.  Their identity\n"
" and components are determined from the config file.\n"
"\n"
" If mdadm can not find all of the components for an array, it will assemble\n"
" it but not activate it unless --run or --scan is given.  To preserve this\n"
" behaviour even with --scan, add --no-degraded.  Note that \"all of the\n"
" components\" means as many as were present the last time the array was running\n"
" as recorded in the superblock.  If the array was already degraded, and\n"
" the missing device is not a new problem, it will still be assembled.  It\n"
" is only newly missing devices that cause the array not to be started.\n"
"\n"
"Options that are valid with --assemble (-A) are:\n"
"  --bitmap=          : bitmap file to use wit the array\n"
"  --uuid=       -u   : uuid of array to assemble. Devices which don't\n"
"                       have this uuid are excluded\n"
"  --super-minor= -m  : minor number to look for in super-block when\n"
"                       choosing devices to use.\n"
"  --name=       -N   : Array name to look for in super-block.\n"
"  --config=     -c   : config file\n"
"  --scan        -s   : scan config file for missing information\n"
"  --run         -R   : Try to start the array even if not enough devices\n"
"                       for a full array are present\n"
"  --force       -f   : Assemble the array even if some superblocks appear\n"
"                     : out-of-date.  This involves modifying the superblocks.\n"
"  --update=     -U   : Update superblock: try '-A --update=?' for option list.\n"
"  --no-degraded      : Assemble but do not start degraded arrays.\n"
;

char Help_manage[] =
"Usage: mdadm arraydevice options component devices...\n"
"\n"
"This usage is for managing the component devices within an array.\n"
"The --manage option is not needed and is assumed if the first argument\n"
"is a device name or a management option.\n"
"The first device listed will be taken to be an md array device, and\n"
"subsequent devices are (potential) components of that array.\n"
"\n"
"Options that are valid with management mode are:\n"
"  --add         -a   : hotadd subsequent devices to the array\n"
"  --remove      -r   : remove subsequent devices, which must not be active\n"
"  --fail        -f   : mark subsequent devices a faulty\n"
"  --set-faulty       : same as --fail\n"
"  --run         -R   : start a partially built array\n"
"  --stop        -S   : deactivate array, releasing all resources\n"
"  --readonly    -o   : mark array as readonly\n"
"  --readwrite   -w   : mark array as readwrite\n"
;

char Help_misc[] =
"Usage: mdadm misc_option  devices...\n"
"\n"
"This usage is for performing some task on one or more devices, which\n"
"may be arrays or components, depending on the task.\n"
"The --misc option is not needed (though it is allowed) and is assumed\n"
"if the first argument in a misc option.\n"
"\n"
"Options that are valid with the miscellaneous mode are:\n"
"  --query       -Q   : Display general information about how a\n"
"                       device relates to the md driver\n"
"  --detail      -D   : Display details of an array\n"
"  --examine     -E   : Examine superblock on an array component\n"
"  --examine-bitmap -X: Display contents of a bitmap file\n"
"  --zero-superblock  : erase the MD superblock from a device.\n"
"  --run         -R   : start a partially built array\n"
"  --stop        -S   : deactivate array, releasing all resources\n"
"  --readonly    -o   : mark array as readonly\n"
"  --readwrite   -w   : mark array as readwrite\n"
"  --test        -t   : exit status 0 if ok, 1 if degrade, 2 if dead, 4 if missing\n"
"  --wait        -W   : wait for resync/rebuild/recovery to finish\n"
;

char Help_monitor[] =
"Usage: mdadm --monitor options devices\n"
"\n"
"This usage causes mdadm to monitor a number of md arrays by periodically\n"
"polling their status and acting on any changes.\n"
"If any devices are listed then those devices are monitored, otherwise\n"
"all devices listed in the config file are monitored.\n"
"The address for mailing advisories to, and the program to handle\n"
"each change can be specified in the config file or on the command line.\n"
"If no mail address or program are specified, then mdadm reports all\n"
"state changes to stdout.\n"
"\n"
"Options that are valid with the monitor (-F --follow) mode are:\n"
"  --mail=       -m   : Address to mail alerts of failure to\n"
"  --program=    -p   : Program to run when an event is detected\n"
"  --alert=           : same as --program\n"
"  --delay=      -d   : seconds of delay between polling state. default=60\n"
"  --config=     -c   : specify a different config file\n"
"  --scan        -s   : find mail-address/program in config file\n"
"  --daemonise   -f   : Fork and continue in child, parent exits\n"
"  --pid-file=   -i   : In daemon mode write pid to specified file instead of stdout\n"
"  --oneshot     -1   : Check for degraded arrays, then exit\n"
"  --test        -t   : Generate a TestMessage event against each array at startup\n"
;

char Help_grow[] =
"Usage: mdadm --grow device options\n"
"\n"
"This usage causes mdadm to attempt to reconfigure a running array.\n"
"This is only possibly if the kernel being used supports a particular\n"
"reconfiguration.  This version only supports changing the number of\n"
"devices in a RAID1, and changing the active size of all devices in\n"
"a RAID1/4/5/6.\n"
"\n"
"Options that are valid with the grow (-G --grow) mode are:\n"
"  --size=        -z   : Change the active size of devices in an array.\n"
"                      : This is useful if all devices have been replaced\n"
"                      : with larger devices.\n"
"  --raid-disks=  -n   : Change the number of active devices in a RAID1\n"
"                      : array.\n"
;

char Help_incr[] =
"Usage: mdadm --incremental [-Rqrs] device\n"
"\n"
"This usage allows for incremental assembly of md arrays.  Devices can be\n"
"added one at a time as they are discovered.  Once an array has all expected\n"
"devices, it will be started.\n"
"\n"
"Options that are valid with incremental assembly (-I --incremental) more are:\n"
"  --run       -R  : run arrays as soon as a minimal number of devices are\n"
"                  : present rather than waiting for all expected.\n"
"  --quiet     -q  : Don't print any information messages, just errors.\n"
"  --rebuild   -r  : Rebuild the 'map' file that mdadm uses for tracking\n"
"                  : partial arrays.\n"
"  --scan      -s  : Use with -R to start any arrays that have the minimal\n"
"                  : required number of devices, but are not yet started.\n"
;

char Help_config[] =
"The /etc/mdadm.conf config file:\n\n"
" The config file contains, apart from blank lines and comment lines that\n"
" start with a hash(#), four sorts of configuration lines: array lines, \n"
" device lines, mailaddr lines and program lines.\n"
" Each configuration line is constructed of a number of space separated\n"
" words, and can be continued on subsequent physical lines by indenting\n"
" those lines.\n"
"\n"
" A device line starts with the word 'device' and then has a number of words\n"
" which identify devices.  These words should be names of devices in the\n"
" filesystem, and can contain wildcards. There can be multiple words or each\n"
" device line, and multiple device lines.  All devices so listed are checked\n"
" for relevant super blocks when assembling arrays.\n"
"\n"
" An array line start with the word 'array'.  This is followed by the name of\n"
" the array device in the filesystem, e.g. '/dev/md2'.  Subsequent words\n"
" describe the identity of the array, used to recognise devices to include in the\n"
" array.  The identity can be given as a UUID with a word starting 'uuid=', or\n"
" as a minor-number stored in the superblock using 'super-minor=', or as a list\n"
" of devices.  This is given as a comma separated list of names, possibly\n"
" containing wildcards, preceded by 'devices='. If multiple critea are given,\n"
" than a device must match all of them to be considered.\n"
"\n"
" A mailaddr line starts with the word 'mailaddr' and should contain exactly\n"
" one Email address.  'mdadm --monitor --scan' will send alerts of failed drives\n"
" to this Email address."
"\n"
" A program line starts with the word 'program' and should contain exactly\n"
" one program name.  'mdadm --monitor --scan' will run this program when any\n"
" event is detected.\n"
"\n"
;


/* name/number mappings */

mapping_t r5layout[] = {
	{ "left-asymmetric", 0},
	{ "right-asymmetric", 1},
	{ "left-symmetric", 2},
	{ "right-symmetric", 3},

	{ "default", 2},
	{ "la", 0},
	{ "ra", 1},
	{ "ls", 2},
	{ "rs", 3},
	{ NULL, 0}
};

mapping_t pers[] = {
	{ "linear", -1},
	{ "raid0", 0},
	{ "0", 0},
	{ "stripe", 0},
	{ "raid1", 1},
	{ "1", 1},
	{ "mirror", 1},
	{ "raid4", 4},
	{ "4", 4},
	{ "raid5", 5},
	{ "5", 5},
	{ "multipath", -4},
	{ "mp", -4},
	{ "raid6", 6},
	{ "6", 6},
	{ "raid10", 10},
	{ "10", 10},
	{ "faulty", -5},
	{ NULL, 0}
};


mapping_t modes[] = {
	{ "assemble", ASSEMBLE},
	{ "build", BUILD},
	{ "create", CREATE},
	{ "manage", MANAGE},
	{ "misc", MISC},
	{ "monitor", MONITOR},
	{ "grow", GROW},
	{ "incremental", INCREMENTAL},
};

mapping_t faultylayout[] = {
	{ "write-transient", WriteTransient },
	{ "wt", WriteTransient },
	{ "read-transient", ReadTransient },
	{ "rt", ReadTransient },
	{ "write-persistent", WritePersistent },
	{ "wp", WritePersistent },
	{ "read-persistent", ReadPersistent },
	{ "rp", ReadPersistent },
	{ "write-all", WriteAll },
	{ "wa", WriteAll },
	{ "read-fixable", ReadFixable },
	{ "rf", ReadFixable },

	{ "clear", ClearErrors},
	{ "flush", ClearFaults},
	{ "none", ClearErrors},
	{ "default", ClearErrors},
	{ NULL, 0}
};
