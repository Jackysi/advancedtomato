export TPROFILE := N

export LINUXDIR := $(SRCBASE)/linux/linux-2.6

export EXTRACFLAGS := -DLINUX26 -DCONFIG_BCMWL5 -DBCMWPA2 -pipe -mips32 -mtune=mips32 -funit-at-a-time -Wno-pointer-sign

export CONFIG_LINUX26=y
export CONFIG_BCMWL5=y
