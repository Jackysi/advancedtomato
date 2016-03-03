#include "rc.h"
#include <shared.h>
#include <string.h>
#include <wlutils.h>

// Remove defines below, rather pass as parameters ...
//#define BLINK_MAXSPEED		10			// Maximum Blink Speed, on/off cycles per second (floating point)
//#define BLINK_THRESHOLD		32768		// Data threshold ... once this much data received, then blink

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
		if(sscanf(p+1, "%lu%*u%*u%*u%*u%*u%*u%*u%lu", &counter1, &counter2)!=2) continue;
		break;
	}
	fclose(f);

	return counter1 + counter2;
}

int blink_main(int argc, char *argv[])
{
	unsigned int ledindex;
	unsigned long count;
	unsigned long oldcount = 0;
	uint32 radioStatus;
	float maxspeed;
	unsigned long threshold;
	unsigned long maxdatarate;
	float currblinkspeed;
	int iter;

	// Check for correct number of arguments
	if (argc != 5) {
		fprintf(stderr, "usage: blink interface led rate threshold\n");
		return(1);
	}
	
	// Fork new process, run in the background (daemon)
	if (fork() != 0) return 0;
	setsid();
	signal(SIGCHLD, chld_reap);
	
	// Get the LED Index for the targeted LED
	ledindex = find_led_name(argv[2]);	
	
	// And determine blink parameters
	maxspeed = atof(argv[3]);
	threshold = atol(argv[4]);
	// Calculate Max Data Rate (Data Rate for maximum blink rate ... on average, exceed data threshold each interval)
	maxdatarate = maxspeed * threshold;
		
	// Loop Through, checking for new data (and blink accordingly ... max speed at max or higher data rate)
	while(1){
		// Get Data Count, check if sufficient data received for blink
		count = get_wl_count(argv[1]);
		if (count >= (oldcount + threshold)) {
			// Sufficient Data Received, so blink - simulate rate, as /proc/net/dev is only updated once per second!
			if (threshold != 0) {
				currblinkspeed = (count-oldcount) / threshold;
				if (currblinkspeed > maxspeed)
					currblinkspeed = maxspeed;
			} else 
				currblinkspeed = maxspeed;
			oldcount = count;

			// Simulate Blink for one second (until we get new data in /proc/net/dev)
			for (iter=0; iter < currblinkspeed; iter++) {
				led(ledindex, LED_OFF);
				usleep((useconds_t)(0.5 * (1.0/currblinkspeed) * 1E6));
				led(ledindex, LED_ON);
				usleep((useconds_t)(0.5 * (1.0/currblinkspeed) * 1E6));
			}

			usleep(50000);
		} else {
			// Get Radio Status ... only blink if Radio is Enabled (otherwise, just turn the LED off)
			wl_ioctl(argv[1], WLC_GET_RADIO, &radioStatus, sizeof(radioStatus));
			
			// radioStatus != 0 for Disabled, using a bit mask defined in wlioctl.h (i.e. 0 = enabled)
			if (radioStatus != 0) {
				// Radio is disabled (in one of multiple ways), so disable LED ... and wait 5 seconds to check again
				led(ledindex, LED_OFF);
				sleep(5);
			} else {
				// Not enough Data, so don't blink ... and wait 200 ms for an update (as /proc/net/dev is only updated once per second!)
				led(ledindex, LED_ON);
				usleep(200000);
			}
		}
	}
}

