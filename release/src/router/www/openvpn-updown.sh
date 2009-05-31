#!/bin/sh
filedir=/etc/openvpn/dns
filename=$filedir/$dev.resolv
fileexists=`if [ -f $filename ]; then echo 1; else echo 0; fi`
if [ ! -d $filedir ]; then mkdir /etc/openvpn/dns; fi
if [ -f $filename ]; then rm $filename; fi

if [ $script_type == 'up' ]
then
	for optionname in `set | grep "^foreign_option_" | sed "s/^\(.*\)=.*$/\1/g"`
	do
		eval "echo \$$optionname" | sed "s/dhcp-option DNS/nameserver/;s/dhcp-option DOMAIN/search/" >> $filename
	done
fi

if [[ -f $filename || $fileexists ]]; then service dnsmasq restart; fi
rmdir /etc/openvpn/dns

exit 0

