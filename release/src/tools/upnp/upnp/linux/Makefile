#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.8 2005/03/07 08:35:32 kanki Exp $
#

ifndef DEBUG
#export DEBUG=1
endif
SRCBASE=../../../..

CC = $(TARGET_PREFIX)gcc
AS = $(TARGET_PREFIX)as
AR = $(TARGET_PREFIX)ar
LD = $(TARGET_PREFIX)ld
CCPP = $(TARGET_PREFIX)g++
COMPILER = $(TARGET_PREFIX)g++
STRIP = $(TARGET_PREFIX)strip
SIZE = $(TARGET_PREFIX)size
RANLIB = $(TARGET_PREFIX)ranlib

VPATH.h=.:..:../../include:$(SRCBASE)/include:$(SRCBASE)/router/shared

vpath %.c .:..:$(SRCBASE)/router/shared:$(SRCBASE)/shared/netconf
vpath %.h $(VPATH.h)

ifeq ($(DEBUG),1)
CFLAGS += -g -DDEBUG -UNDEBUG -DBCMDBG
else
CFLAGS += -O2 -DNDEBUG
endif
CFLAGS += -I$(subst :, -I,$(VPATH.h))
CFLAGS += -Wall -Wunused 
#CFLAGS += --trace-includes
OBJDIR=obj

SOURCES = upnp.c ssdp.c http.c gena.c soap.c \
	  service.c device.c  \
	  linux_osl.c uio.c

SOURCES.OBJ := $(SOURCES)
SOURCES.OBJ := $(patsubst %.c,$(OBJDIR)/%.o,$(SOURCES.OBJ))

vpath %.c .:..:
vpath %.h .:..:../../include

TARGET = libupnp.a
LIBS = 

all : $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(SOURCES.OBJ): upnp.h  $(SRCBASE)/include/cyutils.h $(SRCBASE)/include/code_pattern.h

$(OBJDIR)/%.o:	%.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<


$(TARGET) : $(SOURCES.OBJ)
	$(AR) -rcs $(TARGET) $(SOURCES.OBJ) 
	$(RANLIB) $(TARGET)
	$(SIZE)  $(TARGET)

clean: 
	rm -rf obj 



timertest: $(OBJDIR)/linux_timer.o $(OBJDIR)/timertest.o
	$(CC) $(OBJDIR)/linux_timer.o $(OBJDIR)/timertest.o -o timertest
