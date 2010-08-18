#!/bin/sh
# next lines for bash, ignored by tclsh, restarting in background \
( \
count=120; \
while [ $count != 0 ]; do \
	if [ ! -e "/usr/bin/tclsh" ]; then \
		sleep 1; \
		count=$(($count - 1)); \
	else \
		exec /usr/bin/tclsh "$0" "$@" 2>/dev/null & \
		exit 0; \
	fi; \
done; \
) & \
exit 0


# Wrapper (tcl) for usb_modeswitch, called from
# /lib/udev/rules.d/40-usb_modeswitch.rules
# (part of data pack "usb-modeswitch-data") via
# /lib/udev/usb_modeswitch
#
# Does ID check on hotplugged USB devices and calls the
# mode switching program with the matching parameter file
# from /etc/usb_modeswitch.d
#
# Part of usb-modeswitch-1.1.4 package
# (C) Josua Dietze 2009, 2010


# Setting of the these switches is done in the global config
# file (/etc/usb_modeswitch.conf)

set logging 0
set noswitching 0



set env(PATH) "/bin:/usr/bin"

# Execution starts at file bottom

proc {Main} {argc argv} {

global scsi usb config match wc device logging noswitching settings

# The facility to add a symbolic link pointing to the
# ttyUSB port which provides interrupt transfer, i.e.
# the port to connect through; returns a symlink name
# for udev and exits
# This is run once for every device interface by an
# udev rule

if {[lindex $argv 0] == "--symlink-name"} {
#	set device [clock clicks]
	puts [SymLinkName [lindex $argv 1]]
	SafeExit
}

# The facility to bind the driver on-the-fly after a warm
# boot; the device is still in modem mode but if the
# driver was bound by the switching script before (ID
# not yet added to the driver), the device needs to be
# bound again

if {[lindex $argv 0] == "--driver-bind"} {
	CheckDriverBind [lindex $argv 1] [lindex $argv 2] [lindex $argv 3] [lindex $argv 4]
	SafeExit
}

set settings(dbdir)	/etc/usb_modeswitch.d
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

ParseGlobalConfig

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
	catch {exec logger -p syslog.notice "usb_modeswitch: switching disabled, no action for $usb(idVendor):$usb(idProduct)" 2>/dev/null}
	SafeExit
}

# Check if there is more than one config file for this USB ID,
# which would point to a possible ambiguity. If so, check if
# SCSI values are needed

set configList [ConfigGet list $usb(idVendor):$usb(idProduct)]

if {[llength $configList] == 0} {
	Log "Aargh! Config file missing for $usb(idVendor):$usb(idProduct)! Exiting"
	SafeExit
}

