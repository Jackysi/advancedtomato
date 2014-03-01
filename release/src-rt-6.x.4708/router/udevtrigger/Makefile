CFLAGS	= -Os -Wall $(EXTRACFLAGS)

OBJS = udevtrigger.o

all: udevtrigger

udevtrigger: $(OBJS)
	@echo " [udevtrigger] CC -o $@"
	@$(CC) $(LDFLAGS) -o $@ $(OBJS)

	$(SIZECHECK)
	$(CPTMP)

install: all
	install -D udevtrigger $(INSTALLDIR)/sbin/udevtrigger
	$(STRIP) $(INSTALLDIR)/sbin/udevtrigger
	chmod 0500 $(INSTALLDIR)/sbin/udevtrigger

clean:
	rm -f udevtrigger .*.depend *.o

%.o: %.c .%.depend
	@echo " [udevtrigger] CC $@"
	@$(CC) $(CFLAGS) -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
