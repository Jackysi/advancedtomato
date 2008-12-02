#
# milli_httpd Makefile
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.40.18.1.2.2 2006/02/07 10:56:31 honor Exp $
#
include $(TOP)/.config

ifneq ($(wildcard $(SRCBASE)/cy_conf.mak),)
  include $(SRCBASE)/cy_conf.mak
endif

CFLAGS	+= -I. -I$(TOP)/shared -I$(SRCBASE)/include -Wall -I$(SRCBASE)/

#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
LDFLAGS	+= -L$(TOP)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared


CFLAGS += -I$(TOP)/openssl/include  -I$(TOP)/openssl/include/openssl
LDFLAGS += -L$(TOP)/openssl -L../openssl -lcrypto -lssl
OBJS  += $(SRCBASE)/../tools/src/code_header.o backup_restore.o
OBJS  += setup_wizard.o
OBJS  += qos.o



vpath %.c $(TOP)/shared
vpath %.o $(SRCBASE)/router/httpd/prebuilt

all:clean httpd

clean:
	rm -f *.o *~ httpd
	rm -f $(SRCBASE)/../tools/src/code_header.o 

install:
	install -D httpd $(INSTALLDIR)/usr/sbin/httpd
	$(STRIP) $(INSTALLDIR)/usr/sbin/httpd
	install -d $(INSTALLDIR)/etc
	#install *.pem $(INSTALLDIR)/etc
	install openssl.cnf $(INSTALLDIR)/etc
	install -m 755 gencert.sh $(INSTALLDIR)/usr/sbin

cert:
	# create the key and certificate request
	openssl req -new -out cert.csr -config openssl.cnf -keyout privkey.pem -days 3650 -newkey rsa:512
	# remove the passphrase from the key:
	openssl rsa -in privkey.pem -out key.pem
	# convert the certificate request into a signed certificate
	openssl x509 -in cert.csr -out cert.pem -req -signkey key.pem 
	rm -f cert.csr privkey.pem
	# Show human-readable format
	openssl x509 -in cert.pem -text -noout

#qos_1.o:qos.o interface.o

httpd: cgi.o ej.o httpd.o broadcom.o \
       index.o status.o dhcp.o log.o upgrade.o filters.o forward.o dynamic_route.o static_route.o wireless.o ddns.o \
       find_pattern.o lib.o diag.o sysinfo.o wepkey.o md5c.o wl_test.o getservice.o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

build_date.o: build_date.c
build_date:
	echo "#define BUILD_DATE \"`date \"+%b %d %Y\"`\"" > build_date.c
	echo "#define BUILD_TIME \"`date \"+%H:%M:%S\"`\"" >> build_date.c


*.o: $(CY_DEPS)
