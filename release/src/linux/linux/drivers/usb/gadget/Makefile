#
# Makefile for USB peripheral controller and gadget drivers
# for kbuild 2.4
#

# for static linking
O_TARGET	:= built-in.o

ifdef CONFIG_USB_GADGET_CONTROLLER

list-multi			:= g_zero.o g_ether.o g_simple.o g_rndis.o gadgetfs.o

obj-$(CONFIG_USB_NET2280)	+= net2280.o
obj-$(CONFIG_USB_PXA2XX)	+= pxa2xx_udc.o

# only one of these may be statically linked ...
controller-$(CONFIG_USB_NET2280)	+= net2280.o
controller-$(CONFIG_USB_PXA2XX)		+= pxa2xx_udc.o

# ... and only one of these, too; kbuild/kconfig don't help though.
g_zero-objs			:= zero.o usbstring.o
obj-$(CONFIG_USB_ZERO)		+= g_zero.o

g_ether-objs			:= ether.o usbstring.o
obj-$(CONFIG_USB_ETH)		+= g_ether.o

g_simple-objs			:= usbdev_gadget.o usbdev_simple.o
obj-$(CONFIG_USB_SIMPLE)	+= g_simple.o

g_rndis-objs			:= usbdev_gadget.o usbdev_rndis.o rndis.o rndis_linux.o
obj-$(CONFIG_USB_RNDIS)		+= g_rndis.o

gadgetfs-objs			:= inode.o usbstring.o
obj-$(CONFIG_USB_GADGETFS)	+= gadgetfs.o

export-objs :=			$(controller-y) $(controller-m)

endif

EXTRA_CFLAGS	+= -fshort-wchar

ifdef CONFIG_USBDEV

EXTRA_CFLAGS += -DCONFIG_USBRNDIS_RETAIL -DBCMWPA2

list-multi			+= usbdev-simple.o usbdev-rndis.o

# ... and only one of these, too; kbuild/kconfig don't help though.
usbdev-simple-objs		:= usbdev_sb.o usbdev_linux.o usbdev_simple.o
obj-$(CONFIG_USB_SIMPLE)	+= usbdev-simple.o

usbdev-rndis-objs		:= usbdev_sb.o usbdev_linux.o usbdev_rndis.o rndis.o rndis_linux.o
obj-$(CONFIG_USB_RNDIS)		+= usbdev-rndis.o

endif

# Search for sources or objects under src/usbdev
ifneq ($(wildcard $(SRCBASE)/usbdev/*.c),)
EXTRA_CFLAGS	+= -I$(SRCBASE)/usbdev
vpath %.c $(SRCBASE)/usbdev $(SRCBASE)/shared
else
g_simple-objs		:= $(foreach obj,$(g_simple-objs),$(SRCBASE)/usbdev/linux/$(obj))
g_rndis-objs		:= $(foreach obj,$(g_rndis-objs),$(SRCBASE)/usbdev/linux/$(obj))
usbdev-simple-objs	:= $(foreach obj,$(usbdev-simple-objs),$(SRCBASE)/usbdev/linux/$(obj))
usbdev-rndis-objs	:= $(foreach obj,$(usbdev-rndis-objs),$(SRCBASE)/usbdev/linux/$(obj))
endif

include $(TOPDIR)/Rules.make

g_zero.o: $(g_zero-objs)
	$(LD) -r -o $@ $(g_zero-objs)
g_ether.o: $(g_ether-objs)
	$(LD) -r -o $@ $(g_ether-objs)
g_simple.o: $(g_simple-objs)
	$(LD) -r -o $@ $(g_simple-objs)
g_rndis.o: $(g_rndis-objs)
	$(LD) -r -o $@ $(g_rndis-objs)
gadgetfs.o: $(gadgetfs-objs)
	$(LD) -r -o $@ $(gadgetfs-objs)
usbdev-simple.o: $(usbdev-simple-objs)
	$(LD) -r -o $@ $(usbdev-simple-objs)
usbdev-rndis.o: $(usbdev-rndis-objs)
	$(LD) -r -o $@ $(usbdev-rndis-objs)
