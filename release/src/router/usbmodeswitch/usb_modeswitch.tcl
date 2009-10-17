#!/usr/bin/tclsh

# Wrapper for usb_modeswitch, called by
# /etc/udev/rules.d/80-usb_modeswitch.rules
#
# Does ID check on hotplugged USB devices and calls the
# mode switching program with the matching parameter file
# from /etc/usb_modeswitch.d


# (C) Josua Dietze 2009
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details:
#
# http://www.gnu.org/licenses/gpl.txt



# Change this to 1 if you want a simple logging
# to /var/log/usb_modeswitch

set logging 0


# Execution starts at the file bottom

proc {Main} {argc argv} {

global scsi usb match wc logging
set dbdir	/etc/usb_modeswitch.d
set bindir	/usr/sbin

Log "raw args from udev: $argv"


# Mapping of the short string identifiers (in the config
# file names) to the long name used here
#
# If we need them it's a snap to add new attributes here!

set match(sVe) scsi(vendor)
set match(sMo) scsi(model)
set match(sRe) scsi(rev)
set match(uMa) usb(manufacturer)
set match(uPr) usb(product)
set match(uSe) usb(serial)

# argv contains the values provided from the udev rule
# separated by "/"

set argList [split [lindex $argv 0] /]

# arg 0: the bus id for the device (udev: %b)
# arg 1: the "kernel name" for the device (udev: %k)
#
# Both together give the top directory where the path
# to the SCSI attributes can be determined (further down)

set devdir /sys/bus/usb/devices/[lindex $argList 0]

# The "ready-to-eat" values from the udev command

set usb(VID) [lindex $argList 2]
set usb(PID) [lindex $argList 3]
set usb(manufacturer) [lindex $argList 4]
set usb(product) [lindex $argList 5]
set usb(serial) [lindex $argList 6]

# We don't know these yet

set scsi(vendor) ""
set scsi(model) ""
set scsi(rev) ""

Log "----------------\nUSB values from udev:"
foreach attr {manufacturer product serial} {
	Log "  $attr\t$usb($attr)"
}

# Getting the SCSI values via libusb results in a detached
# usb-storage driver. Not good for devices that want to be
# left alone. Fortunately, the sysfs tree provides the values
# too without need for direct access

# First we wait until the SCSI data is ready - or timeout.
# Timeout means: no storage driver was bound to the device.
# We run 20 times max, every half second (max. 10 seconds
# total)

# We also check if the device itself changes, probably
# because it was switched by the kernel (or even unplugged).
# Then we do simply nothing and exit quietly ...

set counter 0
while {$counter < 20} {
	after 500
	incr counter

	set sysdir $devdir/[lindex $argList 1]

	if {![file isdirectory $sysdir]} {
		# Device is gone. Unplugged? Switched by kernel?
		Log "sysfs device tree is gone; exiting"
		SafeExit
	}
	set rc [open $devdir/product r]
	set newproduct [read -nonewline $rc]
	close $rc
	if {![string match $newproduct $usb(product)]} {
		# Device has just changed. Switched by someone else?
		Log "device has changed; exiting"
		SafeExit
	}

	# Searching the storage/SCSI tree; might take a while
	if {[set dirList [glob -nocomplain $sysdir/host*]] != ""} {
		set sysdir [lindex $dirList 0]
		if {[set dirList [glob -nocomplain $sysdir/target*]] != ""} {
			set sysdir [lindex $dirList 0]
			regexp {.*target(.*)} $sysdir d subdir
			if {[set dirList [glob -nocomplain $sysdir/$subdir*]] != ""} {
				set sysdir [lindex $dirList 0]
				if [file isdirectory $sysdir/block] {
					# Finally SCSI structure is ready, get the values
					ReadStrings $sysdir
					Log "SCSI values read"
					break
				}
			}
		}
	}
}

Log "----------------\nSCSI values from sysfs:"
foreach attr {vendor model rev} {
	Log " $attr\t$scsi($attr)"
}

# If no storage driver is active, we try and get the values
# from a (nonswitching) call of usb_modeswitch

if {$scsi(vendor)==""} {
	set testSCSI [exec $bindir/usb_modeswitch -v 0x$usb(VID) -p 0x$usb(PID)]
	regexp {  Vendor String: (.*?)\n} $testSCSI d scsi(vendor)
	regexp {   Model String: (.*?)\n} $testSCSI d scsi(model)
	regexp {Revision String: (.*?)\n} $testSCSI d scsi(rev)
	Log "SCSI values from usb_modeswitch:"
	foreach attr {vendor model rev} {
		Log " $attr\t$scsi($attr)"
	}
}

# If we don't have the SCSI values by now, we just
# leave the variables empty; they won't match anything

# Time to check for a matching config file.
# Matching itself is done by MatchDevice

set report {}
set configList [glob -nocomplain $dbdir/$usb(VID):$usb(PID)*]
foreach configuration $configList {
	Log "checking config: $configuration"
	if [MatchDevice $configuration] {
		set devList1 [glob -nocomplain /dev/ttyUSB* /dev/ttyACM*]
		Log "! matched, now switching ..."
		if $logging {
			set report [exec $bindir/usb_modeswitch -I -W -c $configuration 2>@ stdout]
		} else {
			set report [exec $bindir/usb_modeswitch -I -Q -c $configuration]
		}
		Log "\nverbose output of usb_modeswitch:"
		Log "--------------------------------"
		Log $report
		Log "--------------------------------"
		break
	} else {
		Log "* no match, not switching"
	}
}

# We're finished with switching; success checking
# was done by usb_modeswitch and logged via syslog.
#
# If switching was OK we now check for drivers by
# simply recounting serial devices under /dev

if [regexp {ok:} $report] {
	# some settle time in ms
	after 500
	set devList2 [glob -nocomplain /dev/ttyUSB* /dev/ttyACM*]
	if {[llength $devList1] >= [llength $devList2]} {
		Log " no new serial devices found"
		if [regexp {ok:(\w{4}):(\w{4})} $report d vend prod] {
			set idfile /sys/bus/usb-serial/drivers/option1/new_id
			if {![file exists $idfile]} {
				Log "\nTrying to load the option driver"
				set loader /sbin/modprobe
				Log " loader is: $loader"
				if [file exists $loader] {
					set result [exec $loader -v option]
					if {[regexp {not found} $result]} {
						Log " option driver not present as module"
					}
				} else {
						Log " /sbin/modprobe not found"
				}
			}
			if [file exists $idfile] {
				Log "Trying to add ID to option driver"
				catch {exec /bin/logger -p syslog.notice "usb_modeswitch.tcl: adding device ID $vend:$prod to driver \"option\""}
				exec echo "$vend $prod" >$idfile
				after 600
				set devList2 [glob -nocomplain /dev/ttyUSB* /dev/ttyACM*]
				if {[llength $devList1] >= [llength $devList2]} {
					Log " still no new serial devices found"
				} else {
					Log " driver successfully bound"
				}
			}
		}
	}
}

Log "\nAll done, exiting\n"
SafeExit

}
# end of proc {Main}


