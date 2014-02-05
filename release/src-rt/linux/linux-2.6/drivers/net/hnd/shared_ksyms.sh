#!/bin/sh
#
# Copyright (C) 2009, Broadcom Corporation      
# All Rights Reserved.      
#       
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY      
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM      
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS      
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.      
#
# $Id: shared_ksyms.sh,v 1.2 2008/12/05 20:10:41 Exp $
#

cat <<EOF
#include <linux/config.h>
#include <linux/module.h>
EOF

for file in $* ; do
    ${NM} $file | sed -ne 's/[0-9A-Fa-f]* [BDRT] \([^ ]*\)/extern void \1; EXPORT_SYMBOL(\1);/p'
done
