#include "dnsmasq.h"
#include <shutils.h>
#include <utils.h>
#include <bcmnvram.h>

#define dprintf(fmt, args...)

#define GOOGLE_DNS_FILE	"/tmp/dns.google"

int
write_google_dns(char *name, char *ip)
{
        FILE *fp;

        /* Write name and IP pair to file */
        if((fp = fopen(GOOGLE_DNS_FILE, "a"))) {
                cprintf("Write \"%s %s\" to %s\n", name, ip, GOOGLE_DNS_FILE);
                fprintf(fp, "%s %s\n", name, ip);
                fclose(fp);
		file2nvram(GOOGLE_DNS_FILE, "google_dns");
		system("touch /tmp/need_commit");
        }
        else {
                dprintf("Cann't open %s\n", GOOGLE_DNS_FILE);
                return -1;
        }
        return 0;
}


int
insert_google_firewall(char *query_name, struct all_addr *addr)
{
        char ip[INET_ADDRSTRLEN];
        char *pass_host = nvram_safe_get("google_pass_host");
        char dnames[254], name[254], *next, *next1;
        char ports[254], port[10], proto[10];
        char cmd[254];
        int ret;

        inet_ntop(AF_INET, addr, ip, INET_ADDRSTRLEN);
        dprintf("query_name[%s] ip[%s]\n", query_name, ip);

        if(nvram_match("google_enable", "0") ||
           nvram_match("google_pass_host", ""))   return 0;

        /* Check name and IP whether exist */
        ret = find_dns_ip_name(GOOGLE_DNS_FILE, ip, query_name);

        if(ret) {
                dprintf("The name and ip is already exist\n");
                return 0;
        }

        /* Scan symc_except_ip table */
        foreach(dnames, pass_host, next) {
                /* Format: name:proto:port1,port2,port3,.... */
                sscanf(dnames, "%[^:]:%[^:]:%s", name, proto, ports);
		dprintf("name[%s] proto[%s] ports[%s]\n", name, proto, ports);

                if(!strcmp(name, query_name))
                {
                        write_google_dns(query_name, ip);
                        _foreach(port, ports, next1, ",", ',') {
                                memset(cmd,0,254);
                                snprintf(cmd, sizeof(cmd),
                                        "iptables -I google -p %s -m %s --dport %s -d %s -j ACCEPT", proto, proto, port, ip);
                                cprintf("cmd=[%s]\n", cmd);
                                system(cmd);
                        }
                        return 0;
                }
        }
        return 0;
}