proc {ReadStrings} {dir} {

global scsi
Log "SCSI dir exists: $dir"

foreach attr {vendor model rev} {
	if [file exists $dir/$attr] {
		set rc [open $dir/$attr r]
		set scsi($attr) [read -nonewline $rc]
		close $rc
	}
}

}
# end of proc {ReadStrings}


proc {MatchDevice} {config} {

global scsi usb match

set devinfo [file tail $config]
set infoList [split $devinfo :]
set stringList [lrange $infoList 2 end]
if {[llength $stringList] == 0} {return 1}

foreach teststring $stringList {
	if {$teststring == "?"} {return 0}
	set tokenList [split $teststring =]
	set id [lindex $tokenList 0]
	set matchstring [lindex $tokenList 1]
	regsub -all {_} $matchstring { } matchstring
	Log "matching $match($id)"
	Log "  match string: $matchstring"
	Log " device string: [set $match($id)]"
	if {![string match $matchstring* [set $match($id)]] } {
		return 0
	}
}
return 1

}
# end of proc {MatchDevice}


proc {Log} {msg} {

global wc logging
if {$logging == 0} {return}
if {![info exists wc]} {
	set wc [open /var/log/usb_modeswitch a+]
	puts $wc "\n\nUSB_ModeSwitch log from [clock format [clock seconds]]\n"
}
puts $wc $msg

}
# end of proc {Log}


proc {SafeExit} {} {
global wc
if [info exists wc] {
	catch {close $wc}
}
exit

}
# end of proc {SafeExit}


# The actual entry point
Main $argc $argv
