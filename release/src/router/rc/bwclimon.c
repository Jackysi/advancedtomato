#include <shared.h>
#include "rc.h"

void start_bwclimon(void) {

	char *nvp, *nv, *b;
//	const char *ipaddr, *hostname;

	nvp = nv = strdup(nvram_safe_get("bwm_client"));
	if (!nv) return;

// clear rules first
	stop_bwclimon();

	eval("iptables", "-N", "traffic_in");
	eval("iptables", "-N", "traffic_out");
	eval("iptables", "-I", "FORWARD", "1", "-j", "traffic_in");
	eval("iptables", "-I", "FORWARD", "2", "-j", "traffic_out");

//	while ((b = strsep(&nvp, ">")) != NULL) {
		/*
			* previously:
			ip.ad.dr.ess<hostname>other.ip.addr.ess<anotherhost>
			currently, only the IP address is used
		*/

//		if ((vstrsep(b, "<", &ipaddr, &hostname)) != 2) continue;

//		eval("iptables", "-A", "traffic_in",  "-d", (char *)ipaddr);
//		eval("iptables", "-A", "traffic_out", "-s", (char *)ipaddr);

		/*
			* currently:
			ip.ad.dr.ess,other.ip.addr.ess,another.ip.addr.ess
		*/

	while ((b = strsep(&nvp, ",")) != NULL) {
		if (strlen(b) > INET_ADDRSTRLEN)
			continue;
		eval("iptables", "-A", "traffic_in",  "-d", (char *)b);
		eval("iptables", "-A", "traffic_out", "-s", (char *)b);
	}
	free(nv);
}

void stop_bwclimon(void) {

	eval("iptables", "-D", "FORWARD", "-j", "traffic_in");
	eval("iptables", "-D", "FORWARD", "-j", "traffic_out");

	eval("iptables", "-F", "traffic_in");
	eval("iptables", "-F", "traffic_out");

	eval("iptables", "-X", "traffic_in");
	eval("iptables", "-X", "traffic_out");
}

