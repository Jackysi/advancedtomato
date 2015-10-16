include ../common.mak

CFLAGS += -Os -Wall $(EXTRACFLAGS)
CFLAGS += -I$(TOP)/shared -I$(SRCBASE)/include
LDFLAGS = -L$(TOP)/shared -lshared -L$(TOP)/nvram$(BCMEX) -lnvram

INSTALL = install

ntpc: ntpc.o
	@echo " [ntpc] CC -o $@"
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

ifeq ($(TCONFIG_BCMARM),y)
LDFLAGS += -lgcc_s
else
	$(SIZECHECK)
	$(CPTMP)
endif

install: ntpc
	install -D ntpc $(INSTALLDIR)/bin/ntpc
	$(STRIP) $(INSTALLDIR)/bin/ntpc
	chmod 0500 $(INSTALLDIR)/bin/ntpc

	cd $(INSTALLDIR)/bin && ln -sf ntpc ntpsync
	cd $(INSTALLDIR)/bin && ln -sf ntpc ntpstep

clean:
	rm -f ntpc .*.depend *.o
