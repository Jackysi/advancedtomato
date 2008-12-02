#
# Router rc control scripts Makefile
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
#
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.37.18.1.2.2 2006/07/19 07:19:13 honor Exp $
#

include $(TOP)/.config

ifneq ($(wildcard $(SRCBASE)/cy_conf.mak),)
  include $(SRCBASE)/cy_conf.mak
endif

CFLAGS	+= -I. -I$(TOP)/shared -I$(TOP)/busybox -I$(SRCBASE)/include -Wall -I$(SRCBASE)/
CFLAGS  += -I$(TOP)/iptables/include -I$(TOP)/iptables/include/libipq -I$(TOP)/libnet/include -DL_ENDIAN -DLIBNET_LIL_ENDIAN -DLIB1X_LIL_ENDIAN
CFLAGS	+= -s -O2
LDFLAGS	+= -L$(TOP)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared -L$(SRCBASE)/router/libnet/lib -lnet

OBJS := rc.o init.o interface.o network.o services.o udhcpc.o http.o stats.o crc.o mtd.o
OBJS += writeimage.o resetbutton.o ntp.o gpio.o iptable.o ddns.o ddns_checkip.o listen.o check_ps.o process_monitor.o ctmisc.o misc.o sendudp.o

OBJS += heartbeat.o
OBJS += eou.o
LDFLAGS += -L$(TOP)/openssl -lcrypto
CFLAGS  += -I$(TOP)/openssl -I$(TOP)/openssl/include
ifeq ($(CONFIG_NAT),y)
OBJS += firewall.o
#LDFLAGS += -L$(TOP)/netconf -L$(INSTALLDIR)/netconf/usr/lib -lnetconf
endif
OBJS += voip_qos.o
CFLAGS += -I$(TOP)/libnet/include -DL_ENDIAN -DLIBNET_LIL_ENDIAN -DLIB1X_LIL_ENDIAN
LDFLAGS += -L$(SRCBASE)/router/libnet/lib -lnet
OBJS += sendudp.o

ifeq ($(CONFIG_NAT),y)
OBJS += ppp.o
endif

vpath %.c $(TOP)/shared $(SRCBASE)/rts/src

all: rc

clean:
	rm -f *.o rc

install: all
	install -d $(INSTALLDIR)/sbin
	install rc $(INSTALLDIR)/sbin
	$(STRIP) $(INSTALLDIR)/sbin/rc
	cd $(INSTALLDIR)/sbin && ln -sf rc init
	cd $(INSTALLDIR)/sbin && ln -sf rc erase
	cd $(INSTALLDIR)/sbin && ln -sf rc write
	cd $(INSTALLDIR)/sbin && ln -sf rc restore
	cd $(INSTALLDIR)/sbin && ln -sf rc stats
	cd $(INSTALLDIR)/sbin && ln -sf rc hotplug

	cd $(INSTALLDIR)/sbin && ln -sf rc filter
	cd $(INSTALLDIR)/sbin && ln -sf rc resetbutton
	cd $(INSTALLDIR)/sbin && ln -sf rc filtersync
	cd $(INSTALLDIR)/sbin && ln -sf rc ntpd
	cd $(INSTALLDIR)/sbin && ln -sf rc ipupdated
	cd $(INSTALLDIR)/sbin && ln -sf rc redial
	cd $(INSTALLDIR)/sbin && ln -sf rc hb_connect
	cd $(INSTALLDIR)/sbin && ln -sf rc hb_disconnect
	cd $(INSTALLDIR)/sbin && ln -sf rc disconnected_pppoe
	cd $(INSTALLDIR)/sbin && ln -sf rc gpio
	cd $(INSTALLDIR)/sbin && ln -sf rc listen
	cd $(INSTALLDIR)/sbin && ln -sf rc check_ps
	cd $(INSTALLDIR)/sbin && ln -sf rc ddns_success
	cd $(INSTALLDIR)/sbin && ln -sf rc process_monitor
	cd $(INSTALLDIR)/sbin && ln -sf rc eou_status
	cd $(INSTALLDIR)/sbin && ln -sf rc qos
	cd $(INSTALLDIR)/sbin && ln -sf rc ppp_event
	cd $(INSTALLDIR)/sbin && ln -sf rc misc	
	cd $(INSTALLDIR)/sbin && ln -sf rc detectwan
	cd $(INSTALLDIR)/sbin && ln -sf rc sendudp
	cd $(INSTALLDIR)/sbin && ln -sf rc check_ses_led
	cd $(INSTALLDIR)/sbin && ln -sf rc ses_led
	cd $(INSTALLDIR)/sbin && ln -sf rc ddns_checkip

rc: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

services.o: $(TOP)/shared/build_date.h

$(OBJS): $(CY_DEPS)

%.o: .%.depend
.%.depend: %.c
	$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
