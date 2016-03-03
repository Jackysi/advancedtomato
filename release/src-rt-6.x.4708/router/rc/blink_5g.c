#include "rc.h"
#include <shared.h>

static char *interface = NULL;
static int rand_seed_by_time(void)
{
	time_t atime;

	time(&atime);
	srand((unsigned long)atime);

        return rand();
}

static unsigned long get_5g_count()
{
	FILE *f;
	char buf[256];
	char *ifname, *p;
	unsigned long counter1, counter2;

	if((f = fopen("/proc/net/dev", "r"))==NULL) return -1;

	fgets(buf, sizeof(buf), f);
	fgets(buf, sizeof(buf), f);

	counter1=counter2=0;

	while (fgets(buf, sizeof(buf), f)) {
		if((p=strchr(buf, ':'))==NULL) continue;
		*p = 0;
		if((ifname = strrchr(buf, ' '))==NULL) ifname = buf;
		else ++ifname;

		if(strcmp(ifname, interface)) continue;

		if(sscanf(p+1, "%lu%*u%*u%*u%*u%*u%*u%*u%*u%lu", &counter1, &counter2)!=2) continue;
		break;
	}
	fclose(f);

	return counter1;
}

int get_lanports_status(void)
{
	int r = 0;
	FILE *f;
	char s[128], a[16];

	if ((f = popen("/usr/sbin/robocfg showports", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if ((sscanf(s, "Port 1: %s", a) == 1) ||
			    (sscanf(s, "Port 2: %s", a) == 1) ||
			    (sscanf(s, "Port 3: %s", a) == 1) ||
			    (sscanf(s, "Port 4: %s", a) == 1)) {
				if (strncmp(a, "DOWN", 4)) {
					r++;
				}
			}
		}
		fclose(f);
	}

	return r;
}

int blink_5g_main(int argc, char *argv[])
{
	static unsigned int blink_5g = 0;
	static unsigned int data_5g = 0;
	unsigned long count_5g;
	int i;
	int model;
	static int j;
	static int status = -1;
	static int status_old;
	if (fork() != 0) return 0;
	setsid();
	signal(SIGCHLD, chld_reap);
#define INTERFACE_MAXLEN 10
	interface = calloc(INTERFACE_MAXLEN,1);
	char *tmp_interface = nvram_get("blink_5g_interface");
	if(tmp_interface)
		strncpy(interface,tmp_interface, INTERFACE_MAXLEN);
	// check data per 10 count
	model = get_model();
	while(1){
		if (model == MODEL_WS880) {
			sleep(5);
			if (get_lanports_status()) {
				led(LED_BRIDGE, LED_ON);
			}
			else {
				led(LED_BRIDGE, LED_OFF);
			}
			continue;
		}
		if(!tmp_interface){
			sleep(5);
			tmp_interface = nvram_get("blink_5g_interface");
			if(tmp_interface)
				strncpy(interface,tmp_interface, INTERFACE_MAXLEN);
			continue;
		}
		count_5g = get_5g_count();
		if(count_5g && data_5g!=count_5g) {
			blink_5g = 1;
			data_5g = count_5g;
		}
		else 
			blink_5g = 0;
		led(LED_5G, LED_ON);

		if(blink_5g) {
			j = rand_seed_by_time() % 3;
			for(i=0;i<10;i++) {
				usleep(33*1000);
				status_old = status;
				if (((i%2)==0) && (i > (3 + 2*j)))
					status = 0;
				else
					status = 1;

				if (status != status_old)
				{
					if (status)
						led(LED_5G, LED_ON);
					else
						led(LED_5G, LED_OFF);
				}
			}
			led(LED_5G, LED_ON);
		}

		usleep(50000);
	}
}

