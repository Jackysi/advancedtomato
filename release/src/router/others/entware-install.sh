#!/bin/sh

export PATH=/opt/sbin:/opt/bin:$PATH
unset LD_LIBRARY_PATH
unset LD_PRELOAD

URL=http://pkg.entware.net/binaries/mipsel/installer

echo "Info: Creating folders..."
for folder in bin etc/init.d lib/opkg sbin share tmp usr var/log var/lock var/run
do
  if [ -d "/opt/$folder" ]
  then
    echo "Warning: Folder /opt/$folder exists! If something goes wrong please clean /opt folder and try again."
  else
    mkdir -p /opt/$folder
  fi
done

dl () {
  # $1 - URL to download
  # $2 - place to store
  # $3 - 'x' if should be executable
  echo -n "Downloading $2... "
  wget -q $1 -O $2
  if [ $? -eq 0 ] ; then
    echo "success!"
  else
    echo "failed!"
    exit 1
  fi
  [ -z "$3" ] || chmod +x $2
}

echo "Info: Deploying opkg package manager..."
dl $URL/opkg /opt/bin/opkg x
dl $URL/opkg.conf /opt/etc/opkg.conf
dl $URL/profile /opt/etc/profile x
dl $URL/rc.func /opt/etc/init.d/rc.func
dl $URL/rc.unslung /opt/etc/init.d/rc.unslung x

echo "Info: Basic packages installation..."
opkg update
opkg install ldconfig findutils
ldconfig > /dev/null 2>&1
[ -f /etc/TZ ] && ln -sf /etc/TZ /opt/etc/TZ

cat << EOF

Congratulations! If there are no errors above then Entware-ng is successfully initialized.

Found a Bug? Please report at https://github.com/Entware-ng/Entware-ng/issues

Type 'opkg install <pkg_name>' to install necessary package.

EOF
