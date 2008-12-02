include ../common.mak

CFLAGS = -O2 -Wall #-mips32

OBJS = \
	igd/igd.o \
	igd/igd_desc.o \
	igd/wancommon.o \
	igd/wanipc.o \
	igd/layer3.o  \
    igd/linux/linux_main.o \
	igd/linux/linux_net.o \
	igd/linux/linux_igd.o \
	igd/portmap.o \
	lib/upnp.o \
	lib/ssdp.o \
	lib/http.o \
	lib/gena.o \
	lib/soap.o \
	lib/service.o \
	lib/device.o \
	lib/uio.o \
	lib/linux/linux_osl.o

ifeq ($(MALLOC_DEBUG),1)
OBJS += ../rmalloc/rmalloc.o
CFLAGS += -DMALLOC_DEBUG=1 -I../rmalloc
endif

all: upnp

upnp: $(OBJS)
	@echo " [upnp] CC -o $@"
	@$(CC) $(CFLAGS) -s -o $@ $^ -L../nvram -lnvram -L../shared -lshared -L../iptables/ -liptc

	$(SIZECHECK)
	$(CPTMP)

install: all
	install -d $(INSTALLDIR)/usr/sbin
	install -m 755 upnp $(INSTALLDIR)/usr/sbin

clean:
	-find -name "*.o" -delete
	-rm -f upnp foo

foo: dummy
	$(CC) foo.c -o foo -I../iptables/include -L../iptables/ -liptc

%.o: %.c
	@echo " [upnp] CC $<"
	@$(CC) $(CFLAGS) -Iinclude -I$(SRCBASE)/include -I../shared -I../iptables/include -o $@ -c $<

.PHONY:	dummy
