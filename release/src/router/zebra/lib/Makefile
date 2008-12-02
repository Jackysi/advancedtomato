prefix = /usr/local/zebra
sysconfdir = ${prefix}/etc
DEFS = -DHAVE_CONFIG_H -DSYSCONFDIR=\"$(sysconfdir)/\"
top_srcdir = ..
INCLUDES =  -I.. -I$(top_srcdir) -I$(top_srcdir)/lib
noinst_LIBRARIES = libzebra.a
noinst_LIBRARIES = libzebra.so
libzebra_a_SOURCES = version.c network.c pid_output.c getopt.c getopt1.c daemon.c    print_version.c checksum.c vector.c linklist.c vty.c command.c  sockunion.c prefix.c thread.c if.c memory.c buffer.c table.c hash.c     filter.c routemap.c distribute.c stream.c str.c log.c plist.c   zclient.c sockopt.c smux.c md5.c if_rmap.c keychain.c
 
libzebra_a_DEPENDENCIES =
 
libzebra_a_LIBADD =
 
noinst_HEADERS = buffer.h command.h filter.h getopt.h hash.h if.h linklist.h log.h memory.h network.h prefix.h routemap.h distribute.h sockunion.h str.h stream.h table.h thread.h vector.h version.h vty.h zebra.h plist.h zclient.h sockopt.h smux.h md5-gnu.h if_rmap.h keychain.h

EXTRA_DIST = regex.c regex-gnu.h
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES =
LIBRARIES =  $(noinst_LIBRARIES) libzebra.a
 
CPPFLAGS =
LDFLAGS =
LIBS = -lm -lcrypt
libzebra_a_OBJECTS =  version.o network.o pid_output.o getopt.o \
getopt1.o daemon.o print_version.o checksum.o vector.o linklist.o vty.o \
command.o sockunion.o prefix.o thread.o if.o memory.o buffer.o table.o \
hash.o filter.o routemap.o distribute.o stream.o str.o log.o plist.o \
zclient.o sockopt.o smux.o md5.o if_rmap.o keychain.o
CFLAGS = -g -O2 -Wall
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
HEADERS =  $(noinst_HEADERS)



libzebra.a: $(libzebra_a_OBJECTS) $(libzebra_a_DEPENDENCIES)
	rm -f libzebra.a
	$(AR) cru libzebra.a $(libzebra_a_OBJECTS) $(libzebra_a_LIBADD)
	$(RANLIB) libzebra.a
 
libzebra.so: $(libzebra_a_OBJECTS) $(libzebra_a_DEPENDENCIES)
	rm -f libzebra.so
	$(LD) -shared -o libzebra.so $(libzebra_a_OBJECTS) $(libzebra_a_LIBADD)


all:libzebra.a libzebra.so

clean:
	rm -rf *.o
	rm -rf *.a
	rm -rf *.so	

.c.o:
	$(COMPILE) -c $<
 
.s.o:
	$(COMPILE) -c $<
 
.S.o:
	$(COMPILE) -c $<



