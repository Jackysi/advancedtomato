/*

	Tomato Firmware
	Copyright (C) 2006-2010 Jonathan Zarate

*/

#include "tomato.h"

#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>


//	#define DEBUG_NOEXECSERVICE
#define DEBUG_NVRAMSET(k, v)	_dprintf("nvram set %s=%s\n", k, v);


char *post_buf = NULL;
int rboot = 0;
extern int post;

static void asp_css(int argc, char **argv);
static void asp_resmsg(int argc, char **argv);

//
static void wo_tomato(char *url);
static void wo_update(char *url);
static void wo_service(char *url);
static void wo_shutdown(char *url);
static void wo_nvcommit(char *url);
//	static void wo_logout(char *url);


// ----------------------------------------------------------------------------


void exec_service(const char *action)
{
	int i;

	_dprintf("exec_service: %s\n", action);

	i = 10;
	while ((!nvram_match("action_service", "")) && (i-- > 0))  {
		_dprintf("%s: waiting before %d\n", __FUNCTION__, i);
		sleep(1);
	}

	nvram_set("action_service", action);
	kill(1, SIGUSR1);

	i = 3;
	while ((nvram_match("action_service", (char *)action)) && (i-- > 0))  {
		_dprintf("%s: waiting after %d\n", __FUNCTION__, i);
		sleep(1);
	}

/*
	if (atoi(webcgi_safeget("_service_wait", ""))) {
		i = 10;
		while ((nvram_match("action_service", (char *)action)) && (i-- > 0))  {
			_dprintf("%s: waiting after %d\n", __FUNCTION__, i);
			sleep(1);
		}
	}
*/
}

static void wi_generic_noid(char *url, int len, char *boundary)
{
	if (post == 1) {
		if (len >= (32 * 1024)) {
//			syslog(LOG_WARNING, "POST max");
			exit(1);
		}

		if (post_buf) free(post_buf);
		if ((post_buf = malloc(len + 1)) == NULL) {
//			syslog(LOG_CRIT, "Unable to allocate post buffer");
			exit(1);
		}

		if (web_read_x(post_buf, len) != len) {
			exit(1);
		}
		post_buf[len] = 0;
		webcgi_init(post_buf);
	}
}

void wi_generic(char *url, int len, char *boundary)
{
	wi_generic_noid(url, len, boundary);
	check_id(url);
}

// !!TB - CGI Support
void wi_cgi_bin(char *url, int len, char *boundary)
{
	if (post_buf) free(post_buf);
	post_buf = NULL;

	if (post) {
		if (len >= (128 * 1024)) {
			syslog(LOG_WARNING, "POST length exceeded maximum allowed");
			exit(1);
		}

		if (len > 0) {
			if ((post_buf = malloc(len + 1)) == NULL) {
				exit(1);
			}
			if (web_read_x(post_buf, len) != len) {
				exit(1);
			}
			post_buf[len] = 0;
		}
	}
}

static void _execute_command(char *url, char *command, char *query, wofilter_t wof)
{
	char webExecFile[]  = "/tmp/.wxXXXXXX";
	char webQueryFile[] = "/tmp/.wqXXXXXX";
	char cmd[sizeof(webExecFile) + 10];
	FILE *f;
	int fe, fq = -1;

	if ((fe = mkstemp(webExecFile)) < 0)
		exit(1);
	if (query) {
		if ((fq = mkstemp(webQueryFile)) < 0) {
			close(fe);
			unlink(webExecFile);
			exit(1);
		}
	}

	if ((f = fdopen(fe, "wb")) != NULL) {
		fprintf(f,
			"#!/bin/sh\n"
			"export REQUEST_METHOD=\"%s\"\n"
			"export PATH=%s\n"
			". /etc/profile\n"
			"%s%s %s%s\n",
			post ? "POST" : "GET", getenv("PATH"),
			command ? "" : "./", command ? command : url,
			query ? "<" : "", query ? webQueryFile : "");
		fclose(f);
	}
	else {
		close(fe);
		unlink(webExecFile);
		if (query) {
			close(fq);
			unlink(webQueryFile);
		}
		exit(1);
	}
	chmod(webExecFile, 0700);

	if (query) {
		if ((f = fdopen(fq, "wb")) != NULL) {
			fprintf(f, "%s\n", query);
			fclose(f);
		}
		else {
			unlink(webExecFile);
			close(fq);
			unlink(webQueryFile);
			exit(1);
		}
	}

	sprintf(cmd, "%s 2>&1", webExecFile);
	web_pipecmd(cmd, wof);
	unlink(webQueryFile);
	unlink(webExecFile);
}

static void wo_cgi_bin(char *url)
{
	if (!header_sent) send_header(200, NULL, mime_html, 0);
	_execute_command(url, NULL, post_buf, WOF_NONE);
	if (post_buf) {
		free(post_buf);
		post_buf = NULL;
	}
}

static void wo_shell(char *url)
{
	web_puts("\ncmdresult = '");
	_execute_command(NULL, webcgi_get("command"), NULL, WOF_JAVASCRIPT);
	web_puts("';");
}

static void wo_blank(char *url)
{
	web_puts("\n\n\n\n");
}

static void wo_favicon(char *url)
{
	send_header(200, NULL, "image/vnd.microsoft.icon", 0);
	do_file(url);
/*
	if (nvram_match("web_favicon", "1")) {
		send_header(200, NULL, "image/vnd.microsoft.icon", 0);
		do_file(url);
	}
	else {
		send_error(404, NULL, NULL);
	}
*/
}

static void wo_cfe(char *url)
{
	do_file(MTD_DEV(0ro));
}

static void wo_nvram(char *url)
{
	web_pipecmd("nvram show", WOF_NONE);
}

static void wo_iptables(char *url)
{
	web_pipecmd("iptables -nvL; echo; iptables -t nat -nvL; echo; iptables -t mangle -nvL", WOF_NONE);
}

#ifdef TCONFIG_IPV6
static void wo_ip6tables(char *url)
{
	web_pipecmd("ip6tables -nvL; echo; ip6tables -t mangle -nvL", WOF_NONE);
}
#endif

/*
static void wo_spin(char *url)
{
	char s[64];

	strlcpy(s, nvram_safe_get("web_css"), sizeof(s));
	strlcat(s, "_spin.gif", sizeof(s));
	if (f_exists(s)) do_file(s);
		else do_file("_spin.gif");
}
*/

void common_redirect(void)
{
	if (atoi(webcgi_safeget("_ajax", ""))) {
		send_header(200, NULL, mime_html, 0);
		web_puts("OK");
	}
	else {
		redirect(webcgi_safeget("_redirect", "/"));
	}
}

// ----------------------------------------------------------------------------

const struct mime_handler mime_handlers[] = {
	{ "update.cgi",		mime_javascript,			0,	wi_generic,			wo_update,		1 },
	{ "tomato.cgi",		NULL,						0,	wi_generic,		    wo_tomato,		1 },

	{ "debug.js",		mime_javascript,			5,	wi_generic_noid,	wo_blank,		1 },	// while debugging
	{ "cfe/*.bin",		mime_binary,				0,	wi_generic,			wo_cfe,			1 },
	{ "nvram/*.txt",	mime_binary,				0,	wi_generic,			wo_nvram,		1 },
	{ "ipt/*.txt",		mime_binary,				0,	wi_generic,			wo_iptables,	1 },
#ifdef TCONFIG_IPV6
	{ "ip6t/*.txt",		mime_binary,				0,	wi_generic,			wo_ip6tables,	1 },
#endif
	{ "cfg/*.cfg",			NULL,					0,	wi_generic,			wo_backup,		1 },
	{ "cfg/restore.cgi",	mime_html,				0,	wi_restore,			wo_restore,		1 },
	{ "cfg/defaults.cgi",	NULL,					0,	wi_generic,	 		wo_defaults,	1 },

	{ "bwm/*.gz",			NULL,					0,	wi_generic,			wo_bwmbackup,	1 },
	{ "bwm/restore.cgi",	NULL,					0,	wi_bwmrestore,		wo_bwmrestore,	1 },

	{ "ipt/*.gz",			NULL,					0,	wi_generic,			wo_iptbackup,	1 },
	{ "ipt/restore.cgi",	NULL,					0,	wi_iptrestore,		wo_iptrestore,	1 },

	{ "logs/view.cgi",	NULL,						0,	wi_generic,			wo_viewlog,		1 },
	{ "logs/*.txt",		NULL,						0,	wi_generic,			wo_syslog,		1 },
	{ "webmon_**",		NULL,						0,	wi_generic,			wo_syslog,		1 },

	{ "logout.asp",			NULL,					0,	wi_generic,			wo_asp,			1 },
	{ "clearcookies.asp",	NULL,					0,	wi_generic,			wo_asp,			1 },

//	{ "spin.gif",		NULL,						0,	wi_generic_noid,	wo_spin,		1 },

	{ "**.asp",			NULL,						0,	wi_generic_noid,	wo_asp,			1 },
	{ "**.css",			"text/css",					2,	wi_generic_noid,	do_file,		1 },
	{ "**.htm|**.html",		mime_html,		  		  	2,	wi_generic_noid,	do_file,		1 },
	{ "**.gif",			"image/gif",				5,	wi_generic_noid,	do_file,		1 },
	{ "**.jpg",			"image/jpeg",				5,	wi_generic_noid,	do_file,		1 },
	{ "**.png",			"image/png",				5,	wi_generic_noid,	do_file,		1 },
	{ "**.js",			mime_javascript,			2,	wi_generic_noid,	do_file,		1 },
	{ "**.jsx",			mime_javascript,			0,	wi_generic,			wo_asp,			1 },
	{ "**.svg",			"image/svg+xml",			2,	wi_generic_noid,	do_file,		1 },

// Required mimetype for fonts & icons
	{ "**.woff",			"application/font-woff",		2,	wi_generic_noid,	do_file,		1 },
	{ "**.eot",			"application/vnd.ms-fontobject",2,	wi_generic_noid,do_file,		1 },
	{ "**.ttf",			"application/octet-stream",	2,	wi_generic_noid,	do_file,		1 },

	{ "**.txt",			mime_plain,					2,	wi_generic_noid,	do_file,		1 },
	{ "**.bin",			mime_binary,				0,	wi_generic_noid,	do_file,		1 },
	{ "**.bino",		mime_octetstream,			0,	wi_generic_noid,	do_file,		1 },
	{ "favicon.ico",	NULL,						5,	wi_generic_noid,	wo_favicon,		1 },
// !!TB - CGI Support, enable downloading archives
	{ "**/cgi-bin/**|**.sh",	NULL,					0,	wi_cgi_bin,		wo_cgi_bin,			1 },
	{ "**.tar|**.gz",		mime_binary,				0,	wi_generic_noid,	do_file,		1 },
	{ "shell.cgi",			mime_javascript,			0,	wi_generic,		wo_shell,		1 },
	{ "wpad.dat|proxy.pac",		"application/x-ns-proxy-autoconfig",	0,	wi_generic_noid,	do_file,		0 },

	{ "webmon.cgi",		mime_javascript,				0,	wi_generic,		wo_webmon,		1 },
	{ "dhcpc.cgi",		NULL,						0,	wi_generic,			wo_dhcpc,		1 },
	{ "dhcpd.cgi",		mime_javascript,			0,	wi_generic,			wo_dhcpd,		1 },
	{ "nvcommit.cgi",	NULL,						0,	wi_generic,			wo_nvcommit,	1 },
	{ "ping.cgi",		mime_javascript,			0,	wi_generic,			wo_ping,		1 },
	{ "trace.cgi",		mime_javascript,			0,	wi_generic,			wo_trace,		1 },
	{ "upgrade.cgi",	mime_html,					0,	wi_upgrade,			wo_flash,		1 },
	{ "upnp.cgi",		NULL,						0,	wi_generic,			wo_upnp,		1 },
	{ "wakeup.cgi",		NULL,						0,	wi_generic,			wo_wakeup,		1 },
	{ "wlmnoise.cgi",	mime_html,					0,	wi_generic,			wo_wlmnoise,	1 },
	{ "wlradio.cgi",	NULL,						0,	wi_generic,			wo_wlradio,		1 },
	{ "resolve.cgi",	mime_javascript,			0,	wi_generic,			wo_resolve,		1 },
	{ "expct.cgi",		mime_html,					0,	wi_generic,			wo_expct,		1 },
	{ "service.cgi",	NULL,						0,	wi_generic,			wo_service,		1 },
//	{ "logout.cgi",		NULL,	   		 			0,	wi_generic,			wo_logout,		0 },
// see httpd.c
	{ "shutdown.cgi",	mime_html,					0,	wi_generic,			wo_shutdown,	1 },
#ifdef TCONFIG_OPENVPN
	{ "vpnstatus.cgi",	mime_javascript,			0,	wi_generic,			wo_vpn_status,		1 },
#endif
#ifdef TCONFIG_PPTPD
	{ "pptpd.cgi",		mime_javascript,			0,	wi_generic,			wo_pptpdcmd,		1 },	//!!AB - PPTPD
#endif
#ifdef TCONFIG_USB
	{ "usbcmd.cgi",			mime_javascript,			0,	wi_generic,		wo_usbcommand,		1 },	//!!TB - USB
#endif
#ifdef BLACKHOLE
	{ "blackhole.cgi",	NULL,						0,	wi_blackhole,		NULL,			1 },
#endif
#ifdef TCONFIG_NOCAT
	{ "uploadsplash.cgi",		NULL,					0,	wi_uploadsplash,	wo_uploadsplash,	1 },
	{ "ext/uploadsplash.cgi",	NULL,					0,	wi_uploadsplash,	wo_uploadsplash,	1 },
#endif
	{ NULL,				NULL,					0,	NULL,		NULL,			1 }
};

