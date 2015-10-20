include ../../common.mak

CC ?= $(CROSS_COMPILE)gcc
LD ?= $(CROSS_COMPILE)ld
AR ?= $(CROSS_COMPILE)ar
RANLIB ?= $(CROSS_COMPILE)ranlib

CFLAGS  := --std=gnu99 -g -Os $(EXTRACFLAGS)
CFLAGS  += -Wall -Wmissing-declarations
CFLAGS  += -ffunction-sections -fdata-sections -I$(TOP)/libjson-c
LDFLAGS += -Wl,--gc-sections -L$(TOP)/libjson-c

TARGET = libubox.a
SOURCES = avl.c avl-cmp.c blob.c blobmsg.c blobmsg_json.c uloop.c usock.c ustream.c ustream-fd.c vlist.c utils.c safe_list.c runqueue.c md5.c kvlist.c

OBJS=$(SOURCES:.c=.o)

# Uncomment next line in case of clock_gettime() in librt
#LIBS=-lrt

PREFIX=/usr

default_target: all
.PHONY : default_target

install: all
	$(INSTALL) -D $(TARGET) $(INSTALLDIR)/$(PREFIX)/lib/$(TARGET)
.PHONY : install

clean:
	rm $(OBJS) $(TARGET)
.PHONY : clean

# The main all target
all: $(TARGET)
.PHONY : all

# Build rule for targets
$(TARGET): $(OBJS)
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

.c.o::
	$(CC) $(CFLAGS) -c $<

# Dependencies
avl-cmp.o: avl-cmp.c avl-cmp.h
avl.o: avl.c avl.h
blob.o: blob.c blob.h
blobmsg.o: blobmsg.c blobmsg.h
blobmsg_json.o: blobmsg_json.c blobmsg_json.h
uloop.o: uloop.c uloop.h utils.h
usock.o: usock.c usock.h
ustream-fd.o: ustream-fd.c ustream.h
ustream.o: ustream.c ustream.h
utils.o: utils.c utils.h
vlist.o: vlist.c vlist.h
safe_list.o: safe_list.c safe_list.h
runqueue.o: runqueue.c runqueue.h
md5.o: md5.c md5.h
kvlist.o: kvlist.c kvlist.h

# Headers
avl.h: list.h
blob.h: utils.h
blobmsg.h: blob.h
blobmsg_json.h: blob.h blobmsg.h utils.h
safe_list.h: list.h utils.h
uloop.h: list.h
ustream.h: uloop.h
vlist.h: avl.h
runqueue.h: list.h safe_list.h uloop.h
kvlist.h: avl.h
