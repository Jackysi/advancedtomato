#!/usr/bin/env tclsh
#
# Usage: gen-rules.tcl [--set-version <date>]
#
# <date> should be in the form YYYYMMDD
#
# Config files are expected to be in subfolder "usb_modeswitch.d"
#
# A config file is expected to have one comment line containing
# a model name or other concise device specifications


# Default version string
set version "20170205"

# Devices excluded from Huawei catch-all rule
set x_huaweiList {12d1:1573 12d1:15c1}

if {[lindex $argv 0] == "--set-version" && [regexp {\d\d\d\d\d\d\d\d} [lindex $argv 1]]} {
	set version [lindex $argv 1]
}

set template {ATTR{idVendor}=="+##+", ATTR{idProduct}=="#++#", RUN+="usb_modeswitch '/%k'"}

if {![file isdirectory usb_modeswitch.d]} {
	puts "No \"usb_modeswitch.d\" subfolder found"
	exit
}

set filelist [lsort [glob -nocomplain ./usb_modeswitch.d/*]]
if {[llength $filelist] == 0} {
	puts "The \"usb_modeswitch.d\" subfolder is empty"
	exit
}

set wc [open "40-usb_modeswitch.rules" w]

# Writing file header with given version

puts -nonewline $wc {# Part of usb-modeswitch-data, version }
puts $wc $version
puts $wc {#
# Works with usb_modeswitch versions >= 2.4.0. Slash before %k parameter
# is for compatibility only. Versions >= 2.5.0 don't need it.
#
ACTION!="add|change", GOTO="modeswitch_rules_end"

# Adds a symlink "gsmmodem[n]" to the lowest ttyUSB port with interrupt
# transfer; checked against a list of known modems, or else no action
KERNEL=="ttyUSB*", ATTRS{bNumConfigurations}=="*", PROGRAM="usb_modeswitch --symlink-name %p %s{idVendor} %s{idProduct} %E{PRODUCT}", SYMLINK+="%c"

SUBSYSTEM!="usb", ACTION!="add",, GOTO="modeswitch_rules_end"

# Generic entry for most Huawei devices, excluding Android phones
ATTRS{idVendor}=="12d1", ATTRS{manufacturer}!="Android", ATTR{bInterfaceNumber}=="00", ATTR{bInterfaceClass}=="08", RUN+="usb_modeswitch '/%k'"}

set vendorList ""
set dvid ""

foreach idfile $filelist {
	if {![regexp -nocase {./([0-9A-F]{4}:[0-9A-F]{4})} $idfile d id]} {continue}
	if {[regexp -nocase {^12d1:} $id] && [lsearch $x_huaweiList $id] == -1} {continue}
	if [info exists entry($id)] {
		append entry($id) ", "
	}
	set rc [open $idfile r]
	set buffer [read $rc]
	close $rc
	foreach line [split $buffer \n] {
		set comment {}
		regexp {# (.*)} $line d comment
		if {[string length $comment] > 0} {
			append entry($id) [string trim $comment]
			break
		}
	}
}
foreach id_entry [lsort [array names entry]] {
	set id [split $id_entry :]
	set rule [regsub {\+##\+} $template [lindex $id 0]]
	set rule [regsub {#\+\+#} $rule [lindex $id 1]]
	puts $wc "\n# $entry($id_entry)\n$rule"
}

puts $wc {
LABEL="modeswitch_rules_end"}
close $wc
