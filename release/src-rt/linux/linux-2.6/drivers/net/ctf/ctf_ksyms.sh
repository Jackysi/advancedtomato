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
# $Id: ctf_ksyms.sh,v 1.1 2009/10/27 01:27:44 Exp $
#

cat <<EOF
#include <ctf/hndctf.h>
EOF

for file in $* ; do
    ${NM} $file | sed -ne 's/[0-9A-Fa-f]* [DRT] \([^ ]*\)/EXPORT_SYMBOL(\1);/p'
done