const aspapi_t aspapi[] = {
	{ "activeroutes",		asp_activeroutes	},
	{ "arplist",			asp_arplist			},
	{ "bandwidth",			asp_bandwidth		},
	{ "build_time",			asp_build_time		},
	{ "cgi_get",			asp_cgi_get			},
	{ "compmac",			asp_compmac			},
	{ "ctcount",			asp_ctcount			},
	{ "ctdump",				asp_ctdump			},
	{ "ctrate",				asp_ctrate			},
	{ "ddnsx",				asp_ddnsx			},
	{ "devlist",			asp_devlist			},
	{ "webmon",				asp_webmon			},
	{ "dhcpc_time",			asp_dhcpc_time		},
	{ "dns",				asp_dns				},
	{ "ident",				asp_ident			},
	{ "lanip",				asp_lanip			},
	{ "layer7",				asp_layer7			},
	{ "link_uptime",		asp_link_uptime		},
	{ "lipp",				asp_lipp			},
	{ "netdev",				asp_netdev			},

	{ "iptraffic",			asp_iptraffic		},
	{ "iptmon",				asp_iptmon			},
	{ "ipt_bandwidth",		asp_ipt_bandwidth	},

	{ "notice",				asp_notice			},
	{ "nv",					asp_nv				},
	{ "nvram",				asp_nvram 			},
	{ "nvramseq",			asp_nvramseq		},
	{ "nvstat",				asp_nvstat 			},
	{ "psup",				asp_psup			},
	{ "qrate",				asp_qrate			},
	{ "resmsg",				asp_resmsg			},
	{ "rrule",				asp_rrule			},
	{ "statfs",				asp_statfs			},
	{ "sysinfo",			asp_sysinfo			},
	{ "time",				asp_time			},
	{ "upnpinfo",			asp_upnpinfo		},
	{ "version",			asp_version			},
	{ "wanstatus",			asp_wanstatus		},
	{ "wanup",				asp_wanup			},
#ifdef TCONFIG_PPTPD
	{ "pptpd_userol",		asp_pptpd_userol	},
#endif
	{ "wlstats",			asp_wlstats		},
	{ "wlclient",			asp_wlclient		},
	{ "wlnoise",			asp_wlnoise			},
	{ "wlscan",				asp_wlscan			},
	{ "wlchannels",			asp_wlchannels	},	//!!TB
	{ "wlcountries",		asp_wlcountries	},
	{ "wlifaces",			asp_wlifaces		},
	{ "wlbands",			asp_wlbands		},
#ifdef TCONFIG_USB
	{ "usbdevices",			asp_usbdevices	},	//!!TB - USB Support
#endif
#ifdef TCONFIG_SDHC
	{ "mmcid",			asp_mmcid	},	//MMC Support
#endif
	{ "etherstates",		asp_etherstates	},	//Ethernet States
	{ "anonupdate",			asp_anonupdate	},	//Tomato update notification system
#ifdef TCONFIG_IPV6
	{ "calc6rdlocalprefix",		asp_calc6rdlocalprefix	},
#endif

	{ "css",				asp_css				},
	{ NULL,					NULL				}
};

// -----------------------------------------------------------------------------

static void asp_css(int argc, char **argv)
{
	const char *css = nvram_safe_get("web_css");
	const char *ttb = nvram_safe_get("ttb_css");

	if( nvram_match( "web_css", "online" ) ) {
		web_printf("<link rel='stylesheet' type='text/css' href='ext/%s.css'>", ttb);
	} else {
		if (strcmp(css, "tomato") != 0) {
			web_printf("<link rel='stylesheet' type='text/css' href='%s.css'>", css);
		}
	}
}

// -----------------------------------------------------------------------------

const char *resmsg_get(void)
{
	return webcgi_safeget("resmsg", "");
}

void resmsg_set(const char *msg)
{
	webcgi_set("resmsg", strdup(msg));	// m ok
}

int resmsg_fread(const char *fname)
{
	char s[256];
	char *p;

	f_read_string(fname, s, sizeof(s));
	if ((p = strchr(s, '\n')) != NULL) *p = 0;
	if (s[0]) {
		resmsg_set(s);
		return 1;
	}
	return 0;
}

static void asp_resmsg(int argc, char **argv)
{
	char *p;

	if ((p = js_string(webcgi_safeget("resmsg", (argc > 0) ? argv[0] : ""))) == NULL) return;
	web_printf("\nresmsg='%s';\n", p);
	free(p);
}

// ----------------------------------------------------------------------------

// verification... simple sanity checks. UI should verify all fields.

// todo: move and re-use for filtering	- zzz

typedef union {
	int i;
	long l;
	const char *s;
} nvset_varg_t;

typedef struct {
	const char *name;
	enum {
		VT_NONE,		// no checking
		VT_LENGTH,		// check length of string
		VT_TEXT,		// strip \r, check length of string
		VT_RANGE,		// expect an integer, check range
		VT_IP,			// expect an ip address
		VT_MAC,			// expect a mac address
#ifdef TCONFIG_IPV6
		VT_IPV6,		// expect an ipv6 address
#endif
		VT_TEMP			// no checks, no commit
	} vtype;
	nvset_varg_t va;
	nvset_varg_t vb;
} nvset_t;


#define	V_NONE				VT_NONE,	{ }, 			{ }
#define V_01				VT_RANGE,	{ .l = 0 },		{ .l = 1 }
#define V_PORT				VT_RANGE,	{ .l = 2 },		{ .l = 65535 }
#define V_ONOFF				VT_LENGTH,	{ .i = 2 },		{ .i = 3 }
#define V_WORD				VT_LENGTH,	{ .i = 1 },		{ .i = 16 }
#define V_LENGTH(min, max)	VT_LENGTH,	{ .i = min },	{ .i = max }
#define V_TEXT(min, max)	VT_TEXT,	{ .i = min },	{ .i = max }
#define V_RANGE(min, max)	VT_RANGE,	{ .l = min },	{ .l = max }
#define V_IP				VT_IP,		{ },			{ }
#define	V_OCTET				VT_RANGE,	{ .l = 0 },		{ .l = 255 }
#define V_NUM				VT_RANGE,	{ .l = 0 },		{ .l = 0x7FFFFFFF }
#define	V_TEMP				VT_TEMP,	{ }, 			{ }
#ifdef TCONFIG_IPV6
#define V_IPV6(required)		VT_IPV6,	{ .i = required },	{ }
#endif

