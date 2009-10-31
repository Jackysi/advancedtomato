
CFLAGS = -s -O2 $(EXTRA_CFLAGS) #-mips32

all: ttcp

ttcp: epi_ttcp.c
	$(CC) $(CFLAGS) -o $@ $^

install:
	install -d $(INSTALLDIR)/usr/sbin
	install ttcp $(INSTALLDIR)/usr/sbin

clean:
	rm -f ttcp nuttcp
