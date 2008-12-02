prefix = /usr/local/zebra
sysconfdir = ${prefix}/etc
DEFS = -DHAVE_CONFIG_H -DSYSCONFDIR=\"$(sysconfdir)/\" -DMULTIPATH_NUM=1
top_srcdir = ..
INCLUDES =  -I.. -I$(top_srcdir) -I$(top_srcdir)/lib
libzebra_a_DEPENDENCIES =
libzebra_a_LIBADD =
sbin_PROGRAMS = zebra
CPPFLAGS =
LDFLAGS =
LIBS = -lm -lcrypt -lzebra -L../lib
CFLAGS = -g -O2 -Wall
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

OBJS := zserv.o main.o interface.o connected.o ioctl.o rib.o redistribute.o debug.o rtadv.o zebra_snmp.o ipforward_proc.o if_netlink.o if_proc.o rt_netlink.o rtread_netlink.o  

all: $(OBJS)
	$(CC) -g -o $(sbin_PROGRAMS) $(OBJS) $(LIBS) 




clean:
	rm -rf *.o
	rm -rf *.a
	rm -rf *.so
	rm -rf $(sbin_PROGRAMS)	

.c.o:
	$(COMPILE) -c $<
 
.s.o:
	$(COMPILE) -c $<
 
.S.o:
	$(COMPILE) -c $<



