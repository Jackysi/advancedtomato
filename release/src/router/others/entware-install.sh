#!/bin/sh

echo Info: Checking for prerequisites and creating folders...
for folder in bin etc include lib sbin share tmp usr var
do
  if [ -d "/opt/$folder" ]
  then
    echo Warning: Folder /opt/$folder exists!
    echo Warning: If something goes wrong please clean /opt folder and try again.
  else
    mkdir /opt/$folder
  fi
done
[ -d "/opt/lib/opkg" ] || mkdir -p /opt/lib/opkg
[ -d "/opt/var/lock" ] || mkdir -p /opt/var/lock
[ -d "/opt/var/log" ] || mkdir -p /opt/var/log
[ -d "/opt/var/run" ] || mkdir -p /opt/var/run

echo Info: Opkg package manager deployment...
cd /opt/bin
wget http://wl500g-repo.googlecode.com/svn/ipkg/opkg
chmod +x /opt/bin/opkg
cd /opt/etc
wget http://wl500g-repo.googlecode.com/files/opkg.conf

echo Info: Basic packages installation...
/opt/bin/opkg update
/opt/bin/opkg install uclibc-opt

echo Info: Cleanup...
if [ -e "/opt/entware_install.sh" ]
then
  rm -f /opt/entware_install.sh
fi

echo Info: Congratulations!
echo Info: If there are no errors above then Entware successfully initialized.
echo Info: Found a Bug? Please report at wl500g-repo.googlecode.com
