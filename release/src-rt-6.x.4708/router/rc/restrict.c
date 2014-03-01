/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate
	Portions rebuild by victek@tomato.raf, 2013. String chain restriction

*/

#include "rc.h"
#include <time.h>
#include <string.h> 

#define MAX_NRULES	50

static inline void unsched_restrictions(void)
{
	system("cru d rcheck");
}

void sched_restrictions(void)
{
	system("rcheck");
}

static int in_sched(int now_mins, int now_dow, int sched_begin, int sched_end, int sched_dow)
{
	// all day
	if ((sched_begin < 0) || (sched_end < 0)) {
		return (sched_dow & now_dow) != 0;
	}

	// simple min to max
	if (sched_begin < sched_end) {
		return (((sched_dow & now_dow) != 0) && (now_mins >= sched_begin) && (now_mins < sched_end));
	}

	// 15:00 - 01:00 = 15:00 Sun to 01:00 Mon
	// 12:00 - 12:00 = 12:00 Sun to 12:00 Mon

	if ((now_dow & sched_dow) != 0) {
		if (now_mins >= sched_begin) return 1;
	}

	// ror now_dow, 1
	if (now_dow == 1) now_dow = (1 << 6);
		else now_dow >>= 1;

	if ((now_dow & sched_dow) != 0) {
		if (now_mins < sched_end) return 1;
	}

//	printf("now_mins=%d	sched_end=%d now_dow=%d sched_dow=%d\n", now_mins, sched_end, now_dow, sched_dow);
	return 0;
}

static int radio_on(int idx, int unit, int subunit, void *param)
{
	return nvram_match(wl_nvname("radio", unit, 0), "1");
}

int rcheck_main(int argc, char *argv[])
{
	char buf[256];
	char *p;
	int sched_begin;
	int sched_end;
	int sched_dow;
	time_t now;
	struct tm *tms;
	int now_dow;
	int now_mins;
	int n;
	int nrule;
	char comp;
	int insch;
	unsigned long long activated;
	int count;
	int radio;
	int r;
#ifdef TCONFIG_IPV6
	int r6;
#endif
#ifdef LINUX26
	int ipt_active;
#endif

	if (!nvram_contains_word("log_events", "acre")) {
		setlogmask(LOG_MASK(LOG_EMERG));	// can't set to 0
	}

	simple_lock("restrictions");

	now = time(0);
	if (now < Y2K) {
		if (!nvram_match("rrules_timewarn", "1")) {
			nvram_set("rrules_timewarn", "1");
			syslog(LOG_INFO, "Time not yet set. Only \"all day, everyday\" restrictions will be activated.");
		}
		now_mins = now_dow = 0;
	}
	else {
		tms = localtime(&now);
		now_dow = 1 << tms->tm_wday;
		now_mins = (tms->tm_hour * 60) + tms->tm_min;
	}

#ifdef LINUX26
	ipt_active = 0;
#endif

	activated = strtoull(nvram_safe_get("rrules_activated"), NULL, 16);
	count = 0;
	radio = foreach_wif(0, NULL, radio_on) ? -1 : -2;
	for (nrule = 0; nrule < MAX_NRULES; ++nrule) {
		sprintf(buf, "rrule%d", nrule);
		if ((p = nvram_get(buf)) == NULL) continue;
		if (sscanf(p, "%d|%d|%d|%d|%c", &n, &sched_begin, &sched_end, &sched_dow, &comp) != 5) continue;
		if (n == 0) continue;

		++count;

		if (now < Y2K) {
			if ((sched_begin >= 0) || (sched_end >= 0) || (sched_dow != 0x7F)) continue;
			insch = 1;
		}
		else {
			insch = in_sched(now_mins, now_dow, sched_begin, sched_end, sched_dow);
		}

#ifdef LINUX26
		if ((insch) && (comp != '~'))
			++ipt_active;
#endif

		n = 1 << nrule;
		if ((insch) == ((activated & n) != 0)) {
			continue;
		}

		syslog(LOG_INFO, "%sctivating rule %d", insch ? "A" : "Dea", nrule);

		if (comp == '~') {
			if ((radio != 0) && (radio != -2)) radio = !insch;
		}
		else {
			sprintf(buf, "r%s%02d", (comp != '|') ? "dev" : "res", nrule);

			r = eval("iptables", "-D", "restrict", "-j", buf);
			if (insch) {
				// ignore error above (if any)

				r = eval("iptables", "-A", "restrict", "-j", buf);
			}

#ifdef TCONFIG_IPV6
			r6 = eval("ip6tables", "-D", "restrict", "-j", buf);
			if (ipv6_enabled()) {
				if (insch) {
					// ignore error above (if any)

					r6 = eval("ip6tables", "-A", "restrict", "-j", buf);
				}
				r |= r6;
			}
#endif

			if (r != 0) {
				syslog(LOG_ERR, "Iptables: %sactivating chain \"%s\" failed. Retrying in 15 minutes.",
					insch ? "" : "de", buf);
				continue;
			}
		}

		if (insch) activated |= n;
			else activated &= ~n;
	}

	sprintf(buf, "%llx", activated);
	nvram_set("rrules_activated", buf);

	if (count > 0) {
		if ((argc != 2) || (strcmp(argv[1], "--cron") != 0)) {
			system("cru a rcheck '*/15 * * * * rcheck --cron'");
		}
	}
	else {
		unsched_restrictions();
	}

	if (radio >= 0) {
		nvram_set("rrules_radio", radio ? "0" : "1");
#if 1
		// changed for dual radio support
		_dprintf("%s: radio = %d\n", __FUNCTION__, radio);
		eval("radio", radio ? "on" : "off");
#else
		if (get_radio() != radio) {
			_dprintf("%s: radio = %d\n", __FUNCTION__, radio);
			eval("radio", radio ? "on" : "off");
		}
		else {
			_dprintf("%s: no radio change = %d\n", __FUNCTION__, radio);
		}
#endif
	}

#ifdef LINUX26
	allow_fastnat("restrictions", (ipt_active == 0));
	try_enabling_fastnat();
#endif
	simple_unlock("restrictions");
	return 0;
}

