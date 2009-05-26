#
# Makefile for the netfilter modules on top of IPv4.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definition is now in the main makefile...

O_TARGET := netfilter.o

export-objs = ip_conntrack_standalone.o ip_fw_compat.o ip_nat_standalone.o ip_tables.o arp_tables.o

# Multipart objects.
list-multi		:= ip_conntrack.o iptable_nat.o ipfwadm.o ipchains.o

# objects for the conntrack and NAT core (used by standalone and backw. compat)
ip_nf_conntrack-objs	:= ip_conntrack_core.o ip_conntrack_proto_generic.o ip_conntrack_proto_tcp.o ip_conntrack_proto_udp.o ip_conntrack_proto_icmp.o
ip_nf_nat-objs		:= ip_nat_core.o ip_nat_helper.o ip_nat_proto_unknown.o ip_nat_proto_tcp.o ip_nat_proto_udp.o ip_nat_proto_icmp.o

# objects for the standalone - connection tracking / NAT
ip_conntrack-objs	:= ip_conntrack_standalone.o $(ip_nf_conntrack-objs)
iptable_nat-objs	:= ip_nat_standalone.o ip_nat_rule.o $(ip_nf_nat-objs)

# objects for backwards compatibility mode
ip_nf_compat-objs	:= ip_fw_compat.o ip_fw_compat_redir.o ip_fw_compat_masq.o $(ip_nf_conntrack-objs) $(ip_nf_nat-objs)

ipfwadm-objs		:= $(ip_nf_compat-objs) ipfwadm_core.o
ipchains-objs		:= $(ip_nf_compat-objs) ipchains_core.o

# connection tracking
obj-$(CONFIG_IP_NF_CONNTRACK) += ip_conntrack.o
 
# H.323 support
obj-$(CONFIG_IP_NF_H323) += ip_conntrack_h323.o
ifdef CONFIG_IP_NF_H323
	export-objs += ip_conntrack_h323.o
endif
obj-$(CONFIG_IP_NF_NAT_H323) += ip_nat_h323.o


# connection tracking protocol helpers
obj-$(CONFIG_IP_NF_CT_PROTO_GRE) += ip_conntrack_proto_gre.o
ifdef CONFIG_IP_NF_CT_PROTO_GRE
	export-objs += ip_conntrack_proto_gre.o
endif

# NAT protocol helpers
obj-$(CONFIG_IP_NF_NAT_PROTO_GRE) += ip_nat_proto_gre.o

# connection tracking protocol helpers
obj-$(CONFIG_IP_NF_CT_PROTO_ESP) += ip_conntrack_proto_esp.o
ifdef CONFIG_IP_NF_CT_PROTO_ESP
	export-objs += ip_conntrack_proto_esp.o
endif
 
# NAT protocol helpers
obj-$(CONFIG_IP_NF_NAT_PROTO_ESP) += ip_nat_proto_esp.o

# connection tracking helpers
obj-$(CONFIG_IP_NF_QUAKE3) += ip_conntrack_quake3.o
ifdef CONFIG_IP_NF_NAT_QUAKE3
    export-objs += ip_conntrack_quake3.o
endif
obj-$(CONFIG_IP_NF_MMS) += ip_conntrack_mms.o
ifdef CONFIG_IP_NF_NAT_MMS
	export-objs += ip_conntrack_mms.o
endif
obj-$(CONFIG_IP_NF_PPTP) += ip_conntrack_pptp.o
ifdef CONFIG_IP_NF_NAT_PPTP
	export-objs += ip_conntrack_pptp.o
endif
obj-$(CONFIG_IP_NF_SIP) += ip_conntrack_sip.o
ifdef CONFIG_IP_NF_NAT_SIP
	export-objs += ip_conntrack_sip.o
endif
obj-$(CONFIG_IP_NF_TFTP) += ip_conntrack_tftp.o
obj-$(CONFIG_IP_NF_FTP) += ip_conntrack_ftp.o
ifdef CONFIG_IP_NF_NAT_FTP
	export-objs += ip_conntrack_ftp.o
endif
obj-$(CONFIG_IP_NF_IRC) += ip_conntrack_irc.o
ifdef CONFIG_IP_NF_NAT_IRC
	export-objs += ip_conntrack_irc.o
endif

