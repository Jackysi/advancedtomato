#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>

#include <iptables.h>

#include <linux/netfilter_ipv4/ipt_ipp2p.h>

static void
help(void)
{
    printf(
    "IPP2P v%s options:\n"
    " --ipp2p\tGrab all known p2p packets\n"
    " --edk\t\t[TCP&UDP]\tAll known eDonkey/eMule/Overnet packets\n"
    " --dc\t\t[TCP]\t\tAll known Direct Connect packets\n"
    " --kazaa\t[TCP&UDP]\tAll known KaZaA packets\n"
    " --gnu\t\t[TCP&UDP]\tAll known Gnutella packets\n"
    " --bit\t\t[TCP&UDP]\tAll known BitTorrent packets\n"
    " --apple\t[TCP]\t\tAll known AppleJuice packets\n"
    " --winmx\t[TCP]\t\tAll known WinMX\n"
    " --soul\t\t[TCP]\t\tAll known SoulSeek\n"
    " --ares\t\t[TCP]\t\tAll known Ares\n"
    " --pp\t\t[TCP&UDP]\tAll known PPLive/PPStream/UUSee/QQLive packets (Please send feedback to: liulei@syiae.edu.cn)\n"
    " --xunlei\t[TCP]\t\tAll known xunlei/QQCyclone packets\n"
    " EXPERIMENTAL protocols (please send feedback to: ipp2p@ipp2p.org) :\n"
    " --mute\t\t[TCP]\t\tAll known Mute packets\n"
    " --waste\t[TCP]\t\tAll known Waste packets\n"
    " --xdcc\t\t[TCP]\t\tAll known XDCC packets (only xdcc login)\n\n"
    " DEBUG SUPPPORT, use only if you know why\n"
    " --debug\t\tGenerate kernel debug output, THIS WILL SLOW DOWN THE FILTER\n"
    "\nNote that the follwing options will have the same meaning:\n"
    " '--ipp2p' is equal to '--edk --dc --kazaa --gnu --bit --apple --winmx --soul --ares --pp'\n"
    "\nIPP2P was intended for TCP only. Due to increasing usage of UDP we needed to change this.\n"
    "You can now use -p udp to search UDP packets only or without -p switch to search UDP and TCP packets.\n"
    "\nSee README included with this package for more details or visit http://www.ipp2p.org\n"
    "\nExamples:\n"
    " iptables -A FORWARD -m ipp2p --ipp2p -j MARK --set-mark 0x01\n"
    " iptables -A FORWARD -p udp -m ipp2p --kazaa --bit -j DROP\n"
    " iptables -A FORWARD -p tcp -m ipp2p --edk --soul -j DROP\n\n"
    , IPP2P_VERSION);
}

static struct option opts[] = {
        { "ipp2p", 0, 0, '1' },
        { "edk", 0, 0, '2' },
        { "pp", 0, 0, '3' },
	{ "xunlei", 0, 0, '4'},
	{ "dc", 0, 0, '7' },
	{ "gnu", 0, 0, '9' },
	{ "kazaa", 0, 0, 'a' },
	{ "bit", 0, 0, 'b' },
	{ "apple", 0, 0, 'c' },
	{ "soul", 0, 0, 'd' },
	{ "winmx", 0, 0, 'e' },
	{ "ares", 0, 0, 'f' },
	{ "mute", 0, 0, 'g' },
	{ "waste", 0, 0, 'h' },
	{ "xdcc", 0, 0, 'i' },
	{ "debug", 0, 0, 'j' },
        {0}
};



static void
init(struct ipt_entry_match *m, unsigned int *nfcache)
{
    struct ipt_p2p_info *info = (struct ipt_p2p_info *)m->data;

    *nfcache |= NFC_UNKNOWN;

    /*init the module with default values*/
    info->cmd = 0;
    info->debug = 0;

}


static int
parse(int c, char **argv, int invert, unsigned int *flags,
	const struct ipt_entry *entry,
	unsigned int *nfcache,
	struct ipt_entry_match **match)
{
    struct ipt_p2p_info *info = (struct ipt_p2p_info *)(*match)->data;

