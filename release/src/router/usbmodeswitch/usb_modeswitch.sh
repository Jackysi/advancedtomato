#!/bin/sh
(
device_in()
{
	if [ ! -e /etc/usb_modeswitch.d/$1 ]; then
		return 0
	fi
	if [ -z "$2$3" ]; then
		return 1
	fi
	while read line
	do
		if [ `expr "$line" : ".*$2:$3.*"` ]; then
#		if [ "$line" != "${line/$2:$3/}" ]; then
			return 1
		fi
	done </etc/usb_modeswitch.d/$1
	if [ `expr "$line" : ".*$2:$3.*"` ]; then
#	if [ "$line" != "${line/$2:$3/}" ]; then
		return 1
	fi
	return 0
}
need_run=1
case "$1" in
	--driver-bind)
		device_in "bind_list" $3 $4
		need_run=$?
		;;
	--symlink-name)
		device_in "link_list" $3 $4
		need_run=$?
		;;
esac
if [ $need_run != 1 ]; then
	exit 0
fi
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
