include ../common.mak

WARN    := -W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-strict-aliasing -mips32
INCLUDE := -isystem $(TOP)/../linux/linux/include
CFLAGS  := -O2 -DMODULE -D__KERNEL__ ${WARN} ${INCLUDE} -fno-pic -mno-abicalls -mlong-calls
CC      := /opt/brcm24/hndtools-mipsel-linux-3.2.3/bin/mipsel-linux-gcc

all:	mmc.o

#clean:
#	rm -f mmc.o

install:	all
	install -m 0644 -D mmc.o $(INSTALLDIR)/lib/modules/2.4.37.11/kernel/drivers/mmc.o

mmc.o:	mmc.c init.c spi.c Makefile
	@echo " [mmc] CC -o $@"
	@$(CC) ${CFLAGS} -c -o mmc.o mmc.c
