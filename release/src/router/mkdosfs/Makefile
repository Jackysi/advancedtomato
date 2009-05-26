include ../common.mak

CFLAGS =	-Os -Wall
LDFLAGS =	-s
LN =		ln -s

mkdosfs: mkdosfs.o
	$(CC) $(LDFLAGS) $< -o $@

.c.o:
	$(CC) -c $(CFLAGS) $<

install: mkdosfs
	mkdir -p $(INSTALLDIR)/sbin
	install -D mkdosfs $(INSTALLDIR)/sbin/mkdosfs
	$(STRIP) -s $(INSTALLDIR)/sbin/mkdosfs

.PHONY: clean
clean:
	rm -f *.s *.o *.a *~ core mkdosfs 
