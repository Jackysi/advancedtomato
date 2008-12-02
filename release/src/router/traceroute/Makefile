# Generated automatically from Makefile.in by configure.
#  Copyright (c) 1988, 1989, 1990, 1991, 1992, 1995, 1996, 1997, 1998, 1999, 2000
# 	The Regents of the University of California.  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that: (1) source code distributions
#  retain the above copyright notice and this paragraph in its entirety, (2)
#  distributions including binary code include the above copyright notice and
#  this paragraph in its entirety in the documentation or other materials
#  provided with the distribution, and (3) all advertising materials mentioning
#  features or use of this software display the following acknowledgement:
#  ``This product includes software developed by the University of California,
#  Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
#  the University nor the names of its contributors may be used to endorse
#  or promote products derived from this software without specific prior
#  written permission.
#  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
#  WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# @(#) $Id: Makefile,v 1.1 2004/02/09 03:33:34 honor Exp $ (LBL)

#
# Various configurable paths (remember to edit Makefile.in, not Makefile)
#

# Top level hierarchy
prefix = /usr/local
exec_prefix = ${prefix}
# Pathname of directory to install the binary
BINDEST = ${exec_prefix}/sbin
# Pathname of directory to install the man page
MANDEST = ${prefix}/man

# VPATH
srcdir = .

#
# You shouldn't need to edit anything below here.
#

CC = gcc
CCOPT = -O
INCLS = -I.  -Ilinux-include
DEFS =  -DHAVE_MALLOC_H=1 -DHAVE_SYS_SELECT_H=1 -DHAVE_NET_ROUTE_H=1 -DHAVE_STRERROR=1 -DHAVE_USLEEP=1 -DHAVE_SETLINEBUF=1 -DBYTESWAP_IP_HDR=1 -DHAVE_ICMP_NEXTMTU=1 

# Standard CFLAGS
CFLAGS = $(CCOPT) $(DEFS) $(INCLS)

# Standard LIBS
LIBS = 

INSTALL = /usr/bin/install -c

# Explicitly define compilation rule since SunOS 4's make doesn't like gcc.
# Also, gcc does not remove the .o before forking 'as', which can be a
# problem if you don't own the file but can write to the directory.
.c.o:
	@rm -f $@
	$(CC) $(CFLAGS) -c $(srcdir)/$*.c

CSRC =	traceroute.c ifaddrlist.c findsaddr-linux.c
GENSRC = version.c

SRC =	$(CSRC) $(GENSRC)
HDR =	findsaddr.h ifaddrlist.h

TAGHDR = \
	/usr/include/netinet/in.h \
	/usr/include/netinet/ip_icmp.h \
	/usr/include/netinet/udp.h

TAGFILES = $(SRC) $(HDR) $(TAGHDR)

# We would like to say "OBJ = $(SRC:.c=.o)" but Ultrix's make cannot
# hack the extra indirection
OBJ =	$(CSRC:.c=.o) $(GENSRC:.c=.o) 

CLEANFILES = $(OBJ) traceroute $(GENSRC)

all: traceroute

traceroute: $(OBJ)
	@rm -f $@
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

version.o: version.c
version.c: $(srcdir)/VERSION
	@rm -f $@
	sed -e 's/.*/char version[] = "&";/' $(srcdir)/VERSION > $@

install: force
	$(INSTALL) -m 4555 -o root -g bin traceroute $(DESTDIR)$(BINDEST)

install-man: force
	$(INSTALL) -m 444 -o bin -g bin traceroute.8 $(DESTDIR)$(MANDEST)/man8

lint: $(GENSRC) force
	lint -hbxn $(SRC) | \
	    grep -v 'struct/union .* never defined' | \
	    grep -v 'possible pointer alignment problem'

clean: force
	rm -f $(CLEANFILES)

distclean: force
	rm -f $(CLEANFILES) Makefile config.cache config.log config.status \
	    gnuc.h os-proto.h

tags: $(TAGFILES)
	ctags -wtd $(TAGFILES)

tar: force
	@cwd=`pwd` ; name=traceroute-`cat VERSION` ; \
	    list="" ; tar="tar chf" ; temp="$$name.tar.gz" ; \
	    for i in `cat FILES` ; do list="$$list $$name/$$i" ; done; \
	    echo \
	    "rm -f $$name; ln -s . $$name" ; \
	     rm -f $$name; ln -s . $$name ; \
	    echo \
	    "$$tar - [lots of files] | gzip > $$temp" ; \
	     $$tar - $$list | gzip > $$temp ; \
	    echo \
	    "rm -f $$name" ; \
	     rm -f $$name

force: /tmp
depend: $(GENSRC) force
	./mkdep -c $(CC) $(DEFS) $(INCLS) $(SRC)
