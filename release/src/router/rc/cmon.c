/*

  Tomato Firmware
  Origin by Toastman
  Modified by Shibby

*/

#include "rc.h"
#include <arpa/inet.h>
#include <sys/stat.h>

void stop_cmon(void)
{
	FILE *fp;

	if( !( fp = fopen( "/tmp/stop_cmon.sh", "w" ) ) )
	{
		perror( "/tmp/stop_cmon.sh" );
	}

	fprintf( fp, "#!/bin/sh\n");
	fprintf( fp, "service firewall restart\n");	//remove iptables rules
	fprintf( fp, "rmmod imq\n");			//remove imq`s interfaces
#ifdef LINUX26
	fprintf( fp, "rmmod xt_IMQ\n");
#else
	fprintf( fp, "rmmod ipt_IMQ\n");
#endif
	fprintf( fp, "service firewall restart\n");	//insert new/clean imq`s modules
	fprintf( fp, "logger CMON: stopped\n");

	fclose(fp);
	chmod("/tmp/stop_cmon.sh", 0755);

	xstart( "/tmp/stop_cmon.sh" );
	return;
}

void start_cmon(void)
{
	FILE *fp;
	char *buf, *g, *p;
	char *cip, *cname; //Address ip and alias
	int i;
	char seq[4];
	int iSeq = 0;


	if( !( fp = fopen( "/tmp/start_cmon.sh", "w" ) ) )
	{
		perror( "/tmp/start_cmon.sh" );
	}

	fprintf( fp, "#!/bin/sh\n");
	fprintf( fp, "service cmon stop\n");
	fprintf( fp, "sleep 5\n");


	//  only if enable...
	if ( nvram_match( "imq_enable", "1") ) {
	if ( nvram_match( "cmon_enable", "1") )
	{

	//read ip address to monitor from nvram
	if ((buf = strdup(nvram_safe_get("cmon_users"))) != NULL) {


		g = buf;

		// cip < cname
		while ((p = strsep(&g, ">")) != NULL) {
		    i = vstrsep(p, "<", &cip, &cname);
		    if (i!=2) continue;

		    sprintf(seq,"%d",iSeq);
		    iSeq++;

		    fprintf( fp, "ip link set imq%s name %s_DL up\n"
			"iptables -t mangle -A POSTROUTING -d %s -j IMQ --todev %s\n"
			,seq,cname,cip,seq);

		    sprintf(seq,"%d",iSeq);
		    iSeq++;

		    fprintf( fp, "ip link set imq%s name %s_UL up\n"
			"iptables -t mangle -A PREROUTING -s %s -j IMQ --todev %s\n"
			,seq,cname,cip,seq);

		}
		free(buf);
		fprintf( fp, "logger CMON: running\n");
	}
	}
	}

	fclose(fp);
	chmod("/tmp/start_cmon.sh", 0755);

	xstart( "/tmp/start_cmon.sh" );
	return;
}