set scsiNeeded 0
if {[llength $configList] > 1} {
	if [regexp {:s} $configList] {
		set scsiNeeded 1
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
	set testSCSI [exec $bindir/usb_modeswitch -v 0x$usb(idVendor) -p 0x$usb(idProduct) 2>/dev/null]
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
#set configList [glob -nocomplain $settings(dbdir)/$usb(idVendor):$usb(idProduct)*]
foreach configuration [lsort -decreasing $configList] {

	# skipping installer leftovers
	if [regexp {\.(dpkg|rpm)} $configuration] {continue}

	Log "checking config: $configuration"
	if [MatchDevice $configuration] {
		ParseDeviceConfig [ConfigGet config $configuration]
		set devList1 [glob -nocomplain /dev/ttyUSB* /dev/ttyACM* /dev/ttyHS*]
		if {$config(waitBefore) == ""} {
			Log "! matched, now switching"
		} else {
			Log "! matched, waiting time set to $config(waitBefore) seconds"
			after [expr $config(waitBefore) * 1000]
			Log " waiting is over, switching starts now"
		}

		# Now we are actually switching
		if $logging {
			Log " (running command: $bindir/usb_modeswitch -I -W -c $settings(tmpConfig))"
			set report [exec $bindir/usb_modeswitch -I -W -D -c $settings(tmpConfig) 2>@ stdout]
		} else {
			set report [exec $bindir/usb_modeswitch -I -Q -D -c $settings(tmpConfig) 2>/dev/null]
		}
		Log "\nverbose output of usb_modeswitch:"
		Log "--------------------------------"
		Log $report
		Log "--------------------------------"
		Log "(end of usb_modeswitch output)\n"
		if [regexp {/tmp/} $settings(tmpConfig)] {
			file delete  $settings(tmpConfig)
		}
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
	if {$config(driverModule) == ""} {
		set config(driverModule) "option"
		set config(driverIDPath) "/sys/bus/usb-serial/drivers/option1"
	} else {
		if {$config(driverIDPath) == ""} {
			set config(driverIDPath) "/sys/bus/usb/drivers/$config(driverModule)"
		}
	}
	Log "Driver module is \"$config(driverModule)\", ID path is $config(driverIDPath)\n"

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
			set idfile $config(driverIDPath)/new_id
			if {![file exists $idfile]} {
				Log "\nTrying to load driver \"$config(driverModule)\""
				set loader /sbin/modprobe
				Log " loader is: $loader"
				if [file exists $loader] {
					if [catch {set result [exec $loader -v $config(driverModule) 2>/dev/null]} err] {
						Log " Running \"$loader $config(driverModule)\" gave an error:\n  $err"
					}
				} else {
					Log " /sbin/modprobe not found"
				}
			}
			if [file exists $idfile] {
				Log "Trying to add ID to driver \"$config(driverModule)\""
				catch {exec logger -p syslog.notice "usb_modeswitch: adding device ID $usb(idVendor):$usb(idProduct) to driver \"$config(driverModule)\"" 2>/dev/null}
				catch {exec echo "$usb(idVendor) $usb(idProduct)" >$idfile 2>/dev/null}
				after 600
				set devList2 [glob -nocomplain /dev/ttyUSB* /dev/ttyACM* /dev/ttyHS*]
				if {[llength $devList1] >= [llength $devList2]} {
					Log " still no new serial devices found"
				} else {
					AddLastSeen "$usb(idVendor):$usb(idProduct)"
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
		if [catch {exec echo "1" >$devdir/avoid_reset_quirk 2>/dev/null} err] {
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


proc {ParseGlobalConfig} {} {

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
# end of proc {ParseGlobalConfig}


proc ParseDeviceConfig {configFile} {

global config
set config(driverModule) ""
set config(driverIDPath) ""
set config(waitBefore) ""
set rc [open $configFile r]
set lineList [split [read $rc] \n]
close $rc
foreach line $lineList {
	regexp {DriverModule[[:blank:]]*=[[:blank:]]*"?(\w+)"?} $line d config(driverModule)
	regexp {DriverIDPath[[:blank:]]*=[[:blank:]]*?"?([/\-\w]+)"?} $line d config(driverIDPath)
	regexp {WaitBefore[[:blank:]]*=[[:blank:]]*?(\d+)} $line d config(waitBefore)
}
set config(waitBefore) [string trimleft $config(waitBefore) 0]

}
# end of proc {ParseDeviceConfig}


proc {ConfigGet} {command config} {

global settings

switch $command {

	list {
		if [file exists $settings(dbdir)/configPack.tar.gz] {
			Log "Found packed config collection $settings(dbdir)/configPack.tar.gz"
			if [catch {set configList [exec tar -tzf $settings(dbdir)/configPack.tar.gz 2>/dev/null]} err] {
				Log "Error: problem opening config package; tar returned\n $err"
				return {}
			}
			set configList [split $configList \n]
			set configList [lsearch -all -inline $configList $config*]
		} else {
			set configList [glob -nocomplain $settings(dbdir)/$config*]
		}

		return $configList
	}
	config {
		if [file exists $settings(dbdir)/configPack.tar.gz] {
			set settings(tmpConfig) /tmp/usb_modeswitch.current_cfg
			Log "Extracting config $config from collection $settings(dbdir)/configPack.tar.gz"
			set wc [open $settings(tmpConfig) w]
			puts -nonewline $wc [exec tar -xzOf $settings(dbdir)/configPack.tar.gz $config 2>/dev/null]
			close $wc
		} else {
			set settings(tmpConfig) $config
		}
		return $settings(tmpConfig)
	}
}

}
# end of proc {ConfigGet}

proc {Log} {msg} {

global wc logging device
if {$logging == 0} {return}
if {![info exists wc]} {
	if [catch {set wc [open /var/log/usb_modeswitch_$device a]} err] {
		set wc "error"
		puts "Error: Can't write to log file, $err"
		return
	}
	puts $wc "\n\nUSB_ModeSwitch log from [clock format [clock seconds]]\n"
}
if {$wc == "error"} {return}
puts $wc $msg

}
# end of proc {Log}


# Closing the log file if open and exit
proc {SafeExit} {} {

global wc
if [info exists wc] {
	catch {close $wc}
}
exit

}
# end of proc {SafeExit}


# Checking for interrupt endpoint in ttyUSB port (lowest if there is
# more than one); if found, check for unused "gsmmodem[n]" name.
# Link for first modem will be "gsmmodem", then "gsmmodem2" and up

proc {SymLinkName} {path} {

# Internal proc, used only here
proc {hasInterrupt} {ifDir} {
	if {[llength [glob -nocomplain $ifDir/ttyUSB*]] == 0} {return 0}
	foreach epDir [glob -nocomplain $ifDir/ep_*] {
		if [file exists $epDir/type] {
			set rc [open $epDir/type r]
			set type [read $rc]
			close $rc
			if [regexp {Interrupt} $type] {
				return 1
			}
		}
	}
	return 0
}

# In case the device path is returned as /class/tty/ttyUSB,
# we need to extract the USB device path from symlink "device"
set linkpath /sys$path/device
if [file exists $linkpath] {
	if {[file type $linkpath] == "link"} {
		set rawpath [file link $linkpath]
		set trimpath [regsub -all {\.\./} $rawpath {}]
		if [file isdirectory /sys/$trimpath] {
			set path /$trimpath
		}
	}
}

if {![regexp {ttyUSB\d+?} $path myPort]} {
	return ""
}
if {![regexp "\\d+\\.(\\d+)/$myPort" $path d myIf]} {
	return ""
}
if {![regexp {usb\d*/(\d+-\d+)/} $path d dev_top]} {
	return ""
}

set dirList [split $path /]
set idx [lsearch $dirList $dev_top]
set devDir /sys[join [lrange $dirList 0 $idx] /]

if {![regexp "$devDir/$dev_top:\[0-9\]" /sys$path ifRoot]} {
	return ""
}
set ifDir $ifRoot.$myIf

set rightPort 0
if [hasInterrupt $ifDir] {
	set rightPort 1
}

# Unfortunately, there are devices with more than one interrupt
# port. The assumption so far is that the lowest of these is
# right. Check all lower interfaces for annother one (if interface)
# is bigger than 0). If found, don't return any name.
if { $rightPort && ($myIf > 0) } {
	for {set i 0} {$i < $myIf} {incr i} {
		set ifDir $ifRoot.$i
		if [hasInterrupt $ifDir] {
			set rightPort 0
			break
		}
	}
}
if {$rightPort == 0} {
	return ""
}

# Use first free "gsmmodem[n]" name
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


# Add serial driver after warm boot
proc {CheckDriverBind} {path vid pid prod} {

if {$vid == ""} {set vid $prod}
if [regexp {/} $vid] {
	set id_list [split $vid /]
	set vid [format %04s [lindex $id_list 0]]
	set pid [format %04s [lindex $id_list 1]]
}

set dirList [glob -nocomplain /sys$path/*]
if [string match *ttyUSB* $dirList] {return}

if {[WasLastSeen $vid:$pid] == 0} {return}

set config(driverModule) "option"
set config(driverIDPath) "/sys/bus/usb-serial/drivers/option1"
set idfile $config(driverIDPath)/new_id
if {![file exists $idfile]} {
	set loader /sbin/modprobe
	if {![file exists $loader]} {return}
	if [catch {exec $loader $config(driverModule) 2>/dev/null}] {return}
	set i 0
	while {$i < 50} {
		if [file exists $idfile] {
			break
		}
		after 20
		incr i
	}
	if {$i == 50} {return}
}
catch {exec echo "$vid $pid" >$idfile 2>/dev/null}

}
# end of proc {CheckDriverBind}


# Add USB ID to list of devices needing driver binding
proc {AddLastSeen} {id} {

set lastseen /etc/usb_modeswitch.d/last_seen
if [file exists $lastseen] {
	set rc [open $lastseen r]
	set buffer [read $rc]
	close $rc
	if [string match *$id* $buffer] {
		return
	}
	set idList [split [string trim $buffer] \n]
}
lappend idList $id
set buffer [join $idList "\n"]
if [catch {set wc [open $lastseen w]}] {return}
puts -nonewline $wc $buffer
close $wc

}
# end of proc {AddLastSeen}


# Check if USB ID is listed as needing driver binding
proc {WasLastSeen} {id} {

set lastseen /etc/usb_modeswitch.d/last_seen
if {![file exists $lastseen]} {return 0}
set rc [open $lastseen r]
set buffer [read $rc]
close $rc
if [string match *$id* $buffer] {
	return 1
} else {
	return 0
}

}
# end of proc {WasLastSeen}


# The actual entry point
Main $argc $argv

