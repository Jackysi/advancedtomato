#include "rc.h"
#include <shared.h>

static int rand_seed_by_time(void)
{
	time_t atime;

	time(&atime);
	srand((unsigned long)atime);

	return rand();
}

static int find_led_name(char *ledname)
{
	int i = 0;
	
	while ((i < LED_COUNT) && (strcmp(ledname, led_names[i])))
		i++;

	if (i < LED_COUNT)
		return(i);
	else {
		printf("blink: Invalid LED name\n");
		exit(2);
	}
}

static unsigned long get_wl_count(char *interface)
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

	}
	fclose(f);

	return counter1;
}

int blink_main(int argc, char *argv[])
{
	static unsigned int blink = 0;
	static unsigned int data = 0;
	unsigned long count;
	static unsigned int ledindex;
	int i;
	static int j;
	static int status = -1;
	static int status_old;

	if (argc != 3) {
		fprintf(stderr, "usage: blink interface led\n");
		return(1);
	}
	
	if (fork() != 0) return 0;
	setsid();
	signal(SIGCHLD, chld_reap);
	
	ledindex = find_led_name(argv[2]);
	// check data per 10 count
	while(1){
		count = get_wl_count(argv[1]);
		if(count && data!=count) {
			blink = 1;
			data = count;
		}
		else 
			blink = 0;
		led(ledindex, LED_ON);

		if(blink) {
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
						led(ledindex, LED_ON);
					else
						led(ledindex, LED_OFF);
				}
			}
			led(ledindex, LED_ON);
		}
		else
			usleep(50000);

	}
}

