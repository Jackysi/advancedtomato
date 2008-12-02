#if 0	// hbobs
/*

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2007 Jonathan Zarate

*/

// checkme: can remove bpalogin soon?	-- zzz

#include "rc.h"

#include <netdb.h>
#include <errno.h>
#include <sys/sysinfo.h>

#undef _dprintf
#define _dprintf(args...)	cprintf(args)
//	#define _dprintf(args...)	do { } while(0)


int start_heartbeat(int mode)
{
#ifdef TCONFIG_HEARTBEAT
	FILE *fp;
	int ret;
	char authserver[80];
	char authdomain[80];
	int n;

	if (nvram_invmatch("wan_proto", "heartbeat")) return 0;

	_dprintf("%s: hb_server_ip=%s wan_get_domain=%s\n", __FUNCTION__,
		nvram_safe_get("hb_server_ip"), nvram_safe_get("wan_get_domain"));

	strlcpy(authdomain, nvram_safe_get("wan_get_domain"), sizeof(authdomain));

	if ((nvram_invmatch("hb_server_ip", "")) && (nvram_invmatch("hb_server_ip", "0.0.0.0"))) {
		strlcpy(authserver, nvram_safe_get("hb_server_ip"), sizeof(authserver));
		_dprintf("trying %s\n", authserver);
		if (gethostbyname(authserver) == NULL) {
			n = strlen(authserver);
			snprintf(authserver + n, sizeof(authserver) - n, ".%s", authdomain);
			_dprintf("trying %s\n", authserver);
			if (gethostbyname(authserver) == NULL) {
				authserver[n] = 0;
				_dprintf("reverting to %s\n", authserver);
			}
		}
	}
	else {
		/* We must find out HB auth server from domain that get by dhcp if user don't input HB sever. */
		snprintf(authserver, sizeof(authserver), "sm-server.%s", nvram_safe_get("wan_get_domain"));
	}


	_dprintf("%s: authserver=%s authdomain=%s\n", __FUNCTION__, authserver, authdomain);

//	snprintf(buf, sizeof(buf), "%s%c%s", authserver, !strcmp(authdomain, "") ? '\0' : '.', authdomain);
//	nvram_set("hb_server_name", buf);
//	_dprintf("heartbeat: Connect to server %s\n", buf);

	if ((fp = fopen("/tmp/bpalogin.conf", "w")) == NULL) {
		perror("/tmp/bpalogin.conf");
		return errno;
	}
	fprintf(fp, "username %s\n", nvram_safe_get("ppp_username"));
	fprintf(fp, "password %s\n", nvram_safe_get("ppp_passwd"));
	fprintf(fp, "authserver %s\n", authserver);
	fprintf(fp, "authdomain %s\n", authdomain);
	fprintf(fp, "localport 5050\n");
	fprintf(fp, "logging stdout\n");
	fprintf(fp, "debuglevel 4\n");
	fprintf(fp, "minheartbeatinterval 60\n");
	fprintf(fp, "maxheartbeatinterval 840\n");
	fprintf(fp, "connectedprog hb_connect\n");
	fprintf(fp, "disconnectedprog hb_disconnect\n");
	fclose(fp);

	mkdir("/tmp/ppp", 0777);
	if ((fp = fopen("/tmp/hb_connect_success", "r"))) {				// ???
		ret = eval("bpalogin", "-c", "/tmp/bpalogin.conf", "-t");
		fclose(fp);
	}
	else ret = eval("bpalogin", "-c", "/tmp/bpalogin.conf");

	if (nvram_invmatch("ppp_demand", "1")) {
		if (mode != REDIAL) start_redial();
	}

	return ret;
#else
	return 0;
#endif
}

int stop_heartbeat(void)
{
#ifdef TCONFIG_HEARTBEAT
	unlink("/tmp/ppp/link");
	killall("bpalogin", SIGTERM);
	killall("bpalogin", SIGKILL);
#endif
	return 0;
}

// <listenport> <pid>
int hb_connect_main(int argc, char **argv)
{
#ifdef TCONFIG_HEARTBEAT
	FILE *fp;
	char buf[254];

	_dprintf("hb_connect_main: init\n");

	mkdir("/tmp/ppp", 0777);

	if ((fp = fopen("/tmp/ppp/link", "a")) == NULL) {
		perror("/tmp/ppp/link");
		return errno;
	}
	fprintf(fp, "%s", argv[2]);
	fclose(fp);

	start_wan_done(nvram_safe_get("wan_ifname"));

	snprintf(buf, sizeof(buf), "iptables -I INPUT -d %s -i %s -p udp --dport %d -j %s",
		nvram_safe_get("wan_ipaddr"), nvram_safe_get("wan_ifname"), 5050,  "ACCEPT");
	system(buf);
#endif
	return 0;
}

int hb_disconnect_main(int argc, char **argv)
{
#ifdef TCONFIG_HEARTBEAT
	_dprintf("hb_disconnect_main\n");

	if (check_wanup()) {
		stop_heartbeat();
	}
#endif
	return 0;
}

int hb_idle_main(int argc, char **argv)
{
#ifdef TCONFIG_HEARTBEAT
	struct sysinfo si;
	long target;
	FILE *f;
	char s[64];
	unsigned long long now;
	unsigned long long last;
	long alive;
	long maxidle;

	if (fork() != 0) return 0;

	maxidle = atoi(nvram_safe_get("ppp_idletime")) * 60;
	if (maxidle < 60) maxidle = 60;

	last = 0;

	sysinfo(&si);
	alive = si.uptime;

	while (1) {
		target = si.uptime + 60;
		do {
//			_dprintf("uptime = %ld, target = %ld\n", si.uptime, target);
			sleep(target - si.uptime);
			sysinfo(&si);
		} while (si.uptime < target);

		if (check_action() != ACT_IDLE) continue;


		// this sucks...	-- zzz

		if ((f = popen("iptables -xnvL FORWARD | grep wanout", "r")) == NULL) {
			_dprintf("hbidle: error obtaining data\n");
			continue;
		}
		fgets(s, sizeof(s), f);
		pclose(f);

		if ((now = strtoull(s, NULL, 10)) == last) {
			if ((si.uptime - alive) > maxidle) {
				_dprintf("hbidle: idled for %d, stopping.\n", si.uptime - alive);
				stop_heartbeat();
				stop_ntpc();
				xstart("listen", nvram_safe_get("lan_ifname"));
				return 0;
			}
		}
		else {
			_dprintf("hbidle: now = %llu, last = %llu\n", now, last);

			last = now;
			alive = si.uptime;
		}

		_dprintf("hbidle: time = %ld\n", (si.uptime - alive) / 60);
	}
#else
	return 0;
#endif
}

void start_hbidle(void)
{
#ifdef TCONFIG_HEARTBEAT
	if ((nvram_match("wan_proto", "heartbeat")) && (nvram_match("ppp_demand", "1")) && (check_wanup())) {
		xstart("hb_idle");
	}
#endif
}

void stop_hbidle(void)
{
#ifdef TCONFIG_HEARTBEAT
	killall("hb_idle", SIGTERM);
#endif
}

#endif	// hbobs

