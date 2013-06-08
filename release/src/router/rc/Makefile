include ../common.mak

CFLAGS = -Wall -Os $(EXTRACFLAGS)
CFLAGS += -I. -I$(TOP)/shared -I$(SRCBASE)/include -idirafter$(LINUXDIR)/include
CFLAGS += -ffunction-sections -fdata-sections

LDFLAGS = -L$(TOP)/nvram -lnvram -L$(TOP)/shared -lshared -lcrypt
LDFLAGS += -ffunction-sections -fdata-sections -Wl,--gc-sections
ifeq ($(STATIC),1)
LDFLAGS += -static
endif

OBJS := rc.o init.o interface.o network.o wan.o services.o dhcp.o
OBJS += firewall.o ppp.o telssh.o wnas.o
OBJS += listen.o redial.o led.o qos.o forward.o misc.o mtd.o
OBJS += buttons.o restrict.o gpio.o sched.o
OBJS += new_qoslimit.o arpbind.o
OBJS += tomatoanon.o

ifeq ($(TCONFIG_USB),y)
OBJS += usb.o
endif
ifeq ($(TCONFIG_DDNS),y)
OBJS += ddns.o
endif
ifeq ($(TCONFIG_CIFS),y)
OBJS += cifs.o
endif
ifeq ($(TCONFIG_JFFS2),y)
OBJS += jffs2.o
endif
ifeq ($(TCONFIG_OPENVPN),y)
OBJS += vpn.o
endif

ifeq ($(TCONFIG_PPTPD),y)
OBJS += pptpd.o
OBJS += pptp_client.o
endif

ifeq ($(TCONFIG_NOCAT),y)
OBJS += nocat.o
endif

ifeq ($(TOMATO_SL),1)
#OBJS += usb.o
OBJS += samba.o
endif

ifeq ($(TCONFIG_BT),y)
OBJS += transmission.o
endif

ifeq ($(TCONFIG_NFS),y)
OBJS += nfs.o
endif

ifeq ($(TCONFIG_SNMP),y)
OBJS += snmp.o
endif

ifeq ($(TCONFIG_SDHC),y)
OBJS += mmc.o
endif

ifeq ($(TCONFIG_UPS),y)
OBJS += ups.o
endif

ifeq ($(TCONFIG_TOR),y)
OBJS += tor.o
endif

all: rc

rc: $(OBJS)
	@echo " [rc] CC $@"
	@$(CC) -o $@ $^ $(LDFLAGS)

	$(SIZECHECK)
	$(CPTMP)

install: all
	@echo "[rc] Installing..."
	@install -D rc $(INSTALLDIR)/sbin/rc
	@$(STRIP) $(INSTALLDIR)/sbin/rc
	@chmod 0500 $(INSTALLDIR)/sbin/rc

	@cd $(INSTALLDIR)/sbin && ln -sf rc init
	@cd $(INSTALLDIR)/sbin && ln -sf rc console
	@cd $(INSTALLDIR)/sbin && ln -sf rc hotplug
	@cd $(INSTALLDIR)/sbin && ln -sf rc service
	@cd $(INSTALLDIR)/sbin && ln -sf rc buttons
	@cd $(INSTALLDIR)/sbin && ln -sf rc rcheck
	@cd $(INSTALLDIR)/sbin && ln -sf rc radio
	@cd $(INSTALLDIR)/sbin && ln -sf rc led
	@cd $(INSTALLDIR)/sbin && ln -sf rc reboot
	@cd $(INSTALLDIR)/sbin && ln -sf rc halt
	@cd $(INSTALLDIR)/sbin && ln -sf rc redial
	@cd $(INSTALLDIR)/sbin && ln -sf rc gpio
	@cd $(INSTALLDIR)/sbin && ln -sf rc sched

	@cd $(INSTALLDIR)/sbin && ln -sf rc disconnected_pppoe
	@cd $(INSTALLDIR)/sbin && ln -sf rc listen
	@cd $(INSTALLDIR)/sbin && ln -sf rc ppp_event

	@cd $(INSTALLDIR)/sbin && ln -sf rc mtd-write
	@cd $(INSTALLDIR)/sbin && ln -sf rc mtd-erase
	@cd $(INSTALLDIR)/sbin && ln -sf rc mtd-unlock

	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcpc-event
	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcpc-release
	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcpc-renew

	@cd $(INSTALLDIR)/sbin && ln -sf rc wldist

ifeq ($(TCONFIG_IPV6),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcp6c-state
endif

ifeq ($(TCONFIG_DDNS),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc ddns-update
endif

ifeq ($(TCONFIG_CIFS),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc mount-cifs
endif

ifeq ($(TOMATO_SL),1)
	@cd $(INSTALLDIR)/sbin && ln -sf rc usbevent
	@cd $(INSTALLDIR)/sbin && ln -sf rc usbrescan
	@cd $(INSTALLDIR)/sbin && ln -sf rc hotdiskadd
	@cd $(INSTALLDIR)/sbin && ln -sf rc hotdiskremove
	@cd $(INSTALLDIR)/sbin && ln -sf rc hotdiskerror
	@cd $(INSTALLDIR)/sbin && ln -sf rc umountx
endif

ifeq ($(TCONFIG_OPENVPN),y)
	@mkdir -p $(INSTALLDIR)/rom/openvpn
	@cp openvpn-updown.sh $(INSTALLDIR)/rom/openvpn/updown.sh
	@chmod 0755 $(INSTALLDIR)/rom/openvpn/updown.sh
endif

clean:
	rm -f rc *.o .*.depend

size: rc
	mipsel-uclibc-nm --print-size --size-sort rc

%.o: %.c .%.depend
	@echo " [rc] CC $@"
	@$(CC) $(CFLAGS) -c $<

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)



pswatch: pswatch.c
	$(CC) -o $@ $^
	$(STRIP) $@
	$(CPTMP)
