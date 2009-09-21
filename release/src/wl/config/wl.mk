# Helper makefile for building Broadcom wl device driver
# This file maps wl driver feature flags (import) to WLFLAGS and WLFILES (export).
#
# Copyright 2007, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
# $Id: wl.mk,v 1.1.1.1 2008/07/21 09:20:39 james26_jang Exp $

# os-independent config flag -> WLFLAGS and WLFILES mapping

# debug/internal
ifeq ($(DEBUG),1)
	WLFLAGS += -DBCMDBG -DWLTEST
else
	# This is true for mfgtest builds.
	ifeq ($(WLTEST),1)
	WLFLAGS += -DWLTEST -DBCMNVRAMW
	BCMNVRAMW=1
	endif
endif


ifeq ($(BCMDBG_MEM),1)
	WLFLAGS += -DBCMDBG_MEM
endif

ifeq ($(BCMDBG_PKT),1)
	WLFLAGS += -DBCMDBG_PKT
endif

## wl driver common 
#w/wpa
ifeq ($(WL),1)
	WLFILES += wlc.c d11ucode.c wlc_phy.c wlc_rate.c wlc_security.c
	WLFILES += wlc_key.c wlc_event.c wlc_scb.c wlc_rate_sel.c wlc_channel.c
	WLFILES += wlc_bsscfg.c mimophytbls.c lpphytbls.c wlc_scan.c
	ifneq ($(BCMROMOFFLOAD),1)
		WLFILES += bcmwpa.c rc4.c tkhash.c tkmic.c wep.c
	endif
endif

## wl OSL
ifeq ($(WLVX),1)
	WLFILES += wl_vx.c
	WLFILES += bcmstdlib.c
	WLFLAGS += -DWSEC_TXC_ENABLED
endif

ifeq ($(WLBSD),1)
	WLFILES += wl_bsd.c
endif

ifeq ($(WLLX),1)
	WLFILES += wl_linux.c
endif

ifeq ($(WLLXIW),1)
	WLFILES += wl_iw.c
endif

ifeq ($(WLNDIS),1)
	WLFILES += wl_ndis.c
	WLFILES += wl_ndconfig.c
	WLFILES += bcmstdlib.c
	WLFILES += bcmwifi.c
endif

ifeq ($(WLCFE),1)
	WLFILES += wl_cfe.c
endif

ifeq ($(WLRTE),1)
	WLFILES += wl_rte.c
endif


## wl special
# oids

#ifdef BINOSL
	ifeq ($(BINOSL),1)
		WLFLAGS += -DBINOSL
	endif
#endif

## wl features
# ap
ifeq ($(AP),1)
	WLFILES += wlc_ap.c
	WLFILES += wlc_apps.c
	WLFILES += wlc_apcs.c
	WLFLAGS += -DAP
	WLFLAGS += -DMBSS
	WLFLAGS += -DWME_PER_AC_TX_PARAMS -DWME_PER_AC_TUNING
endif

# sta
ifeq ($(STA),1)
	WLFLAGS += -DSTA
endif

# apsta
ifeq ($(APSTA),1)
	WLFLAGS += -DAPSTA
endif
# apsta

# wet
ifeq ($(WET),1)
	WLFLAGS += -DWET
	WLFILES += wlc_wet.c
endif

# mac spoof
ifeq ($(MAC_SPOOF),1)
	WLFLAGS += -DMAC_SPOOF
endif

# IBSS Security Support
ifeq ($(IBSS_WPA2_SUPPORT),1)
	WLFLAGS += -DIBSS_PEER_GROUP_KEY
	WLFLAGS += -DIBSS_WPA2_PSK
	WLFLAGS += -DIBSS_PEER_DISCOVERY_EVENT
endif

# led
ifeq ($(WLLED),1)
	WLFLAGS += -DWLLED
	WLFILES += wlc_led.c
endif

# WME
ifeq ($(WME),1)
	WLFLAGS += -DWME
endif

