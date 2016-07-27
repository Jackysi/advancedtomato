/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "rc.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <arpa/inet.h>


//	#define DLOG(args...) syslog(LOG_DEBUG, args)
#define DLOG(fmt, args...) _dprintf(fmt"\n", args)

static void update(int num, int *dirty, int force)
{
	char config[2048];
	char *p;
	char *serv, *user, *pass, *host, *wild, *mx, *bmx, *cust;
	time_t t;
	struct tm *tm;
	int n;
	char ddnsx[16];
	char ddnsx_path[32];
	char s[128];
	char v[128];
	char cache_fn[32];
	char conf_fn[32];
	char cache_nv[32];
	char msg_fn[32];
	char ip[32];
	int exitcode;
	int errors;
	FILE *f;

	char prefix[] = "wanXX";

	if (nvram_match("ddnsx_ip", "wan") || nvram_match("ddnsx_ip", "wan2")
#ifdef TCONFIG_MULTIWAN
		 || nvram_match("ddnsx_ip", "wan3") || nvram_match("ddnsx_ip", "wan4")
#endif
	) {
		strcpy(prefix, nvram_safe_get("ddnsx_ip"));
	} else {
		strcpy(prefix, "wan");
	}

	DLOG("%s", __FUNCTION__);

	sprintf(s, "ddns%d", num);
	eval("cru", "d", s);
	DLOG("%s: cru d %s", __FUNCTION__, s);

	sprintf(s, "ddnsf%d", num);
	eval("cru", "d", s);
	DLOG("%s: cru d %s", __FUNCTION__, s);

	sprintf(ddnsx, "ddnsx%d", num);
	sprintf(ddnsx_path, "/var/lib/mdu/%s", ddnsx);
	strlcpy(config, nvram_safe_get(ddnsx), sizeof(config));

	mkdir("/var/lib/mdu", 0700);
	sprintf(msg_fn, "%s.msg", ddnsx_path);

	if ((vstrsep(config, "<", &serv, &user, &host, &wild, &mx, &bmx, &cust) != 7) || (*serv == 0)) {
		DLOG("%s: msg=''\n", __FUNCTION__);
		f_write(msg_fn, NULL, 0, 0, 0);
		return;
	}

	if ((pass = strchr(user, ':')) != NULL) *pass++ = 0;
		else pass = "";

	for (n = 120; (n > 0) && (time(0) < Y2K); --n) {
		sleep(1);
	}
	if (n <= 0) {
		syslog(LOG_INFO, "Time not yet set.");
	}

	if (!wait_action_idle(10)) {
		DLOG("%s: !wait_action_idle", __FUNCTION__);
		return;
	}

	sprintf(cache_nv, "%s_cache", ddnsx);
	if (force) {
		DLOG("%s: force=1", __FUNCTION__);
		nvram_set(cache_nv, "");
	}

	simple_lock("ddns");

	strlcpy(ip, nvram_safe_get("ddnsx_ip"), sizeof(ip));

	if (!check_wanup(prefix)) {
		if ((get_wan_proto() != WP_DISABLED) || (ip[0] == 0)) {
			DLOG("%s: !check_wanup", __FUNCTION__);
			goto CLEANUP;
		}
	}

	if (ip[0] == '@') {
		if ((strcmp(serv, "zoneedit") == 0) || (strcmp(serv, "tzo") == 0) || (strcmp(serv, "noip") == 0) || (strcmp(serv, "dnsomatic") == 0)) {
			strcpy(ip + 1, serv);
		}
		else {
			strcpy(ip + 1, "dyndns");
		}
	}
	else if (inet_addr(ip) == -1) {
		strcpy(ip, get_wanip(prefix));
	}

	sprintf(cache_fn, "%s.cache", ddnsx_path);
	f_write_string(cache_fn, nvram_safe_get(cache_nv), 0, 0);

	if (!f_exists(msg_fn)) {
		DLOG("%s: !f_exist(%s)", __FUNCTION__, msg_fn);
		f_write(msg_fn, NULL, 0, 0, 0);
	}


	sprintf(conf_fn, "%s.conf", ddnsx_path);
	if ((f = fopen(conf_fn, "w")) == NULL) goto CLEANUP;
	// note: options not needed for the service are ignored by mdu
	fprintf(f,
		"user %s\n"
		"pass %s\n"
		"host %s\n"
		"addr %s\n"
		"mx %s\n"
		"backmx %s\n"
		"wildcard %s\n"
		"url %s\n"
		"ahash %s\n"
		"msg %s\n"
		"cookie %s\n"
		"addrcache extip\n"
		"",
		user,
		pass,
		host,
		ip,
		mx,
		bmx,
		wild,
		cust,
		cust,
		msg_fn,
		cache_fn);

	if (nvram_get_int("debug_ddns")) {
		fprintf(f, "dump /tmp/mdu-%s.txt\n", serv);
	}

	fclose(f);

	exitcode = eval("mdu", "--service", serv, "--conf", conf_fn);
	DLOG("%s: mdu exitcode=%d", __FUNCTION__, exitcode);

	sprintf(s, "%s_errors", ddnsx);
	if ((exitcode == 1) || (exitcode == 2)) {
		if (nvram_match("ddnsx_retry", "0")) goto CLEANUP;

		if (force) {
			errors = 0;
		}
		else {
			errors = nvram_get_int(s) + 1;
			if (errors < 1) errors = 1;
			if (errors >= 3) {
				nvram_unset(s);
				goto CLEANUP;
			}
		}
		sprintf(v, "%d", errors);
		nvram_set(s, v);
		goto SCHED;
	}
	else {
		nvram_unset(s);
		errors = 0;
	}

	f_read_string(cache_fn, s, sizeof(s));
	if ((p = strchr(s, '\n')) != NULL) *p = 0;
	t = strtoul(s, &p, 10);
	if (*p != ',') goto CLEANUP;

	if (!nvram_match(cache_nv, s)) {
		nvram_set(cache_nv, s);
		if (nvram_get_int("ddnsx_save")) {
			if (strstr(serv, "dyndns") == 0) *dirty = 1;
		}
	}

	n = 28;
	if (((p = nvram_get("ddnsx_refresh")) != NULL) && (*p != 0)) {
		n = atoi(p);
	}
	if (n) {
		if ((n < 0) || (n > 90)) n = 28;
		t += (n * 86400);	// refresh every n days
		
		//!!TB - fix: if time is in the past, make it current
		time_t now = time(0) + (60 * 5);
		if (t < now) t = now;

		tm = localtime(&t);
		sprintf(s, "ddnsf%d", num);
		sprintf(v, "%d %d %d %d * ddns-update %d force",
			tm->tm_min, tm->tm_hour, tm->tm_mday, tm->tm_mon + 1, num);
		DLOG("%s: cru a %s %s", __FUNCTION__, s, v);
		eval("cru", "a", s, v);
	}

	if (ip[0] == '@') {
SCHED:
		DLOG("%s: SCHED", __FUNCTION__);
#if 0
		t = time(0);
		tm = localtime(&t);
		DLOG("%s: now: %d:%d errors=%d", __FUNCTION__, tm->tm_hour, tm->tm_min, errors);
#endif

		// need at least 10m spacing for checkip
		// +1m to not trip over mdu's ip caching
		// +5m for every error
		n = (11 + (errors * 5));
		if ((exitcode == 1) || (exitcode == 2)) {
			if (exitcode == 2) n = 30;
			sprintf(s, "\n#RETRY %d %d\n", n, errors);	// should be localized in basic-ddns.asp
			f_write_string(msg_fn, s, FW_APPEND, 0);
			DLOG("%s: msg='retry n=%d errors=%d'", __FUNCTION__, n, errors);
		}

		t = time(0) + (n * 60);
		tm = localtime(&t);
		DLOG("%s: sch: %d:%d\n", __FUNCTION__, tm->tm_hour, tm->tm_min);

		sprintf(s, "ddns%d", num);
		sprintf(v, "%d * * * * ddns-update %d", tm->tm_min, num);
		DLOG("%s: cru a %s %s", __FUNCTION__, s, v);
		eval("cru", "a", s, v);

		//	sprintf(s, "cru a ddns%d \"*/10 * * * * ddns-update %d\"", num);
		//	system(s);
	}

CLEANUP:
	DLOG("%s: CLEANUP", __FUNCTION__);
	simple_unlock("ddns");
}

