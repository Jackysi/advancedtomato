#!/usr/bin/perl
#
#	uversion.pl
#	Copyright (C) 2006 Jonathan Zarate
#
#	- update the build number for Tomato
#

use POSIX qw(strftime);

sub error
{
	print "\nuversion error: $@\n";
	exit(1);
}

sub help
{
	print "Usage: uversion --bump|--gen\n";
	exit(1);
}

#
#

if ($#ARGV != 0) {
	help();
}

$path = "router/shared";
$major = 0;
$minor = 0;
$vpn = 0;
$build = 0;

open(F, "$path/tomato_version") || error("opening tomato_version: $!");
$_ = <F>;
if (!(($major, $minor, $vpnmajor, $vpnminor) = /^(\d+)\.(\d+)vpn(\d+)\.(\d+)$/)) {
	error("Invalid version: '$_'");
}
close(F);


if ($ARGV[0] ne "--gen") {
	help();
}

$time = strftime("%a, %d %b %Y %H:%M:%S %z", localtime());
$minor = sprintf("%02d", $minor);
$build = sprintf("%x", time());

$fullversion = "$major.$minor\vpn$vpnmajor.$vpnminor.$build(ND)";

open(F, ">$path/tomato_version.h~") || error("creating temp file: $!");
print F <<"END";
#ifndef __TOMATO_VERSION_H__
#define __TOMATO_VERSION_H__
#define TOMATO_MAJOR		"$major"
#define TOMATO_MINOR		"$minor"
#define TOMATO_VPN          "$vpn"
#define TOMATO_BUILD		"$build"
#define	TOMATO_BUILDTIME	"$time"
#define TOMATO_VERSION		"$fullversion"
#endif
END
close(F);
rename("$path/tomato_version.h~", "$path/tomato_version.h") || error("renaming: $!");

open(F, ">build_version~") || error("creating temp file: $!");
print F "$fullversion";
close(F);
rename("build_version~", "build_version") || error("renaming: $!");

print "Version: $fullversion ($time)\n";
exit(0);
