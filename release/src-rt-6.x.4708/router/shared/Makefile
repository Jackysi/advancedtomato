include ../common.mak

CFLAGS = -Os -Wall $(EXTRACFLAGS) -fPIC
CFLAGS += -I$(TOP)/shared -I$(SRCBASE)/include
CFLAGS += -I$(LINUXDIR)/include

ifeq ($(TCONFIG_BCMWL6),y)
CFLAGS += -I$(SRCBASE)/shared/bcmwifi/include
CFLAGS += -I$(SRCBASE)/common/include
else
CFLAGS += -I$(SRCBASE)/shared
endif

CFLAGS += -DTRAFFIC_MGMT

ifeq (2_6_36,$(LINUX_VERSION))
CFLAGS += -DAUTOFW_PORT_DEPRECATED
endif

ifeq ($(TCONFIG_BCMSMP),y)
CFLAGS += -DSMP
endif

ifeq ($(TCONFIG_DHDAP),y)
export CFLAGS += -D__CONFIG_DHDAP__
endif

ifeq ($(TCONFIG_BCMBSD),y)
export CFLAGS += -DBCM_BSD
endif

OBJS = shutils.o wl.o wl_linux.o linux_timer.o defaults.o id.o
OBJS += misc.o led.o version.o base64.o files.o strings.o process.o
OBJS += nvparse.o
ifeq ($(TCONFIG_BCM7),y)
OBJS += et_linux.o bcmwifi_channels.o
endif
ifeq ($(TCONFIG_USB),y)
OBJS += usb.o
OBJS += $(TOP)/busybox/util-linux/volume_id/ext.o
OBJS += $(TOP)/busybox/util-linux/volume_id/util2.o
OBJS += $(TOP)/busybox/util-linux/volume_id/fat.o
OBJS += $(TOP)/busybox/util-linux/volume_id/ntfs.o
OBJS += $(TOP)/busybox/util-linux/volume_id/exfat.o
OBJS += $(TOP)/busybox/util-linux/volume_id/linux_swap.o
endif

ifeq ($(CONFIG_BCMWL5),y)
OBJS += wlif_utils.o UdpLib.o bcmutils.o
OBJS += bcmwifi_channels.o
endif

ifeq ($(TCONFIG_BCMARM),y)
OBJS += ifaddrs.o
endif

vpath %.c $(SRCBASE)/shared/ $(SRCBASE)/shared/bcmwifi/src $(WLAN_ComponentSrcDirs)

all: libshared.so libshared.a


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

