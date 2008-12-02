#!/bin/sh
#
# Copyright 2005, Broadcom Corporation      
# All Rights Reserved.      
#       
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY      
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM      
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS      
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.      
#
# $Id: shared_ksyms.sh,v 1.1.1.7 2005/03/07 07:30:48 kanki Exp $
#

cat <<EOF
#include <linux/config.h>
#include <linux/module.h>
EOF

for file in $* ; do
    ${NM} $file | sed -ne 's/[0-9A-Fa-f]* [DT] \([^ ]*\)/extern void \1; EXPORT_SYMBOL(\1);/p'
done
