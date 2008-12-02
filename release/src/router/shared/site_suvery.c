

/*
 *********************************************************
 *   Copyright 2003, CyberTAN  Inc.  All Rights Reserved *
 *********************************************************

 This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
 the contents of this file may not be disclosed to third parties,
 copied or duplicated in any form without the prior written
 permission of CyberTAN Inc.

 This software should be used as a reference only, and it not
 intended for production use!


 THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
 KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
 SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <broadcom.h>
#include <cyutils.h>
#include <code_pattern.h>

#define SITE_SUVERY_DB  "/tmp/site_suvery"
#define SITE_SUVERY_NUM 50


struct site_suvery_list {
	uint8		SSID[32];
	unsigned char   BSSID[18];
	uint8		channel;	/* Channel no. */
	int16		RSSI;		/* receive signal strength (in dBm) */
	int8		phy_noise;	/* noise (in dBm) */
	uint16		beacon_period;	/* units are Kusec */
	uint16		capability;	/* Capability information */
	uint		rate_count;	/* # rates in this set */
        uint8           dtim_period;    /* DTIM period */
} site_suvery_lists[SITE_SUVERY_NUM];

int
open_site_suvery(void)
{
        FILE *fp;
	
	bzero(site_suvery_lists, sizeof(site_suvery_lists));

        if((fp = fopen(SITE_SUVERY_DB, "r"))){
                fread(&site_suvery_lists[0], sizeof(site_suvery_lists), 1, fp);
                fclose(fp);
                return TRUE;
        }
        return FALSE;
}

int
ej_dump_site_suvery(int eid, webs_t wp, int argc, char_t **argv)
{
	int i;
	int ret;

	system("site_suvery");
	
	open_site_suvery();
	for(i=0 ; i<SITE_SUVERY_NUM && site_suvery_lists[i].SSID[0] ; i++) {
		ret += websWrite(wp, "%c'%s','%s','%d','%d','%d','%d','%d','%d','%d'\n", i ? ',' : ' ',
		site_suvery_lists[i].SSID, 
		site_suvery_lists[i].BSSID,
		site_suvery_lists[i].channel,
		site_suvery_lists[i].RSSI,
		site_suvery_lists[i].phy_noise,
		site_suvery_lists[i].beacon_period,
		site_suvery_lists[i].capability,
		site_suvery_lists[i].dtim_period,
		site_suvery_lists[i].rate_count);
	}	
	
	return ret;
}
