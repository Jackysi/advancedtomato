# $Id: Makefile,v 1.1.1.4 2003/10/14 08:09:37 sparq Exp $

CFLAGS = -Wall -g -O2 $(EXTRACFLAGS)
ifeq ($(strip $(DOSTATIC)),true)
    CFLAGS += --static
endif


all:		bidi tap

clean:
		-rcsclean *
		rm -f *~
		rm -f bidi
		rm -f tap

bidi:		bidi.c
		$(CC) $(CFLAGS) -o bidi bidi.c

tap:		tap.c
		$(CC) $(CFLAGS) -o tap tap.c
