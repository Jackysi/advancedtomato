
O_TARGET := wl.o
obj-m := $(O_TARGET)

ifeq ($(CONFIG_WL_USE_AP),y)
obj-y := $(SRCBASE)/wl/linux/wl_ap.o
else
ifeq ($(CONFIG_WL_USE_APSTA),y)
obj-y := $(SRCBASE)/wl/linux/wl_apsta.o
else
$(error Unsupported)
endif
endif

include $(TOPDIR)/Rules.make
