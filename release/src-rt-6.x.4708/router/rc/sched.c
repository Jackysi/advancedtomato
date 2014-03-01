/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include <time.h>
#include "rc.h"


//	#define DLOG(args...) syslog(LOG_DEBUG, args)
#define DLOG(fmt, args...) _dprintf(fmt"\n", args)

static inline void unsched(const char *key)
{
	DLOG("%s: %s", __FUNCTION__, key);

	eval("cru", "d", (char *) key);
}

static void sched(const char *key, int resched)
{
	int en;
	int t;
	int dow;
	char s[64];
	char w[32];
	int i;
	struct tm tm;
	long tt, qq;

	// en,time,days
	if ((sscanf(nvram_safe_get(key), "%d,%d,%d", &en, &t, &dow) != 3) || (!en)) {
		unsched(key);
		return;
	}

	DLOG("%s: %s", __FUNCTION__, key);

	if (resched) {
		sprintf(s, "%s_last", key);
		sprintf(w, "%ld", time(0));
		nvram_set(s, w);

		DLOG("%s: set %s=%s", __FUNCTION__, s, w);

		if (t >= -5) return;
	}

	if ((dow & 0x7F) == 0) dow = 0x7F;
	w[0] = 0;
	w[1] = 0;
	for (i = 0; i < 7; ++i) {
		if (dow & (1 << i)) {
			sprintf(w + strlen(w), ",%d", i);
		}
	}

	if (t >= 0) {	// specific time
		sprintf(s, "%d %d * * %s sched %s", t % 60, t / 60, w + 1, key);
	}
	else {	// every ...
		t = -t;
		if (t <= 5) {	// 1 to 5m = a simple cron job
			sprintf(s, "*/%d * * * %s sched %s", t, w + 1, key);
		}
		else {
			t *= 60;
			DLOG("%s: t=%d", __FUNCTION__, t);

			tt = time(0) + 59;
			tm = *localtime(&tt);
			DLOG("%s: now=%d:%02d %02d/%02d", __FUNCTION__, tm.tm_hour, tm.tm_min, tm.tm_mon + 1, tm.tm_mday);

			sprintf(s, "%s_last", key);
			DLOG("%s: %s_last=%s, tt=%ld", __FUNCTION__, key, nvram_safe_get(s), tt);
			qq = strtoul(nvram_safe_get(s), NULL, 10);
			if ((qq + t) > tt) tt = qq;

			tt += t;
			while (1) {
				tm = *localtime(&tt);	// copy struct, otherwise we get weird stuff (!?)
				DLOG("%s: %d:%02d %02d/%02d, tt=%ld", __FUNCTION__, tm.tm_hour, tm.tm_min, tm.tm_mon + 1, tm.tm_mday, tt);
//				printf("%d:%02d %02d/%02d, w=%d, dow=%d, x=%d, tt=%ld\n", tm.tm_hour, tm.tm_min, tm.tm_mon + 1, tm.tm_mday, tm.tm_wday,
//					dow, (dow & (1 << tm.tm_wday)), tt);
				if (dow & (1 << tm.tm_wday)) break;
				tt += 60;
			}

			sprintf(s, "%d %d %d %d * sched %s", tm.tm_min, tm.tm_hour, tm.tm_mday, tm.tm_mon + 1, key);
		}
	}

	DLOG("%s: cru a %s %s", __FUNCTION__, key, s);
	eval("cru", "a", (char *) key, s);
}

static inline int is_sched(const char *key)
{
	return *nvram_safe_get(key) == '1';
}

int sched_main(int argc, char *argv[])
{
	int n;
	char s[64];
	int log;

	if (argc == 2) {
		DLOG("%s: %s", __FUNCTION__, argv[1]);

		log = nvram_contains_word("log_events", "sched");

		if (strncmp(argv[1], "sch_", 4) == 0) {
			wait_action_idle(5 * 60);

			if (is_sched(argv[1])) {
				if (strcmp(argv[1], "sch_rboot") == 0) {
					if (log) syslog(LOG_INFO, "Performing scheduled %s...", "reboot");
					eval("reboot");
					return 0;
				}
				else if (strcmp(argv[1], "sch_rcon") == 0) {
					sched(argv[1], 1);
					if (log) syslog(LOG_INFO, "Performing scheduled %s...", "reconnect");
					eval("service", "wan", "restart");
				}
				else if (strncmp(argv[1], "sch_c", 5) == 0) {
					n = atoi(argv[1] + 5);
					if ((n >= 1) && (n <= 5)) {
						sched(argv[1], 1);

						if (log) {
							sprintf(s, "custom #%d", n);
							syslog(LOG_INFO, "Performing scheduled %s...", s);
						}

						signal(SIGCHLD, chld_reap);

						sprintf(s, "%s_cmd", argv[1]);
						DLOG("%s: run=%s", __FUNCTION__, nvram_safe_get(s));
						run_nvscript(s, "", 60);
					}
				}
			}
			else {
				unsched(argv[1]);
			}
		}
		else if (strcmp(argv[1], "start") == 0) {
			while (time(0) < Y2K) {
				sleep(1);
			}

			sched("sch_rboot", 0);
			sched("sch_rcon", 0);
			sched("sch_c1", 0);
			sched("sch_c2", 0);
			sched("sch_c3", 0);
			sched("sch_c4", 0);
			sched("sch_c5", 0);
		}
	}

	return 0;
}

void start_sched(void)
{
	DLOG("%s", __FUNCTION__);

	killall("sched", SIGTERM);
	xstart("sched", "start");
}

void stop_sched(void)
{
	DLOG("%s", __FUNCTION__);

	killall("sched", SIGTERM);
	unsched("sch_rboot");
	unsched("sch_rcon");
	unsched("sch_sc1");
	unsched("sch_sc2");
	unsched("sch_sc3");
	unsched("sch_sc4");
	unsched("sch_sc5");
}