static const nvset_t nvset_list[] = {

// basic-ident
	{ "router_name",		V_LENGTH(0, 32)		},
	{ "wan_hostname",		V_LENGTH(0, 32)		},
	{ "wan_domain",			V_LENGTH(0, 32)		},

// basic-time
	{ "tm_tz",				V_LENGTH(1, 64)		},	// PST8PDT
	{ "tm_sel",				V_LENGTH(1, 64)		},	// PST8PDT
	{ "tm_dst",				V_01				},
	{ "ntp_updates",		V_RANGE(-1, 24)		},
	{ "ntp_tdod",			V_01				},
	{ "ntp_server",			V_LENGTH(1, 150)	},	// x y z
	{ "ntp_kiss",			V_LENGTH(0, 255)	},

// basic-static
	{ "dhcpd_static",		V_LENGTH(0, 108*251)	},	// 108 (max chars per entry) x 250 entries
	{ "dhcpd_static_only",		V_01			},

// basic-ddns
	{ "ddnsx0",				V_LENGTH(0, 2048)	},
	{ "ddnsx1",				V_LENGTH(0, 2048)	},
	{ "ddnsx0_cache",		V_LENGTH(0, 1)		},	// only to clear
	{ "ddnsx1_cache",		V_LENGTH(0, 1)		},
	{ "ddnsx_ip",			V_LENGTH(0, 32)		},
	{ "ddnsx_save",			V_01				},
	{ "ddnsx_refresh",		V_RANGE(0, 365)		},

// basic-network
	// WAN
	{ "wan_proto",			V_LENGTH(1, 16)		},	// disabled, dhcp, static, pppoe, pptp, l2tp
	{ "wan_ipaddr",			V_IP				},
	{ "wan_netmask",		V_IP				},
	{ "wan_gateway",		V_IP				},
	{ "hb_server_ip",		V_LENGTH(0, 32)		},
	{ "wan_l2tp_server_ip",		V_LENGTH(0, 128)		},
	{ "wan_pptp_server_ip",		V_LENGTH(0, 128)		},
	{ "wan_pptp_dhcp",		V_01				},
	{ "wan_ppp_username",		V_LENGTH(0, 60)		},
	{ "wan_ppp_passwd",		V_LENGTH(0, 60)		},
	{ "wan_ppp_service",		V_LENGTH(0, 50)		},
	{ "wan_ppp_demand",		V_01				},
	{ "wan_ppp_custom",		V_LENGTH(0, 256)		},
	{ "wan_ppp_idletime",		V_RANGE(0, 1440)	},
	{ "wan_ppp_redialperiod",	V_RANGE(1, 86400)	},
	{ "wan_ppp_mlppp",		V_01				},
	{ "wan_mtu_enable",		V_01				},
	{ "wan_mtu",			V_RANGE(576, 1500)	},
	{ "wan_islan",			V_01				},
	{ "wan_modem_ipaddr",		V_IP				},
	{ "wan_pppoe_lei",		V_RANGE(1, 60)			},
	{ "wan_pppoe_lef",		V_RANGE(1, 10)			},
	{ "wan_sta",			V_LENGTH(0, 10)		},
	{ "wan_dns",			V_LENGTH(0, 50)		},	// ip ip ip
	{ "wan_ckmtd",			V_LENGTH(1, 2)		}, // check method: 1 - ping, 2 - traceroute

#ifdef TCONFIG_MULTIWAN
	{ "mwan_num",			V_RANGE(1, 4)		},
#else
	{ "mwan_num",			V_RANGE(1, 2)		},
#endif
	{ "mwan_init",			V_01			},
	{ "mwan_cktime",		V_RANGE(0, 3600)	},
	{ "mwan_ckdst",			V_LENGTH(0, 64)		},
	{ "pbr_rules",			V_LENGTH(0, 2048)	},

	{ "wan_weight",			V_RANGE(0, 256)			},
	{ "wan_dns_auto",		V_01				},

	{ "wan2_proto",			V_LENGTH(1, 16)		},	// disabled, dhcp, static, pppoe, pptp, l2tp
	{ "wan2_weight",		V_RANGE(0, 256)			},
	{ "wan2_ipaddr",		V_IP				},
	{ "wan2_netmask",		V_IP				},
	{ "wan2_gateway",		V_IP				},
	{ "wan2_l2tp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan2_pptp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan2_pptp_dhcp",		V_01				},
	{ "wan2_ppp_username",		V_LENGTH(0, 60)		},
	{ "wan2_ppp_passwd",		V_LENGTH(0, 60)		},
	{ "wan2_ppp_service",		V_LENGTH(0, 50)		},
	{ "wan2_ppp_demand",		V_01				},
	{ "wan2_ppp_custom",		V_LENGTH(0, 256)		},
	{ "wan2_ppp_idletime",		V_RANGE(0, 1440)	},
	{ "wan2_ppp_redialperiod",	V_RANGE(1, 86400)	},
	{ "wan2_ppp_mlppp",		V_01				},
	{ "wan2_mtu_enable",		V_01				},
	{ "wan2_mtu",			V_RANGE(576, 1500)	},
	{ "wan2_islan",			V_01				},
	{ "wan2_modem_ipaddr",		V_IP				},
	{ "wan2_pppoe_lei",		V_RANGE(1, 60)			},
	{ "wan2_pppoe_lef",		V_RANGE(1, 10)			},
	{ "wan2_sta",			V_LENGTH(0, 10)		},
	{ "wan2_dns",			V_LENGTH(0, 50)		},	// ip ip ip
	{ "wan2_dns_auto",		V_01				},
	{ "wan2_ckmtd",			V_LENGTH(1, 2)		}, // check method: 1 - ping, 2 - traceroute

#ifdef TCONFIG_MULTIWAN
	{ "wan3_proto",			V_LENGTH(1, 16)		},	// disabled, dhcp, static, pppoe, pptp, l2tp
	{ "wan3_weight",		V_RANGE(0, 256)			},
	{ "wan3_ipaddr",		V_IP				},
	{ "wan3_netmask",		V_IP				},
	{ "wan3_gateway",		V_IP				},
	{ "wan3_l2tp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan3_pptp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan3_pptp_dhcp",		V_01				},
	{ "wan3_ppp_username",		V_LENGTH(0, 60)		},
	{ "wan3_ppp_passwd",		V_LENGTH(0, 60)		},
	{ "wan3_ppp_service",		V_LENGTH(0, 50)		},
	{ "wan3_ppp_demand",		V_01				},
	{ "wan3_ppp_custom",		V_LENGTH(0, 256)		},
	{ "wan3_ppp_idletime",		V_RANGE(0, 1440)	},
	{ "wan3_ppp_redialperiod",	V_RANGE(1, 86400)	},
	{ "wan3_ppp_mlppp",		V_01				},
	{ "wan3_mtu_enable",		V_01				},
	{ "wan3_mtu",			V_RANGE(576, 1500)	},
	{ "wan3_islan",			V_01				},
	{ "wan3_modem_ipaddr",		V_IP				},
	{ "wan3_pppoe_lei",		V_RANGE(1, 60)			},
	{ "wan3_pppoe_lef",		V_RANGE(1, 10)			},
	{ "wan3_sta",			V_LENGTH(0, 10)		},
	{ "wan3_dns",			V_LENGTH(0, 50)		},	// ip ip ip
	{ "wan3_dns_auto",		V_01				},
	{ "wan3_ckmtd",			V_LENGTH(1, 2)		}, // check method: 1 - ping, 2 - traceroute

	{ "wan4_proto",			V_LENGTH(1, 16)		},	// disabled, dhcp, static, pppoe, pptp, l2tp
	{ "wan4_weight",		V_RANGE(0, 256)			},
	{ "wan4_ipaddr",		V_IP				},
	{ "wan4_netmask",		V_IP				},
	{ "wan4_gateway",		V_IP				},
	{ "wan4_l2tp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan4_pptp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan4_pptp_dhcp",		V_01				},
	{ "wan4_ppp_username",		V_LENGTH(0, 60)		},
	{ "wan4_ppp_passwd",		V_LENGTH(0, 60)		},
	{ "wan4_ppp_service",		V_LENGTH(0, 50)		},
	{ "wan4_ppp_demand",		V_01				},
	{ "wan4_ppp_custom",		V_LENGTH(0, 256)		},
	{ "wan4_ppp_idletime",		V_RANGE(0, 1440)	},
	{ "wan4_ppp_redialperiod",	V_RANGE(1, 86400)	},
	{ "wan4_ppp_mlppp",		V_01				},
	{ "wan4_mtu_enable",		V_01				},
	{ "wan4_mtu",			V_RANGE(576, 1500)	},
	{ "wan4_islan",			V_01				},
	{ "wan4_modem_ipaddr",		V_IP				},
	{ "wan4_pppoe_lei",		V_RANGE(1, 60)			},
	{ "wan4_pppoe_lef",		V_RANGE(1, 10)			},
	{ "wan4_sta",			V_LENGTH(0, 10)		},
	{ "wan4_dns",			V_LENGTH(0, 50)		},	// ip ip ip
	{ "wan4_dns_auto",		V_01				},
	{ "wan4_ckmtd",			V_LENGTH(1, 2)		}, // check method: 1 - ping, 2 - traceroute
#endif

	// LAN
	{ "lan_ipaddr",			V_IP				},
	{ "lan_netmask",		V_IP				},
	{ "lan_gateway",		V_IP				},
	{ "lan_dns",			V_LENGTH(0, 50)		},	// ip ip ip

#ifdef TCONFIG_DNSSEC
	{ "dnssec_enable",		V_01			},
#endif

#ifdef TCONFIG_DNSCRYPT
	{ "dnscrypt_proxy",		V_01				},
	{ "dnscrypt_priority",		V_RANGE(0, 2)			}, // 0=none, 1=preferred, 2=exclusive
	{ "dnscrypt_port",		V_PORT				},
	{ "dnscrypt_resolver",		V_LENGTH(0, 40)			},
	{ "dnscrypt_log",		V_RANGE(0, 99)			},
	{ "dnscrypt_manual",		V_01				},
	{ "dnscrypt_provider_name",	V_LENGTH(0, 60)			},
	{ "dnscrypt_provider_key",	V_LENGTH(0, 80)			},
	{ "dnscrypt_resolver_address",	V_LENGTH(0, 50)			},
#endif
	{ "lan_state",			V_01				},
	{ "lan_desc",			V_01				},
	{ "lan_invert",			V_01				},
	{ "lan_proto",			V_WORD				},	// static, dhcp
	{ "dhcp_start",			V_LENGTH(0, 15)		},	// remove !
	{ "dhcp_num",			V_LENGTH(0, 4)		},	// remove !
	{ "dhcpd_startip",		V_LENGTH(0, 15)		},
	{ "dhcpd_endip",		V_LENGTH(0, 15)		},
	{ "dhcp_lease",			V_LENGTH(0, 5)		},
	{ "wan_wins",			V_IP				},

#ifdef TCONFIG_USB
	// 3G MODEM
	{ "wan_modem_pin",		V_LENGTH(0,6)			},
	{ "wan_modem_dev",		V_LENGTH(0,8)			},
	{ "wan_modem_init",		V_LENGTH(0,25)			},
	{ "wan_modem_apn",		V_LENGTH(0,25)			},
	{ "wan_modem_speed",		V_LENGTH(0,6)			},

	{ "wan2_modem_pin",		V_LENGTH(0,6)			},
	{ "wan2_modem_dev",		V_LENGTH(0,8)			},
	{ "wan2_modem_init",		V_LENGTH(0,25)			},
	{ "wan2_modem_apn",		V_LENGTH(0,25)			},
	{ "wan2_modem_speed",		V_LENGTH(0,6)			},

#ifdef TCONFIG_MULTIWAN
	{ "wan3_modem_pin",		V_LENGTH(0,6)			},
	{ "wan3_modem_dev",		V_LENGTH(0,8)			},
	{ "wan3_modem_init",		V_LENGTH(0,25)			},
	{ "wan3_modem_apn",		V_LENGTH(0,25)			},
	{ "wan3_modem_speed",		V_LENGTH(0,6)			},

	{ "wan4_modem_pin",		V_LENGTH(0,6)			},
	{ "wan4_modem_dev",		V_LENGTH(0,8)			},
	{ "wan4_modem_init",		V_LENGTH(0,25)			},
	{ "wan4_modem_apn",		V_LENGTH(0,25)			},
	{ "wan4_modem_speed",		V_LENGTH(0,6)			},
#endif
#endif

	// LAN networks
	{ "lan_ifname",			V_LENGTH(0, 5)			},

	{ "lan1_ifname",		V_LENGTH(0, 5)			},
	{ "lan1_ifnames",		V_TEXT(0,64)			},
	{ "lan1_ipaddr",		V_LENGTH(0, 15)			},
	{ "lan1_netmask",		V_LENGTH(0, 15)			},
	{ "lan1_proto",			V_LENGTH(0, 6)			},
	{ "lan1_stp",			V_LENGTH(0, 1)			},
	{ "dhcp1_start",		V_LENGTH(0, 15)			},
	{ "dhcp1_num",			V_LENGTH(0, 4)			},
	{ "dhcpd1_startip",		V_LENGTH(0, 15)			},
	{ "dhcpd1_endip",		V_LENGTH(0, 15)			},
	{ "dhcp1_lease",		V_LENGTH(0, 5)			},

	{ "lan2_ifname",		V_LENGTH(0, 5)			},
	{ "lan2_ifnames",		V_TEXT(0,64)			},
	{ "lan2_ipaddr",		V_LENGTH(0, 15)			},
	{ "lan2_netmask",		V_LENGTH(0, 15)			},
	{ "lan2_proto",			V_LENGTH(0, 6)			},
	{ "lan2_stp",			V_LENGTH(0, 1)			},
	{ "dhcp2_start",		V_LENGTH(0, 15)			},
	{ "dhcp2_num",			V_LENGTH(0, 4)			},
	{ "dhcpd2_startip",		V_LENGTH(0, 15)			},
	{ "dhcpd2_endip",		V_LENGTH(0, 15)			},
	{ "dhcp2_lease",		V_LENGTH(0, 5)			},

	{ "lan3_ifname",		V_LENGTH(0, 5)			},
	{ "lan3_ifnames",		V_TEXT(0,64)			},
	{ "lan3_ipaddr",		V_LENGTH(0, 15)			},
	{ "lan3_netmask",		V_LENGTH(0, 15)			},
	{ "lan3_proto",			V_LENGTH(0, 6)			},
	{ "lan3_stp",			V_LENGTH(0, 1)			},
	{ "dhcp3_start",		V_LENGTH(0, 15)			},
	{ "dhcp3_num",			V_LENGTH(0, 4)			},
	{ "dhcpd3_startip",		V_LENGTH(0, 15)			},
	{ "dhcpd3_endip",		V_LENGTH(0, 15)			},
	{ "dhcp3_lease",		V_LENGTH(0, 5)			},

	// wireless
	{ "wl_radio",			V_01				},
	{ "wl_mode",			V_LENGTH(2, 3)		},	// ap, sta, wet, wds
	{ "wl_net_mode",		V_LENGTH(5, 8)		},  // disabled, mixed, b-only, g-only, bg-mixed, n-only [speedbooster]
	{ "wl_ssid",			V_LENGTH(1, 32)		},
	{ "wl_closed",			V_01				},
	{ "wl_channel",			V_RANGE(0, 216)		},

	{ "wl_vifs",			V_LENGTH(0, 64)		},	// multiple/virtual BSSIDs
#ifndef LINUX26
	{ "nas_alternate",			V_01			},	// only meaningful for ND/K24 builds
#endif

	{ "wl_security_mode",		V_LENGTH(1, 32)		},	// disabled, radius, wep, wpa_personal, wpa_enterprise, wpa2_personal, wpa2_enterprise
	{ "wl_radius_ipaddr",	V_IP				},
	{ "wl_radius_port",		V_PORT				},
	{ "wl_radius_key",		V_LENGTH(1, 64)		},
	{ "wl_wep_bit",			V_RANGE(64, 128)	},	// 64 or 128
	{ "wl_passphrase",		V_LENGTH(0, 20)		},
	{ "wl_key",				V_RANGE(1, 4)		},
	{ "wl_key1",			V_LENGTH(0, 26)		},
	{ "wl_key2",			V_LENGTH(0, 26)		},
	{ "wl_key3",			V_LENGTH(0, 26)		},
	{ "wl_key4",			V_LENGTH(0, 26)		},
	{ "wl_crypto",			V_LENGTH(3, 8)		},	// tkip, aes, tkip+aes
	{ "wl_wpa_psk",			V_LENGTH(8, 64)		},
	{ "wl_wpa_gtk_rekey",	V_RANGE(60, 7200)	},

	{ "wl_lazywds",			V_01				},
	{ "wl_wds",				V_LENGTH(0, 180)	},	// mac mac mac (x 10)

	{ "wl_wds_enable",		V_01				},
	{ "wl_gmode",			V_RANGE(-1, 6)		},
	{ "wl_wep",				V_LENGTH(1, 32)		},	//  off, on, restricted,tkip,aes,tkip+aes
	{ "wl_akm",				V_LENGTH(0, 32)		},	//  wpa, wpa2, psk, psk2, wpa wpa2, psk psk2, ""
	{ "wl_auth_mode",	   	V_LENGTH(4, 6)		},	//  none, radius

	{ "wl_nmode",			V_NONE				},
	{ "wl_nband",			V_RANGE(0, 2)			},	// 2 - 2.4GHz, 1 - 5GHz, 0 - Auto
	{ "wl_nreqd",			V_NONE				},
	{ "wl_nbw_cap",			V_RANGE(0, 3)			},	// 0 - 20MHz, 1 - 40MHz, 2 - Auto, 3 - 80M
	{ "wl_nbw",			V_NONE				},
	{ "wl_mimo_preamble",		V_WORD				},	// 802.11n Preamble: mm/gf/auto/gfbcm
	{ "wl_nctrlsb",			V_NONE				},	// none, lower, upper

#ifdef TCONFIG_IPV6
// basic-ipv6
	{ "ipv6_service",		V_LENGTH(0, 16)			},	// '', native, native-pd, 6to4, sit, other
	{ "ipv6_prefix",		V_IPV6(0)			},
	{ "ipv6_prefix_length",		V_RANGE(3, 127)			},
	{ "ipv6_rtr_addr",		V_IPV6(0)			},
	{ "ipv6_radvd",			V_01				},
	{ "ipv6_dhcpd",			V_01				},
	{ "ipv6_accept_ra",		V_NUM				},
	{ "ipv6_tun_addr",		V_IPV6(1)			},
	{ "ipv6_tun_addrlen",		V_RANGE(3, 127)			},
	{ "ipv6_ifname",		V_LENGTH(0, 8)			},
	{ "ipv6_tun_v4end",		V_IP				},
	{ "ipv6_relay",			V_RANGE(1, 254)			},
	{ "ipv6_tun_mtu",		V_NUM				},	// Tunnel MTU
	{ "ipv6_tun_ttl",		V_NUM				},	// Tunnel TTL
	{ "ipv6_dns",			V_LENGTH(0, 40*3)		},	// ip6 ip6 ip6
	{ "ipv6_6rd_prefix",		V_IPV6(0)			},
	{ "ipv6_6rd_prefix_length",	V_RANGE(3, 127)			},
	{ "ipv6_6rd_borderrelay",	V_IP				},
	{ "ipv6_6rd_ipv4masklen",	V_RANGE(0, 32)			},
	{ "ipv6_vlan",			V_RANGE(0, 7)			},	// Enable IPv6 on 1=LAN1 2=LAN2 4=LAN3
	{ "ipv6_pdonly",		V_01				},	// Request DHCPv6 Prefix Delegation Only
#endif

// basic-wfilter
	{ "wl_macmode",			V_NONE				},	// allow, deny, disabled
	{ "wl_maclist",			V_LENGTH(0, 18*201)	},	// 18 x 200		(11:22:33:44:55:66 ...)
	{ "macnames",			V_LENGTH(0, 62*201)	},	// 62 (12+1+48+1) x 50	(112233445566<..>)		todo: re-use -- zzz

// advanced-ctnf
	{ "ct_max",			V_NUM			},
	{ "ct_tcp_timeout",		V_LENGTH(20, 70)	},
	{ "ct_udp_timeout",		V_LENGTH(5, 15)		},
	{ "ct_timeout",			V_LENGTH(5, 15)		},
	{ "nf_ttl",			V_LENGTH(1, 6)		},
	{ "nf_l7in",			V_01				},
#ifdef LINUX26
	{ "nf_sip",			V_01				},
	{ "ct_hashsize",		V_NUM				},
#endif
	{ "nf_rtsp",			V_01				},
	{ "nf_pptp",			V_01				},
	{ "nf_h323",			V_01				},
	{ "nf_ftp",				V_01				},

// advanced-dhcpdns
	{ "dhcpd_slt",			V_RANGE(-1, 43200)	},	// -1=infinite, 0=follow normal lease time, >=1 custom
	{ "dhcpd_dmdns",		V_01				},
	{ "dhcpd_lmax",			V_NUM				},
	{ "dhcpd_gwmode",		V_NUM				},
	{ "dns_addget",			V_01				},
	{ "dns_intcpt",			V_01				},
	{ "dhcpc_minpkt",		V_01				},
	{ "dhcpc_custom",		V_LENGTH(0, 80)			},
	{ "dns_norebind",		V_01				},
	{ "dnsmasq_custom",		V_TEXT(0, 2048)			},
	{ "dnsmasq_q",			V_RANGE(0,7)			}, //bitfield quiet bit0=dhcp, 1=dhcp6, 2=ra
//	{ "dnsmasq_norw",		V_01				},

// advanced-firewall
	{ "block_wan",			V_01				},
	{ "block_wan_limit",		V_01				},
	{ "block_wan_limit_icmp",	V_RANGE(1, 300)			},
	{ "block_wan_limit_tr",		V_RANGE(1, 300)			},
	{ "multicast_pass",		V_01				},
	{ "multicast_lan",		V_01				},
	{ "multicast_lan1",		V_01				},
	{ "multicast_lan2",		V_01				},
	{ "multicast_lan3",		V_01				},
	{ "block_loopback",		V_01				},
	{ "udpxy_enable",		V_01				},
	{ "udpxy_stats",		V_01				},
	{ "udpxy_clients",		V_RANGE(1, 5000)		},
	{ "udpxy_port",			V_RANGE(0, 65535)		},
	{ "nf_loopback",		V_NUM				},
	{ "ne_syncookies",		V_01				},
	{ "DSCP_fix_enable",		V_01				},
	{ "ne_snat",			V_01				},
	{ "dhcp_pass",			V_01				},
#ifdef TCONFIG_EMF
	{ "emf_entry",			V_NONE				},
	{ "emf_uffp_entry",		V_NONE				},
	{ "emf_rtport_entry",		V_NONE				},
	{ "emf_enable",			V_01				},
#endif

// advanced-misc
	{ "wait_time",			V_RANGE(3, 20)		},
	{ "wan_speed",			V_RANGE(0, 4)		},
	{ "jumbo_frame_enable",		V_01			},	// Jumbo Frames support (for RT-N16/WNR3500L)
	{ "jumbo_frame_size",		V_RANGE(1, 9720)	},
#ifdef CONFIG_BCMWL5
	{ "ctf_disable",		V_01			},
#endif
// advanced-vlan
	{ "vlan0ports",			V_TEXT(0,16)			},
	{ "vlan1ports",			V_TEXT(0,16)			},
	{ "vlan2ports",			V_TEXT(0,16)			},
	{ "vlan3ports",			V_TEXT(0,16)			},
	{ "vlan4ports",			V_TEXT(0,16)			},
	{ "vlan5ports",			V_TEXT(0,16)			},
	{ "vlan6ports",			V_TEXT(0,16)			},
	{ "vlan7ports",			V_TEXT(0,16)			},
	{ "vlan8ports",			V_TEXT(0,16)			},
	{ "vlan9ports",			V_TEXT(0,16)			},
	{ "vlan10ports",		V_TEXT(0,16)			},
	{ "vlan11ports",		V_TEXT(0,16)			},
	{ "vlan12ports",		V_TEXT(0,16)			},
	{ "vlan13ports",		V_TEXT(0,16)			},
	{ "vlan14ports",		V_TEXT(0,16)			},
	{ "vlan15ports",		V_TEXT(0,16)			},
	{ "vlan0hwname",		V_TEXT(0,8)			},
	{ "vlan1hwname",		V_TEXT(0,8)			},
	{ "vlan2hwname",		V_TEXT(0,8)			},
	{ "vlan3hwname",		V_TEXT(0,8)			},
	{ "vlan4hwname",		V_TEXT(0,8)			},
	{ "vlan5hwname",		V_TEXT(0,8)			},
	{ "vlan6hwname",		V_TEXT(0,8)			},
	{ "vlan7hwname",		V_TEXT(0,8)			},
	{ "vlan8hwname",		V_TEXT(0,8)			},
	{ "vlan9hwname",		V_TEXT(0,8)			},
	{ "vlan10hwname",		V_TEXT(0,8)			},
	{ "vlan11hwname",		V_TEXT(0,8)			},
	{ "vlan12hwname",		V_TEXT(0,8)			},
	{ "vlan13hwname",		V_TEXT(0,8)			},
	{ "vlan14hwname",		V_TEXT(0,8)			},
	{ "vlan15hwname",		V_TEXT(0,8)			},
	{ "wan_ifnameX",		V_TEXT(0,8)			},
	{ "wan2_ifnameX",		V_TEXT(0,8)			},
	{ "wan3_ifnameX",		V_TEXT(0,8)			},
	{ "wan4_ifnameX",		V_TEXT(0,8)			},
	{ "lan_ifnames",		V_TEXT(0,64)			},
	{ "manual_boot_nv",		V_01				},
	{ "trunk_vlan_so",		V_01				},
	{ "vlan0tag",		V_TEXT(0,5)			},
	{ "vlan0vid",		V_TEXT(0,5)			},
	{ "vlan1vid",		V_TEXT(0,5)			},
	{ "vlan2vid",		V_TEXT(0,5)			},
	{ "vlan3vid",		V_TEXT(0,5)			},
	{ "vlan4vid",		V_TEXT(0,5)			},
	{ "vlan5vid",		V_TEXT(0,5)			},
	{ "vlan6vid",		V_TEXT(0,5)			},
	{ "vlan7vid",		V_TEXT(0,5)			},
	{ "vlan8vid",		V_TEXT(0,5)			},
	{ "vlan9vid",		V_TEXT(0,5)			},
	{ "vlan10vid",		V_TEXT(0,5)			},
	{ "vlan11vid",		V_TEXT(0,5)			},
	{ "vlan12vid",		V_TEXT(0,5)			},
	{ "vlan13vid",		V_TEXT(0,5)			},
	{ "vlan14vid",		V_TEXT(0,5)			},
	{ "vlan15vid",		V_TEXT(0,5)			},

// advanced-mac
	{ "wan_mac",			V_LENGTH(0, 17)		},
	{ "wan2_mac",			V_LENGTH(0, 17)		},
#ifdef TCONFIG_MULTIWAN
	{ "wan3_mac",			V_LENGTH(0, 17)		},
	{ "wan4_mac",			V_LENGTH(0, 17)		},
#endif
	{ "wl_macaddr",			V_LENGTH(0, 17)		},
	{ "wl_hwaddr",			V_LENGTH(0, 17)		},

// advanced-routing
	{ "routes_static",		V_LENGTH(0, 2048)	},
	{ "dhcp_routes",		V_01			},
	{ "lan_stp",			V_RANGE(0, 1)		},
	{ "wk_mode",			V_LENGTH(1, 32)		},	// gateway, router
#ifdef TCONFIG_ZEBRA
	{ "dr_setting",			V_RANGE(0, 3)		},
	{ "dr_lan_tx",			V_LENGTH(0, 32)		},
	{ "dr_lan_rx",			V_LENGTH(0, 32)		},
	{ "dr_lan1_tx",			V_LENGTH(0, 32)		},
	{ "dr_lan1_rx",			V_LENGTH(0, 32)		},
	{ "dr_lan2_tx",			V_LENGTH(0, 32)		},
	{ "dr_lan2_rx",			V_LENGTH(0, 32)		},
	{ "dr_lan3_tx",			V_LENGTH(0, 32)		},
	{ "dr_lan3_rx",			V_LENGTH(0, 32)		},
	{ "dr_wan_tx",			V_LENGTH(0, 32)		},
	{ "dr_wan_rx",			V_LENGTH(0, 32)		},
#endif

// advanced-access
	{ "lan_access",			V_LENGTH(0, 4096)	},

// advanced-wireless
	{ "wl_country",			V_LENGTH(0, 64)		},	// !!TB - Country code
	{ "wl_country_code",		V_LENGTH(0, 4)		},	// !!TB - Country code
	{ "wl_btc_mode",		V_RANGE(0, 2)		},	// !!TB - BT Coexistence Mode: 0 (disable), 1 (enable), 2 (preemption)
	{ "wl_afterburner",		V_LENGTH(2, 4)		},	// off, on, auto
	{ "wl_auth",			V_01				},
	{ "wl_rateset",			V_LENGTH(2, 7)		},	// all, default, 12
	{ "wl_rate",			V_RANGE(0, 54 * 1000 * 1000)	},
	{ "wl_mrate",			V_RANGE(0, 54 * 1000 * 1000)	},
	{ "wl_gmode_protection",V_LENGTH(3, 4)		},	// off, auto
	{ "wl_frameburst",		V_ONOFF				},	// off, on
	{ "wl_bcn",				V_RANGE(1, 65535)	},
	{ "wl_dtim",			V_RANGE(1, 255)		},
	{ "wl_frag",			V_RANGE(256, 2346)	},
	{ "wl_rts",				V_RANGE(0, 2347)	},
	{ "wl_ap_isolate",		V_01				},
	{ "wl_plcphdr",			V_LENGTH(4, 5)		},	// long, short
	{ "wl_antdiv",			V_RANGE(0, 3)		},
	{ "wl_txant",			V_RANGE(0, 3)		},
	{ "wl_txpwr",			V_RANGE(0, 400)		},
	{ "wl_wme",			V_WORD				},	// auto, off, on
	{ "wl_wme_no_ack",		V_ONOFF				},	// off, on
	{ "wl_wme_apsd",		V_ONOFF				},	// off, on
	{ "wl_maxassoc",		V_RANGE(0, 255)	},
	{ "wl_distance",		V_LENGTH(0, 5)		},	// "", 1-99999
	{ "wlx_hpamp",			V_01				},
	{ "wlx_hperx",			V_01				},
	{ "wl_reg_mode",		V_LENGTH(1, 3)			},	// !!TB - Regulatory: off, h, d
	{ "wl_mitigation",		V_RANGE(0, 4)			},	// Interference Mitigation Mode (0|1|2|3|4)

	{ "wl_nmode_protection",	V_WORD,				},	// off, auto
	{ "wl_nmcsidx",			V_RANGE(-2, 32),	},	// -2 - 32
	{ "wl_obss_coex",		V_01			},
	{ "wl_wmf_bss_enable",		V_01			},	// Toastman

// forward-dmz
	{ "dmz_enable",			V_01				},
	{ "dmz_ipaddr",			V_LENGTH(0, 15)		},
	{ "dmz_sip",			V_LENGTH(0, 512)	},
	{ "dmz_ifname",			V_LENGTH(0, 5)			},
	{ "dmz_ra",			V_01			},

// forward-upnp
	{ "upnp_enable",		V_NUM				},
	{ "upnp_secure",		V_01				},
	{ "upnp_port",			V_RANGE(0, 65535)		},
	{ "upnp_ssdp_interval",		V_RANGE(10, 9999)		},
	{ "upnp_mnp",			V_01				},
	{ "upnp_clean",			V_01				},
	{ "upnp_clean_interval",	V_RANGE(60, 65535)		},
	{ "upnp_clean_threshold",	V_RANGE(0, 9999)		},
	{ "upnp_min_port_int",		V_PORT				},
	{ "upnp_max_port_int",		V_PORT				},
	{ "upnp_min_port_ext",		V_PORT				},
	{ "upnp_max_port_ext",		V_PORT				},
	{ "upnp_lan",			V_01				},
	{ "upnp_lan1",			V_01				},
	{ "upnp_lan2",			V_01				},
	{ "upnp_lan3",			V_01				},
	{ "upnp_custom",		V_TEXT(0, 2048)			},

// forward-basic
	{ "portforward",		V_LENGTH(0, 4096)	},

#ifdef TCONFIG_IPV6
// forward-basic-ipv6
	{ "ipv6_portforward",	V_LENGTH(0, 4096)	},
#endif

// forward-triggered
	{ "trigforward",		V_LENGTH(0, 4096)	},


// access restriction
	{ "rruleN",				V_RANGE(0, 99)		},
//	{ "rrule##",			V_LENGTH(0, 2048)	},	// in save_variables()

// admin-access
	{ "http_enable",		V_01				},
	{ "https_enable",		V_01				},
	{ "https_crt_save",		V_01				},
	{ "https_crt_cn",		V_LENGTH(0, 64)		},
	{ "https_crt_gen",		V_TEMP				},
	{ "remote_management",	V_01				},
	{ "remote_mgt_https",	V_01				},
	{ "http_lanport",		V_PORT				},
	{ "https_lanport",		V_PORT				},
	{ "web_wl_filter",		V_01				},
	{ "web_css",			V_LENGTH(1, 32)		},
	{ "web_dir",			V_LENGTH(1, 32)		},
	{ "ttb_css",			V_LENGTH(0, 128)		},
	{ "web_mx",				V_LENGTH(0, 128)	},
	{ "http_wanport",		V_PORT				},
	{ "telnetd_eas",		V_01				},
	{ "telnetd_port",		V_PORT				},
	{ "sshd_eas",			V_01				},
	{ "sshd_pass",			V_01				},
	{ "sshd_port",			V_PORT				},
	{ "sshd_remote",		V_01				},
	{ "sshd_motd",			V_01				},
	{ "sshd_forwarding",		V_01				},
	{ "sshd_rport", 		V_PORT				},
	{ "sshd_authkeys",		V_TEXT(0, 4096)		},
	{ "rmgt_sip",			V_LENGTH(0, 512)	},
	{ "ne_shlimit",			V_TEXT(1, 50)		},
	{ "http_username",		V_LENGTH(0, 32)		},
	{ "http_root",			V_01				},

// admin-bwm
	{ "rstats_enable",		V_01				},
	{ "rstats_path",		V_LENGTH(0, 48)		},
	{ "rstats_stime",		V_RANGE(1, 168)		},
	{ "rstats_offset",		V_RANGE(1, 31)		},
	{ "rstats_exclude",		V_LENGTH(0, 64)		},
	{ "rstats_sshut",		V_01				},
	{ "rstats_bak",			V_01				},

// admin-ipt
	{ "cstats_enable",		V_01				},
	{ "cstats_path",		V_LENGTH(0, 48)			},
	{ "cstats_stime",		V_RANGE(1, 168)			},
	{ "cstats_offset",		V_RANGE(1, 31)			},
	{ "cstats_labels",		V_RANGE(0, 2)			},
	{ "cstats_exclude",		V_LENGTH(0, 512)		},
	{ "cstats_include",		V_LENGTH(0, 2048)		},
	{ "cstats_all",			V_01				},
	{ "cstats_sshut",		V_01				},
	{ "cstats_bak",			V_01				},

// admin-buttons
	{ "sesx_led",			V_RANGE(0, 255)		},	// amber, white, aoss
	{ "sesx_b0",			V_RANGE(0, 5)		},	// 0-5: toggle wireless, reboot, shutdown, script, usb unmount
	{ "sesx_b1",			V_RANGE(0, 5)		},	// "
	{ "sesx_b2",			V_RANGE(0, 5)		},	// "
	{ "sesx_b3",			V_RANGE(0, 5)		},	// "
	{ "sesx_script",		V_TEXT(0, 1024)		},	//
	{ "script_brau",		V_TEXT(0, 1024)		},	//

// admin-debug
	{ "debug_nocommit",		V_01				},
	{ "debug_cprintf",		V_01				},
	{ "debug_cprintf_file",	V_01				},
//	{ "debug_keepfiles",	V_01				},
	{ "debug_ddns",			V_01				},
	{ "debug_norestart",	V_TEXT(0, 128)		},
	{ "console_loglevel",	V_RANGE(1, 8)		},
	{ "t_cafree",			V_01				},
	{ "t_hidelr",			V_01				},

// admin-sched
	{ "sch_rboot", 			V_TEXT(0, 64)		},
	{ "sch_rcon", 			V_TEXT(0, 64)		},
	{ "sch_c1",			V_TEXT(0, 64)		},
	{ "sch_c1_cmd",			V_TEXT(0, 2048)		},
	{ "sch_c2",			V_TEXT(0, 64)		},
	{ "sch_c2_cmd",			V_TEXT(0, 2048)		},
	{ "sch_c3",			V_TEXT(0, 64)		},
	{ "sch_c3_cmd",			V_TEXT(0, 2048)		},
	{ "sch_c4",			V_TEXT(0, 64)		},
	{ "sch_c4_cmd",			V_TEXT(0, 2048)		},
	{ "sch_c5",			V_TEXT(0, 64)		},
	{ "sch_c5_cmd",			V_TEXT(0, 2048)		},

// admin-scripts
	{ "script_init", 		V_TEXT(0, 4096)		},
	{ "script_shut", 		V_TEXT(0, 4096)		},
	{ "script_fire", 		V_TEXT(0, 8192)		},
	{ "script_wanup", 		V_TEXT(0, 4096)		},

// admin-log
	{ "log_remote",			V_01				},
	{ "log_remoteip",		V_LENGTH(0, 512)		},
	{ "log_remoteport",		V_PORT				},
	{ "log_file",			V_01				},
	{ "log_file_custom",		V_01				},
	{ "log_file_path",		V_TEXT(0, 4096)			},
	{ "log_file_size",		V_RANGE(0, 99999)	},
	{ "log_file_keep",		V_RANGE(0, 99)		},
	{ "log_limit",			V_RANGE(0, 2400)	},
	{ "log_in",				V_RANGE(0, 3)		},
	{ "log_out",			V_RANGE(0, 3)		},
	{ "log_mark",			V_RANGE(0, 99999)	},
	{ "log_events",			V_TEXT(0, 32)		},	// "acre,crond,ntp"

// admin-log-webmonitor
	{ "log_wm",			V_01				},
	{ "log_wmtype",			V_RANGE(0, 2)			},
	{ "log_wmip",			V_LENGTH(0, 512)		},
	{ "log_wmdmax",			V_RANGE(0, 9999)		},
	{ "log_wmsmax",			V_RANGE(0, 9999)		},
	{ "webmon_bkp",			V_01				},
	{ "webmon_dir",			V_LENGTH(0, 256)		},
	{ "webmon_shrink",		V_01				},


// admin-cifs
	{ "cifs1",				V_LENGTH(1, 1024)	},
	{ "cifs2",				V_LENGTH(1, 1024)	},

// admin-jffs2
	{ "jffs2_on",			V_01				},
	{ "jffs2_exec",			V_LENGTH(0, 64)		},
	{ "jffs2_format",		V_01				},

#ifdef TCONFIG_SDHC
// admin-sdhc
	{ "mmc_on",			V_01				},
	{ "mmc_cs",			V_RANGE(1, 7)			},	// GPIO pin
	{ "mmc_clk",			V_RANGE(1, 7)			},	// GPIO pin
	{ "mmc_din",			V_RANGE(1, 7)			},	// GPIO pin
	{ "mmc_dout",			V_RANGE(1, 7)			},	// GPIO pin
	{ "mmc_fs_partition",		V_RANGE(1, 4)			},	// partition number in partition table
	{ "mmc_fs_type",		V_LENGTH(4, 4)			},	// ext2, ext3, vfat
	{ "mmc_exec_mount",		V_LENGTH(0, 64)			},
	{ "mmc_exec_umount",		V_LENGTH(0, 64)			},
#endif

// admin-tomatoanon
	{ "tomatoanon_answer",		V_RANGE(0, 1)			},
	{ "tomatoanon_enable",		V_RANGE(-1, 1)			},
	{ "tomatoanon_id",		V_LENGTH(0, 32)			},
	{ "tomatoanon_notify",		V_01					},
		
// AdvancedTomato
	{ "at_update",      V_LENGTH(0,32)    },
	{ "at_navi",        V_LENGTH(0,32)    },
	{ "web_nav",        V_TEXT(0, 4096)   }, 

// nas-usb - !!TB
#ifdef TCONFIG_USB
	{ "usb_enable",			V_01				},
	{ "usb_uhci",			V_RANGE(-1, 1)			},	// -1 - disabled, 0 - off, 1 - on
	{ "usb_ohci",			V_RANGE(-1, 1)			},
	{ "usb_usb2",			V_RANGE(-1, 1)			},
	{ "usb_irq_thresh",		V_RANGE(0, 6)			},
	{ "usb_storage",		V_01				},
	{ "usb_printer",		V_01				},
	{ "usb_printer_bidirect",	V_01				},
	{ "usb_fs_ext3",		V_01				},
	{ "usb_fs_fat",			V_01				},
#ifdef TCONFIG_NTFS
	{ "usb_fs_ntfs",		V_01				},
#endif
#ifdef TCONFIG_HFS
	{ "usb_fs_hfs",			V_01				}, //!Victek
#endif
	{ "usb_automount",		V_01				},
	{ "script_usbhotplug", 		V_TEXT(0, 2048)			},
	{ "script_usbmount", 		V_TEXT(0, 2048)			},
	{ "script_usbumount", 		V_TEXT(0, 2048)			},
	{ "idle_enable",		V_01				},
	{ "usb_3g",			V_01				},
#endif

// nas-ftp - !!TB
#ifdef TCONFIG_FTP
	{ "ftp_enable",			V_RANGE(0, 2)			},
	{ "ftp_super",			V_01				},
	{ "ftp_anonymous",		V_RANGE(0, 3)			},
	{ "ftp_dirlist",		V_RANGE(0, 2)			},
	{ "ftp_port",			V_PORT				},
	{ "ftp_max",			V_RANGE(0, 12)			},
	{ "ftp_ipmax",			V_RANGE(0, 12)			},
	{ "ftp_staytimeout",		V_RANGE(0, 65535)		},
	{ "ftp_rate",			V_RANGE(0, 99999)		},
	{ "ftp_anonrate",		V_RANGE(0, 99999)		},
	{ "ftp_anonroot",		V_LENGTH(0, 256)		},
	{ "ftp_pubroot",		V_LENGTH(0, 256)		},
	{ "ftp_pvtroot",		V_LENGTH(0, 256)		},
	{ "ftp_users",			V_LENGTH(0, 4096)		},
	{ "ftp_custom",			V_TEXT(0, 2048)			},
	{ "ftp_sip",			V_LENGTH(0, 512)		},
	{ "ftp_limit",			V_TEXT(1, 50)			},
	{ "log_ftp",			V_01				},
#endif

#ifdef TCONFIG_SNMP
	{ "snmp_enable",		V_RANGE(0, 1)			},
	{ "snmp_port",			V_RANGE(1, 65535)		},
	{ "snmp_remote",		V_RANGE(0, 1)			},
	{ "snmp_remote_sip",		V_LENGTH(0, 512)		},
	{ "snmp_location",		V_LENGTH(0, 40)			},
	{ "snmp_contact",		V_LENGTH(0, 40)			},
	{ "snmp_ro",			V_LENGTH(0, 40)			},
#endif

#ifdef TCONFIG_SAMBASRV
// nas-samba - !!TB
	{ "smbd_enable",		V_RANGE(0, 2)			},
	{ "smbd_wgroup",		V_LENGTH(0, 20)			},
	{ "smbd_master",		V_01				},
	{ "smbd_wins",			V_01				},
	{ "smbd_cpage",			V_LENGTH(0, 4)			},
	{ "smbd_cset",			V_LENGTH(0, 20)			},
	{ "smbd_custom",		V_TEXT(0, 2048)			},
	{ "smbd_autoshare",		V_RANGE(0, 3)			},
	{ "smbd_shares",		V_LENGTH(0, 4096)		},
	{ "smbd_user",			V_LENGTH(0, 50)			},
	{ "smbd_passwd",		V_LENGTH(0, 50)			},
#endif

#ifdef TCONFIG_MEDIA_SERVER
// nas-media
	{ "ms_enable",			V_01				},
	{ "ms_dirs",			V_LENGTH(0, 1024)		},
	{ "ms_port",			V_RANGE(0, 65535)		},
	{ "ms_dbdir",			V_LENGTH(0, 256)		},
	{ "ms_tivo",			V_01				},
	{ "ms_stdlna",			V_01				},
	{ "ms_rescan",			V_01				},
	{ "ms_sas",			V_01				},
#endif

//	qos
	{ "qos_enable",			V_01				},
	{ "qos_ack",			V_01				},
	{ "qos_syn",			V_01				},
	{ "qos_fin",			V_01				},
	{ "qos_rst",			V_01				},
	{ "qos_icmp",			V_01				},
	{ "qos_udp",			V_01				},
	{ "qos_reset",			V_01				},
	{ "qos_pfifo",			V_01				}, // !!TB
	{ "wan_qos_obw",		V_RANGE(10, 999999)		},
	{ "wan_qos_ibw",		V_RANGE(10, 999999)		},
	{ "wan2_qos_obw",		V_RANGE(10, 999999)		},
	{ "wan2_qos_ibw",		V_RANGE(10, 999999)		},
#ifdef TCONFIG_MULTIWAN
	{ "wan3_qos_obw",		V_RANGE(10, 999999)		},
	{ "wan3_qos_ibw",		V_RANGE(10, 999999)		},
	{ "wan4_qos_obw",		V_RANGE(10, 999999)		},
	{ "wan4_qos_ibw",		V_RANGE(10, 999999)		},
#endif

	{ "qos_orules",			V_LENGTH(0, 4096)	},
	{ "qos_default",		V_RANGE(0, 9)		},
	{ "qos_irates",			V_LENGTH(0, 128)	},
	{ "qos_orates",			V_LENGTH(0, 128)	},
	{ "qos_classnames",		V_LENGTH(10, 128)		}, // !!TOASTMAN
	{ "atm_overhead",		V_RANGE(-127, 128)	},
	{ "ne_vegas",			V_01				},
	{ "ne_valpha",			V_NUM				},
	{ "ne_vbeta",			V_NUM				},
	{ "ne_vgamma",			V_NUM				},

// new_qoslimit
	{ "new_qoslimit_enable",         V_01                   },
	{ "new_qoslimit_rules",          V_LENGTH(0, 4096)      },
	{ "qosl_enable",                 V_01                   },
	{ "qosl_ulr",                    V_RANGE(0, 999999)     },
	{ "qosl_ulc",                    V_RANGE(0, 999999)     },
	{ "qosl_dlr",                    V_RANGE(0, 999999)     },
	{ "qosl_dlc",                    V_RANGE(0, 999999)     },
	{ "qosl_tcp",                    V_RANGE(0, 1000)       },
	{ "qosl_udp",                    V_RANGE(0, 100)        },
	{ "limit_br0_prio",              V_RANGE(0, 5)          },
	{ "limit_br1_enable",            V_01                   },
	{ "limit_br1_ulr",               V_RANGE(0, 999999)     },
	{ "limit_br1_ulc",               V_RANGE(0, 999999)     },
	{ "limit_br1_dlr",               V_RANGE(0, 999999)     },
	{ "limit_br1_dlc",               V_RANGE(0, 999999)     },
	{ "limit_br1_prio",              V_RANGE(0, 5)          },
	{ "limit_br2_enable",            V_01                   },
	{ "limit_br2_ulr",               V_RANGE(0, 999999)     },
	{ "limit_br2_ulc",               V_RANGE(0, 999999)     },
	{ "limit_br2_dlr",               V_RANGE(0, 999999)     },
	{ "limit_br2_dlc",               V_RANGE(0, 999999)     },
	{ "limit_br2_prio",              V_RANGE(0, 5)          },
	{ "limit_br3_enable",            V_01                   },
	{ "limit_br3_ulr",               V_RANGE(0, 999999)     },
	{ "limit_br3_ulc",               V_RANGE(0, 999999)     },
	{ "limit_br3_dlr",               V_RANGE(0, 999999)     },
	{ "limit_br3_dlc",               V_RANGE(0, 999999)     },
	{ "limit_br3_prio",              V_RANGE(0, 5)          },


#ifdef TCONFIG_BT
// nas-transmission
	{ "bt_enable",                  V_01                            },
	{ "bt_binary",                  V_LENGTH(0, 50)                 },
	{ "bt_binary_custom",           V_LENGTH(0, 50)                 },
	{ "bt_custom",                  V_TEXT(0, 2048)                 },
	{ "bt_port",                    V_PORT                          },
	{ "bt_dir",                     V_LENGTH(0, 50)                 },
	{ "bt_settings",                V_LENGTH(0, 50)                 },
	{ "bt_settings_custom",         V_LENGTH(0, 50)                 },
	{ "bt_incomplete",              V_01                            },
	{ "bt_rpc_enable",              V_01                            },
	{ "bt_rpc_wan",                 V_01                            },
	{ "bt_auth",                    V_01                            },
	{ "bt_login",                   V_LENGTH(0, 50)                 },
	{ "bt_password",                V_LENGTH(0, 50)                 },
	{ "bt_port_gui",                V_PORT                          },
	{ "bt_dl_enable",               V_01                            },
	{ "bt_ul_enable",               V_01                            },
	{ "bt_dl",                      V_RANGE(0, 999999)              },
	{ "bt_ul",                      V_RANGE(0, 999999)              },
	{ "bt_peer_limit_global",       V_RANGE(10, 1000)               },
	{ "bt_peer_limit_per_torrent",  V_RANGE(1, 200)                 },
	{ "bt_ul_slot_per_torrent",     V_RANGE(1, 50)                  },
	{ "bt_ratio_enable",            V_01                            },
	{ "bt_ratio",                   V_LENGTH(0, 999999)             },
	{ "bt_ratio_idle_enable",       V_01                            },
	{ "bt_ratio_idle",              V_RANGE(1, 55)                  },
	{ "bt_dht",                     V_01                            },
	{ "bt_pex",                     V_01                            },
	{ "bt_lpd",                     V_01                            },
	{ "bt_utp",                     V_01                            },
	{ "bt_blocklist",               V_01                            },
	{ "bt_blocklist_url",           V_LENGTH(0, 80)                 },
	{ "bt_sleep",                   V_RANGE(1, 60)                  },
	{ "bt_check",                   V_01                            },
	{ "bt_check_time",              V_RANGE(1, 55)                  },
	{ "bt_dl_queue_enable",         V_01                            },
	{ "bt_dl_queue_size",           V_RANGE(1, 30)                  },
	{ "bt_ul_queue_enable",         V_01                            },
	{ "bt_ul_queue_size",           V_RANGE(1, 30)                  },
	{ "bt_message",                 V_RANGE(0, 3)                   },
	{ "bt_log",                     V_01                            },
	{ "bt_log_path",                V_LENGTH(0, 50)                 },

#endif

#ifdef TCONFIG_NFS
	{ "nfs_enable",			V_01				},
	{ "nfs_exports",		V_LENGTH(0, 4096)		},
#endif

//NotCatSplash. Victek.
#ifdef TCONFIG_NOCAT
	{ "NC_enable",			V_01				},
	{ "NC_Verbosity",		V_RANGE(0, 10)			},
	{ "NC_GatewayName",		V_LENGTH(0, 255)		},
	{ "NC_GatewayPort",		V_PORT				},
	{ "NC_ForcedRedirect",		V_01				},
	{ "NC_HomePage",		V_LENGTH(0, 255)		},
	{ "NC_DocumentRoot",		V_LENGTH(0, 255)		},
	{ "NC_SplashURL",		V_LENGTH(0, 255)		},
	{ "NC_LoginTimeout",		V_RANGE(0, 86400000)		},
	{ "NC_IdleTimeout",		V_RANGE(0, 86400000)		},
	{ "NC_MaxMissedARP",		V_RANGE(0, 10)			},
	{ "NC_PeerChecktimeout",	V_RANGE(0, 60)			},
	{ "NC_ExcludePorts",		V_LENGTH(0, 255)		},
	{ "NC_IncludePorts",		V_LENGTH(0, 255)		},
	{ "NC_AllowedWebHosts",		V_LENGTH(0, 255)		},
	{ "NC_MACWhiteList",		V_LENGTH(0, 255)		},
	{ "NC_SplashFile",		V_LENGTH(0, 8192)		},
	{ "NC_BridgeLAN",		V_LENGTH(0, 50)			},
#endif

// NGinX Roadkill-Victek
#ifdef TCONFIG_NGINX
	{"nginx_enable",		V_01			}, // NGinX enabled
	{"nginx_php",			V_01			}, // PHP enabled
	{"nginx_keepconf",		V_01			}, // NGinX configuration files overwrite flag
	{"nginx_docroot",		V_LENGTH(0, 255)	}, // root files path
	{"nginx_port",			V_PORT			}, // listening port
	{"nginx_fqdn",			V_LENGTH(0, 255)	}, // server name
	{"nginx_upload",		V_LENGTH(1, 1000)	}, // upload file size limit
	{"nginx_remote",		V_01			},
	{"nginx_priority",		V_LENGTH(0, 255)	}, // server priority
	{"nginx_custom",		V_TEXT(0, 4096)		}, // user window to add parameters to nginx.conf
	{"nginx_httpcustom",		V_TEXT(0, 4096)		}, // user window to add parameters to nginx.conf
	{"nginx_servercustom",		V_TEXT(0, 4096)		}, // user window to add parameters to nginx.conf
	{"nginx_phpconf",		V_TEXT(0, 4096)		}, // user window to add parameters to php.ini
	{"nginx_user",			V_LENGTH(0, 255)	}, // user used to start nginx and spawn-fcgi
	{"nginx_override",		V_01			},
	{"nginx_overridefile",		V_TEXT(0, 4096)		},

// bwq518 - MySQL
        { "mysql_enable",                               V_01                    },
        { "mysql_sleep",                                V_RANGE(1,60)           },
        { "mysql_check",                                V_01                    },
        { "mysql_check_time",                           V_RANGE(1,55)           },
        { "mysql_binary",                               V_LENGTH(0, 50)         },
        { "mysql_binary_custom",                        V_LENGTH(0, 50)         },
        { "mysql_usb_enable",                           V_01                    },
        { "mysql_dlroot",                               V_LENGTH(0,50)          },
        { "mysql_datadir",                              V_LENGTH(0,64)          },
        { "mysql_tmpdir",                               V_LENGTH(0,64)          },
        { "mysql_server_custom",                        V_TEXT(0,1024)          },
        { "mysql_port",                                 V_PORT                  },
        { "mysql_allow_anyhost",                        V_01                    },
        { "mysql_init_rootpass",                        V_01                    },
        { "mysql_username",                             V_TEXT(0,50)            }, // mysqladmin username
        { "mysql_passwd",                               V_TEXT(0,50)            }, // mysqladmin password
        { "mysql_key_buffer",                           V_RANGE(0,1024)         }, // MB
        { "mysql_max_allowed_packet",                   V_RANGE(0,1024)         }, // MB
        { "mysql_thread_stack",                         V_RANGE(0,1024000)      }, // KB
        { "mysql_thread_cache_size",                    V_RANGE(0,999999)       },
        { "mysql_init_priv",                            V_01                    },
        { "mysql_table_open_cache",                     V_RANGE(1,999999)       },
        { "mysql_sort_buffer_size",                     V_RANGE(0,1024000)      }, //KB
        { "mysql_read_buffer_size",                     V_RANGE(0,1024000)      }, //KB
        { "mysql_query_cache_size",                     V_RANGE(0,1024)         }, //MB
        { "mysql_read_rnd_buffer_size",                 V_RANGE(0,1024000)      }, //KB
        { "mysql_net_buffer_length",                    V_RANGE(0,1024)         }, //K
        { "mysql_max_connections",                      V_RANGE(0,999999)       },
#endif

#ifdef TCONFIG_OPENVPN
// vpn
	{ "vpn_debug",            V_01                },
	{ "vpn_server_eas",       V_NONE              },
	{ "vpn_server_dns",       V_NONE              },
	{ "vpn_server1_poll",     V_RANGE(0, 1440)    },
	{ "vpn_server1_if",       V_TEXT(3, 3)        },  // tap, tun
	{ "vpn_server1_proto",    V_TEXT(3, 10)       },  // udp, tcp-server
	{ "vpn_server1_port",     V_PORT              },
	{ "vpn_server1_firewall", V_TEXT(0, 8)        },  // auto, external, custom
	{ "vpn_server1_crypt",    V_TEXT(0, 6)        },  // tls, secret, custom
	{ "vpn_server1_comp",     V_TEXT(0, 8)        },  // yes, no, adaptive
	{ "vpn_server1_cipher",   V_TEXT(0, 16)       },
	{ "vpn_server1_dhcp",     V_01                },
	{ "vpn_server1_r1",       V_IP                },
	{ "vpn_server1_r2",       V_IP                },
	{ "vpn_server1_sn",       V_IP                },
	{ "vpn_server1_nm",       V_IP                },
	{ "vpn_server1_local",    V_IP                },
	{ "vpn_server1_remote",   V_IP                },
	{ "vpn_server1_reneg",    V_RANGE(-1,2147483647)},
	{ "vpn_server1_hmac",     V_RANGE(-1, 2)      },
	{ "vpn_server1_plan",     V_01                },
	{ "vpn_server1_ccd",      V_01                },
	{ "vpn_server1_c2c",      V_01                },
	{ "vpn_server1_ccd_excl", V_01                },
	{ "vpn_server1_ccd_val",  V_NONE              },
	{ "vpn_server1_pdns",     V_01                },
	{ "vpn_server1_rgw",      V_01                },
	{ "vpn_server1_userpass", V_01                },
	{ "vpn_server1_nocert",   V_01                },
	{ "vpn_server1_users_val",V_NONE              },
	{ "vpn_server1_custom",   V_NONE              },
	{ "vpn_server1_static",   V_NONE              },
	{ "vpn_server1_ca",       V_NONE              },
	{ "vpn_server1_crt",      V_NONE              },
	{ "vpn_server1_key",      V_NONE              },
	{ "vpn_server1_dh",       V_NONE              },
	{ "vpn_server1_br",       V_LENGTH(0, 50)     },
	{ "vpn_server1_route",    V_01                },
	{ "vpn_server1_routing_val", V_NONE           },
	{ "vpn_server2_poll",     V_RANGE(0, 1440)    },
	{ "vpn_server2_if",       V_TEXT(3, 3)        },  // tap, tun
	{ "vpn_server2_proto",    V_TEXT(3, 10)       },  // udp, tcp-server
	{ "vpn_server2_port",     V_PORT              },
	{ "vpn_server2_firewall", V_TEXT(0, 8)        },  // auto, external, custom
	{ "vpn_server2_crypt",    V_TEXT(0, 6)        },  // tls, secret, custom
	{ "vpn_server2_comp",     V_TEXT(0, 8)        },  // yes, no, adaptive
	{ "vpn_server2_cipher",   V_TEXT(0, 16)       },
	{ "vpn_server2_dhcp",     V_01                },
	{ "vpn_server2_r1",       V_IP                },
	{ "vpn_server2_r2",       V_IP                },
	{ "vpn_server2_sn",       V_IP                },
	{ "vpn_server2_nm",       V_IP                },
	{ "vpn_server2_local",    V_IP                },
	{ "vpn_server2_remote",   V_IP                },
	{ "vpn_server2_reneg",    V_RANGE(-1,2147483647)},
	{ "vpn_server2_hmac",     V_RANGE(-1, 2)      },
	{ "vpn_server2_plan",     V_01                },
	{ "vpn_server2_pdns",     V_01                },
	{ "vpn_server2_rgw",      V_01                },
	{ "vpn_server2_userpass", V_01                },
	{ "vpn_server2_nocert",   V_01                },
	{ "vpn_server2_users_val",V_NONE              },
	{ "vpn_server2_custom",   V_NONE              },
	{ "vpn_server2_ccd",      V_01                },
	{ "vpn_server2_c2c",      V_01                },
	{ "vpn_server2_ccd_excl", V_01                },
	{ "vpn_server2_ccd_val",  V_NONE              },
	{ "vpn_server2_static",   V_NONE              },
	{ "vpn_server2_ca",       V_NONE              },
	{ "vpn_server2_crt",      V_NONE              },
	{ "vpn_server2_key",      V_NONE              },
	{ "vpn_server2_dh",       V_NONE              },
	{ "vpn_server2_br",       V_LENGTH(0, 50)     },
	{ "vpn_server2_route",    V_01                },
	{ "vpn_server2_routing_val", V_NONE           },
	{ "vpn_client_eas",       V_NONE              },
	{ "vpn_client1_poll",     V_RANGE(0, 1440)    },
	{ "vpn_client1_if",       V_TEXT(3, 3)        },  // tap, tun
	{ "vpn_client1_bridge",   V_01                },
	{ "vpn_client1_nat",      V_01                },
	{ "vpn_client1_proto",    V_TEXT(3, 10)       },  // udp, tcp-server
	{ "vpn_client1_addr",     V_NONE              },
	{ "vpn_client1_port",     V_PORT              },
	{ "vpn_client1_retry",    V_RANGE(-1,32767)   },  // -1 infinite, 0 disabled, >= 1 custom
	{ "vpn_client1_firewall", V_TEXT(0, 6)        },  // auto, custom
	{ "vpn_client1_crypt",    V_TEXT(0, 6)        },  // tls, secret, custom
	{ "vpn_client1_comp",     V_TEXT(0, 8)        },  // yes, no, adaptive
	{ "vpn_client1_cipher",   V_TEXT(0, 16)       },
	{ "vpn_client1_local",    V_IP                },
	{ "vpn_client1_remote",   V_IP                },
	{ "vpn_client1_nm",       V_IP                },
	{ "vpn_client1_reneg",    V_RANGE(-1,2147483647)},
	{ "vpn_client1_hmac",     V_RANGE(-1, 2)      },
	{ "vpn_client1_adns",     V_RANGE(0, 3)       },
	{ "vpn_client1_rgw",      V_01                },
	{ "vpn_client1_gw",       V_TEXT(0, 15)       },
	{ "vpn_client1_custom",   V_NONE              },
	{ "vpn_client1_static",   V_NONE              },
	{ "vpn_client1_ca",       V_NONE              },
	{ "vpn_client1_crt",      V_NONE              },
	{ "vpn_client1_key",      V_NONE              },
	{ "vpn_client1_userauth", V_01                },
	{ "vpn_client1_username", V_TEXT(0,50)        },
	{ "vpn_client1_password", V_TEXT(0,50)        },
	{ "vpn_client1_useronly", V_01                },
	{ "vpn_client1_tlsremote",V_01                },
	{ "vpn_client1_cn",       V_NONE              },
	{ "vpn_client1_br",       V_LENGTH(0, 50)     },
	{ "vpn_client1_nopull",   V_01                },
	{ "vpn_client1_route",    V_01                },
	{ "vpn_client1_routing_val", V_NONE           },
	{ "vpn_client2_poll",     V_RANGE(0, 1440)    },
	{ "vpn_client2_if",       V_TEXT(3, 3)        },  // tap, tun
	{ "vpn_client2_bridge",   V_01                },
	{ "vpn_client2_nat",      V_01                },
	{ "vpn_client2_proto",    V_TEXT(3, 10)       },  // udp, tcp-server
	{ "vpn_client2_addr",     V_NONE              },
	{ "vpn_client2_port",     V_PORT              },
	{ "vpn_client2_retry",    V_RANGE(-1,32767)   },  // -1 infinite, 0 disabled, >= 1 custom
	{ "vpn_client2_firewall", V_TEXT(0, 6)        },  // auto, custom
	{ "vpn_client2_crypt",    V_TEXT(0, 6)        },  // tls, secret, custom
	{ "vpn_client2_comp",     V_TEXT(0, 8)        },  // yes, no, adaptive
	{ "vpn_client2_cipher",   V_TEXT(0, 16)       },
	{ "vpn_client2_local",    V_IP                },
	{ "vpn_client2_remote",   V_IP                },
	{ "vpn_client2_nm",       V_IP                },
	{ "vpn_client2_reneg",    V_RANGE(-1,2147483647)},
	{ "vpn_client2_hmac",     V_RANGE(-1, 2)      },
	{ "vpn_client2_adns",     V_RANGE(0, 3)       },
	{ "vpn_client2_rgw",      V_01                },
	{ "vpn_client2_gw",       V_TEXT(0, 15)       },
	{ "vpn_client2_custom",   V_NONE              },
	{ "vpn_client2_static",   V_NONE              },
	{ "vpn_client2_ca",       V_NONE              },
	{ "vpn_client2_crt",      V_NONE              },
	{ "vpn_client2_key",      V_NONE              },
	{ "vpn_client2_userauth", V_01                },
	{ "vpn_client2_username", V_TEXT(0,50)        },
	{ "vpn_client2_password", V_TEXT(0,50)        },
	{ "vpn_client2_useronly", V_01                },
	{ "vpn_client2_tlsremote",V_01                },
	{ "vpn_client2_cn",       V_NONE              },
	{ "vpn_client2_br",       V_LENGTH(0, 50)     },
	{ "vpn_client2_nopull",   V_01                },
	{ "vpn_client2_route",    V_01                },
	{ "vpn_client2_routing_val", V_NONE           },
#endif // vpn

#ifdef TCONFIG_PPTPD
// pptp server
	{ "pptpd_enable",		V_01			},
	{ "pptpd_remoteip",		V_TEXT(0,24)		},
	{ "pptpd_forcemppe",		V_01			},
	{ "pptpd_users",		V_TEXT(0, 67*16)	},
	{ "pptpd_broadcast",		V_TEXT(0,8)		},
	{ "pptpd_dns1",			V_TEXT(0, 15)		},
	{ "pptpd_dns2",			V_TEXT(0, 15)		},
	{ "pptpd_wins1",		V_TEXT(0, 15)		},
	{ "pptpd_wins2",		V_TEXT(0, 15)		},
	{ "pptpd_mtu",			V_RANGE(576, 1500)	},
	{ "pptpd_mru",			V_RANGE(576, 1500)	},
	{ "pptpd_custom",		V_TEXT(0, 2048)		},
#endif

#ifdef TCONFIG_TINC
	{"tinc_wanup",			V_RANGE(0, 1)		},
	{"tinc_name",			V_LENGTH(0, 30)		},
	{"tinc_devicetype",		V_TEXT(3, 3)		}, // tun, tap
	{"tinc_mode",			V_TEXT(3, 6)		}, // switch, hub
	{"tinc_vpn_netmask",		V_IP			},
	{"tinc_private_rsa",		V_NONE			},
	{"tinc_private_ed25519",	V_NONE			},
	{"tinc_custom",			V_NONE			},
	{"tinc_hosts",			V_NONE			},
	{"tinc_manual_firewall",	V_RANGE(0, 2)		},
	{"tinc_manual_tinc_up",		V_RANGE(0, 1)		},
	// scripts
	{"tinc_tinc_up",		V_NONE			},
	{"tinc_tinc_down",		V_NONE			},
	{"tinc_host_up",		V_NONE			},
	{"tinc_host_down",		V_NONE			},
	{"tinc_subnet_up",		V_NONE			},
	{"tinc_subnet_down",		V_NONE			},
	{"tinc_firewall",		V_NONE			},
#endif

#ifdef TCONFIG_TOR
	{ "tor_enable",			V_01			},
	{ "tor_socksport",		V_RANGE(1,65535)	},
	{ "tor_transport",		V_RANGE(1,65535)	},
	{ "tor_dnsport",		V_RANGE(1,65535)	},
	{ "tor_datadir",		V_TEXT(0,24)		},
	{ "tor_iface",			V_LENGTH(0, 50)		},
	{ "tor_users",			V_LENGTH(0, 4096)	},
	{ "tor_ports",			V_LENGTH(0, 50)		},
	{ "tor_ports_custom",		V_LENGTH(0, 4096)	},
	{ "tor_custom",			V_TEXT(0, 2048)		},
#endif

/*
ppp_static			0/1
ppp_static_ip		IP
wl_enable			0/1
wl_wds_timeout
wl_maxassoc			1-256
wl_phytype			a,b,g
wl_net_reauth
wl_preauth
wl_wme_ap_bk
wl_wme_ap_be
wl_wme_ap_vi
wl_wme_ap_vo
wl_wme_sta_bk
wl_wme_sta_be
wl_wme_sta_vi
wl_wme_sta_vo
QoS
port_priority_1		0-2
port_flow_control_1	0,1
port_rate_limit_1	0-8
port_priority_2		0-2
port_flow_control_2	0,1
port_rate_limit_2	0-8
port_priority_3		0-2
port_flow_control_3	0,1
port_rate_limit_3	0-8
port_priority_4		0-2
port_flow_control_4	0,1
port_rate_limit_4	0-8
wl_ap_ip
wl_ap_ssid
*/
#ifdef TCONFIG_PPTPD
	{ "pptp_client_enable",   V_01                  },
	{ "pptp_client_peerdns",  V_RANGE(0,2)          },
	{ "pptp_client_mtuenable",V_01                  },
	{ "pptp_client_mtu",      V_RANGE(576, 1500)	},
	{ "pptp_client_mruenable",V_01                  },
	{ "pptp_client_mru",      V_RANGE(576, 1500)	},
	{ "pptp_client_nat",      V_01                  },
	{ "pptp_client_srvip",    V_NONE                },
	{ "pptp_client_srvsub",   V_IP                  },
	{ "pptp_client_srvsubmsk",V_IP                  },
	{ "pptp_client_username", V_TEXT(0,50)          },
	{ "pptp_client_passwd",   V_TEXT(0,50)          },
	{ "pptp_client_crypt",    V_RANGE(0, 3)         },
	{ "pptp_client_custom",   V_NONE                },
	{ "pptp_client_dfltroute",V_01                  },
	{ "pptp_client_stateless",V_01                  },
#endif

	{ NULL }
};


