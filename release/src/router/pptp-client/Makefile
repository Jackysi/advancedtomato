VERSION = 1.1.0

CT_SPEC = 1

#################################################################
# CHANGE THIS LINE to point to the location of your pppd binary.
PPPD = /usr/sbin/pppd
#################################################################

#CROSS =mipsel-linux-
#CC = $(CROSS)gcc -s
RM	= rm -f
#DEBUG	= -g
INCLUDE =
CFLAGS  = -Wall -O1 $(DEBUG) $(INCLUDE)	-I$(SRCBASE)/include $(EXTRA_CFLAGS)
LIBS	=
#LDFLAGS	= -lutil

PPTP_BIN = pptp

PPTP_OBJS = pptp.o pptp_gre.o ppp_fcs.o \
            pptp_ctrl.o dirutil.o vector.o \
            inststr.o util.o version.o \
	    pptp_quirks.o orckit_quirks.o

PPTP_DEPS = pptp_callmgr.h pptp_gre.h ppp_fcs.h util.h \
	    pptp_quirks.h orckit_quirks.h config.h pptp_options.h

all: config.h $(PPTP_BIN)

$(PPTP_BIN): $(PPTP_OBJS) $(PPTP_DEPS)
	$(CC) -o $(PPTP_BIN) $(PPTP_OBJS) $(LDFLAGS) $(LIBS)

config.h: 
	echo "/* text added by Makefile target config.h */" > config.h
	echo "#define PPTP_LINUX_VERSION \"$(VERSION)\"" >> config.h
	echo "#define PPPD_BINARY \"$(PPPD)\"" >> config.h

vector_test: vector_test.o vector.o
	$(CC) -o vector_test vector_test.o vector.o

clean:
	$(RM) *.o pptp

clobber: clean
	$(RM) $(PPTP_BIN) vector_test

distclean: clobber
	$(RM) pptp-linux-*.tar.gz
	$(RM) -r pptp-linux-*

test: vector_test

dist: clobber
	$(RM) pptp-linux-$(VERSION).tar.gz
	$(RM) -r pptp-linux-$(VERSION)
	mkdir pptp-linux-$(VERSION)
	mkdir pptp-linux-$(VERSION)/Documentation
	mkdir pptp-linux-$(VERSION)/Reference
	cp \
AUTHORS COPYING INSTALL Makefile NEWS README TODO USING		\
dirutil.c dirutil.h inststr.c inststr.h ppp_fcs.c ppp_fcs.h	\
pptp.c pptp_callmgr.c pptp_callmgr.h pptp_ctrl.c pptp_ctrl.h	\
pptp_gre.c pptp_gre.h pptp_msg.h pptp_options.h			\
util.c util.h vector.c vector.h vector_test.c	\
version.c version.h \
  pptp-linux-$(VERSION)
	cp \
Documentation/DESIGN.CALLMGR	\
Documentation/DESIGN.PPTP	\
Documentation/PORTING	\
  pptp-linux-$(VERSION)/Documentation
	cp \
Reference/README	\
Reference/ms-chap.txt	\
Reference/pptp-draft.txt	\
Reference/rfc1661.txt	\
Reference/rfc1662.txt	\
Reference/rfc1701.txt	\
Reference/rfc1702.txt	\
Reference/rfc1990.txt	\
Reference/rfc791.txt	\
Reference/rfc793.txt	\
  pptp-linux-$(VERSION)/Reference
	tar czf pptp-linux-$(VERSION).tar.gz pptp-linux-$(VERSION)
	$(RM) -r pptp-linux-$(VERSION)

pptp_ctrl.o: $(SRCBASE)/include/cyutils.h
