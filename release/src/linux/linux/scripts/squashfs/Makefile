CC=gcc
CXX=g++
INCLUDEDIR = .
LZMADIR = $(SRCBASE)/lzma/CPP/7zip/Compress
LZMALIB = $(LZMADIR)/LZMA_Lib
LZMAPATH = $(LZMADIR)/LZMA_Alone

CFLAGS := -I$(INCLUDEDIR) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -O2

all: liblzma.a mksquashfs unsquashfs mksquashfs-lzma lzma

mksquashfs: mksquashfs.o read_fs.o sort.o
	$(CC) mksquashfs.o read_fs.o sort.o -lz -o $@

mksquashfs-lzma: liblzma.a mksquashfs.o read_fs.o sort.o
	$(CXX) mksquashfs.o read_fs.o sort.o -L$(LZMALIB) -llzma -o $@

mksquashfs.o: mksquashfs.c squashfs_fs.h mksquashfs.h global.h sort.h

read_fs.o: read_fs.c squashfs_fs.h read_fs.h global.h

sort.o: sort.c squashfs_fs.h global.h sort.h

unsquashfs: unsquashfs.o
	$(CC) unsquashfs.o -lz -o $@

unsquashfs.o: unsquashfs.c squashfs_fs.h read_fs.h global.h

liblzma.a:
	$(MAKE) -C $(LZMALIB)

lzma:
	$(MAKE) -C $(LZMAPATH) -f makefile.gcc

clean:
	find . -iname "*.o" -exec rm -f {} \;
	find . -iname "*.a" -exec rm -f {} \;
	rm -f mksquashfs unsquashfs mksquashfs-lzma $(LZMAPATH)/lzma

.PHONY: lzma
