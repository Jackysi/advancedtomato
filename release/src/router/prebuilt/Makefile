include ../common.mak
include $(LINUXDIR)/.config

ifneq ($(wildcard $(SRCBASE)/wl/$(PLATFORM)/eapd),)
UTILS := wl nas eapd
else
UTILS := wl nas
endif

all: $(UTILS)

clean:
	@rm -f *~ $(UTILS)
	@rm -f wlu.o wlu_cmd.o wlu_iov.o wlu_linux.o bcmutils.o bcmwifi.o

ifneq ($(wildcard $(SRCBASE)/wl/exe/*.c),)
vpath %.c $(SRCBASE)/wl/exe $(SRCBASE)/shared
CFLAGS += -I$(SRCBASE)/include -I$(SRCBASE)/wl/exe -Os $(EXTRACFLAGS)
ifeq ($(CONFIG_BCM_CTF),y)
CFLAGS += -DHNDCTF -DCTFPOOL
else
ifeq ($(CONFIG_BCM_CTF),m)
CFLAGS += -DHNDCTF -DCTFPOOL
endif
endif

wl: wlu.o wlu_cmd.o wlu_iov.o wlu_linux.o bcmutils.o bcmwifi.o
	$(CC) $^ -o $@ $(LDFLAGS)
else
ifneq ($(wildcard $(SRCBASE)/wl/$(PLATFORM)/wl),)
wl:
	@cp -f $(SRCBASE)/wl/$(PLATFORM)/wl wl
else
vpath %.o $(SRCBASE)/wl/$(PLATFORM)
wl: wl_exe.o
	$(CC) $(CCFLAGS) -DBCMWPA2 -o $@ $<

endif
endif

ifneq ($(wildcard $(SRCBASE)/wl/$(PLATFORM)/nas),)
nas:
	@cp -f $(SRCBASE)/wl/$(PLATFORM)/nas nas
else
vpath %.o $(SRCBASE)/wl/$(PLATFORM)
nas: nas_exe.o
	$(CC) $(CCFLAGS) -DBCMWPA2 -o $@ $< -L../shared  -L../nvram -lnvram -lshared
endif

ifneq ($(wildcard $(SRCBASE)/wl/$(PLATFORM)/eapd),)
eapd:
	@cp -f $(SRCBASE)/wl/$(PLATFORM)/eapd eapd
endif

install: all
	install -d $(INSTALLDIR)/usr/sbin

	install -m 0755 nas $(INSTALLDIR)/usr/sbin/nas
	$(STRIP) $(INSTALLDIR)/usr/sbin/nas
	# nas4not is now a symbolic link to nas
	ln -sf nas $(INSTALLDIR)/usr/sbin/nas4not
	
	install -m 0755 wl $(INSTALLDIR)/usr/sbin/wl
	$(STRIP) $(INSTALLDIR)/usr/sbin/wl
	
ifneq ($(wildcard $(SRCBASE)/wl/$(PLATFORM)/eapd),)
	install -d $(INSTALLDIR)/bin
	install -m 0755 eapd $(INSTALLDIR)/bin/eapd
	$(STRIP) $(INSTALLDIR)/bin/eapd
endif

