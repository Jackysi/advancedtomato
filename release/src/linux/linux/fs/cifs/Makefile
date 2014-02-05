#
# Makefile for Linux CIFS VFS client 
#
O_TARGET := cifs.o

obj-y := nterr.o md4.o md5.o netmisc.o smbdes.o smbencrypt.o asn1.o cifs_unicode.o cifsencrypt.o cifs_debug.o sess.o link.o cifssmb.o connect.o misc.o dir.o inode.o transport.o cifsfs24.o file.o readdir.o ioctl.o

obj-m := $(O_TARGET)

include $(TOPDIR)/Rules.make