# WLBA
ifeq ($(WLBA),1)
	WLFLAGS += -DWLBA
	WLFILES += wlc_ba.c
endif

# WLPIO 
ifeq ($(WLPIO),1)
	WLFLAGS += -DWLPIO
	WLFILES += wlc_pio.c
endif

# CRAM
ifeq ($(CRAM),1)
	WLFLAGS += -DCRAM
	WLFILES += wlc_cram.c
endif

# 11H 
ifeq ($(WL11H),1)
	WLFLAGS += -DWL11H
endif

# 11D 
ifeq ($(WL11D),1)
	WLFLAGS += -DWL11D
endif

# DBAND
ifeq ($(DBAND),1)
	WLFLAGS += -DDBAND
endif

# WLRM
ifeq ($(WLRM),1)
	WLFLAGS += -DWLRM
endif

# WLCQ
ifeq ($(WLCQ),1)
	WLFLAGS += -DWLCQ
endif

# WLCNT
ifeq ($(WLCNT),1)
	WLFLAGS += -DWLCNT
endif

# WLCNTSCB
ifeq ($(WLCNTSCB),1)
	WLFLAGS += -DWLCNTSCB
endif

## wl security
# in-driver supplicant
ifeq ($(BCMSUP_PSK),1)
	WLFLAGS += -DBCMSUP_PSK
	WLFILES += wlc_sup.c
	ifneq ($(BCMROMOFFLOAD),1)
		WLFILES += aes.c aeskeywrap.c hmac.c prf.c sha1.c
		##NetBSD 2.0 has MD5 and AES built in
		ifneq ($(OSLBSD),1)
			WLFILES += md5.c rijndael-alg-fst.c
		endif
	endif
	WLFILES += passhash.c
endif

# bcmccx

# BCMWPA2
ifeq ($(BCMWPA2),1)
	WLFLAGS += -DBCMWPA2
endif

# Soft AES CCMP
ifeq ($(BCMCCMP),1)
	WLFLAGS += -DBCMCCMP
	ifneq ($(BCMROMOFFLOAD),1)
		WLFILES += aes.c
		##BSD has  AES built in
		ifneq ($(BSD),1)
			WLFILES +=rijndael-alg-fst.c
		endif
	endif
endif

# FIPS
ifeq ($(WLFIPS),1)
	WLFLAGS += -DWLFIPS
	WLFILES += wl_ndfips.c
	ifneq ($(BCMROMOFFLOAD),1)
		WLFILES += aes.c
	endif
endif

# BCMDMA64
ifeq ($(BCMDMA64),1)
	WLFLAGS += -DBCMDMA64
endif

## wl over jtag
#ifdef BCMJTAG
	ifeq ($(BCMJTAG),1)
		WLFLAGS += -DBCMJTAG -DBCMSLTGT
		WLFILES += bcmjtag.c bcmjtag_linux.c ejtag.c jtagm.c
	endif
#endif

ifeq ($(WLAMSDU),1)
	WLFLAGS += -DWLAMSDU
	WLFILES += wlc_amsdu.c
endif

ifeq ($(WLAMSDU_SWDEAGG),1)
	WLFLAGS += -DWLAMSDU_SWDEAGG
endif

ifeq ($(WLAMPDU),1)
	WLFLAGS += -DWLAMPDU
	WLFILES += wlc_ampdu.c
endif

ifeq ($(WOWL),1)
	WLFLAGS += -DWOWL
	WLFILES += d11wakeucode.c wlc_wowl.c
endif

ifeq ($(WLDPT),1)
	WLFLAGS += -DWLDPT
	WLFILES += wlc_dpt.c
endif


## --- which buses

# silicon backplane

ifeq ($(BCMSBBUS),1)
	WLFLAGS += -DBCMBUSTYPE=SB_BUS
endif


# sdio


# AP with SDSTD
ifeq ($(WLAPSDSTD),1)
	WLFILES += sbutils.c nvramstubs.c bcmsrom.c
endif

## --- basic shared files

ifeq ($(HNDDMA),1)
	WLFILES += hnddma.c
