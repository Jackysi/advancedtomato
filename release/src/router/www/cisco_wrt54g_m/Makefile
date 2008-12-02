#
# *********************************************************
# *   Copyright 2003, CyberTAN  Inc.  All Rights Reserved *
# *********************************************************

# This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
# the contents of this file may not be disclosed to third parties,
# copied or duplicated in any form without the prior written
# permission of CyberTAN Inc.

# This software should be used as a reference only, and it not
# intended for production use!


# THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
#

ifneq ($(wildcard $(SRCBASE)/cy_conf.mak),)
  include $(SRCBASE)/cy_conf.mak
endif

all:

install:
	rm -rf $(INSTALLDIR)/www
	install -d $(INSTALLDIR)/www
	install -m 644 $(wildcard *.asp) $(wildcard *.jpg) $(wildcard *.gif) $(wildcard *.css) $(wildcard *.js) $(wildcard *.htm) $(wildcard *.xml) $(INSTALLDIR)/www
	cp -a image $(INSTALLDIR)/www
	cp -a help $(INSTALLDIR)/www
ifeq ($(MULTILANG_SUPPORT),1)
	rm -rf $(INSTALLDIR)/www/help
	rm -rf $(INSTALLDIR)/www/lang_pack

	if grep -q "LANG_SEL=SC" $(SRCBASE)/include/code_pattern.h ; then \
		cp -a ../lang_pack/sc_lang_pack $(INSTALLDIR)/www/lang_pack ; \
		cp -a ../lang_pack/all_help/sc_help $(INSTALLDIR)/www/help ; \
		cp -a ../lang_pack/sc_image/*.gif $(INSTALLDIR)/www/image/ ; \
	elif grep -q "LANG_SEL=SP" $(SRCBASE)/include/code_pattern.h ; then \
		cp -a ../lang_pack/sp_lang_pack $(INSTALLDIR)/www/lang_pack ; \
		cp -a ../lang_pack/all_help/sp_help $(INSTALLDIR)/www/help ; \
	elif grep -q "LANG_SEL=SW" $(SRCBASE)/include/code_pattern.h ; then \
		cp -a ../lang_pack/sw_lang_pack $(INSTALLDIR)/www/lang_pack ; \
		cp -a ../lang_pack/all_help/sw_help $(INSTALLDIR)/www/help ; \
	elif grep -q "LANG_SEL=IT" $(SRCBASE)/include/code_pattern.h ; then \
		cp -a ../lang_pack/it_lang_pack $(INSTALLDIR)/www/lang_pack ; \
		cp -a ../lang_pack/all_help/it_help $(INSTALLDIR)/www/help ; \
	elif grep -q "LANG_SEL=FR" $(SRCBASE)/include/code_pattern.h ; then \
		cp -a ../lang_pack/fr_lang_pack $(INSTALLDIR)/www/lang_pack ; \
		cp -a ../lang_pack/all_help/fr_help $(INSTALLDIR)/www/help ; \
	elif grep -q "LANG_SEL=DE" $(SRCBASE)/include/code_pattern.h ; then \
		cp -a ../lang_pack/de_lang_pack $(INSTALLDIR)/www/lang_pack ; \
		cp -a ../lang_pack/all_help/de_help $(INSTALLDIR)/www/help ; \
		cp -a ../lang_pack/de_image/*.gif $(INSTALLDIR)/www/image/ ; \
	elif grep -q "LANG_SEL=EN" $(SRCBASE)/include/code_pattern.h ; then \
		cp -a ../lang_pack/en_lang_pack $(INSTALLDIR)/www/lang_pack ; \
		cp -a ../lang_pack/all_help/en_help $(INSTALLDIR)/www/help ; \
	fi	
else
	rm -rf $(INSTALLDIR)/www/lang_pack
endif


ifeq ($(DDM_SUPPORT),1)
	cp -a verizon $(INSTALLDIR)/www
else
	rm -rf $(INSTALLDIR)/www/verizon
endif

ifneq ($(AOL_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/aol.asp
	rm -f $(INSTALLDIR)/www/aol_error.asp
endif

ifneq ($(HSIAB_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/HotSpot_*
	rm -f $(INSTALLDIR)/www/Config.asp
	rm -f $(INSTALLDIR)/www/New_device.asp
	rm -f $(INSTALLDIR)/www/Old_device.asp
	rm -f $(INSTALLDIR)/www/Register*
endif

ifneq ($(L2TP_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/index_l2tp.asp
endif

ifneq ($(HEARTBEAT_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/index_heartbeat.asp
endif

ifneq ($(MPPPOE_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/index_mpppoe.asp
	rm -f $(INSTALLDIR)/www/Status_mpppoe.asp
endif

ifneq ($(PARENTAL_CONTROL_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/Parental_Control.asp
endif

ifneq ($(BACKUP_RESTORE_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/Backup_Restore.asp
endif

ifneq ($(PORT_TRIGGER_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/PortTriggerTable.asp
endif

ifneq ($(UPNP_FORWARD_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/Forward_UPnP.asp
endif

ifneq ($(WAKE_ON_LAN_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/Wol.asp
endif

ifneq ($(SAMBA_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/SMB*.asp
endif

ifneq ($(HW_QOS_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/QoS.asp
endif

ifneq ($(PERFORMANCE_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/Status_Performance.asp
endif
ifneq ($(WL_STA_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/Site_Suvery.asp
endif

ifneq ($(DDNS3322_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/3322.asp
endif

ifneq ($(PEANUTHULL_SUPPORT),1)
	rm -f $(INSTALLDIR)/www/peanuthull.asp
endif

	$(MAKE) -C $(SRCBASE)/router/www/common/ install

webcomp:
	webcomp . list > webrom.c

clean:
