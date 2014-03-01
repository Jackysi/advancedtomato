#! /bin/sh
#
# /etc/init.d/shutdown (and symlinked to reboot)
#

# This umount is wrong here, but one tester reported data loss without it.
# If your partition gets destroyed, try to uncomment the umount.

echo "Unmounting file systems"
umount -a

#Powerfail situation, kill power
if [ -f /etc/powerfail ]; then
    echo "APCUPSD to the Rescue!"
    echo
    /sbin/apccontrol killpower
    echo
fi

echo "Halting System"
exec halt >/dev/console 2>&1
