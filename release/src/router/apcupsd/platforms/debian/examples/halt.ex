#! /bin/sh
#
# halt		Execute the halt command.
#
# Version:      @(#)halt  2.75  19-May-1998  miquels@cistron.nl
#

PATH=/sbin:/bin:/usr/sbin:/usr/bin

# See if we need to cut the power.
if [ -x /etc/init.d/ups-monitor ]
then
	/etc/init.d/ups-monitor poweroff
fi

halt -d -f -i -p