void ipt_restrictions(void)
{
	char buf[8192];
	char *p, *q;
	int n;
	char *comps, *matches, *http;
	int nrule;
	int blockall;
	char reschain[32];
	char devchain[32];
	char strchain[32];
	char nextchain[32];
	int need_string;
	char *pproto;
	char *dir;
	char *pport;
	int proto;
	char *ipp2p;
	char *layer7;
	char *addr_type, *addr;
	char app[256];
	char ports[256];
	char iptaddr[192];
	int http_file;
	int ex;
	int first;
	int v4v6_ok;

	need_string = 0;
	first = 1;
	nvram_unset("rrules_timewarn");
	nvram_set("rrules_radio", "-1");
	unsched_restrictions();

	for (nrule = 0; nrule < MAX_NRULES; ++nrule) {
		sprintf(buf, "rrule%d", nrule);
		if ((p = nvram_get(buf)) == NULL) continue;
		if (strlen(p) >= sizeof(buf)) continue;
		strcpy(buf, p);

		if ((vstrsep(buf, "|",
			&q,		// 0/1
			&p, &p, &p,	// time (ignored)
			&comps,		//
			&matches,	//
			&http,		//
			&p		// http file match
			) != 8) || (*q != '1')) continue;
		http_file = atoi(p);

		if (comps[0] == '~') {
			// a wireless disable rule, skip
			continue;
		}

		if (first) {
			first = 0;

			ip46t_write(":restrict - [0:0]\n");
#ifdef TCONFIG_IPV6
			if (*wan6face)
				ip6t_write("-A FORWARD -o %s -j restrict\n",
					  wan6face);
#endif
			for (n = 0; n < wanfaces.count; ++n) {
				if (*(wanfaces.iface[n].name)) {
					ipt_write("-A FORWARD -o %s -j restrict\n",
						  wanfaces.iface[n].name);
				}
			}
		// Only mess with DNS requests that are coming in on INPUT
		ip46t_write("-I INPUT 1 -i ! lo -p udp --dport 53 -j restrict\n");
	}

		sprintf(reschain, "rres%02d", nrule);
		ip46t_write(":%s - [0:0]\n", reschain);

		blockall = 1;

		while ((q = strsep(&matches, ">")) != NULL) {
			n = vstrsep(q, "<", &pproto, &dir, &pport, &ipp2p, &layer7, &addr_type, &addr);
			if (n == 5) {
				// fixup for backward compatibility
				addr_type = "0";
			}
			else if (n != 7) continue;

			if ((*dir != 'a') && (*dir != 's') && (*dir != 'd') && (*dir != 'x')) continue;

			// p2p, layer7
			if (!ipt_ipp2p(ipp2p, app)) {
				if (ipt_layer7(layer7, app) == -1) continue;
			}
#ifdef TCONFIG_IPV6
			v4v6_ok = ((*app) ? 0 : IPT_V6) | IPT_V4;
#else
			v4v6_ok = IPT_V4;
#endif

			// dest ip/domain address
			if ((*addr_type == '1') || (*addr_type == '2')) {
				v4v6_ok = ipt_addr(iptaddr, sizeof(iptaddr), addr, (*addr_type == '1') ? "dst" : "src", v4v6_ok, (v4v6_ok == IPT_V4), "restrictions", NULL);
				if (!v4v6_ok)
					continue;
			}
			else {
				iptaddr[0] = 0;
			}

			blockall = 0;

			// proto & ports
			proto = atoi(pproto);
			if (proto <= -2) {
				// shortcut if any proto+any port
				ip46t_flagged_write(v4v6_ok, "-A %s %s %s -j %s\n", reschain, iptaddr, app, chain_out_drop);
				continue;
			}
			else if ((proto == 6) || (proto == 17) || (proto == -1)) {
				if ((*dir != 'a') && (*pport)) {
					if ((*dir == 'x') || (strchr(pport, ','))) {
						// use multiport for multiple ports or src-or-dst type matches
						snprintf(ports, sizeof(ports), "-m multiport --%sports %s", (*dir == 'x') ? "" : dir, pport);
					}
					else {
						// else, use built-in
						snprintf(ports, sizeof(ports), "--%sport %s", dir, pport);
					}
				}
				else {
					ports[0] = 0;
				}
				if (proto != 17)
					ip46t_flagged_write(v4v6_ok, "-A %s -p tcp %s %s %s -j %s\n", reschain, ports, iptaddr, app, chain_out_drop);
				if (proto != 6)
					ip46t_flagged_write(v4v6_ok, "-A %s -p udp %s %s %s -j %s\n", reschain, ports, iptaddr, app, chain_out_drop);
			}
			else {
				ip46t_flagged_write(v4v6_ok, "-A %s -p %d %s %s -j %s\n", reschain, proto, iptaddr, app, chain_out_drop);
			}
		}

// Build chain to perform string matching 
		sprintf(strchain, "rstr%02d", nrule);
		ip46t_write(":%s - [0:0]\n", strchain);
		
// Multiport match for ports 53,80,443 goto strchain 
		ip46t_write("-A %s -p tcp -m multiport --dports 53,80,443 -j %s\n", reschain, strchain);
		ip46t_write("-A %s -p udp --dport 53 -j %s\n", reschain, strchain);

		p = http;
		while (*p) {
			if ((*p == '\t') || (*p == '\r') || (*p == '\n') || (*p == '"')) *p = ' ';
			++p;
		}
		while ((n = strlen(http)) > 0) {
			if (n >= 511) {
				p = http + 510;
				while ((p > http) && (*p != ' ')) --p;
				if (p <= http) {
					// too long
					break;
				}
				*p = 0;
			}
			else p = NULL;

// Trim trailing whitespace from http
			char *p2 = http + strlen(http) - 1;
			while (*p2 == ' ')
			{
			     *p2-- = '\0';
			}

// Split the string delimited by whitespace.  Each substring should be a new iptables entry.
			char delim[] = " ";
			p2 = strtok(http, delim);
			while (p2 != NULL)
			{
			  ip46t_write("-I %s 1 -p tcp -m string --string \"%s\" --algo bm  --from 1 --to 600 -j %s\n", strchain, p2, chain_out_reject);
			  ip46t_write("-I %s 1 -p udp -m string --string \"%s\" --algo bm  --from 1 --to 600 -j REJECT\n", strchain, p2);
			     p2 = strtok(NULL, delim);
			}
			
			need_string = 1;
			blockall = 0;
			if (p == NULL) break;
			http = p + 1;
		}


		//
		app[0] = 0;
		if (http_file & 1) strcat(app, ".ocx$ .cab$ ");
		if (http_file & 2) strcpy(app, ".swf$ ");
		if (http_file & 4) strcat(app, ".class$ .jar$");
		if (app[0]) {
			ip46t_write("-A %s -p tcp -m multiport --dports %s -m web --path \"%s\" -j %s\n",
				reschain, nvram_safe_get("rrulewp"), app, chain_out_reject);
			need_string = 1;
			blockall = 0;
		}

		if (*comps) {
			if (blockall) {
				ip46t_write("-F %s\n", reschain); // https://github.com/ReliefLabs/EasyTomato/
				ip46t_write("-X %s\n", reschain); // chain not needed
				sprintf(nextchain, "-j %s", chain_out_drop);
			}
			else {
				sprintf(nextchain, "-g %s", reschain);
			}

			ex = 0;
			sprintf(devchain, "rdev%02d", nrule);
			ip46t_write(":%s - [0:0]\n", devchain);
			while ((q = strsep(&comps, ">")) != NULL) {
				if (*q == 0) continue;
				if (*q == '!') {
					ex = 1;
					continue;
				}
#ifdef TCONFIG_IPV6
				v4v6_ok = IPT_V6 | IPT_V4;
#else
				v4v6_ok = IPT_V4;
#endif
				if (sscanf(q, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
					   iptaddr, iptaddr, iptaddr, iptaddr, iptaddr, iptaddr) == 6) {
					snprintf(iptaddr, sizeof(iptaddr), "-m mac --mac-source %s", q);
				}
				else {
					v4v6_ok = ipt_addr(iptaddr, sizeof(iptaddr), q, "src", v4v6_ok, (v4v6_ok == IPT_V4), "restrictions", "filtering");
					if (!v4v6_ok)
						continue;
				}
				ip46t_flagged_write(v4v6_ok,
					"-A %s %s %s\n", devchain, iptaddr, ex ? "-j RETURN" : nextchain);
			}

			if (ex) {
				ip46t_write("-A %s %s\n", devchain, nextchain);
			}
		}
		else if (blockall) {
			ip46t_write("-A %s -j %s\n", reschain, chain_out_drop);
		}
	}

	nvram_set("rrules_activated", "0");

	if (need_string)
#ifdef LINUX26
		modprobe("xt_string");
#else
		modprobe("ipt_string");
#endif
}
