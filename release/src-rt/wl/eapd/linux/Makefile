#Linux Makefile

include $(SRCBASE)/router/.config

INCLUDE += -I. -I.. -I$(SRCBASE)/include -I$(SRCBASE)/shared -I$(SRCBASE)/router/shared -I$(SRCBASE)/router/nas

CFLAGS += $(INCLUDE)
CFLAGS += -DBCMWPA2 -DEAPD_WKSP_AUTO_CONFIG
CFLAGS += -Wall -DEAPDDUMP

CFLAGS  += -s
ifeq ($(CONFIG_RTR_OPTIMIZE_SIZE),y)
CFLAGS  += -Os
else
CFLAGS  += -O2
endif

LDFLAGS += -L$(TOP)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram
LDFLAGS += -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared

vpath %.c $(SRCBASE)/router/eapd $(SRCBASE)/shared

OBJS = eapd.o eapd_linux.o
OBJS += nas_eap.o wps_eap.o ses_eap.o


#ifdef BCM_DCS
ifeq ($(CONFIG_BCMDCS),y)
OBJS += dcs_eap.o
CFLAGS += -DBCM_DCS
endif
#endif

ifneq ($(wildcard ./prebuilt),)
all:
	-cp prebuilt/eapd eapd
else
all: eapd
endif

eapd: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	install -d $(INSTALLDIR)/bin
	install eapd $(INSTALLDIR)/bin
	$(STRIP) $(INSTALLDIR)/bin/eapd

clean:
	rm -f *.o eapd
