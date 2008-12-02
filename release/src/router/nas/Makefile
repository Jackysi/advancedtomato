#
# 802.1x Linux Builds Makefile
#
# Copyright (C) 2002 Broadcom Corporation
#
# $Id: Makefile,v 1.1.3.1 2004/12/29 07:07:42 honor Exp $
#
# targets/builds
all:

# install to target
install: all
	install -D nas $(INSTALLDIR)/usr/sbin/nas
	$(STRIP) $(INSTALLDIR)/usr/sbin/nas
	cd $(INSTALLDIR)/usr/sbin && ln -sf nas nas4not

clean:

