#
# drivers/scsi/aic7xxx/Makefile
#
# Makefile for the Linux aic7xxx SCSI driver.
#

O_TARGET := aic7xxx_drv.o

list-multi	:= aic7xxx.o

obj-$(CONFIG_SCSI_AIC7XXX)	+= aic7xxx.o

#EXTRA_CFLAGS += -g

# Platform Specific Files
obj-aic7xxx = aic7xxx_osm.o aic7xxx_proc.o aic7770_osm.o
#PCI Specific Platform Files
ifeq ($(CONFIG_PCI),y)
obj-aic7xxx += aic7xxx_osm_pci.o
endif
# Core Files
obj-aic7xxx += aic7xxx_core.o aic7xxx_93cx6.o aic7770.o
#PCI Specific Core Files
ifeq ($(CONFIG_PCI),y)
obj-aic7xxx += aic7xxx_pci.o
endif

# Override our module desitnation
MOD_DESTDIR = $(shell cd .. && $(CONFIG_SHELL) $(TOPDIR)/scripts/pathdown.sh)

include $(TOPDIR)/Rules.make

aic7xxx.o: $(obj-aic7xxx)
	$(LD) $(LD_RFLAG) -r -o $@ $(obj-aic7xxx)

ifeq ($(CONFIG_AIC7XXX_BUILD_FIRMWARE),y)
aic7xxx_core.o: aic7xxx_seq.h

$(obj-aic7xxx): aic7xxx_reg.h

aic7xxx_seq.h aic7xxx_reg.h: aic7xxx.seq aic7xxx.reg aicasm/aicasm
	aicasm/aicasm -I. -r aic7xxx_reg.h -o aic7xxx_seq.h aic7xxx.seq
endif

aicasm/aicasm: aicasm/*.[chyl]
	$(MAKE) -C aicasm
