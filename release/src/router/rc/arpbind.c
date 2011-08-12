#include "rc.h"

void start_arpbind(void) {

	char *nvp, *nv, *b;
	const char *ipaddr, *macaddr;

	nvp = nv = strdup(nvram_safe_get("arpbind_static"));
	if (!nv) return;

// clear arp table first
	stop_arpbind();

	while ((b = strsep(&nvp, ">")) != NULL) {
		/*
			ip.ad.dr.ess<macaddr>other.ip.addr.ess<anotherhwaddr>
		*/
		if ((vstrsep(b, "<", &ipaddr, &macaddr)) != 2) continue;

		eval ("arp", "-s", (char *)ipaddr, (char *)macaddr);
	}
	free(nv);
}

void stop_arpbind(void) {

	FILE *f;
	char buf[512];
	char ipaddr[48] = "";

	if ((f = fopen("/proc/net/arp", "r")) != NULL) {
//		fgets(buf, sizeof(buf), f);	// header
		while (fgets(buf, sizeof(buf), f)) {
			if (sscanf(buf, "%s %*s %*s %*s %*s %*s", ipaddr) != 1) continue;
			eval ("arp", "-d", (char *)ipaddr);
		}
		fclose(f);
	}

}

