#include <stdio.h>
#include <sys/sysinfo.h>
#include <time.h>

int main(int argc, char **argv)
{
	char prefix[] = "wanXX";
	if(argc > 0){
		strcpy(prefix, argv[0]); }
	else{
		strcpy(prefix, "wan"); }

 	struct sysinfo si;
	time_t uptime;
	char wantime_file[128];

	memset(wantime_file, 0, 128);
	sprintf(wantime_file, "/var/lib/misc/%s_time", prefix);

	sysinfo(&si);
	if (check_wanup(prefix) && (f_read(wantime_file, &uptime, sizeof(time_t)) ==  sizeof(uptime))) {
		printf("%ld\n",si.uptime - uptime);
	}
	else
		printf("0\n");

	return 0;
}
