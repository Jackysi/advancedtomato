export TPROFILE := N

export LINUXDIR := $(SRCBASE)/linux/linux

export EXTRACFLAGS := -DBCMWPA2 -fno-delete-null-pointer-checks -funit-at-a-time -Wno-pointer-sign -mtune=mips32 -mips32

# CONFIG_LINUX26 is not set