endif

ifeq ($(BCMUTILS),1)
	WLFILES += bcmutils.c
endif

ifeq ($(BCMSROM),1)
	WLFILES += bcmsrom.c bcmotp.c
endif

ifeq ($(SBUTILS),1)
	WLFILES += sbutils.c hndpmu.c
endif

ifeq ($(SBMIPS),1)
	WLFILES += hndmips.c hndchipc.c
endif

ifeq ($(SBSDRAM),1)
	WLFILES += sbsdram.c
endif

ifeq ($(SBPCI),1)
	WLFILES += hndpci.c
endif

ifeq ($(SFLASH),1)
	WLFILES += sflash.c
endif

ifeq ($(FLASHUTL),1)
	WLFILES += flashutl.c
endif


## --- shared OSL
# linux osl
ifeq ($(OSLLX),1)
	WLFILES += linux_osl.c
endif

ifeq ($(OSLLXPCI),1)
	WLFILES += linux_pci.c
endif

# vx osl
ifeq ($(OSLVX),1)
	WLFILES += vx_osl.c
	WLFILES += bcmallocache.c
endif

# bsd osl
ifeq ($(OSLBSD),1)
	WLFILES += bsd_osl.c nvramstubs.c
endif

ifeq ($(OSLCFE),1)
	WLFILES += cfe_osl.c
endif

ifeq ($(OSLRTE),1)
	WLFILES += hndrte_osl.c
endif

ifeq ($(OSLNDIS),1)
	WLFILES += ndshared.c ndis_osl.c
endif

ifeq ($(CONFIG_USBRNDIS_RETAIL),1)
	WLFLAGS += -DCONFIG_USBRNDIS_RETAIL
	WLFILES += wl_ndconfig.c
	WLFILES += bcmwifi.c
endif

ifeq ($(NVRAM),1)
	WLFILES += nvram.c
endif

ifeq ($(NVRAMVX),1)
	WLFILES += nvram_rw.c
endif

ifeq ($(BCMNVRAMR),1)
	WLFILES += nvram_ro.c sflash.c bcmotp.c
	WLFLAGS += -DBCMNVRAMR
else
	ifeq ($(BCMNVRAMW),1)
		WLFILES += bcmotp.c
	endif
endif

## --- DSLCPE
ifeq ($(DSLCPE),1)
	WLFILES += wl_linux_dslcpe.c
	WLFLAGS += -DDSLCPE
	WLFLAGS += -DDSLCPE_DELAY
endif

ifeq ($(WLDIAG),1)
	WLFLAGS += -DWLDIAG
	WLFILES += wlc_diag.c
endif

ifeq ($(WLTIMER),1)
	WLFLAGS += -DWLTIMER
endif

ifneq ($(BCMDBG),1)
	ifeq ($(WLTINYDUMP),1)
		WLFLAGS += -DWLTINYDUMP
	endif
endif

ifeq ($(BCMQT),1)
  # Set flag to indicate emulated chip
  WLFLAGS += -DBCMSLTGT -DBCMQT
  ifeq ($(WLRTE),1)
    # Use of RTE implies embedded (CPU emulated)
    WLFLAGS += -DBCMQT_CPU
  endif
endif

ifeq ($(BCM4312),1)
  WLFLAGS += -DBCM4312
endif

ifeq ($(WLPFN),1)
	WLFLAGS += -DWLPFN
	WLFILES += wl_pfn.c
	ifeq ($(WLPFN_AUTO_CONNECT),1)
		WLFLAGS += -DWLPFN_AUTO_CONNECT
	endif
endif

ifeq ($(TOE),1)
	WLFLAGS += -DTOE
	WLFILES += wl_toe.c
endif

ifeq ($(ARPOE),1)
	WLFLAGS += -DARPOE
	WLFILES += wl_arpoe.c
endif

#wlinfo:
#	@echo "WLFLAGS=\"$(WLFLAGS)\""
#	@echo "WLFILES=\"$(WLFILES)\""