static int webcgi_nvram_set(const nvset_t *v, const char *name, int write)
{
	char *p, *e;
	int n;
	long l;
	unsigned u[6];
	int ok;
	int dirty;
#ifdef TCONFIG_IPV6
	struct in6_addr addr;
#endif

	if ((p = webcgi_get((char*)name)) == NULL) return 0;

	_dprintf("[%s] %s=%s\n", v->name, (char*)name, p);
	dirty = 0;
	ok = 1;
	switch (v->vtype) {
	case VT_TEXT:
		p = unix_string(p);	// NOTE: p = malloc'd
		// drop
	case VT_LENGTH:
		n = strlen(p);
		if ((n < v->va.i) || (n > v->vb.i)) ok = 0;
		break;
	case VT_RANGE:
		l = strtol(p, &e, 10);
		if ((p == e) || (*e) || (l < v->va.l) || (l > v->vb.l)) ok = 0;
		break;
	case VT_IP:
		if ((sscanf(p, "%3u.%3u.%3u.%3u", &u[0], &u[1], &u[2], &u[3]) != 4) ||
			(u[0] > 255) || (u[1] > 255) || (u[2] > 255) || (u[3] > 255)) ok = 0;
		break;
	case VT_MAC:
		if ((sscanf(p, "%2x:%2x:%2x:%2x:%2x:%2x", &u[0], &u[1], &u[2], &u[3], &u[4], &u[5]) != 6) ||
			(u[0] > 255) || (u[1] > 255) || (u[2] > 255) || (u[3] > 255) || (u[4] > 255) || (u[5] > 255)) ok = 0;
		break;
#ifdef TCONFIG_IPV6
	case VT_IPV6:
		if (strlen(p) > 0 || v->va.i) {
			if (inet_pton(AF_INET6, p, &addr) != 1) ok = 0;
		}
		break;
#endif
	default:
		// shutup gcc
		break;
	}
	if (!ok) {
		if (v->vtype == VT_TEXT) free(p);
		return -1;
	}
	if (write) {
		if (!nvram_match((char *)name, p)) {
			if (v->vtype != VT_TEMP) dirty = 1;
			DEBUG_NVRAMSET(name, p);
			nvram_set(name, p);
		}
	}
	if (v->vtype == VT_TEXT) free(p);

	return dirty;
}

