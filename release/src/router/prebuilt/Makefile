include ../common.mak

UTILS := wl nas

all: $(UTILS)

clean:
	rm -f *~
ifneq ($(wildcard wl_exe.o),)
	rm -f wl
endif
ifneq ($(wildcard nas_exe.o),)
	rm -f nas
endif

ifneq ($(wildcard wl_exe.o),)
wl: wl_exe.o
	$(CC) $(CCFLAGS) -DBCMWPA2 -o $@ $<
else
wl:
endif

ifneq ($(wildcard nas_exe.o),)
nas: nas_exe.o
	$(CC) $(CCFLAGS) -DBCMWPA2 -o $@ $< -L../shared  -L../nvram -lnvram -lshared
else
nas:
endif

install: all
	install -d $(INSTALLDIR)/usr/sbin

	install -m 0755 nas $(INSTALLDIR)/usr/sbin/nas
	$(STRIP) $(INSTALLDIR)/usr/sbin/nas
	# nas4not is now a symbolic link to nas
	ln -sf nas $(INSTALLDIR)/usr/sbin/nas4not
	
	install -m 0755 wl $(INSTALLDIR)/usr/sbin/wl
	$(STRIP) $(INSTALLDIR)/usr/sbin/wl
	

