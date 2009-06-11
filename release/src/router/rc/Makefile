include ../common.mak

CFLAGS = -Wall -Os #-mips32
CFLAGS += -I. -I../shared -I$(SRCBASE)/include

LDFLAGS = -L../nvram -lnvram -L../shared -lshared -lcrypt
ifeq ($(STATIC),1)
LDFLAGS += -static
endif

OBJS := rc.o init.o interface.o network.o wan.o services.o dhcp.o
OBJS += firewall.o ppp.o telssh.o wnas.o
OBJS += listen.o redial.o led.o qos.o forward.o misc.o mtd.o
OBJS += buttons.o restrict.o gpio.o sched.o
OBJS += new_qoslimit.o
#	heartbeat.o

ifeq ($(TCONFIG_DDNS),y)
OBJS += ddns.o
endif
ifeq ($(TCONFIG_CIFS),y)
OBJS += cifs.o
endif
ifeq ($(TCONFIG_JFFS2),y)
OBJS += jffs2.o
endif

ifeq ($(TOMATO_SL),1)
OBJS += usb.o
OBJS += samba.o
endif


all: rc

rc: $(OBJS)
	@echo " [rc] CC $@"
	@$(CC) -o $@ $^ $(LDFLAGS)

	$(SIZECHECK)
	$(CPTMP)

install: all
	@echo "[rc] Installing..."
	@install -m 0500 -D rc $(INSTALLDIR)/sbin/rc
	@$(STRIP) $(INSTALLDIR)/sbin/rc

	@cd $(INSTALLDIR)/sbin && ln -sf rc init
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
	@cd $(INSTALLDIR)/sbin && ln -sf rc pppoe_down
	@cd $(INSTALLDIR)/sbin && ln -sf rc listen
	@cd $(INSTALLDIR)/sbin && ln -sf rc ppp_event

	@cd $(INSTALLDIR)/sbin && ln -sf rc mtd-write
	@cd $(INSTALLDIR)/sbin && ln -sf rc mtd-erase
	@cd $(INSTALLDIR)/sbin && ln -sf rc mtd-unlock

	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcpc-event
	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcpc-release
	@cd $(INSTALLDIR)/sbin && ln -sf rc dhcpc-renew

	@cd $(INSTALLDIR)/sbin && ln -sf rc wldist

ifeq ($(TCONFIG_DDNS),y)
	@cd $(INSTALLDIR)/sbin && ln -sf rc ddns-update
endif

#	ifeq ($(TCONFIG_HEARTBEAT),y)
#		@cd $(INSTALLDIR)/sbin && ln -sf rc hb_connect
#		@cd $(INSTALLDIR)/sbin && ln -sf rc hb_disconnect
#		@cd $(INSTALLDIR)/sbin && ln -sf rc hb_idle
#	endif

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