typedef struct {
	const nvset_t *v;
	int write;
	int dirty;
} nv_list_t;

static int nv_wl_find(int idx, int unit, int subunit, void *param)
{
	nv_list_t *p = param;

	int ok = webcgi_nvram_set(p->v, wl_nvname(p->v->name + 3, unit, subunit), p->write);
	if (ok < 0)
		return 1;
	else {
		p->dirty |= ok;
		return 0;
	}
}

#ifdef CONFIG_BCMWL6
static int nv_wl_bwcap_chanspec(int idx, int unit, int subunit, void *param){
	char		chan_spec[32];
	char		*ch,*nbw_cap,*nctrlsb;
	int 		write = *((int *)param);
	ch	= webcgi_get(wl_nvname("channel",unit,0));
	nbw_cap = webcgi_get(wl_nvname("nbw_cap",unit,0));
	nctrlsb = webcgi_get(wl_nvname("nctrlsb",unit,0));
	if(!ch && !nbw_cap && !nctrlsb)
		return 0;
	if(ch == NULL || !*ch)	ch = nvram_get(wl_nvname("channel",unit,0));
	if(nbw_cap == NULL || !*nbw_cap)  nbw_cap = nvram_get(wl_nvname("nbw_cap",unit,0));
	if(nctrlsb == NULL || !*nctrlsb)  nctrlsb = nvram_get(wl_nvname("nctrlsb",unit,0));

	if(!ch || !nbw_cap || !nctrlsb || !*ch || !*nbw_cap || !*nctrlsb)
		return 1;

	memset(chan_spec,0,sizeof(chan_spec));
	strncpy(chan_spec,ch,sizeof(chan_spec));
	switch(atoi(nbw_cap)){
		case 0:
			if(write)
				nvram_set(wl_nvname("bw_cap",unit,0), "1");
			break;
		case 1:
			if(write)
				nvram_set(wl_nvname("bw_cap",unit,0), "3");
			if(*ch != '0')
				*(chan_spec + strlen(chan_spec)) = *nctrlsb;
			break;
		case 3:
			if(write)
				nvram_set(wl_nvname("bw_cap",unit,0), "7");
			if(*ch != '0')
				strcpy(chan_spec + strlen(chan_spec),"/80");
			break;
	}
	if(write)
		nvram_set(wl_nvname("chanspec", unit, 0), chan_spec);
	return 0;
}
#endif

