include ../common.mak

all:	sd-idle

clean:
	rm -f sd-idle

install:	all
	install -m 0755 -D sd-idle $(INSTALLDIR)/usr/bin/sd-idle

sd-idle:	sd-idle.c Makefile
	@echo " [sd-idle] CC -o $@"
	@$(CC) ${CFLAGS} ${LDFLAGS} -o sd-idle sd-idle.c
