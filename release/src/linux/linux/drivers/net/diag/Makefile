#  Copyright 2001, Cybertan Corporation
#  All Rights Reserved.
#  
#  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Cybertan Corporation;
#  the contents of this file may not be disclosed to third parties, copied or
#  duplicated in any form, in whole or in part, without the prior written
#  permission of Cybertan Corporation.
#
#
# $Id: Makefile,v 1.1 2003/07/09 14:09:58 honor Exp $
#

O_TARGET	:= diag.o

MAC_OBJS	:= diag_led.o

export-objs	:= 
obj-y		:= $(MAC_OBJS)
obj-m		:= $(O_TARGET)

SRCBASE		:= $(TOPDIR)/../..
EXTRA_CFLAGS	+= -I$(SRCBASE)/include

vpath %.c $(SRCBASE)/shared

include $(TOPDIR)/Rules.make

