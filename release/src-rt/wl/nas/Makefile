#
# 802.1x Linux Builds Makefile
#
# Copyright (C) 2002 Broadcom Corporation
#
# $Id: Makefile 241388 2011-02-18 03:33:22Z stakita $
#
# targets/builds
BUILDS = nas

${BUILDS} ::
	$(MAKE) -C $@

# clean up
BUILDS-CLEAN = ${BUILDS:=-clean}

clean : ${BUILDS-CLEAN}

${BUILDS-CLEAN} ::
	$(MAKE) -C ${@:-clean=} clean

# default build
ifneq ($(wildcard ./prebuilt),)
all:
	-cp prebuilt/nas nas/nas 
else
all : ${BUILDS}
endif

# install
BUILDS-INSTALL = ${BUILDS:=-install}

install : ${BUILDS-INSTALL}

${BUILDS-INSTALL} ::
	$(MAKE) -C ${@:-install=} install
