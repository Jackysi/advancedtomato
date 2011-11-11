include ../common.mak
DESTDIR=$(INSTALLDIR)

# Define USE_WRAP if you want to compile with
# libwrap (hosts.{allow,deny} access control)
ifneq ($(USE_WRAP),)
LIBS += -lwrap
DEFINES += -DUSE_LIBWRAP
endif

# If you don't have it in /var/log/subsys, uncomment and define
CFLAGS += -DLOCKFILE_DIR=\"/var/lock\"

# GNU target string
#CROSS = 

#CC = $(CROSS)gcc
#STRIP = $(CROSS)strip

CFLAGS += -O2  -Wall $(EXTRACFLAGS)

PROG = p910nd
CONFIG = p910nd.conf
INITSCRIPT = p910nd.init
MANPAGE = p910nd.8
INSTALL = install
BINDIR = /usr/sbin
CONFIGDIR = /etc/sysconfig
SCRIPTDIR = /etc/init.d
MANDIR = /usr/share/man/man8

$(PROG):	p910nd.c
	$(CC) -o $@ $^ $(CFLAGS) $(DEFINES) $(LIBS)

strip: $(PROG)
	$(STRIP) -s $(PROG)

full_install: $(PROG) $(CONFIG) $(INITSCRIPT) $(MANPAGE)
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(CONFIGDIR) \
		$(DESTDIR)$(SCRIPTDIR) $(DESTDIR)$(MANDIR)
	$(INSTALL) $(PROG) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 644 $(CONFIG) $(DESTDIR)$(CONFIGDIR)/$(PROG)
	$(INSTALL) $(INITSCRIPT) $(DESTDIR)$(SCRIPTDIR)/$(PROG)
	$(INSTALL) -m 644 $(MANPAGE) $(DESTDIR)$(MANDIR)

install: $(PROG)
	mkdir -p $(DESTDIR)$(BINDIR)
	@$(INSTALL) $(PROG) $(DESTDIR)$(BINDIR)
	@$(STRIP) -s $(DESTDIR)$(BINDIR)/$(PROG)

.PHONY: clean
clean:
	rm -f *.o $(PROG)
