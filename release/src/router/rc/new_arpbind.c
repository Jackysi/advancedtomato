/*

  Tomato Firmware
  Copyright (C) 2006-2008 Jonathan Zarate
  rate limit & connection limit by conanxu
*/

#include "rc.h"

//#include <sys/stat.h>

// read nvram into files
void new_arpbind_start(void)
{
	FILE *f;
	char *buf;
	char *g;
	char *p;
	char *ipaddr;//ip address
  char *macaddr;//mac address
  char *s = "/tmp/new_arpbind_start.sh";
  char *argv[3];
  int pid;
  int i;
  char lan[24];
  const char *router_ip;
  int host[256];

  //arpbind is enable
  if (!nvram_get_int("new_arpbind_enable")) return;

  //read arpbind_list from nvram
  g = buf = strdup(nvram_safe_get("new_arpbind_list"));

  //read arpbind_list into file 
  if ((f = fopen(s, "w")) == NULL) return;
  fprintf(f,
  	"#!/bin/sh\n"
  	"for HOST in `awk '{if($1!=\"IP\")print $1}' /proc/net/arp`; do arp -d $HOST; done\n"
  	);
  memset(host, 0, sizeof(host));

  //get network ip prefix
  router_ip = nvram_safe_get("lan_ipaddr");
  strlcpy(lan, router_ip, sizeof(lan));
  if ((p = strrchr(lan, '.')) != NULL) {
    host[atoi(p+1)] = 1;
    *p = '\0';
  }

  while (g) {
    /*
    macaddr<ipaddr
    */
    if ((p = strsep(&g, ">")) == NULL) break;
    i = vstrsep(p, "<", &macaddr, &ipaddr);
    fprintf(f, "arp -s %s %s\n", ipaddr, macaddr);
    if ((p = strrchr(ipaddr, '.')) != NULL) {
      *p = '\0';
      if (!strcmp(ipaddr, lan)) host[atoi(p+1)] = 1;
    }
  }

  if (nvram_get_int("new_arpbind_only")) {
      for (i = 1; i < 255; i++) {
        if (!host[i]) {
          fprintf(f, "arp -s %s.%d 00:00:00:00:00:00\n", lan, i);
        }
      }
  }

  free(buf);

  fclose(f);
  chmod(s, 0700);
  chdir("/tmp");

  argv[0] = s;
  argv[1] = NULL;
  argv[2] = NULL;
  if (_eval(argv, NULL, 0, &pid) != 0) {
    pid = -1;
  }
  else {
    kill(pid, 0);
  }
      
  chdir("/");
}

void new_arpbind_stop(void)
{
  FILE *f;
  char *s = "/tmp/new_arpbind_stop.sh";
  char *argv[3];
  int pid;

  if (nvram_get_int("new_arpbind_enable")) return;

  if ((f = fopen(s, "w")) == NULL) return;

  fprintf(f,
    "#!/bin/sh\n"
    "for HOST in `awk '{if($1!=\"IP\")print $1}' /proc/net/arp`; do arp -d $HOST; done\n"
    );

  fclose(f);
  chmod(s, 0700);
  chdir("/tmp");
  
  argv[0] = s;
  argv[1] = NULL;
  argv[2] = NULL;
  if (_eval(argv, NULL, 0, &pid) != 0) {
    pid = -1;
  }
  else {
    kill(pid, 0);
  }
      
  chdir("/");
}
