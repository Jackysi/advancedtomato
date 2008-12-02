#
#       Copyright (C) 1997 Claus Heine.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Source: /home/cvsroot/wrt54g/src/linux/linux/drivers/char/ftape/Makefile,v $
# $Revision: 1.1.1.2 $
# $Date: 2003/10/14 08:08:06 $
#
#      Makefile for the QIC-40/80/3010/3020 floppy-tape driver for
#      Linux.
#

O_TARGET := ftape.o

subdir-$(CONFIG_FTAPE) += lowlevel
subdir-$(CONFIG_ZFTAPE) += zftape
subdir-$(CONFIG_ZFT_COMPRESSOR) += compressor

ifeq ($(CONFIG_FTAPE),y)
  obj-y    += lowlevel/ftape.o
endif

ifeq ($(CONFIG_ZFTAPE),y)
  obj-y   += zftape/zftape.o
endif

ifeq ($(CONFIG_ZFT_COMPRESSOR),y)
  obj-y += compressor/zft-compressor.o
endif

include $(TOPDIR)/Rules.make