static int save_variables(int write)
{
	const nvset_t *v;
	char *p;
	int n;
	int ok;
	char s[256];
	int dirty;
	static const char *msgf = "The field \"%s\" is invalid. Please report this problem.";
	nv_list_t nv;

	dirty = 0;
	nv.write = write;
	for (v = nvset_list; v->name; ++v) {
		ok = webcgi_nvram_set(v, v->name, write);

		if ((ok >= 0) && (strncmp(v->name, "wl_", 3) == 0)) {
			nv.dirty = dirty;
			nv.v = v;
			if (foreach_wif(1, &nv, nv_wl_find) == 0)
				ok |= nv.dirty;
			else
				ok = -1;
		}

		if (ok < 0) {
			sprintf(s, msgf, v->name);
			resmsg_set(s);
			return 0;
		}
		dirty |= ok;
	}

	// special cases
	#ifdef CONFIG_BCMWL6
	    foreach_wif(0, &write, nv_wl_bwcap_chanspec);
	#endif

	char *p1, *p2;
	if (((p1 = webcgi_get("set_password_1")) != NULL) && (strcmp(p1, "**********") != 0)) {
		if (((p2 = webcgi_get("set_password_2")) != NULL) && (strcmp(p1, p2) == 0)) {
			if ((write) && (!nvram_match("http_passwd", p1))) {
				dirty = 1;
				nvram_set("http_passwd", p1);
			}
  		}
  		else {
			sprintf(s, msgf, "password");
			resmsg_set(s);
			return 0;
  		}
	}

	for (n = 0; n < 50; ++n) {
		sprintf(s, "rrule%d", n);
		if ((p = webcgi_get(s)) != NULL) {
	        	if (strlen(p) > 2048) {
				sprintf(s, msgf, s);
				resmsg_set(s);
				return 0;
	        	}
			if ((write) && (!nvram_match(s, p))) {
				dirty = 1;
				DEBUG_NVRAMSET(s, p);
				nvram_set(s, p);
			}
		}
	}

	return (write) ? dirty : 1;
}

