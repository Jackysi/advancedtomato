#include <stdio.h>
#include <sys/sysinfo.h>
#include <time.h>

int main(int argc, char **argv)
{
  struct sysinfo si;
  time_t uptime;

  sysinfo(&si);
  if (check_wanup() && (f_read("/var/lib/misc/wantime", &uptime, sizeof(time_t)) ==  sizeof(uptime))) {
    printf("%ld\n",si.uptime - uptime);
  }
  else
    printf("0\n");
  return 0;
}