int ddns_update_main(int argc, char **argv)
{
	int num;
	int dirty;

	DLOG("%s: %s %s", __FUNCTION__, (argc >= 2) ? argv[1] : "", (argc >= 3) ? argv[2] : "");

	dirty = 0;
	umask(077);

	if (argc == 1) {
		update(0, &dirty, 0);
		update(1, &dirty, 0);
	}
	else if ((argc == 2) || (argc == 3)) {
		num = atoi(argv[1]);
		if ((num == 0) || (num == 1)) {
			update(num, &dirty, (argc == 3) && (strcmp(argv[2], "force") == 0));
		}
	}
	if (dirty) nvram_commit_x();
	return 0;
}

void start_ddns(void)
{
	DLOG("%s", __FUNCTION__);

	stop_ddns();

	// cleanup
	simple_unlock("ddns");
	nvram_unset("ddnsx0_errors");
	nvram_unset("ddnsx1_errors");

	xstart("ddns-update");
}

void stop_ddns(void)
{
	DLOG("%s", __FUNCTION__);

	eval("cru", "d", "ddns0");
	eval("cru", "d", "ddns1");
	eval("cru", "d", "ddnsf0");
	eval("cru", "d", "ddnsf1");
	killall("ddns-update", SIGKILL);
	killall("mdu", SIGKILL);
}