static void wo_tomato(char *url)
{
	char *v;
	int i;
	int ajax;
	int nvset;
	const char *red;
	int commit;
	int force_commit;

//	_dprintf("tomato.cgi\n");

	red = webcgi_safeget("_redirect", "");
	if (!*red) send_header(200, NULL, mime_html, 0);

	commit = atoi(webcgi_safeget("_commit", "1"));
	force_commit = atoi(webcgi_safeget("_force_commit", "0"));
	ajax = atoi(webcgi_safeget("_ajax", "0"));

	nvset = atoi(webcgi_safeget("_nvset", "1"));
	if (nvset) {
		if (!save_variables(0)) {
			if (ajax) {
				web_printf("@msg:%s", resmsg_get());
			}
			else {
				parse_asp("error.asp");
			}
			return;
		}
		commit = save_variables(1) && commit;

		resmsg_set("Settings saved.");
	}

	rboot = atoi(webcgi_safeget("_reboot", "0"));
	if (rboot) {
		parse_asp("reboot.asp");
	}
	else {
		if (ajax) {
			web_printf("@msg:%s", resmsg_get());
		}
		else if (atoi(webcgi_safeget("_moveip", "0"))) {
			parse_asp("saved-moved.asp");
		}
		else if (!*red) {
			parse_asp("saved.asp");
		}
	}

	if (commit || force_commit) {
		_dprintf("commit from tomato.cgi\n");
		nvram_commit_x();
	}

	if ((v = webcgi_get("_service")) != NULL && *v != 0) {
		if (!*red) {
			if (ajax) web_printf(" Some services are being restarted...");
			web_close();
		}
		sleep(1);

		if (*v == '*') {
			kill(1, SIGHUP);
		}
		else {
			exec_service(v);
		}
	}

	for (i = atoi(webcgi_safeget("_sleep", "0")); i > 0; --i) sleep(1);

	if (*red) redirect(red);

	if (rboot) {
		web_close();
		sleep(1);
		kill(1, SIGTERM);
	}
}


// ----------------------------------------------------------------------------


static void wo_update(char *url)
{
	const aspapi_t *api;
	const char *name;
	int argc;
	char *argv[16];
	char s[32];

	if ((name = webcgi_get("exec")) != NULL) {
		for (api = aspapi; api->name; ++api) {
			if (strcmp(api->name, name) == 0) {
				for (argc = 0; argc < 16; ++argc) {
					sprintf(s, "arg%d", argc);
					if ((argv[argc] = (char *)webcgi_get(s)) == NULL) break;
				}
				api->exec(argc, argv);
				break;
			}
		}
	}
}

static void wo_service(char *url)
{
	int n;

	exec_service(webcgi_safeget("_service", ""));

	if ((n = atoi(webcgi_safeget("_sleep", "2"))) <= 0) n = 2;
	sleep(n);

	common_redirect();
}

static void wo_shutdown(char *url)
{
	parse_asp("shutdown.asp");
	web_close();
	sleep(1);

	kill(1, SIGQUIT);
}

static void wo_nvcommit(char *url)
{
	parse_asp("saved.asp");
	web_close();
	nvram_commit();
}
