#
#   $Id: Makefile.am,v 1.12 2011/01/08 04:08:30 reubenhwk Exp $
#
#   Authors:
#    Lars Fenneberg		<lf@elemental.net>	 
#
#   This software is Copyright 1996,1997 by the above mentioned author(s), 
#   All Rights Reserved.
#
#   The license which is distributed with this software in the file COPYRIGHT
#   applies to this software. If your distribution is missing this file, you
#   may request it from <pekkas@netcore.fi>.
#

AUTOMAKE_OPTIONS = 1.2 foreign

DEFS=  -DPATH_RADVD_CONF=\"$(PATH_RADVD_CONF)\" \
	-DPATH_RADVD_LOG=\"$(PATH_RADVD_LOG)\" -DLOG_FACILITY=$(LOG_FACILITY) \
	-DPATH_RADVD_PID=\"$(PATH_RADVD_PID)\" \
	-DVERSION=\"$(VERSION)\" -DINET6=1
INCLUDES=-I$(srcdir) -I.

########################################################################

COMMON_SRC = log.c socket.c recv.c util.c radvd.h defaults.h pathnames.h \
	includes.h

sbin_PROGRAMS = radvd radvdump

radvd_SOURCES = $(COMMON_SRC) radvd.c timer.c send.c process.c interface.c \
	device.c device-common.c privsep.c gram.y gram.h scanner.l
radvd_LDADD = -lfl
EXTRA_radvd_SOURCES = device-linux.c device-bsd44.c privsep-linux.c privsep-bsd44.c
YFLAGS = -d
CFLAGS = @CFLAGS@ -Wall -Wpointer-arith  -Wcast-qual -Wcast-align -Wstrict-prototypes \
	-Wmissing-prototypes -Wmissing-declarations -Wno-strict-aliasing

radvdump_SOURCES = $(COMMON_SRC) radvdump.c

man_MANS = radvd.8 radvdump.8 radvd.conf.5
EXTRA_DIST = radvd.8.man radvdump.8.man radvd.conf.5.man VERSION CHANGES \
	COPYRIGHT INTRO.html copyright.blurb radvd.conf.example
BUILT_SOURCES = device.c

CLEANFILES = radvd.8 radvdump.8 radvd.conf.5 gram.c gram.h scanner.c
DISTCLEANFILES = device.c privsep.c

SUFFIXES = .man

.man:
	$(SED) 	-e 's,@''VERSION''@,$(VERSION),g' \
		-e 's,@''sbindir''@,$(sbindir),g' \
		-e 's,@''PATH_RADVD_CONF''@,$(PATH_RADVD_CONF),g' \
		-e 's,@''PATH_RADVD_PID''@,$(PATH_RADVD_PID),g' \
		-e 's,@''PATH_RADVD_LOG''@,$(PATH_RADVD_LOG),g' \
		-e 's,@''LOG_FACILITY''@,$(LOG_FACILITY),g' \
		$< > $@

dist-hook:
	rm -f $(distdir)/device.c
	rm -f $(distdir)/privsep.c
	rm -f $(distdir)/scanner.c
	rm -f $(distdir)/gram.c
	rm -f $(distdir)/gram.h
	mkdir $(distdir)/redhat
	cp -p $(srcdir)/redhat/radvd.init $(distdir)/redhat
	cp -p $(srcdir)/redhat/radvd.spec $(distdir)/redhat
	cp -p $(srcdir)/redhat/radvd.sysconfig $(distdir)/redhat
	cp -p $(srcdir)/redhat/radvd.conf.empty $(distdir)/redhat

scanner.o: gram.h
