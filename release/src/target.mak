export TPROFILE := G

export LINUXDIR := $(SRCBASE)/linux/linux

export EXTRACFLAGS := -DBCMWPA2 -fno-delete-null-pointer-checks -mtune=mips32 -mips32

# CONFIG_LINUX26 is not set
