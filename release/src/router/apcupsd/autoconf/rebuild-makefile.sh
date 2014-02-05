#!/bin/sh
#
# It takes as argument the absolute source directory.
# Rebuilds the Makefile in CWD.
#
# $1 = abssrcdir
CURDIR="`pwd`"
if test "$CURDIR" = "$1"
then
    RELPATH="."
else
    RELPATH="`echo $CURDIR | sed -e "s|$1/||g"`"
fi

export SINGLE_MAKEFILE=yes
export CONFIG_FILES=$RELPATH/Makefile
export CONFIG_HEADERS=
cd $1
./config.status
