#!/bin/sh

echo "Info: Checking for prerequisites and creating folders..."

if [ -d /opt ]
then
    echo "Warning: Folder /opt exists!"
else
    mkdir /opt
fi
for folder in bin etc include lib sbin share tmp usr var
do
  if [ -d "/opt/$folder" ]
  then
    echo "Warning: Folder /opt/$folder exists!"
    echo "Warning: If something goes wrong please clean /opt folder and try again."
  else
    mkdir /opt/$folder
  fi
done
[ -d "/opt/lib/opkg" ] || mkdir -p /opt/lib/opkg
[ -d "/opt/var/lock" ] || mkdir -p /opt/var/lock
[ -d "/opt/var/log" ] || mkdir -p /opt/var/log
[ -d "/opt/var/run" ] || mkdir -p /opt/var/run

echo "Info: Opkg package manager deployment..."
cd /opt/bin
wget http://qnapware.zyxmon.org/binaries-armv7/installer/opkg
chmod +x /opt/bin/opkg
cd /opt/etc
wget http://qnapware.zyxmon.org/binaries-armv7/installer/opkg.conf
cd /opt/lib
wget http://qnapware.zyxmon.org/binaries-armv7/installer/ld-2.20.so
chmod +x ld-2.20.so
ln -s ld-2.20.so ld-linux.so.3
wget http://qnapware.zyxmon.org/binaries-armv7/installer/libc-2.20.so
ln -s libc-2.20.so libc.so.6

echo "Info: Basic packages installation..."
/opt/bin/opkg update
/opt/bin/opkg install glibc-opt
if [ ! -f /opt/usr/lib/locale/locale-archive ]
then
        wget http://qnapware.zyxmon.org/binaries-armv7/installer/locale-archive -O /opt/usr/lib/locale/locale-archive
fi

echo "Info: Congratulations!"
echo "Info: If there are no errors above then Entware.arm successfully initialized."
echo "Info: Add /opt/bin & /opt/sbin to your PATH variable"
echo "Info: Add '/opt/etc/init.d/rc.unslung start' to startup script for Entware.arm services to start"
echo "Info: Found a Bug? Please report at https://github.com/zyxmon/entware-arm/issues"