    switch (c) {
	case '1':		/*cmd: ipp2p*/
	    if ((*flags & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified once!");
	    if ((*flags) != 0)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified alone!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
	    *flags += SHORT_HAND_IPP2P;
	    info->cmd = *flags;
	    break;

	case '2':		/*cmd: edk*/
	    if ((*flags & IPP2P_EDK) == IPP2P_EDK)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--edk' may only be "
				"specified once");
	    if ((*flags & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified alone!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
	    *flags += IPP2P_EDK;
	    info->cmd = *flags;
	    break;

	case '3':		/*cmd: pp*/
            if ((*flags & IPP2P_PP) == IPP2P_PP)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--pp' may only be "
                                "specified once!");
	    if ((*flags & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified alone!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_PP;
	    info->cmd = *flags;
	    break;

	case '4':
	    if ((*flags & IPP2P_XUNLEI) == IPP2P_XUNLEI)
	    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--xunlei' may only be "
				"specified once!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
	    *flags += IPP2P_XUNLEI;
	    info->cmd = *flags;
	    break;

	case '7':		/*cmd: dc*/
            if ((*flags & IPP2P_DC) == IPP2P_DC)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--dc' may only be "
                                "specified once!");
	    if ((*flags & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified alone!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_DC;
	    info->cmd = *flags;
	    break;


	case '9':		/*cmd: gnu*/
            if ((*flags & IPP2P_GNU) == IPP2P_GNU)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--gnu' may only be "
                                "specified once!");
	    if ((*flags & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified alone!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_GNU;
	    info->cmd = *flags;
	    break;

	case 'a':		/*cmd: kazaa*/
            if ((*flags & IPP2P_KAZAA) == IPP2P_KAZAA)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--kazaa' may only be "
                                "specified once!");
	    if ((*flags & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified alone!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_KAZAA;
	    info->cmd = *flags;
	    break;

	case 'b':		/*cmd: bit*/
            if ((*flags & IPP2P_BIT) == IPP2P_BIT)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--bit' may only be "
                                "specified once!");
	    if ((*flags & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified alone!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_BIT;
	    info->cmd = *flags;
	    break;

	case 'c':		/*cmd: apple*/
            if ((*flags & IPP2P_APPLE) == IPP2P_APPLE)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--apple' may only be "
                                "specified once!");
	    if ((*flags & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified alone!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_APPLE;
	    info->cmd = *flags;
	    break;


	case 'd':		/*cmd: soul*/
            if ((*flags & IPP2P_SOUL) == IPP2P_SOUL)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--soul' may only be "
                                "specified once!");
	    if ((*flags & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified alone!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_SOUL;
	    info->cmd = *flags;
	    break;


	case 'e':		/*cmd: winmx*/
            if ((*flags & IPP2P_WINMX) == IPP2P_WINMX)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--winmx' may only be "
                                "specified once!");
	    if ((*flags & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified alone!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_WINMX;
	    info->cmd = *flags;
	    break;

	case 'f':		/*cmd: ares*/
            if ((*flags & IPP2P_ARES) == IPP2P_ARES)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--ares' may only be "
                                "specified once!");
	    if ((*flags & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P)
		    exit_error(PARAMETER_PROBLEM,
				"ipp2p: `--ipp2p' may only be "
				"specified alone!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_ARES;
	    info->cmd = *flags;
	    break;

	case 'g':		/*cmd: mute*/
            if ((*flags & IPP2P_MUTE) == IPP2P_MUTE)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--mute' may only be "
                                "specified once!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_MUTE;
	    info->cmd = *flags;
	    break;
	case 'h':		/*cmd: waste*/
            if ((*flags & IPP2P_WASTE) == IPP2P_WASTE)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--waste' may only be "
                                "specified once!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_WASTE;
	    info->cmd = *flags;
	    break;
	case 'i':		/*cmd: xdcc*/
            if ((*flags & IPP2P_XDCC) == IPP2P_XDCC)
            exit_error(PARAMETER_PROBLEM,
                                "ipp2p: `--xdcc' may only be "
                                "specified once!");
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
            *flags += IPP2P_XDCC;
	    info->cmd = *flags;
	    break;

	case 'j':		/*cmd: debug*/
	    if (invert) exit_error(PARAMETER_PROBLEM, "ipp2p: invert [!] is not allowed!");
	    info->debug = 1;
	    break;

	default:
//            exit_error(PARAMETER_PROBLEM,
//	    "\nipp2p-parameter problem: for ipp2p usage type: iptables -m ipp2p --help\n");
	    return 0;
    }
    return 1;
}


static void
final_check(unsigned int flags)
{
    if (!flags)
            exit_error(PARAMETER_PROBLEM,
	    "\nipp2p-parameter problem: for ipp2p usage type: iptables -m ipp2p --help\n");
}



static void
print(const struct ipt_ip *ip,
        const struct ipt_entry_match *match,
	int numeric)
{
    struct ipt_p2p_info *info = (struct ipt_p2p_info *)match->data;

    printf("ipp2p v%s", IPP2P_VERSION);
    if ((info->cmd & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P) printf(" --ipp2p");
    if ((info->cmd & IPP2P_KAZAA) == IPP2P_KAZAA) printf(" --kazaa");
    if ((info->cmd & IPP2P_GNU) == IPP2P_GNU) printf(" --gnu");
    if ((info->cmd & IPP2P_EDK) == IPP2P_EDK) printf(" --edk");
    if ((info->cmd & IPP2P_DC) == IPP2P_DC) printf(" --dc");
    if ((info->cmd & IPP2P_BIT) == IPP2P_BIT) printf(" --bit");
    if ((info->cmd & IPP2P_PP) == IPP2P_PP) printf(" --pp");
    if ((info->cmd & IPP2P_XUNLEI) == IPP2P_XUNLEI) printf(" --xunlei");
    if ((info->cmd & IPP2P_APPLE) == IPP2P_APPLE) printf(" --apple");
    if ((info->cmd & IPP2P_SOUL) == IPP2P_SOUL) printf(" --soul");
    if ((info->cmd & IPP2P_WINMX) == IPP2P_WINMX) printf(" --winmx");
    if ((info->cmd & IPP2P_ARES) == IPP2P_ARES) printf(" --ares");
    if ((info->cmd & IPP2P_MUTE) == IPP2P_MUTE) printf(" --mute");
    if ((info->cmd & IPP2P_WASTE) == IPP2P_WASTE) printf(" --waste");
    if ((info->cmd & IPP2P_XDCC) == IPP2P_XDCC) printf(" --xdcc");
    if (info->debug != 0) printf(" --debug");
    printf(" ");
}



static void
save(const struct ipt_ip *ip, const struct ipt_entry_match *match)
{
    struct ipt_p2p_info *info = (struct ipt_p2p_info *)match->data;

    if ((info->cmd & SHORT_HAND_IPP2P) == SHORT_HAND_IPP2P) printf("--ipp2p ");
    if ((info->cmd & IPP2P_KAZAA) == IPP2P_KAZAA) printf("--kazaa ");
    if ((info->cmd & IPP2P_GNU) == IPP2P_GNU) printf("--gnu ");
    if ((info->cmd & IPP2P_EDK) == IPP2P_EDK) printf("--edk ");
    if ((info->cmd & IPP2P_DC) == IPP2P_DC) printf("--dc ");
    if ((info->cmd & IPP2P_BIT) == IPP2P_BIT) printf("--bit ");
    if ((info->cmd & IPP2P_PP) == IPP2P_PP) printf("--pp ");
    if ((info->cmd & IPP2P_XUNLEI) == IPP2P_XUNLEI) printf("--xunlei ");
    if ((info->cmd & IPP2P_APPLE) == IPP2P_APPLE) printf("--apple ");
    if ((info->cmd & IPP2P_SOUL) == IPP2P_SOUL) printf("--soul ");
    if ((info->cmd & IPP2P_WINMX) == IPP2P_WINMX) printf("--winmx ");
    if ((info->cmd & IPP2P_ARES) == IPP2P_ARES) printf("--ares ");
    if ((info->cmd & IPP2P_MUTE) == IPP2P_MUTE) printf("--mute ");
    if ((info->cmd & IPP2P_WASTE) == IPP2P_WASTE) printf("--waste ");
    if ((info->cmd & IPP2P_XDCC) == IPP2P_XDCC) printf("--xdcc ");
    if (info->debug != 0) printf("--debug ");
}




static
struct iptables_match ipp2p=
{
    .next           = NULL,
    .name           = "ipp2p",
    .version        = IPTABLES_VERSION,
    .size           = IPT_ALIGN(sizeof(struct ipt_p2p_info)),
    .userspacesize  = IPT_ALIGN(sizeof(struct ipt_p2p_info)),
    .help           = &help,
    .init           = &init,
    .parse          = &parse,
    .final_check    = &final_check,
    .print          = &print,
    .save           = &save,
    .extra_opts     = opts
};



void _init(void)
{
    register_match(&ipp2p);
}

