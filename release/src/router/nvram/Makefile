include ../common.mak

CFLAGS = -O2 -Wall #-mips32
CFLAGS += -I../shared -I../../include -I.
ifeq ($(STATIC),1)
CFLAGS += -static
endif

all: libnvram.so libnvram.a nvram

libnvram.so: nvram_linux.o nvram_convert.o
	@echo " [nvram] CC $@"
	@$(CC) -shared $(CFLAGS) -o $@ $^

libnvram.a: nvram_linux.o nvram_convert.o
	@echo " [nvram] AR $@"
	@$(AR) cruv $@ $^
	
nvram: nvram.o defaults.o libnvram.so
	@echo " [nvram] CC $@"
ifeq ($(STATIC),1)
	$(CC) $(CFLAGS) -static -o $@ nvram.o defaults.o -L. -lnvram -L../shared -lshared
else
	@$(CC) $(CFLAGS) -o $@ nvram.o defaults.o -L. -lnvram -L../shared -lshared
endif
	
	$(SIZECHECK)
	$(CPTMP)
	
install: all
	install -D -m 0500 libnvram.so $(INSTALLDIR)/usr/lib/libnvram.so
	install -D -m 0500 nvram $(INSTALLDIR)/bin/nvram
	$(STRIP) $(INSTALLDIR)/usr/lib/libnvram.so
	$(STRIP) $(INSTALLDIR)/bin/nvram

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
