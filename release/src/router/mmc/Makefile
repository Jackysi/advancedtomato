include ../common.mak

WARN    := -W -Wall -Wstrict-prototypes -Wmissing-prototypes -fno-strict-aliasing -mips32
INCLUDE := -isystem $(TOP)/../linux/linux/include 
CFLAGS  := -O2 -DMODULE -D__KERNEL__ ${WARN} ${INCLUDE} -fno-pic -mno-abicalls -mlong-calls

all:

clean:
	rm -f mmc.o

install:
	@echo " [mmc] Installing"
	install -m 0555 -D mmc.o $(INSTALLDIR)/lib/modules/2.4.20/kernel/drivers/

mmc.o:	mmc.c init.c spi.c Makefile
	$(CC) ${CFLAGS} -c -o mmc.o mmc.c
