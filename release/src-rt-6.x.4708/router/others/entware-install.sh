#!/bin/sh

echo "Info: Checking for prerequisites and creating folders..."

if [ -d /opt ]
then
    echo "Warning: Folder /opt exists!"
else
    mkdir /opt
fi
for folder in bin etc/init.d lib/opkg sbin share tmp usr var/log var/lock var/run
do
  if [ -d "/opt/$folder" ]
  then
    echo "Warning: Folder /opt/$folder exists!"
    echo "Warning: If something goes wrong please clean /opt folder and try again."
  else
    mkdir -p /opt/$folder
  fi
done

echo "Info: Opkg package manager deployment..."
CURARCH="armv7"
DLOADER="ld-linux.so.3"
URL=http://pkg.entware.net/binaries/$CURARCH/installer
wget $URL/opkg -O /opt/bin/opkg
chmod +x /opt/bin/opkg
wget $URL/opkg.conf -O /opt/etc/opkg.conf
wget $URL/ld-2.23.so -O /opt/lib/ld-2.23.so
wget $URL/libc-2.23.so -O/opt/lib/libc-2.23.so
wget $URL/libgcc_s.so.1.2.23 -O /opt/lib/libgcc_s.so.1
cd /opt/lib
chmod +x ld-2.23.so
ln -s ld-2.23.so $DLOADER
ln -s libc-2.23.so libc.so.6

echo "Info: Basic packages installation..."
/opt/bin/opkg update
/opt/bin/opkg install entware-opt
if [ ! -f /opt/usr/lib/locale/locale-archive ]
then
        wget http://pkg.entware.net/binaries/other/locale-archive.2.23 -O /opt/usr/lib/locale/locale-archive
fi

echo "Info: Congratulations!"
echo "Info: If there are no errors above then Entware-ng was successfully initialized."
echo "Info: Add /opt/bin & /opt/sbin to your PATH variable"
echo "Info: Add '/opt/etc/init.d/rc.unslung start' to startup script for Entware-ng services to start"
echo "Info: Found a Bug? Please report at https://github.com/Entware-ng/Entware-ng/issues"
