#!/bin/sh
#
# Simple script for regenerating the autoconf and autoheader files.
#

autoconf --localdir=autoconf autoconf/configure.in > configure
chmod 755 configure
autoheader --localdir=autoconf autoconf/configure.in > autoconf/config.h.in
./configure --enable-powerflute --enable-cgi --enable-nls --with-included-gettext --with-catgets --with-libwrap=yes --enable-usb --enable-snmp --enable-net --enable-test --enable-pthreads --enable-oldnet

if [ "$1" != "config" ]
then
	make
	if [ "$1" != "compile" ]
	then
		make distclean
	fi
fi
