prefix = /usr/local/zebra
sysconfdir = ${prefix}/etc
DEFS = -DHAVE_CONFIG_H -DSYSCONFDIR=\"$(sysconfdir)/\" 
top_srcdir = ..
INCLUDES =  -I.. -I$(top_srcdir) -I$(top_srcdir)/lib
libzebra_a_DEPENDENCIES =
libzebra_a_LIBADD =
sbin_PROGRAMS = ripd

CPPFLAGS =
LDFLAGS =
LIBS = -lm -lcrypt -lzebra -L../lib
CFLAGS = -g -O2 -Wall 
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

OBJS1 := ripd.o rip_zebra.o rip_interface.o rip_debug.o rip_snmp.o rip_routemap.o rip_peer.o rip_offset.o

OBJS2 := rip_main.o

all: $(OBJS1) $(OBJS2) librip.a
	$(CC) $(CFLAGS) -o $(sbin_PROGRAMS) $(OBJS1) $(OBJS2) $(LIBS) 

librip.a: $(OBJS1) 
	rm -f librip.a
	$(AR) cru librip.a $(OBJS1) 
	$(RANLIB) librip.a

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



