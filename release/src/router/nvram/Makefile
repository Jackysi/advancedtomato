include ../common.mak

CFLAGS = -O2 -Wall $(EXTRACFLAGS) -fPIC
CFLAGS += -I$(TOP)/shared -I$(SRCBASE)/include -I.
ifeq ($(STATIC),1)
CFLAGS += -static
endif

ifdef MOD_NVR
all: libnvram.a nvram2
CFLAGS += -DMOD_NVR -static
.PHONY:  nvram_linux.c nvram.c
else
all: libnvram.so libnvram.a nvram
endif

libnvram.so: nvram_linux.o nvram_convert.o
	@echo " [nvram] CC $@"
	@$(CC) -shared $(CFLAGS) -o $@ $^

libnvram.a: nvram_linux.o nvram_convert.o
	@echo " [nvram] AR $@"
	@$(AR) cruv $@ $^

nvram: nvram.o defaults.o libnvram.so
	@echo " [nvram] CC $@"
	@$(CC) $(CFLAGS) -o $@ nvram.o defaults.o -L. -lnvram -L$(TOP)/shared -lshared

	$(SIZECHECK)
	$(CPTMP)

nvram2: defaults.o libnvram.a nvram.o
	@echo " [nvram] CC $@"
	@$(CC) $(CFLAGS) -o $@ nvram.o defaults.o -L. -lnvram -L../shared -lshared
	$(SIZECHECK)
	$(CPTMP)

install: all
	install -D libnvram.so $(INSTALLDIR)/usr/lib/libnvram.so
	install -D nvram $(INSTALLDIR)/bin/nvram
	$(STRIP) $(INSTALLDIR)/usr/lib/libnvram.so
	$(STRIP) $(INSTALLDIR)/bin/nvram
	chmod 0555 $(INSTALLDIR)/bin/nvram

clean:
	rm -f nvram *.o *.a *.so


%.o: %.c .%.depend
	@echo " [nvram] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
