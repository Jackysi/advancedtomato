#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "tomato.h"

struct occupy
{
	char name[20];
	unsigned int user;
	unsigned int nice;
	unsigned int system;
	unsigned int idle;
	unsigned int io;
	unsigned int irq;
	unsigned int sirq;
};

void trim( char *str)
{
        char *copied, *tail = NULL;
        if ( str == NULL )
                return ;

        for( copied = str; *str; str++ )
        {
                if ( (unsigned char)*str > 0x20 )
                {
                        *copied++ = *str;
                        tail = copied;
                }
                else {
                        if ( tail )
                        	*copied++ = *str;
                }
        }

        if ( tail )
                *tail = 0;
        else
                *copied = 0;
        return ;
}

static float g_cpu_used;
static int cpu_num;

static void cal_occupy(struct occupy *, struct occupy *);
static void get_occupy(struct occupy *);


/*
Processor               : ARMv7 Processor rev 0 (v7l
processor               : 0
cpu model               : BCM3302 V2.9
BogoMIPS                : 238.38
*/
int strncmp_ex(char *str1, char *str2)
{
	return strncmp(str1, str2, strlen(str2));
}

int get_cpuinfo(char *system_type, char *cpu_model, char *bogomips, char *cpuclk, char *cputemp)

{
	FILE *fd;
        char *next;
	char buff[1024];
	char title[128], value[512];
	int okcount=0;

	fd = fopen ("/proc/cpuinfo", "r");
	while (fgets(buff, sizeof(buff), fd)) {
		next = buff;
		strcpy(title, strsep(&next, ":"));
		if (next == NULL) continue;
		strcpy(value, next);
		trim(value);
		if (strncmp_ex(title, "Processor")==0) {
			okcount++;
			//printf("Processor: %s\n", value);
			strcpy(system_type, value);
		}
		if (strncmp_ex(title, "cpu model")==0) {
			okcount++;
			//printf("cpu model: %s\n", value);
			strcpy(cpu_model, value);
		}
		if (strncmp_ex(title, "BogoMIPS")==0) {
			okcount++;
			//printf("bogomips: %s\n", value);
			strcpy(bogomips, value);
		}
//		if (strncmp_ex(title, "cpu MHz")==0) {
//			okcount++;
//			//printf("cpuclk: %s\n", value);
//			strcpy(cpuclk, value);
//		}

		//fprintf (stderr, "%s - %s", title, value);
	}
	fclose(fd);

	system("/usr/sbin/sysinfo-helper");
	fd = fopen ("/tmp/sysinfo-helper", "r");
	while (fgets(buff, sizeof(buff), fd)) {
		next = buff;
		strcpy(title, strsep(&next, ":"));
		if (next == NULL) continue;
		strcpy(value, next);
		trim(value);
		if (strncmp_ex(title, "cpu MHz")==0) {
			okcount++;
			strcpy(cpuclk, value);
		}
		if (strncmp_ex(title, "cpu Temp")==0) {
			okcount++;
			strcpy(cputemp, value);
		}
	}
	fclose(fd);

	return (okcount==4);
}

/*
int main()
{
	char system_type[64];
	char cpu_model[64];
	char bogomips[64];
	char cpuclk[64];
	if get_cpuinfo(system_type, cpu_model, bogomips, cpuclk) {
		printf("system type: %s, cpu model: %s, bogomips: %s, cpuclk: %s\n", system_type, cpu_model, bogomips, cpuclk);
	}
}
*/

float get_cpupercent()
{
	struct occupy ocpu[10];
	struct occupy ncpu[10];
	int i;
  
//	cpu_num = sysconf(_SC_NPROCESSORS_ONLN);
	cpu_num = 1;
	get_occupy(ocpu);
	sleep(1);
	get_occupy(ncpu);
	for (i=0; i<cpu_num; i++)
	{
		cal_occupy(&ocpu[i], &ncpu[i]);
		//printf("%f \n", g_cpu_used);
	}
	return g_cpu_used;	
}

static void cal_occupy (struct occupy *o, struct occupy *n)
{
	double od, nd;
	double id, sd;
	double scale;

	od = (double) (o->user + o->nice + o->system + o->idle + o->io + o->irq + o->sirq);
	nd = (double) (n->user + n->nice + n->system + n->idle + n->io + n->irq + n->sirq);
	scale = 100.0 / (float)(nd-od);
	id = (double) (n->user - o->user);
	sd = (double) (n->system - o->system);
	g_cpu_used = ((sd+id)*100.0)/(nd-od);
}

static void get_occupy (struct occupy *o)
{
	FILE *fd;
	int n;
	char buff[1024];
                                                                                                              
	fd = fopen ("/proc/stat", "r");
	fgets (buff, sizeof(buff), fd);
	for(n=0;n<cpu_num;n++)
	{
		fgets (buff, sizeof(buff),fd);
		sscanf (buff, "%s %u %u %u %u %u %u %u", o[n].name, &o[n].user, &o[n].nice, &o[n].system, &o[n].idle, &o[n].io, &o[n].irq, &o[n].sirq);
		//fprintf (stderr, "%s %u %u %u %u %u %u %u\n", o[n].name, o[n].user, o[n].nice, o[n].system, o[n].idle, o[n].io, o[n].irq, o[n].sirq);
	}
	fclose(fd);
}