# rtsp protocol support
obj-$(CONFIG_IP_NF_RTSP) += ip_conntrack_rtsp.o
ifdef CONFIG_IP_NF_NAT_RTSP
	export-objs += ip_conntrack_rtsp.o
endif
obj-$(CONFIG_IP_NF_NAT_RTSP) += ip_nat_rtsp.o

# NAT helpers 
obj-$(CONFIG_IP_NF_NAT_CUSEEME) += ip_nat_cuseeme.o
obj-$(CONFIG_IP_NF_NAT_PPTP) += ip_nat_pptp.o
obj-$(CONFIG_IP_NF_NAT_SIP) += ip_nat_sip.o
obj-$(CONFIG_IP_NF_NAT_TFTP) += ip_nat_tftp.o
obj-$(CONFIG_IP_NF_NAT_FTP) += ip_nat_ftp.o
obj-$(CONFIG_IP_NF_NAT_IRC) += ip_nat_irc.o
obj-$(CONFIG_IP_NF_NAT_QUAKE3) += ip_nat_quake3.o
obj-$(CONFIG_IP_NF_NAT_MMS) += ip_nat_mms.o

# generic IP tables 
obj-$(CONFIG_IP_NF_IPTABLES) += ip_tables.o

# the three instances of ip_tables
obj-$(CONFIG_IP_NF_FILTER) += iptable_filter.o
obj-$(CONFIG_IP_NF_MANGLE) += iptable_mangle.o
obj-$(CONFIG_IP_NF_NAT) += iptable_nat.o

# matches
obj-$(CONFIG_IP_NF_MATCH_HELPER) += ipt_helper.o
obj-$(CONFIG_IP_NF_MATCH_LIMIT) += ipt_limit.o
obj-$(CONFIG_IP_NF_MATCH_IPP2P) += ipt_ipp2p.o
obj-$(CONFIG_IP_NF_MATCH_GEOIP) += ipt_geoip.o
obj-$(CONFIG_IP_NF_MATCH_QUOTA) += ipt_quota.o
obj-$(CONFIG_IP_NF_MATCH_IPRANGE) += ipt_iprange.o
obj-$(CONFIG_IP_NF_MATCH_MARK) += ipt_mark.o
obj-$(CONFIG_IP_NF_POOL) += ipt_pool.o ip_pool.o
obj-$(CONFIG_IP_NF_MATCH_MAC) += ipt_mac.o
obj-$(CONFIG_IP_NF_MATCH_PKTTYPE) += ipt_pkttype.o
obj-$(CONFIG_IP_NF_MATCH_MULTIPORT) += ipt_multiport.o
obj-$(CONFIG_IP_NF_MATCH_MPORT) += ipt_mport.o
obj-$(CONFIG_IP_NF_MATCH_OWNER) += ipt_owner.o
obj-$(CONFIG_IP_NF_MATCH_TOS) += ipt_tos.o
obj-$(CONFIG_IP_NF_MATCH_RECENT) += ipt_recent.o
obj-$(CONFIG_IP_NF_MATCH_ACCOUNT) += ipt_account.o
obj-$(CONFIG_IP_NF_MATCH_CONDITION) += ipt_condition.o
obj-$(CONFIG_IP_NF_MATCH_TIME) += ipt_time.o
obj-$(CONFIG_IP_NF_MATCH_ECN) += ipt_ecn.o
obj-$(CONFIG_IP_NF_MATCH_DSCP) += ipt_dscp.o
obj-$(CONFIG_IP_NF_MATCH_AH_ESP) += ipt_ah.o ipt_esp.o
obj-$(CONFIG_IP_NF_MATCH_LENGTH) += ipt_length.o
obj-$(CONFIG_IP_NF_MATCH_U32) += ipt_u32.o
obj-$(CONFIG_IP_NF_MATCH_TTL) += ipt_ttl.o
obj-$(CONFIG_IP_NF_MATCH_STATE) += ipt_state.o
obj-$(CONFIG_IP_NF_MATCH_CONNLIMIT) += ipt_connlimit.o
obj-$(CONFIG_IP_NF_MATCH_CONNMARK) += ipt_connmark.o
obj-$(CONFIG_IP_NF_MATCH_CONNTRACK) += ipt_conntrack.o
obj-$(CONFIG_IP_NF_MATCH_UNCLEAN) += ipt_unclean.o
obj-$(CONFIG_IP_NF_MATCH_STRING) += ipt_string.o
obj-$(CONFIG_IP_NF_MATCH_WEBSTR) += ipt_webstr.o
obj-$(CONFIG_IP_NF_MATCH_TCPMSS) += ipt_tcpmss.o
obj-$(CONFIG_IP_NF_MATCH_LAYER7) += ipt_layer7.o
obj-$(CONFIG_IP_NF_MATCH_WEB) += ipt_web.o
obj-$(CONFIG_IP_NF_MATCH_MACSAVE) += ipt_macsave.o
obj-$(CONFIG_IP_NF_MATCH_EXP) += ipt_exp.o
obj-$(CONFIG_IP_NF_MATCH_BCOUNT) += ipt_bcount.o

