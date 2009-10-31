include ../common.mak

CFLAGS += -Wall -Os $(EXTRA_CFLAGS) -I$(SRCBASE) -I$(SRCBASE)/include -I.

TARGETS = scsi-start scsi-idle # scsi-stop

all: $(TARGETS)

scsi-start:	scsi-start.c
	@$(CC) -o $@ $^ $(CFLAGS)

scsi-idle:	scsi-idle.c
	@$(CC) -o $@ $^ $(CFLAGS)

#scsi-stop:	scsi-start
#	ln -s scsi-start scsi-stop

clean:
	rm -f *.o $(TARGETS) *~

install:	all
	@install -d $(INSTALLDIR)/usr/sbin
	@install -D scsi-start $(INSTALLDIR)/usr/sbin/scsi-start
##	@install -D scsi-idle $(INSTALLDIR)/usr/sbin/scsi-idle
	@$(STRIP) -s $(INSTALLDIR)/usr/sbin/scsi-start
##	@$(STRIP) -s $(INSTALLDIR)/usr/sbin/scsi-idle
	# scsi-stop is a symbolic link to scsi-start
	ln -sf scsi-start $(INSTALLDIR)/usr/sbin/scsi-stop

