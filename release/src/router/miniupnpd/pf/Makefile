# $Id: Makefile,v 1.1 2007/10/06 10:42:53 nanard Exp $
# made for GNU Make
CFLAGS = -Wall -g
EXECUTABLES = testobsdrdr

all:	$(EXECUTABLES)

clean:
	rm -f *.o $(EXECUTABLES)

testobsdrdr:	testobsdrdr.o obsdrdr.o

obsdrdr.o:	obsdrdr.c obsdrdr.h

testobsdrdr.o:	testobsdrdr.c obsdrdr.h

