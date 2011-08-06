include ../common.mak
include $(LINUXDIR)/.config

UTILS := wl nas
ifneq ($(wildcard $(SRCBASE)/wl/eapd/*.c),)
UTILS += eapd
else
ifneq ($(wildcard $(SRCBASE)/wl/$(PLATFORM)/eapd),)
UTILS += eapd
endif
endif

WLOBJS = wlu.o wlu_cmd.o wlu_iov.o wlu_linux.o bcmutils.o bcmwifi.o
ifneq ($(wildcard $(SRCBASE)/shared/miniopt.c),)
WLOBJS += miniopt.o
endif
ifneq ($(wildcard $(SRCBASE)/shared/bcm_app_utils.c),)
WLOBJS += bcm_app_utils.o
endif
ifneq ($(wildcard $(SRCBASE)/wl/exe/wlu_client_shared.c),)
WLOBJS += wlu_client_shared.o wlu_pipe.o wlu_pipe_linux.o
endif

EAPDOBJS = eapd.o eapd_linux.o nas_eap.o wps_eap.o ses_eap.o
ifeq ($(CONFIG_BCMDCS),y)
EAPDOBJS += dcs_eap.o
endif

NASOBJS = mppe.o wpa.o nas.o nas_linux.o nas_wl.o nas_wksp.o nas_radius.o nas_wksp_radius.o bcmwpa.o bcmutils.o

all: $(UTILS)

clean:
	@rm -f *~ $(UTILS)
	@rm -f $(WLOBJS) $(EAPDOBJS) $(NASOBJS)

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
wl: $(WLOBJS)
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

ifneq ($(wildcard $(SRCBASE)/wl/nas/*.c),)
vpath %.c $(SRCBASE)/wl/nas $(SRCBASE)/shared $(SRCBASE)/router/shared
CFLAGS += -I$(SRCBASE)/include -I$(TOP)/shared -I$(SRCBASE)/wl/nas -I$(SRCBASE)/wl/eapd -Os $(EXTRACFLAGS) -s -Wall
CFLAGS += -DBCMWPA2 -DNAS_WKSP_BUILD_NAS_AUTH -DNAS_WKSP_BUILD_NAS_SUPPL -DNAS_RADIUS -DNAS_WKSP_ON_DEMAND -DBCMSUPPL
LDFLAGS += -L$(TOP)/nvram -lnvram -L$(TOP)/shared -lshared -L$(TOP)/libbcmcrypto -lbcmcrypto
nas: $(NASOBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
else
ifneq ($(wildcard $(SRCBASE)/wl/$(PLATFORM)/nas),)
nas:
	@cp -f $(SRCBASE)/wl/$(PLATFORM)/nas nas
else
vpath %.o $(SRCBASE)/wl/$(PLATFORM)
nas: nas_exe.o
	$(CC) $(CCFLAGS) -DBCMWPA2 -o $@ $< -L../shared  -L../nvram -lnvram -lshared
endif
endif

ifneq ($(wildcard $(SRCBASE)/wl/eapd/*.c),)
vpath %.c $(SRCBASE)/wl/eapd $(SRCBASE)/shared
CFLAGS += -I$(SRCBASE)/include -I$(SRCBASE)/wl/eapd -Os $(EXTRACFLAGS) -s
CFLAGS += -DBCMWPA2 -DEAPD_WKSP_AUTO_CONFIG -DEAPDDUMP
LDFLAGS += -L$(TOP)/nvram -lnvram -L$(TOP)/shared -lshared
ifeq ($(CONFIG_BCMDCS),y)
CFLAGS += -DBCM_DCS
endif
eapd: $(EAPDOBJS)
	$(CC) $^ -o $@ $(LDFLAGS)
else
ifneq ($(wildcard $(SRCBASE)/wl/$(PLATFORM)/eapd),)
eapd:
	@cp -f $(SRCBASE)/wl/$(PLATFORM)/eapd eapd
endif
endif

install: all
	install -d $(INSTALLDIR)/usr/sbin

	install -m 0755 nas $(INSTALLDIR)/usr/sbin/nas
	$(STRIP) $(INSTALLDIR)/usr/sbin/nas
	# nas4not is now a symbolic link to nas
	ln -sf nas $(INSTALLDIR)/usr/sbin/nas4not
	
	install -m 0755 wl $(INSTALLDIR)/usr/sbin/wl
	$(STRIP) $(INSTALLDIR)/usr/sbin/wl
	
ifneq ($(wildcard $(TOP)/prebuilt/eapd),)
	install -d $(INSTALLDIR)/bin
	install -m 0755 eapd $(INSTALLDIR)/bin/eapd
	$(STRIP) $(INSTALLDIR)/bin/eapd
endif