# targets
obj-$(CONFIG_IP_NF_TARGET_REJECT) += ipt_REJECT.o
obj-$(CONFIG_IP_NF_TARGET_MIRROR) += ipt_MIRROR.o
obj-$(CONFIG_IP_NF_TARGET_CLASSIFY) += ipt_CLASSIFY.o
obj-$(CONFIG_IP_NF_TARGET_TOS) += ipt_TOS.o
obj-$(CONFIG_IP_NF_TARGET_ECN) += ipt_ECN.o
obj-$(CONFIG_IP_NF_TARGET_DSCP) += ipt_DSCP.o
obj-$(CONFIG_IP_NF_TARGET_MARK) += ipt_MARK.o
obj-$(CONFIG_IP_NF_TARGET_IMQ) += ipt_IMQ.o
obj-$(CONFIG_IP_NF_TARGET_MASQUERADE) += ipt_MASQUERADE.o
ifdef CONFIG_IP_NF_TARGET_MASQUERADE
	export-objs += ipt_MASQUERADE.o
endif
obj-$(CONFIG_IP_NF_TARGET_REDIRECT) += ipt_REDIRECT.o
obj-$(CONFIG_IP_NF_TARGET_ROUTE) += ipt_ROUTE.o
obj-$(CONFIG_IP_NF_NAT_SNMP_BASIC) += ip_nat_snmp_basic.o
obj-$(CONFIG_IP_NF_TARGET_LOG) += ipt_LOG.o
obj-$(CONFIG_IP_NF_TARGET_CONNMARK) += ipt_CONNMARK.o
obj-$(CONFIG_IP_NF_TARGET_TTL) += ipt_TTL.o
obj-$(CONFIG_IP_NF_TARGET_ULOG) += ipt_ULOG.o
obj-$(CONFIG_IP_NF_TARGET_TCPMSS) += ipt_TCPMSS.o
obj-$(CONFIG_IP_NF_AUTOFW) += ip_autofw.o
obj-$(CONFIG_IP_NF_TARGET_TRIGGER) += ipt_TRIGGER.o
obj-$(CONFIG_IP_NF_TARGET_MACSAVE) += ipt_MACSAVE.o
obj-$(CONFIG_IP_NF_TARGET_BCOUNT) += ipt_BCOUNT.o

# generic ARP tables
obj-$(CONFIG_IP_NF_ARPTABLES) += arp_tables.o

# just filtering instance of ARP tables for now
obj-$(CONFIG_IP_NF_ARPFILTER) += arptable_filter.o

# backwards compatibility 
obj-$(CONFIG_IP_NF_COMPAT_IPCHAINS) += ipchains.o
obj-$(CONFIG_IP_NF_COMPAT_IPFWADM) += ipfwadm.o

obj-$(CONFIG_IP_NF_QUEUE) += ip_queue.o

obj-$(CONFIG_IP_NF_TOMATOCT) += tomato_ct.o

include $(TOPDIR)/Rules.make

ip_conntrack.o: $(ip_conntrack-objs)
	$(LD) -r -o $@ $(ip_conntrack-objs)

iptable_nat.o: $(iptable_nat-objs)
	$(LD) -r -o $@ $(iptable_nat-objs)

ipfwadm.o: $(ipfwadm-objs)
	$(LD) -r -o $@ $(ipfwadm-objs)

ipchains.o: $(ipchains-objs)
	$(LD) -r -o $@ $(ipchains-objs)
