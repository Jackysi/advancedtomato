IPTABLES_SRC=../iptables
IPTABLES_VERSION = $(shell cat $(IPTABLES_SRC)/Makefile | grep -e '^IPTABLES_VERSION:=' | cut -d"=" -f2)
IPTABLES_OPTION = -DIPTABLES_VERSION=\"$(IPTABLES_VERSION)\"


all: libipt_ipp2p.so ipt_ipp2p.o

modules: ipt_ipp2p.h ipt_ipp2p.c Makefile
	$(CC) $(CFLAGS) -c ipt_ipp2p.c -o ipt_ipp2p_mod.o
	$(LD) -o ipt_ipp2p.o -r ipt_ipp2p_mod.o

ipt_ipp2p.o:
	@make -C $(LINUXDIR) SUBDIRS=$(shell pwd) modules

libipt_ipp2p.so: libipt_ipp2p.c ipt_ipp2p.h Makefile
	$(CC) -Os -Wall $(IPTABLES_OPTION) -I$(LINUXDIR)/include -I$(IPTABLES_SRC)/include -fPIC -c libipt_ipp2p.c
	$(CC) -shared -o $@ libipt_ipp2p.o
#	mipsel-uclibc-gcc -O2 -Wall -Wunused -I/wg/tomato/release/src/linux/linux/include -I../iptables/include/ -DIPTABLES_VERSION=\"1.3.6\"  -fPIC -o libipt_ipp2p_sh.o -c libipt_ipp2p.c
#	mipsel-uclibc-gcc -shared  -o libipt_ipp2p.so libipt_ipp2p_sh.o

install:
	install -D -m 0644 ipt_ipp2p.o $(INSTALLDIR)/lib/modules/2.4.20/kernel/net/ipv4/netfilter/ipt_ipp2p.o
	$(STRIP) -x $(INSTALLDIR)/lib/modules/2.4.20/kernel/net/ipv4/netfilter/ipt_ipp2p.o
	install -D libipt_ipp2p.so $(INSTALLDIR)/usr/lib/iptables/libipt_ipp2p.so
	$(STRIP) $(INSTALLDIR)/usr/lib/iptables/libipt_ipp2p.so


clean:
	-rm -f *.o *.so *.o~
