#!/bin/bash
#
# Miscellaneous steps to prepare the root filesystem
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: rootprep.sh,v 1.1.1.7 2005/03/07 07:31:19 kanki Exp $
#

ROOTDIR=$PWD

# tmp
mkdir -p tmp
ln -sf tmp/var var
(cd $ROOTDIR/usr && ln -sf ../tmp)

# dev
mkdir -p dev

# etc
mkdir -p etc
echo "/lib" > etc/ld.so.conf
echo "/usr/lib" >> etc/ld.so.conf
/sbin/ldconfig -r $ROOTDIR

# miscellaneous
mkdir -p mnt
mkdir -p proc
