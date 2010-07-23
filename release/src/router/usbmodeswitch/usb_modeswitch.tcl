#!/bin/sh
# next lines for bash, ignored by tclsh, restarting in background\
export PATH=/bin:/usr/bin; \
if [ ! -e "/usr/bin/tclsh" ]; then \
	logger -p syslog.error "usb_modeswitch: tcl shell not found, install tcl package!"; \
fi; \
(/usr/bin/tclsh "$0" "$@" >/dev/null 2>&1 &); \
sleep 1; \
exit


# Wrapper (tcl) for usb_modeswitch, called from
# /lib/udev/rules.d/40-usb_modeswitch.rules
# (part of data pack "usb-modeswitch-data")
#
# Does ID check on hotplugged USB devices and calls the
# mode switching program with the matching parameter file
# from /etc/usb_modeswitch.d
#
# Part of usb-modeswitch-1.1.3 package
# (C) Josua Dietze 2009, 2010


# Setting of the following switches is done in an external config
# file (/etc/usb_modeswitch.conf)

set logging 0
set noswitching 0



set env(PATH) "/bin:/usr/bin"

# Execution starts at file bottom

proc {Main} {argc argv} {

global scsi usb match wc device logging noswitching

# The facility to add a symbolic link pointing to the
# ttyUSB port which provides interrupt transfer, i.e.
# the port to connect through; returns a symlink name
# for udev and exits
# This is run once for every device interface by an
# udev rule

if {[lindex $argv 0] == "symlink"} {
#	puts "symlink: udev path is [lindex $argv 2]"
#	set device [clock clicks]
#	set logging 1
#	Log "symlink: udev path is [lindex $argv 2]"
	if [llength [glob -nocomplain /tmp/gsmmodem_*]] {
		set sl [SymLinkName [lindex $argv 2]]
#		Log "symlink name is :$sl:"
		puts $sl
#		puts [SymLinkName [lindex $argv 2]]
	} else {
		puts ""
	}
	SafeExit
}

set dbdir	/etc/usb_modeswitch.d
set bindir	/usr/sbin

set devList1 {}
set devList2 {}


# argv contains the values provided from the udev rule
# separated by "/"

set argList [split [lindex $argv 0] /]

if [string length [lindex $argList 1]] {
	set device [lindex $argList 1]
} else {
	set device "noname"
}

ParseConfigFile

Log "raw args from udev: $argv"

if {$device == "noname"} {
	Log "No data from udev. Exiting"
	SafeExit
}

# arg 0: the bus id for the device (udev: %b)
# arg 1: the "kernel name" for the device (udev: %k)
#
# Both together give the top directory where the path
# to the SCSI attributes can be determined (further down)
# Addendum: older kernel/udev version seem to differ in
# providing these attributes - or not. So more probing
# is needed

if {[string length [lindex $argList 0]] == 0} {
	if {[string length [lindex $argList 1]] == 0} {
		Log "No device number values given from udev! Exiting"
		SafeExit
	} else {
		Log "Bus ID for device not given by udev."
		Log " Trying to determine it from kernel name ([lindex $argList 1]) ..."
		if {![regexp {(.*?):} [lindex $argList 1] d dev_top]} {
			Log "Could not determine top device dir from udev values! Exiting"
			SafeExit
		}
	}
} else {
	set dev_top [lindex $argList 0]
	regexp {(.*?):} $dev_top d dev_top
}


set devdir /sys/bus/usb/devices/$dev_top
if {![file isdirectory $devdir]} {
	Log "Top sysfs directory not found ($devdir)! Exiting"
	SafeExit
}


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


# Now reading the USB attributes

ReadUSBAttrs $devdir

if {[string length "$usb(idVendor)$usb(idProduct)"] < 8} {
	Log "USB IDs not found in sysfs tree. Exiting"
	SafeExit
}

Log "----------------\nUSB values from sysfs:"
foreach attr {manufacturer product serial} {
	Log "  $attr\t$usb($attr)"
}
Log "----------------"

if $noswitching {
	Log "\nSwitching globally disabled. Exiting\n"
	catch {exec logger -p syslog.notice "usb_modeswitch: switching disabled, no action for $usb(idVendor):$usb(idProduct)"}
	SafeExit
}

# Special ZTE check
if {"$usb(idVendor)$usb(idProduct)" == "19d22000"} {
	foreach dir {/etc/udev/rules.d /lib/udev/rules.d} {
		catch {eval exec grep {"19d2.*2000.*eject"} [glob -nocomplain $dir/*]} result
		if [regexp {(.*?):.*19d2} $result d ruleFile] {
			Log "\nWarning: existing ZTE rule found in $ruleFile. Might cause problems\n"
		}
	}
}

# Check if there is more than one config file for this USB ID,
# which would point to a possible ambiguity. If so, check if
# SCSI values are needed

set configList [glob -nocomplain $dbdir/$usb(idVendor):$usb(idProduct)*]
if {[llength $configList] == 0} {
	Log "Aargh! Config file missing for $usb(idVendor):$usb(idProduct)! Exiting"
	SafeExit
}

set scsiNeeded false
if {[llength $configList] > 1} {
	if [regexp {:s} $configList] {
		set scsiNeeded true
	}
}
if {!$scsiNeeded} {
	Log "SCSI attributes not needed, moving on"
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
while {$scsiNeeded && $counter < 20} {
	after 500
	incr counter
	Log "waiting for storage tree in sysfs"

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
				if [file exists $sysdir/vendor] {
					# Finally SCSI structure is ready, get the values
					ReadSCSIAttrs $sysdir
					Log "SCSI values read"
					break
				}
			}
		}
	}
}
if $scsiNeeded {
	if {$counter == 20 && [string length $scsi(vendor)] == 0} {
		Log "SCSI tree not found; you may want to check if this path/file exists:"
		Log "$sysdir/vendor\n"
	} else {
		Log "----------------\nSCSI values from sysfs:"
		foreach attr {vendor model rev} {
			Log " $attr\t$scsi($attr)"
		}
		Log "----------------"
	}
	Log "Waiting 3 secs. after SCSI device was added"
	after 3000
} else {
	after 500
}

# If SCSI tree in sysfs was not identified, try and get the values
# from a (nonswitching) call of usb_modeswitch; this detaches the
# storage driver, so it's just the last resort

if {$scsiNeeded && $scsi(vendor)==""} {
	set testSCSI [exec $bindir/usb_modeswitch -v 0x$usb(idVendor) -p 0x$usb(idProduct)]
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
#
# Sorting the configuration file names reverse so that
# the ones with matching additions are tried first; the
# common configs without match attributes are used at the
# end and provide a fallback

set report {}
set configList [glob -nocomplain $dbdir/$usb(idVendor):$usb(idProduct)*]
foreach configuration [lsort -decreasing $configList] {

	# skipping installer leftovers
	if [regexp {\.(dpkg|rpm)} $configuration] {continue}

	Log "checking config: $configuration"
	if [MatchDevice $configuration] {
		set switch_config $configuration
		set devList1 [glob -nocomplain /dev/ttyUSB* /dev/ttyACM* /dev/ttyHS*]
		Log "! matched, now switching"
		set tc [open /tmp/gsmmodem_$dev_top w]
		close $tc

		# Now we are actually switching
		if $logging {
			Log " (running command: $bindir/usb_modeswitch -I -W -c $configuration)"
			set report [exec $bindir/usb_modeswitch -I -W -D -c $configuration 2>@ stdout]
		} else {
			set report [exec $bindir/usb_modeswitch -I -Q -D -c $configuration]
		}
		Log "\nverbose output of usb_modeswitch:"
		Log "--------------------------------"
		Log $report
		Log "--------------------------------"
		Log "(end of usb_modeswitch output)\n"
		break
	} else {
		Log "* no match, not switching with this config"
	}
}

# We're finished with switching; success checking
# was done by usb_modeswitch and logged via syslog.
#
# If switching was OK we now check for drivers by
# simply recounting serial devices under /dev

# If target ID given, driver shall be loaded
if [regexp -nocase {ok:[0-9a-f]{4}:[0-9a-f]{4}} $report] {

	# For general driver loading; TODO: add respective device names.
	# Presently only useful for HSO devices (which are recounted now)
	set driverModule ""
	set driverIDPath ""
	set rc [open $configuration r]
	set lineList [split [read $rc] \n]
	close $rc
	foreach line $lineList {
		regexp {DriverModule[[:blank:]]*=[[:blank:]]*"?(\w+)"?} $line d driverModule
		regexp {DriverIDPath[[:blank:]]*=[[:blank:]]*?"?([/\-\w]+)"?} $line d driverIDPath
	}
	if {$driverModule == ""} {
		set driverModule "option"
		set driverIDPath "/sys/bus/usb-serial/drivers/option1"
	} else {
		if {$driverIDPath == ""} {
			set driverIDPath "/sys/bus/usb/drivers/$driverModule"
		}
	}
	Log "Driver module is \"$driverModule\", ID path is $driverIDPath\n"

	# some settling time in ms
	after 500

	Log "Now checking for newly created serial devices ..."
	set devList2 [glob -nocomplain /dev/ttyUSB* /dev/ttyACM* /dev/ttyHS*]

	if {[llength $devList1] >= [llength $devList2]} {
		Log " no new serial devices found"

		if {![file isdirectory $devdir]} {
			Log "Device directory in sysfs is gone! Something went wrong, aborting"
			SafeExit
		}

		# Give the device annother second if it's not fully back yet
		if {![file exists $devdir/idProduct]} {
			after 1000
		}

		ReadUSBAttrs $devdir
		if {[string length "$usb(idVendor)$usb(idProduct)"] < 8} {
			regexp {ok:(\w{4}):(\w{4})} $report d usb(idVendor) usb(idProduct)
		}
		set t "$usb(idVendor)$usb(idProduct)"
		if {[string length $t] == 8 && [string trim $t 0] != ""} {
			set idfile $driverIDPath/new_id
			if {![file exists $idfile]} {
				Log "\nTrying to load driver \"$driverModule\""
				set loader /sbin/modprobe
				Log " loader is: $loader"
				if [file exists $loader] {
					if [catch {set result [exec $loader -v $driverModule]} err] {
						Log " Running \"$loader $driverModule\" gave an error:\n  $err"
					}
				} else {
					Log " /sbin/modprobe not found"
				}
			}
			if [file exists $idfile] {
				Log "Trying to add ID to driver \"$driverModule\""
				catch {exec logger -p syslog.notice "usb_modeswitch: adding device ID $usb(idVendor):$usb(idProduct) to driver \"$driverModule\""}
				catch {exec echo "$usb(idVendor) $usb(idProduct)" >$idfile}
				after 600
				set devList2 [glob -nocomplain /dev/ttyUSB* /dev/ttyACM* /dev/ttyHS*]
				if {[llength $devList1] >= [llength $devList2]} {
					Log " still no new serial devices found"
				} else {
					Log " driver successfully bound"
				}
			} else {
				Log " \"$idfile\" not found, can't add ID"
			}
		}
	} else {
		Log " new serial devices found, driver has bound"
	}
}



if [regexp {ok:$} $report] {
	Log "Doing no driver checking or binding for this device"
}

# In newer kernels there is a switch to avoid the use of a device
# reset (e.g. from usb-storage) which would likely switch back
# a mode-switching device
if [regexp {ok:} $report] {
	Log "Checking for AVOID_RESET_QUIRK attribute"
	if [file exists $devdir/avoid_reset_quirk] {
		if [catch {exec echo "1" >$devdir/avoid_reset_quirk} err] {
			Log " Error setting the attribute: $err"
		} else {
			Log " AVOID_RESET_QUIRK activated"
		}
	} else {
		Log " AVOID_RESET_QUIRK not present"
	}
}

Log "\nAll done, exiting\n"
SafeExit

}
# end of proc {Main}


proc {ReadSCSIAttrs} {dir} {

global scsi
Log "SCSI dir exists: $dir"

foreach attr {vendor model rev} {
	if [file exists $dir/$attr] {
		set rc [open $dir/$attr r]
		set scsi($attr) [read -nonewline $rc]
		close $rc
	} else {
		set scsi($attr) ""
		Log "Warning: SCSI attribute \"$attr\" not found."
	}
}

}
# end of proc {ReadSCSIAttrs}


proc {ReadUSBAttrs} {dir} {

global usb
Log "USB dir exists: $dir"

foreach attr {idVendor idProduct manufacturer product serial} {
	if [file exists $dir/$attr] {
		set rc [open $dir/$attr r]
		set usb($attr) [read -nonewline $rc]
		close $rc
	} else {
		set usb($attr) ""
		Log "Warning: USB attribute \"$attr\" not found."
	}
}

}
# end of proc {ReadUSBAttrs}


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
	set blankstring ""
	regsub -all {_} $matchstring { } blankstring
	Log "matching $match($id)"
	Log "  match string1: $matchstring"
	Log "  match string2: $blankstring"
	Log " device string: [set $match($id)]"
	if {!([string match *$matchstring* [set $match($id)]] || [string match *$blankstring* [set $match($id)]])} {
		return 0
	}
}
return 1

}
# end of proc {MatchDevice}


proc {ParseConfigFile} {} {

global logging noswitching

set configFile ""
set places [list /etc/usb_modeswitch.conf /etc/sysconfig/usb_modeswitch /etc/default/usb_modeswitch]
foreach cfg $places {
	if [file exists $cfg] {
		set configFile $cfg
		break
	}
}

if {$configFile == ""} {return}

set rc [open $configFile r]
while {![eof $rc]} {
	gets $rc line
	if [regexp {DisableSwitching\s*=\s*([^\s]+)} $line d val] {
		if [regexp -nocase {1|yes|true} $val] {
			set noswitching 1
		}
	}
	if [regexp {EnableLogging\s*=\s*([^\s]+)} $line d val] {
		if [regexp -nocase {1|yes|true} $val] {
			set logging 1
		}
	}

}
Log "Using global config file: $configFile"

}
# end of proc {ParseConfigFile}


proc {Log} {msg} {

global wc logging device
if {$logging == 0} {return}
if {![info exists wc]} {
	set wc [open /var/log/usb_modeswitch_$device a]
	puts $wc "\n\nUSB_ModeSwitch log from [clock format [clock seconds]]\n"
}
puts $wc $msg

}
# end of proc {Log}



# Checking for interrupt endpoint in ttyUSB port; if found,
# check for unused "gsmmodem[n]" name.
# First link will be "gsmmodem", then "gsmmodem2" and up

proc {SymLinkName} {path} {

# HACK ... /tmp/gsmmodem_* was generated by a switching run before;
# no way found to signal annother instance in the udev environment

set idx -1
set tmpNames [glob -nocomplain /tmp/gsmmodem_*]
foreach tmpName $tmpNames {
	set dev_top [lindex [split $tmpName _] 1]
	set dirList [split $path /]
	set idx [lsearch $dirList $dev_top]
	if {$idx == -1} {
		continue
	} else {break}
}
if {$idx == -1} {return ""}

regexp {ttyUSB\d+?} $path myPort

#Log "symlink: dev_top is $dev_top \npath is $path \nport is $myPort"

# Unfortunately, there are devices with more than one interrupt
# port. We have to check all ports and assume that the lowest
# number denotes the working port

set devDir /sys[join [lrange $dirList 0 $idx] /]
set portList {}
foreach ifDir [glob -nocomplain $devDir/$dev_top:\[0-9\].\[0-9\]] {
#	Log "ifDir is $ifDir"
	if {![regexp {ttyUSB\d+?} [glob -nocomplain $ifDir/*] port]} {continue}
	foreach epDir [glob -nocomplain $ifDir/ep_*] {
		if [file exists $epDir/type] {
			set rc [open $epDir/type r]
			set type [read $rc]
			close $rc
			if [regexp {Interrupt} $type] {
				lappend portList $port
			}
		}
	}
}
set lowestPort [lindex [lsort -increasing $portList] 0]
if {$lowestPort != $myPort} {
	return ""
}
eval file delete dummy $tmpName
cd /dev
set idx 2
set symlinkName "gsmmodem"
while {$idx < 256} {
	if {![file exists $symlinkName]} {
		break
	}
	set symlinkName gsmmodem$idx
	incr idx
}

return $symlinkName

}
# end of proc {SymLinkName}


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

