include ../common.mak

CFLAGS = -Os -Wall $(EXTRACFLAGS)
CFLAGS += -I$(SRCBASE) -I$(SRCBASE)/include -I. -I$(TOP)/shared

ifeq ($(TCONFIG_BCMARM),y)
CFLAGS += -I$(SRCBASE)/shared/bcmwifi/include
CFLAGS += -I$(SRCBASE)/common/include
LIBS += -lgcc_s
endif

OBJS = httpd.o cgi.o tomato.o version.o 
OBJS += misc.o dhcp.o upgrade.o traceping.o parser.o upnp.o ctnf.o
OBJS += nvram.o log.o webio.o wl.o devlist.o ddns.o config.o bwm.o
OBJS += blackhole.o utf8.o
OBJS += cpuinfo.o

OBJS += iptraffic.o
ifeq ($(TCONFIG_USB),y)
OBJS += usb.o
endif

ifeq ($(TCONFIG_NOCAT),y)
OBJS += nocat.o
endif

LIBS = -L$(TOP)/nvram${BCMEX} -lnvram -L$(TOP)/shared -lshared
LIBS += -L$(TOP)/mssl -lmssl

ifeq ($(FULL_OPENSSL),y)
LIBS += -L$(TOP)/openssl -lssl -lcrypto
endif

ifeq ($(TCONFIG_PPTPD),y)
OBJS += pptpd.o
endif

ifeq ($(TCONFIG_BCMARM),y)
LIBS += -lgcc_s
endif

all: httpd

httpd: $(OBJS)
	@echo " [httpd] CC $@"
	@$(CC) -o $@ $(OBJS) $(LIBS)

#	$(SIZECHECK)
#	$(CPTMP)


install:
	@echo " [httpd] Installing to $(INSTALLDIR)"
	@install -D httpd $(INSTALLDIR)/usr/sbin/httpd
	@$(STRIP) $(INSTALLDIR)/usr/sbin/httpd
	@chmod 0500 $(INSTALLDIR)/usr/sbin/httpd

clean:
	rm -f httpd *.o .*.depend

#size: httpd
#	mipsel-uclibc-nm --print-size --size-sort httpd

%.o: %.c .%.depend
	@echo " [httpd] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
