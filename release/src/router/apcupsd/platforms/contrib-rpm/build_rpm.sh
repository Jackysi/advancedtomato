#!/bin/bash

# shell script to build apcupsd rpm release
# copy this script into a working directory with the src rpm to build and execute
# 19 Aug 2006 D. Scott Barninger

# licensed under GPL-v2

# signing rpms
# Make sure you have a .rpmmacros file in your home directory containing the following:
#
# %_signature gpg
# %_gpg_name Your Name <your-email@site.org>
#
# the %_gpg_name information must match your key


# usage: ./build_rpm.sh

###########################################################################################
# script configuration section

VERSION=3.13.12
RELEASE=1

# build platform for spec
# set to one of rh7,rh8,rh9,fedora_core,rhel3,rhel4,rhel5,suse,mdk
PLATFORM=suse

# platform designator for file names
# for RedHat/Fedora set to one of rh7,rh8,rh9,fc1,fc3,fc4,fc5 OR
# for RHEL3/clones set to el3 OR
# for RHEL4/clones set to el4 OR
# for RHEL5/clones set to el5 OR
# for SuSE set to su90, su91, su92, su100 or su101 OR
# for Mandrake set to 101mdk or 20060mdk
FILENAME=su100

# enter your name and email address here
PACKAGER="Your Name <your-email@site.org>"

# enter the full path to your RPMS output directory
RPMDIR=/usr/src/packages/RPMS/i586

# enter your arch string here (i386, i586, i686, x86_64)
ARCH=i586

# if the src rpm is not in the current working directory enter the directory location
# with trailing slash where it is found.
SRPMDIR=

# set to 1 to sign packages, 0 not to sign if you want to sign on another machine.
SIGN=1

# set to 1 to build gapcmon package (requires Gtk2 >= 2.4) or 0 to not build
GAPCMON=1

# to override your language shell variable uncomment and edit this
# export LANG=en_US.UTF-8

# Make no changes below this point without consensus

############################################################################################

SRPM=${SRPMDIR}apcupsd-$VERSION-$RELEASE.src.rpm

echo Building packages for "$PLATFORM"...
sleep 2

if [ "$GAPCMON" = "1" ]; then
	rpmbuild --rebuild --define "build_${PLATFORM} 1" \
	--define "contrib_packager ${PACKAGER}" \
	--define "build_snmp 1" \
	--define "build_gapcmon 1" ${SRPM}
fi
if [ "$GAPCMON" = "0" ]; then
	rpmbuild --rebuild --define "build_${PLATFORM} 1" \
	--define "build_snmp 1" \
	--define "contrib_packager ${PACKAGER}" ${SRPM}
fi

# delete any debuginfo packages built
rm -f ${RPMDIR}/apcupsd*debug*

# copy files to cwd and rename files to final upload names

mv -f ${RPMDIR}/apcupsd-${VERSION}-${RELEASE}.${ARCH}.rpm \
./apcupsd-${VERSION}-${RELEASE}.${FILENAME}.${ARCH}.rpm

mv -f ${RPMDIR}/apcupsd-multimon-${VERSION}-${RELEASE}.${ARCH}.rpm \
./apcupsd-multimon-${VERSION}-${RELEASE}.${FILENAME}.${ARCH}.rpm

mv -f ${RPMDIR}/apcupsd-gapcmon-${VERSION}-${RELEASE}.${ARCH}.rpm \
./apcupsd-gapcmon-${VERSION}-${RELEASE}.${FILENAME}.${ARCH}.rpm

# now sign the packages
if [ "$SIGN" = "1" ]; then
	echo Ready to sign packages...;
	sleep 2;
	rpm --addsign ./*.rpm;
fi

echo
echo Finished.
echo
ls

# changelog
# 19 Aug 2006 initial release
# 21 Jan 2007 add new gapcmon build switch
# 27 Jan 2007 add new snmp build switch
# 27 May 2007 add rhel5


