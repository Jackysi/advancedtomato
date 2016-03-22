#include <stdio.h>
#include <sys/sysinfo.h>
#include <time.h>

int main(int argc, char *argv[])
{
	char prefix[] = "wanXX_";
	if (argc > 1)
		strcpy(prefix, argv[1]);
	else
		strcpy(prefix, "wan");

 	struct sysinfo si;
	time_t uptime;
	char wantime_file[128];

	memset(wantime_file, 0, 128);
	sprintf(wantime_file, "/var/lib/misc/%s_time", prefix);

	if (sysinfo(&si) == -1) {
        	return 1;
	}

//	printf("check_wanup(%s) returns %d.\n", prefix, check_wanup(prefix));
	
	if (check_wanup(prefix) && f_read(wantime_file, &uptime, sizeof(time_t)) ==  sizeof(uptime)) {
		printf("%ld\n",si.uptime - uptime);
	}
	else
		printf("0\n");

	return 0;
}

