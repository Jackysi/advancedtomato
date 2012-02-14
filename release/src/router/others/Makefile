include ../common.mak

all:

install:
	install -d $(INSTALLDIR)/sbin
	install -d $(INSTALLDIR)/usr/sbin
	install -d $(INSTALLDIR)/lib

ifeq ($(TCONFIG_USB),y)
# Optware perl symlink
	install -d $(INSTALLDIR)/usr/bin
	ln -sf /opt/bin/perl $(INSTALLDIR)/usr/bin/perl

ifeq ($(CONFIG_LINUX26),y)
#3G modem switch script
	install -m 0700 switch3g $(INSTALLDIR)/usr/sbin
endif
endif

# cron helper
	install -m 0700 cru $(INSTALLDIR)/usr/sbin

# playing with ipkg / from OpenWrt svn
#!!TB	install -m 0700 ipkg $(INSTALLDIR)/usr/sbin

# system info
	install -m 0700 sysinfo $(INSTALLDIR)/usr/sbin

# nas starter # AB multiSSID
	install -m 0700 nas.sh $(INSTALLDIR)/usr/sbin

# keep winscp happy
#	install -m 0700 groups $(INSTALLDIR)/usr/bin

clean:
