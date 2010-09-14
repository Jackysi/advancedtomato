#!/bin/sh
(
count=120
while [ $count != 0 ]; do
	if [ ! -e "/usr/bin/tclsh" ]; then
		sleep 1
		count=$(($count - 1))
	else
		exec /usr/bin/tclsh /usr/sbin/usb_modeswitch_dispatcher "$@" 2>/dev/null &
		exit 0
	fi
done
) &
