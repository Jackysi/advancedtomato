include ../common.mak

CFLAGS = -Os -Wall $(EXTRACFLAGS) -fPIC
CFLAGS += -I. -I$(SRCBASE)/include -I$(SRCBASE)/

LDFLAGS =

OBJS = shutils.o wl.o wl_linux.o linux_timer.o defaults.o id.o
OBJS += misc.o led.o version.o base64.o files.o strings.o process.o

ifeq ($(TCONFIG_USB),y)
OBJS += usb.o
OBJS += $(TOP)/busybox/util-linux/volume_id/ext.o
OBJS += $(TOP)/busybox/util-linux/volume_id/util2.o
OBJS += $(TOP)/busybox/util-linux/volume_id/fat.o
OBJS += $(TOP)/busybox/util-linux/volume_id/ntfs.o
OBJS += $(TOP)/busybox/util-linux/volume_id/linux_swap.o
endif

all: libshared.so libshared.a

ifeq ($(CONFIG_BCMWL5),y)
OBJS += wlif_utils.o UdpLib.o
endif

libshared.so: $(OBJS)
	@echo " [shared] LD $@"
	@$(LD) $(LDFLAGS) -shared -o $@ $^

	$(SIZECHECK)
	$(CPTMP)

libshared.a: $(OBJS)
	@echo " [shared] AR $@"
	@rm -f $@
	@$(AR) cruv $@ $^


install: all
	@echo " [shared] Installing as $(INSTALLDIR)/usr/lib/libshared.so"
	@install -d $(INSTALLDIR)/usr/lib
	@install -m 755 libshared.so $(INSTALLDIR)/usr/lib
	@$(STRIP) $(INSTALLDIR)/usr/lib/libshared.so


clean:
	rm -f *.o *.so *.a .*.depend

%.o: %.c .%.depend
	@echo " [shared] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)

